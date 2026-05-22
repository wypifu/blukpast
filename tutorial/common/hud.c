#include "hud.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

/* Build the orthographic matrix that maps glyph-space pixel coords (origin
 * at top-left of the glyph run) to Vulkan NDC.
 *
 * BkpMat4 stores rows, but GLSL reads column-major, so mLineJ = GLSL column J.
 * The resulting transform maps vertex (vx, vy, 0, 1) to:
 *   NDC_x = (2/W)*vx + (2*px/W - 1)
 *   NDC_y = (2/H)*vy + (2*py/H - 1)   (Vulkan: y=-1 top, y=+1 bottom)
 */
static BkpMat4 textTransform(int px, int py, int W, int H)
{
    float sx = 2.0f / (float)W;
    float sy = 2.0f / (float)H;
    BkpMat4 m;
    m.mLine0 = bkpVec4(sx,                    0.0f, 0.0f, 0.0f);
    m.mLine1 = bkpVec4(0.0f,                  sy,   0.0f, 0.0f);
    m.mLine2 = bkpVec4(0.0f,                  0.0f, 1.0f, 0.0f);
    m.mLine3 = bkpVec4(2.0f*px/(float)W-1.0f, 2.0f*py/(float)H-1.0f, 0.0f, 1.0f);
    return m;
}

static void uploadTextUBO(BkpGpuAdapter adp, BkpText text, uint32_t frame)
{
    bkpUploadBufferData(adp, text->ubo.buffer, &text->data,
                        frame * text->ubo.size, text->ubo.size);
}

static void drawText(BkpGpuAdapter adp, VkCommandBuffer cmd,
                     Hud * h, BkpText text, int px, int py, uint32_t frame)
{
    if(text->vertexCount == 0) { return; }

    text->data.transformation = textTransform(px, py, h->width, h->height);
    uploadTextUBO(adp, text, frame);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        h->hudPipeline.pipelineLayout.layout, 0, 1,
        &text->set.descriptorSets[frame], 0, NULL);

    VkBuffer vbuf; VkDeviceSize voff;
    bkpGetBuffer(text->vertexBuffer, &vbuf);
    bkpGetBufferOffset(text->vertexBuffer, &voff);
    vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &voff);
    vkCmdDraw(cmd, text->vertexCount, 1, 0, 0);
}

