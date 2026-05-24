#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <system/include/bkp_log.h>
#include "include/vulkan.h"
#include "include/vk_types.h"
#include "include/vkAllocator.h"

/* stb_image declarations — implementation is in vulkan.c */
#include <thirdparty/stb/stb_image.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#endif

/* internal bkp functions not yet in the public header */
void   bkpBeginCmdBufferUniqUsage(BkpGpuAdapter adapter,
                                   BkpCommandBuffer * cmd, EGpuQueue type);
void   bkpEndCmdBufferUniqUsage  (BkpGpuAdapter adapter,
                                   BkpCommandBuffer * cmd);
void   bkpMapBuffer  (BkpGpuAdapter adapter, BkpBuffer buffer);
void   bkpUnmapBuffer(BkpGpuAdapter adapter, BkpBuffer buffer);
void   bkpGetBuffer      (BkpBuffer buffer, VkBuffer * out);
void   bkpGetBufferOffset(BkpBuffer buffer, VkDeviceSize * out);
size_t bkpResolveStagingBudget(BkpGpuAdapter adapter);

static const char * gTag = "TexBatch";

/*___________________________________________________________________*/
typedef struct
{
    /* input */
    const char * filePath;
    void       * encodedData;
    int          encodedSize;
    /* output */
    uint8_t    * pixels;
    int          width, height;
    BkpBool      hasAlpha;
} DecodeJob;

/*___________________________________________________________________*/
/* ---- CPU decode workers ----------------------------------------- */
#ifdef _WIN32
static DWORD WINAPI decodeWorker(LPVOID arg)
#else
static void * decodeWorker(void * arg)
#endif
{
    DecodeJob * j = (DecodeJob *)arg;
    int w, h, ch;
    if(j->filePath)
        j->pixels = stbi_load(j->filePath, &w, &h, &ch, STBI_rgb_alpha);
    else
        j->pixels = stbi_load_from_memory(
                        (const stbi_uc *)j->encodedData, j->encodedSize,
                        &w, &h, &ch, STBI_rgb_alpha);
    if(j->pixels) { j->width = w; j->height = h;
                    j->hasAlpha = (ch==4||ch==2) ? BKP_TRUE : BKP_FALSE; }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void spawnAndJoin(DecodeJob * jobs, size_t count)
{
#ifdef _WIN32
    HANDLE * h = (HANDLE *)malloc(count * sizeof(HANDLE));
    for(size_t base = 0; base < count; base += MAXIMUM_WAIT_OBJECTS)
    {
        size_t chunk = count - base;
        if(chunk > MAXIMUM_WAIT_OBJECTS)
        {
          chunk = MAXIMUM_WAIT_OBJECTS;
        }
        HANDLE tmp[MAXIMUM_WAIT_OBJECTS]; DWORD tc = 0;
        for(size_t k = 0; k < chunk; ++k)
        {
            size_t i = base + k;
            if(jobs[i].filePath || jobs[i].encodedData)
            {
              h[i] = CreateThread(NULL,0,decodeWorker,&jobs[i],0,NULL);
              tmp[tc++]=h[i];
            }
            else
            {
              h[i] = NULL;
            }
        }
        if(tc)
        {
          WaitForMultipleObjects(tc, tmp, TRUE, INFINITE);
        }
        for(size_t k = 0; k < chunk; ++k)
        {
          if(h[base+k])
          {
            CloseHandle(h[base+k]);
          }
        }
    }
    free(h);
#else
    pthread_t * t  = (pthread_t *)malloc(count * sizeof(pthread_t));
    int       * ok = (int *)calloc(count, sizeof(int));
    for(size_t i = 0; i < count; ++i)
    {
      if(jobs[i].filePath || jobs[i].encodedData)
      {
        pthread_create(&t[i], NULL, decodeWorker, &jobs[i]);
        ok[i]=1;
      }
    }
    for(size_t i = 0; i < count; ++i)
    {
      if(ok[i]) pthread_join(t[i], NULL);
    }
    free(t); free(ok);
#endif
}

/*___________________________________________________________________*/
/* ---- Helpers for recording barriers inside a cmd buffer --------- */
static void recordBarrier(VkCommandBuffer cmd,
                           VkImage image, VkImageAspectFlags aspect,
                           uint32_t mipBase, uint32_t mipCount,
                           VkImageLayout oldLay, VkImageLayout newLay,
                           VkAccessFlags srcAcc, VkAccessFlags dstAcc,
                           VkPipelineStageFlags srcStage,
                           VkPipelineStageFlags dstStage)
{
    VkImageMemoryBarrier b = {};
    b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.oldLayout           = oldLay;
    b.newLayout           = newLay;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image               = image;
    b.subresourceRange.aspectMask     = aspect;
    b.subresourceRange.baseMipLevel   = mipBase;
    b.subresourceRange.levelCount     = mipCount;
    b.subresourceRange.baseArrayLayer = 0;
    b.subresourceRange.layerCount     = 1;
    b.srcAccessMask = srcAcc;
    b.dstAccessMask = dstAcc;
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                         0, NULL, 0, NULL, 1, &b);
}