static void drawRect(VkCommandBuffer cmd, Hud * h,
                     float x, float y, float w, float ht,
                     float r, float g, float b, float a)
{
    RectPC pc;
    pc.rect[0] = x;  pc.rect[1] = y;  pc.rect[2] = w;  pc.rect[3] = ht;
    pc.color[0] = r; pc.color[1] = g; pc.color[2] = b; pc.color[3] = a;
    pc.screen[0] = (float)h->width;
    pc.screen[1] = (float)h->height;
    pc.pad[0] = pc.pad[1] = 0.0f;
    bkpCmdPushConstants(cmd, h->rectPipeline.pipelineLayout.layout,
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

static void setAlphaBlend(VkPipelineColorBlendAttachmentState * att)
{
    att->blendEnable         = VK_TRUE;
    att->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    att->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    att->colorBlendOp        = VK_BLEND_OP_ADD;
    att->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    att->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    att->alphaBlendOp        = VK_BLEND_OP_ADD;
    att->colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                             | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

static void configurePipeline(BkpPipelineGraphicInfo * info, int w, int ht,
                               VkFormat colorFmt, VkFormat depthFmt,
                               BkpShaderProgram * prog, BkpVertexLayout layout)
{
    bkpCreatePipelineMinimalInfo(info, (VkExtent2D){(uint32_t)w, (uint32_t)ht});
    info->shaderProgram         = prog;
    info->colorAttachmentFormat = colorFmt;
    info->depthAttachmentFormat = depthFmt;
    info->vertexLayout          = layout;

    info->depthStencil.depthTestEnable  = VK_FALSE;
    info->depthStencil.depthWriteEnable = VK_FALSE;

    setAlphaBlend(&info->colorAttachments[0]);
    info->colorAttachmentCount = 1;
    info->colorBlending = bkpDefaultPipelineColorBlend(info->colorAttachments, 1);

    info->dynamicStates[0]    = VK_DYNAMIC_STATE_VIEWPORT;
    info->dynamicStates[1]    = VK_DYNAMIC_STATE_SCISSOR;
    info->dynamicState        = bkpDefaultDynamicStates(info->dynamicStates, 2);
    info->dynamicStateEnabled = VK_TRUE;
    info->viewportState       = bkpDefaultPipelineViewport(NULL, 1, NULL, 1);
}

static BkpBool cb_hkey(BkpInputState state, void * data)
{
    if(state.action == eINPUT_PRESSED)
    {
        Hud * h = (Hud *)data;
        h->hPressed = 1;
    }
    return BKP_TRUE;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void hudInit(BkpGpuAdapter adp, Hud * h,
             VkFormat colorFmt, VkFormat depthFmt, int w, int ht,
             const char * shaderDir, const char * fontPath)
{
    h->width  = w;
    h->height = ht;
    h->helpVisible = 0;
    h->hPressed    = 0;
    h->fpsTimer    = 0.0f;
    h->fpsCount    = 0;
    h->fpsValue    = 0.0f;

    bkpAddKeyBoardAction(eKB_H, cb_hkey, h);

    /* --- font sampler ---------------------------------------------------- */
    h->fontSampler.info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    h->fontSampler.info.magFilter    = VK_FILTER_LINEAR;
    h->fontSampler.info.minFilter    = VK_FILTER_LINEAR;
    h->fontSampler.info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    h->fontSampler.info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    h->fontSampler.info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    h->fontSampler.info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    bkpCreateSampler(adp, &h->fontSampler);

    /* --- font: TTF atlas 512×512, 18-px glyphs -------------------------- */
    bkpSetDefaultFontSize(512, 512);
    BkpTrueTypeFontConfig cfg = {NULL, NULL, 0, HUD_FONT_SIZE};
    h->font       = bkpCreateFontFromTrueType(adp, fontPath, &h->fontSampler, &cfg);
    h->lineHeight = bkpGetFontLineHeight(h->font);

    /* --- text shader pipeline -------------------------------------------- */
    char path[512];
    snprintf(path, sizeof(path), "%s/hud.vert.spv", shaderDir);
    bkpCreateShaderModule(adp, path, &h->hudVert);
    snprintf(path, sizeof(path), "%s/hud.frag.spv", shaderDir);
    bkpCreateShaderModule(adp, path, &h->hudFrag);
    BkpShaderModule * hudMods[] = {&h->hudVert, &h->hudFrag};
    bkpCreateShader(adp, hudMods, 2, &h->hudProg);
    bkpCreatePipelineLayoutFromShader(adp, &h->hudProg, &h->hudPipeline.pipelineLayout);

    /* --- rect shader pipeline -------------------------------------------- */
    snprintf(path, sizeof(path), "%s/rect.vert.spv", shaderDir);
    bkpCreateShaderModule(adp, path, &h->rectVert);
    snprintf(path, sizeof(path), "%s/rect.frag.spv", shaderDir);
    bkpCreateShaderModule(adp, path, &h->rectFrag);
    BkpShaderModule * rectMods[] = {&h->rectVert, &h->rectFrag};
    bkpCreateShader(adp, rectMods, 2, &h->rectProg);
    bkpCreatePipelineLayoutFromShader(adp, &h->rectProg, &h->rectPipeline.pipelineLayout);

    /* --- descriptor pool ------------------------------------------------- */
    VkDescriptorPoolSize poolSizes[2] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         (uint32_t)(HUD_TEXT_COUNT * FRAMES)},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (uint32_t)(HUD_TEXT_COUNT * FRAMES)},
    };
    h->descPool.info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    h->descPool.info.maxSets       = (uint32_t)(HUD_TEXT_COUNT * FRAMES);
    h->descPool.info.poolSizeCount = 2;
    h->descPool.info.pPoolSizes    = poolSizes;
    bkpCreateDescriptorPool(adp, &h->descPool);

    /* --- vertex layout for text pipeline: BkpVertexUI ------------------- */
    static VkVertexInputBindingDescription hudBinding = {
        0, (uint32_t)sizeof(BkpVertexUI), VK_VERTEX_INPUT_RATE_VERTEX
    };
    static VkVertexInputAttributeDescription hudAttrs[2] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT,    (uint32_t)offsetof(BkpVertexUI, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(BkpVertexUI, uv)},
    };
    BkpVertexLayout hudLayout   = {hudAttrs, &hudBinding, 2, 1};
    BkpVertexLayout emptyLayout = {NULL, NULL, 0, 0};

    configurePipeline(&h->hudPipeline.info,  w, ht, colorFmt, depthFmt, &h->hudProg,  hudLayout);
    bkpCreatePipelineGraphic(adp, &h->hudPipeline);

    configurePipeline(&h->rectPipeline.info, w, ht, colorFmt, depthFmt, &h->rectProg, emptyLayout);
    bkpCreatePipelineGraphic(adp, &h->rectPipeline);

    /* --- text objects ---------------------------------------------------- */
    h->fpsText  = bkpCreateText(h->font, "FPS: --",
                                &h->descPool, &h->hudProg.layouts[0]);
    h->hintText = bkpCreateText(h->font, "press H to toggle help",
                                &h->descPool, &h->hudProg.layouts[0]);
    h->helpText = bkpCreateText(h->font,
                                "WASD EQ movement\nright-click drag to rotate\nr reset camera",
                                &h->descPool, &h->hudProg.layouts[0]);

    bkpSetTextColor(h->fpsText,  bkpVec4(1.0f, 1.0f, 0.0f, 1.0f));
    bkpSetTextColor(h->hintText, bkpVec4(0.05f, 0.05f, 0.25f, 1.0f));
    bkpSetTextColor(h->helpText, bkpVec4(1.0f, 1.0f, 0.0f, 1.0f));

    for(uint32_t f = 0; f < (uint32_t)FRAMES; ++f)
    {
        uploadTextUBO(adp, h->fpsText,  f);
        uploadTextUBO(adp, h->hintText, f);
        uploadTextUBO(adp, h->helpText, f);
    }
}

void hudUpdate(Hud * h, float dt)
{
    h->fpsTimer += dt;
    h->fpsCount++;
    if(h->fpsTimer >= 0.5f)
    {
        h->fpsValue = (float)h->fpsCount / h->fpsTimer;
        h->fpsTimer = 0.0f;
        h->fpsCount = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "FPS: %.0f", h->fpsValue);
        bkpSetText(h->fpsText, buf);
    }

    if(h->hPressed)
    {
        h->helpVisible = !h->helpVisible;
        h->hPressed    = 0;
    }
}

void hudDraw(BkpGpuAdapter adp, VkCommandBuffer cmd, Hud * h, uint32_t frame)
{
    int lh   = h->lineHeight;
    int pad  = 5;
    int left = 10;
    int top  = 10;
    int bgW  = h->width / 2 + 30;

    bkpCmdBindPipeline(cmd, h->rectPipeline.pipeline);

    drawRect(cmd, h, (float)(left - pad), (float)(top - pad),
             (float)bgW, (float)(2 * lh + 2 * pad),
             0.08f, 0.08f, 0.08f, 0.35f);

    if(h->helpVisible)
    {
        drawRect(cmd, h, (float)(left - pad), (float)(top + 3 * lh - pad),
                 (float)bgW, (float)(6 * lh + 2 * pad),
                 0.08f, 0.08f, 0.08f, 0.35f);
    }

    bkpCmdBindPipeline(cmd, h->hudPipeline.pipeline);
    drawText(adp, cmd, h, h->fpsText,  left, top,          frame);
    drawText(adp, cmd, h, h->hintText, left, top + lh,     frame);
    if(h->helpVisible)
    {
      drawText(adp, cmd, h, h->helpText, left, top + 3 * lh, frame);
    }
}

void hudCleanup(BkpGpuAdapter adp, Hud * h)
{
    bkpRemoveKeyBoardAction(eKB_H);
    bkpDestroyText(h->fpsText);
    bkpDestroyText(h->hintText);
    bkpDestroyText(h->helpText);
    bkpDestroyFont(h->font);
    bkpDestroySampler(adp, &h->fontSampler);
    bkpDestroyDescriptorPool(adp, &h->descPool);
    bkpDestroyGraphicPipeline(adp, &h->hudPipeline);
    bkpDestroyPipelineLayout(adp, &h->hudPipeline.pipelineLayout);
    bkpDestroyShader(adp, &h->hudProg);
    bkpDestroyShaderModule(adp, &h->hudVert);
    bkpDestroyShaderModule(adp, &h->hudFrag);
    bkpDestroyGraphicPipeline(adp, &h->rectPipeline);
    bkpDestroyPipelineLayout(adp, &h->rectPipeline.pipelineLayout);
    bkpDestroyShader(adp, &h->rectProg);
    bkpDestroyShaderModule(adp, &h->rectVert);
    bkpDestroyShaderModule(adp, &h->rectFrag);
}