/*___________________________________________________________________*/
/* ---- GPU batch: upload + optional mip generation (2 submits) ---- */
static void gpuBatch(BkpGpuAdapter adapter,
                     DecodeJob * jobs, size_t count,
                     BkpImageResource * targets)
{
    BkpBuffer * stagings = (BkpBuffer *)calloc(count, sizeof(BkpBuffer));

    /* ---- allocate VkImages + staging buffers ---- */
    for(size_t i = 0; i < count; ++i)
    {
        if(!jobs[i].pixels) continue;

        VkDeviceSize sz = (VkDeviceSize)jobs[i].width * jobs[i].height * 4;
        //LOG(eDEBUG, gTag, "  tex[%zu] %dx%d (%zu MiB) — createImage...", i, jobs[i].width, jobs[i].height, (size_t)(sz / (1024u*1024u)));

        BkpImageResource * t = &targets[i];
        t->imageInfo.extent.width  = (uint32_t)jobs[i].width;
        t->imageInfo.extent.height = (uint32_t)jobs[i].height;
        t->imageInfo.extent.depth  = 1;
        t->imageInfo.arrayLayers   = 1;
        t->imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        t->hasAlpha = jobs[i].hasAlpha;

        /* compute mip levels if runtime-generate mode */
        if(t->mipType == eMIPMAP_RUNTIME_GENERATE)
        {
            uint32_t v = (uint32_t)jobs[i].width > (uint32_t)jobs[i].height
                       ? (uint32_t)jobs[i].width : (uint32_t)jobs[i].height;
            t->imageInfo.mipLevels = (uint32_t)floorf(log2f((float)v)) + 1u;
        }
        else if(t->mipType == eMIPMAP_NONE || t->imageInfo.mipLevels < 1)
        {
            t->imageInfo.mipLevels = 1;
        }

        t->viewInfo.subresourceRange.levelCount = t->imageInfo.mipLevels;
        t->viewInfo.subresourceRange.layerCount = 1;

        bkpCreateImage(adapter, t);
        //LOG(eDEBUG, gTag, "  tex[%zu] image OK — allocBuffer...", i);

        /* staging buffer */
        BkpBufferInfo info = {};
        info.usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        info.type            = eBUFFER_CPU_GPU;
        info.size            = sz;
        info.queueIndices[0] = adapter->graphicQueue.index;
        info.qCount          = 1;
        info.isImage         = BKP_FALSE;
        bkpAllocateBuffer(adapter, &stagings[i], &info);
        //LOG(eDEBUG, gTag, "  tex[%zu] buffer OK — upload...", i);
        bkpMapBuffer(adapter, stagings[i]);
        bkpUploadBufferData(adapter, stagings[i], jobs[i].pixels, 0, sz);
        bkpUnmapBuffer(adapter, stagings[i]);
        //LOG(eDEBUG, gTag, "  tex[%zu] upload OK", i);
    }

    //LOG(eDEBUG, gTag, "gpuBatch: stagings allocated");

    /* ==== Command buffer 1 : upload batch ==== */
    BkpCommandBuffer cmd;
    bkpBeginCmdBufferUniqUsage(adapter, &cmd, eQFAMILY_GRAPHIC);

    /* transition all mip levels UNDEFINED → TRANSFER_DST.
       Mip-0 receives the pixel upload; higher mips are blit
       destinations later — they must not be in UNDEFINED. */
    for(size_t i = 0; i < count; ++i)
    {
        if(!jobs[i].pixels) continue;
        recordBarrier(cmd.cmds[0], targets[i].images[0],
                      targets[i].viewInfo.subresourceRange.aspectMask,
                      0, targets[i].imageInfo.mipLevels,
                      VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      0, VK_ACCESS_TRANSFER_WRITE_BIT,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT);
    }

    /* copy pixel data from staging into mip-0 */
    for(size_t i = 0; i < count; ++i)
    {
        if(!jobs[i].pixels) continue;
        VkBuffer     vkBuf; VkDeviceSize vkOff;
        bkpGetBuffer(stagings[i], &vkBuf);
        bkpGetBufferOffset(stagings[i], &vkOff);

        VkBufferImageCopy r = {};
        r.bufferOffset                    = vkOff;
        r.imageSubresource.aspectMask     = targets[i].viewInfo.subresourceRange.aspectMask;
        r.imageSubresource.mipLevel       = 0;
        r.imageSubresource.layerCount     = 1;
        r.imageExtent = (VkExtent3D){(uint32_t)jobs[i].width, (uint32_t)jobs[i].height, 1};
        vkCmdCopyBufferToImage(cmd.cmds[0], vkBuf, targets[i].images[0],
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);
    }

    /* for textures with mipLevels==1 → SHADER_READ_ONLY directly */
    for(size_t i = 0; i < count; ++i)
    {
        if(!jobs[i].pixels) {continue;}
        if(targets[i].imageInfo.mipLevels > 1) {continue;}
        recordBarrier(cmd.cmds[0], targets[i].images[0],
                      targets[i].viewInfo.subresourceRange.aspectMask,
                      0, 1,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    //LOG(eDEBUG, gTag, "gpuBatch: submit 1 (upload)...");
    bkpEndCmdBufferUniqUsage(adapter, &cmd); /* SUBMIT 1 */
    //LOG(eDEBUG, gTag, "gpuBatch: submit 1 done");

    /* ==== Command buffer 2 : mip generation batch ==== */
    /* check if any texture needs mipmaps */
    int anyMips = 0;
    for(size_t i = 0; i < count; ++i)
    {
        if(jobs[i].pixels && targets[i].imageInfo.mipLevels > 1)
        {
          anyMips=1; break;
        }
    }

    if(anyMips)
    {
        bkpBeginCmdBufferUniqUsage(adapter, &cmd, eQFAMILY_GRAPHIC);

        for(size_t i = 0; i < count; ++i)
        {
            if(!jobs[i].pixels || targets[i].imageInfo.mipLevels <= 1)
            {continue;}

            BkpImageResource * t    = &targets[i];
            VkImage            img  = t->images[0];
            VkImageAspectFlags asp  = t->viewInfo.subresourceRange.aspectMask;
            uint32_t mips = t->imageInfo.mipLevels;
            uint32_t w = (uint32_t)jobs[i].width;
            uint32_t h = (uint32_t)jobs[i].height;

            for(uint32_t m = 1; m < mips; ++m)
            {
                /* mip[m-1]: TRANSFER_DST → TRANSFER_SRC (blit source) */
                recordBarrier(cmd.cmds[0], img, asp, m-1, 1,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT);

                uint32_t w2 = w > 1 ? w/2 : 1;
                uint32_t h2 = h > 1 ? h/2 : 1;

                VkImageBlit blit = {};
                blit.srcOffsets[0] = (VkOffset3D){0,0,0};
                blit.srcOffsets[1] = (VkOffset3D){(int)w,(int)h,1};
                blit.srcSubresource.aspectMask   = asp;
                blit.srcSubresource.mipLevel     = m-1;
                blit.srcSubresource.layerCount   = 1;
                blit.dstOffsets[0] = (VkOffset3D){0,0,0};
                blit.dstOffsets[1] = (VkOffset3D){(int)w2,(int)h2,1};
                blit.dstSubresource.aspectMask   = asp;
                blit.dstSubresource.mipLevel     = m;
                blit.dstSubresource.layerCount   = 1;
                vkCmdBlitImage(cmd.cmds[0],
                               img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit, t->filter);

                /* mip[m-1]: TRANSFER_SRC → SHADER_READ_ONLY (done) */
                recordBarrier(cmd.cmds[0], img, asp, m-1, 1,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_ACCESS_TRANSFER_READ_BIT,
                              VK_ACCESS_SHADER_READ_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                w = w2; h = h2;
            }

            /* last mip: TRANSFER_DST → SHADER_READ_ONLY */
            recordBarrier(cmd.cmds[0], img, asp, mips-1, 1,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }

        //LOG(eDEBUG, gTag, "gpuBatch: submit 2 (mipmaps)...");
        bkpEndCmdBufferUniqUsage(adapter, &cmd); /* SUBMIT 2 */
        //LOG(eDEBUG, gTag, "gpuBatch: submit 2 done");
    }

    /* ==== create image views + free stagings ==== */
    for(size_t i = 0; i < count; ++i)
    {
        if(!jobs[i].pixels) continue;

        BkpImageResource * t = &targets[i];
        t->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        t->imageViews = (BkpArray(VkImageView))
                        bkpArrayCreateFrom(VkImageView, adapter->memoryGroupId);
        VkImageView view;
        t->viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        t->viewInfo.image = t->images[0];
        if(vkCreateImageView(adapter->device, &t->viewInfo,
                              adapter->allocator, &view) != VK_SUCCESS)
        {
            LOG(eWARNING, gTag, "vkCreateImageView failed for texture %zu", i);
        }
        else
        {
            bkpArrayPush(&t->imageViews, view);
        }

        bkpFreeBuffer(adapter, &stagings[i]);
        stbi_image_free(jobs[i].pixels);
        jobs[i].pixels = NULL;
    }

    free(stagings);
}

/*___________________________________________________________________*/
void bkpUploadTextureBatch(BkpGpuAdapter adapter,
                           BkpTextureSource * sources, size_t count,
                           BkpImageResource * targets)
{
    if(!count) return;

    DecodeJob * jobs = (DecodeJob *)calloc(count, sizeof(DecodeJob));

    for(size_t i = 0; i < count; ++i)
    {
        if(sources[i].filePath)
        {
            jobs[i].filePath = sources[i].filePath;
        }
        else if(sources[i].encoded.size > 0)
        {
            jobs[i].encodedData = (uint8_t *)sources[i].encoded.buffer
                                  + sources[i].encoded.offset;
            jobs[i].encodedSize = (int)sources[i].encoded.size;
        }
    }

    /* Split uploads into memory-aware sub-batches.
       Each batch stays within the staging budget so that APUs (shared VRAM) and
       systems with many large textures never exhaust memory in one shot. */
    size_t budget = bkpResolveStagingBudget(adapter);
    size_t base   = 0;
    size_t nBatch = 0;

    while(base < count)
    {
        /* Accumulate textures until the estimated decoded size exceeds the budget. */
        size_t batchEnd   = base;
        size_t accumulated = 0;

        while(batchEnd < count)
        {
            size_t est;
            if(jobs[batchEnd].encodedData)
            {
                /* embedded: compressed size × 6 is a conservative RGBA decode estimate */
                est = (size_t)jobs[batchEnd].encodedSize * 6;
            }
            else if(jobs[batchEnd].filePath)
            {
                /* file-based: unknown until decoded — assume a typical 1 K texture (16 MiB) */
                est = 16u * 1024u * 1024u;
            }
            else
            {
                est = 0; /* null source — gpuBatch will skip it */
            }

            /* Always include at least one texture per batch even if it exceeds the budget. */
            if(batchEnd > base && accumulated + est > budget)
            {
                break;
            }
            accumulated += est;
            batchEnd++;
        }

        size_t batch = batchEnd - base;
        /*
        LOG(eDEBUG, gTag, "sub-batch %zu: textures [%zu..%zu) est=%zu MiB",
            nBatch, base, batchEnd, accumulated / (1024u * 1024u));
            */
        spawnAndJoin(jobs + base, batch);
        //LOG(eDEBUG, gTag, "sub-batch %zu: decode done", nBatch);

        /* Second split on actual GPU image sizes (with mipmaps).
           Keeps each gpuBatch under half the staging budget so the
           GPU allocator chunk (typically 200 MiB) is not exhausted. */
        const size_t gpuBudget = budget / 2;
        size_t gpuBase = base;
        while(gpuBase < batchEnd)
        {
            size_t gpuEnd   = gpuBase;
            size_t gpuAccum = 0;
            while(gpuEnd < batchEnd)
            {
                if(!jobs[gpuEnd].pixels) { gpuEnd++; continue; }
                /* actual decoded RGBA size × 4/3 ≈ image + all mip levels */
                size_t imgSz = (size_t)jobs[gpuEnd].width
                             * (size_t)jobs[gpuEnd].height * 4 * 4 / 3;
                if(gpuEnd > gpuBase && gpuAccum + imgSz > gpuBudget)
                {
                    break;
                }
                gpuAccum += imgSz;
                gpuEnd++;
            }
            if(gpuEnd == gpuBase) { gpuEnd = gpuBase + 1; } /* always at least 1 */
            /*
            LOG(eDEBUG, gTag, "  gpu-batch %zu: [%zu..%zu) ~%zu MiB GPU",
                nBatch, gpuBase, gpuEnd, gpuAccum / (1024u * 1024u));
                */
            gpuBatch(adapter, jobs + gpuBase, gpuEnd - gpuBase, targets + gpuBase);
            //LOG(eDEBUG, gTag, "  gpu-batch %zu: done", nBatch);
            gpuBase = gpuEnd;
            nBatch++;
        }
        base = batchEnd;
    }

    free(jobs);

    LOG(eDEBUG, gTag, "Uploaded %zu texture(s) in %zu gpu-batch(es) (budget %zu MiB)",
        count, nBatch, budget / (1024u * 1024u));
}

/*___________________________________________________________________*/
/* ---- ORM packing from separate greyscale files ------------------ */

typedef struct
{
    const char * path;   /* NULL = no source (AO optional) */
    uint8_t    * pixels; /* STBI_grey, 1 byte per pixel    */
    int          w, h;
    int          failed;
} GreyJob;

#ifdef _WIN32
static DWORD WINAPI greyWorker(LPVOID arg)
#else
static void * greyWorker(void * arg)
#endif
{
    GreyJob * j = (GreyJob *)arg;
    if(!j->path)
    {
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    int w, h, ch;
    j->pixels = stbi_load(j->path, &w, &h, &ch, STBI_grey);
    if(j->pixels) { j->w = w; j->h = h; }
    else          { j->failed = 1;
                    LOG(eERROR, gTag, "ORM: failed to load '%s'", j->path); }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void spawnAndJoinGrey(GreyJob * jobs, int count)
{
#ifdef _WIN32
    HANDLE h[3]; int nc = 0;
    for(int i = 0; i < count; ++i)
    {
        if(jobs[i].path) { h[nc++] = CreateThread(NULL,0,greyWorker,&jobs[i],0,NULL); }
        else               h[i] = NULL;
    }
    if(nc) WaitForMultipleObjects((DWORD)nc, h, TRUE, INFINITE);
    for(int i = 0; i < count; ++i) if(h[i]) CloseHandle(h[i]);
#else
    pthread_t t[3]; int ok[3] = {0,0,0};
    for(int i = 0; i < count; ++i)
    {
        if(jobs[i].path) { pthread_create(&t[i], NULL, greyWorker, &jobs[i]); ok[i]=1; }
    }
    for(int i = 0; i < count; ++i)
    {
        if(ok[i]) pthread_join(t[i], NULL);
    }
#endif
}

/*___________________________________________________________________*/
BkpBool bkpLoadORMFromFiles(BkpGpuAdapter adapter,
                             const char * metallicPath,
                             const char * roughnessPath,
                             const char * aoPath,
                             BkpImageResource * target)
{
    GreyJob jobs[3] = {
        {metallicPath,  NULL, 0, 0, 0},
        {roughnessPath, NULL, 0, 0, 0},
        {aoPath,        NULL, 0, 0, 0},  /* aoPath may be NULL */
    };

    /* parallel decode — 2 or 3 threads */
    spawnAndJoinGrey(jobs, 3);

    if(jobs[0].failed || jobs[1].failed)
    {
        for(int i = 0; i < 3; ++i) stbi_image_free(jobs[i].pixels);
        return BKP_FALSE;
    }

    /* validate dimensions */
    int w = jobs[0].w, h = jobs[0].h;
    if(jobs[1].w != w || jobs[1].h != h)
    {
        LOG(eERROR, gTag, "ORM: metallic (%dx%d) and roughness (%dx%d) dimensions mismatch",
            w, h, jobs[1].w, jobs[1].h);
        for(int i = 0; i < 3; ++i) stbi_image_free(jobs[i].pixels);
        return BKP_FALSE;
    }
    if(aoPath && jobs[2].pixels && (jobs[2].w != w || jobs[2].h != h))
    {
        LOG(eWARNING, gTag, "ORM: AO (%dx%d) differs from metallic (%dx%d) — AO ignored",
            jobs[2].w, jobs[2].h, w, h);
        stbi_image_free(jobs[2].pixels);
        jobs[2].pixels = NULL;
    }

    /* pack R=AO  G=Roughness  B=Metallic  A=255 */
    size_t pixCount = (size_t)w * (size_t)h;
    uint8_t * packed = (uint8_t *)malloc(pixCount * 4);
    uint8_t * metal  = jobs[0].pixels;
    uint8_t * rough  = jobs[1].pixels;
    uint8_t * ao     = jobs[2].pixels;  /* NULL if no AO */

    for(size_t i = 0; i < pixCount; ++i)
    {
        packed[i*4+0] = ao ? ao[i] : 0xFF;  /* R = AO        */
        packed[i*4+1] = rough[i];            /* G = Roughness */
        packed[i*4+2] = metal[i];            /* B = Metallic  */
        packed[i*4+3] = 0xFF;                /* A = 1         */
    }

    for(int i = 0; i < 3; ++i) stbi_image_free(jobs[i].pixels);

    /* ORM is linear data — override format */
    target->imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    target->viewInfo.format  = VK_FORMAT_R8G8B8A8_UNORM;
    target->hasAlpha         = BKP_FALSE;

    /* single texture upload via existing function (handles mip setup) */
    bkpCreateTextureLayersFromData(adapter, &packed, 1, w, h, 1, target);

    free(packed);

    LOG(eDEBUG, gTag, "ORM loaded: %dx%d (AO=%s)", w, h, ao ? "yes" : "no");
    return BKP_TRUE;
}

#ifdef __cplusplus
}
#endif
