#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>

#include <system/include/bkp_log.h>
#include <system/include/macro.h>
#include <system/include/bkp_allocator.h>
#include <system/include/bkp_array.h>
#include "include/vk_pipeline.h"

/**************************************************************************
 *	Defines & Maro
 **************************************************************************/

/**************************************************************************
 *	Structs, Enum, Unio and Typesdef
 **************************************************************************/

/**************************************************************************
 *	Globals
 **************************************************************************/
static const char * bkpColor = "\x1b[0;38;5;75m";
static const char * bkpTAG = "GFX";

/***************************************************************************
 *	Prototypes
 **************************************************************************/

/**************************************************************************
 *	Implementations
 **************************************************************************/

void bkpCreatePipelineCache(BkpGpuAdapter  adapter, BkpPipelineCache * cache)
{
  if(vkCreatePipelineCache(adapter->device, &cache->info, NULL, &cache->pipelineCache) != VK_SUCCESS)
  {
    LOGC(eFATAL, bkpTAG, bkpColor, "\tUnable to create PipelineCache!");
  }

  LOGC(eDEBUG, bkpTAG, bkpColor, "\tPipelineCache created!");
}

/*___________________________________________________________________*/
void bkpCreateDescriptorPool(BkpGpuAdapter  adapter, BkpDescriptorPool * descpool)
{
  if(vkCreateDescriptorPool(adapter->device, &descpool->info, NULL, &descpool->pool) != VK_SUCCESS)
  {
    LOGC(eFATAL, bkpTAG, bkpColor, "\tUnable to create Descriptor pools!");
  }

  LOGC(eDEBUG, bkpTAG, bkpColor, "\tDescriptor pool created!");
}

/*___________________________________________________________________*/
void bkpCreatePipelineGraphic(BkpGpuAdapter  adapter, BkpPipelineGraphic * ppl)
{
  BkpShaderProgram * shaderProg  = ppl->info.shaderProgram;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = (uint32_t)ppl->info.vertexLayout.vertexBindingCount;
  vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)ppl->info.vertexLayout.vertexAttributeCount;
  if(vertexInputInfo.vertexBindingDescriptionCount > 0)
  {
    vertexInputInfo.pVertexBindingDescriptions      = ppl->info.vertexLayout.vertexBindings;
  }
  if(vertexInputInfo.vertexAttributeDescriptionCount > 0)
  {
    vertexInputInfo.pVertexAttributeDescriptions    = ppl->info.vertexLayout.vertexAttributes;
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount                   = bkpArraySize(shaderProg->stages);
  pipelineInfo.pStages                      = shaderProg->stages;
  pipelineInfo.pVertexInputState            = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState          = &ppl->info.inputAssembly;
  pipelineInfo.pViewportState               = &ppl->info.viewportState;
  pipelineInfo.pRasterizationState          = &ppl->info.rasterizer;
  pipelineInfo.pMultisampleState            = &ppl->info.multisampler;
  pipelineInfo.pDepthStencilState           = ppl->info.depthStencilEnabled ? &ppl->info.depthStencil : NULL;
  pipelineInfo.pColorBlendState             = &ppl->info.colorBlending;
  pipelineInfo.pDynamicState                = ppl->info.dynamicStateEnabled ? &ppl->info.dynamicState : NULL;

  VkPipelineRenderingCreateInfo renderingInfo = {};
  renderingInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingInfo.pNext                   = ppl->info.pNext;
  renderingInfo.colorAttachmentCount    = ppl->info.colorAttachmentCount;
  renderingInfo.pColorAttachmentFormats = &ppl->info.colorAttachmentFormat;
  renderingInfo.depthAttachmentFormat   = ppl->info.depthAttachmentFormat;

  pipelineInfo.pNext              = &renderingInfo;
  pipelineInfo.renderPass         = VK_NULL_HANDLE;
  pipelineInfo.subpass            = 0;
  pipelineInfo.basePipelineHandle = ppl->info.basePipelineHandle;
  pipelineInfo.basePipelineIndex  = ppl->info.basePipelineIndex;
  pipelineInfo.layout             = ppl->pipelineLayout.layout;

  if(vkCreateGraphicsPipelines(adapter->device, ppl->pipelineCache.pipelineCache, 1, &pipelineInfo, NULL, &ppl->pipeline) != VK_SUCCESS)
  {
    LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create graphics pipeline!\n");
  }

  return ;
}

/*___________________________________________________________________*/
void bkpCreatePipelineCompute(BkpGpuAdapter  adapter, BkpPipelineCompute * ppl)
{
  VkComputePipelineCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  info.layout = ppl->pipelineLayout.layout;
  info.stage	= ppl->info.shaderProgram->stages[0];
  //LOGC(eDEBUG, bkpTAG, bkpColor, "stagecount : %u", bkpArraySize(ppl->info.shaderProgram->stages));
  //info.flags = ppl->info.flags;
  //
  if(vkCreateComputePipelines(adapter->device, ppl->pipelineCache.pipelineCache, 1, &info, NULL, &ppl->pipeline) != VK_SUCCESS)
  {
    LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create compute pipeline!\n");
  }

  LOGC(eDEBUG, bkpTAG, bkpColor, "compute pipeline created!\n");
}

/*___________________________________________________________________*/
void bkpCreatePipelineLayout(BkpGpuAdapter  adapter, BkpPipelineLayout * ppll)
{
  if (vkCreatePipelineLayout(adapter->device, &ppll->info, NULL, &ppll->layout) != VK_SUCCESS)
  {
    LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create pipeline layout!\n");
  }
}

/*___________________________________________________________________*/
void bkpCreatePipelineLayoutFromShader(BkpGpuAdapter adapter,
                                       BkpShaderProgram * prog,
                                       BkpPipelineLayout * layout)
{
  /* descriptor set layouts from reflection */
  size_t setCount = prog->layoutCount;
#ifdef _WIN32
  VkDescriptorSetLayout * setLayouts = (VkDescriptorSetLayout *)_alloca(setCount * sizeof(VkDescriptorSetLayout));
#else
  VkDescriptorSetLayout * setLayouts = (VkDescriptorSetLayout *)alloca(setCount * sizeof(VkDescriptorSetLayout));
#endif
  for(size_t i = 0; i < setCount; ++i)
    setLayouts[i] = prog->layouts[i].layout;

  /* push constant ranges — merge stageFlags for ranges at the same offset */
  uint32_t rangeCount = 0;
  VkPushConstantRange ranges[16] = {};
  if(prog->pushConstants != NULL)
  {
    size_t pcCount = bkpArraySize(prog->pushConstants);
    for(size_t i = 0; i < pcCount; ++i)
    {
      BkpPushConstantInfo * pc = &prog->pushConstants[i];
      BkpBool found = BKP_FALSE;
      for(uint32_t r = 0; r < rangeCount; ++r)
      {
        if(ranges[r].offset == (uint32_t)pc->offset)
        {
          ranges[r].stageFlags |= pc->stage;
          found = BKP_TRUE;
          break;
        }
      }
      if(!found && rangeCount < 16)
      {
        ranges[rangeCount].stageFlags = pc->stage;
        ranges[rangeCount].offset     = (uint32_t)pc->offset;
        ranges[rangeCount].size       = (uint32_t)pc->size;
        ++rangeCount;
      }
    }
  }

  VkPipelineLayoutCreateInfo ci = {};
  ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  ci.setLayoutCount         = (uint32_t)setCount;
  ci.pSetLayouts            = setLayouts;
  ci.pushConstantRangeCount = rangeCount;
  ci.pPushConstantRanges    = rangeCount ? ranges : NULL;

  layout->info = ci;
  bkpCreatePipelineLayout(adapter, layout);
}

/*___________________________________________________________________*/
void bkpCreateDescriptorSetLayout(BkpGpuAdapter  adapter, BkpDescriptorSetLayout * descSetLayout)
{
  /* copy user-facing bindings into _info so the caller's array can go out of scope */
  if(descSetLayout->_info == NULL && descSetLayout->bindings != NULL)
  {
    descSetLayout->_info = (VkDescriptorSetLayoutBinding *)
        bkpArrayCreate(VkDescriptorSetLayoutBinding);
    bkpArrayResize(&descSetLayout->_info, descSetLayout->bindingCount);
    for(uint32_t k = 0; k < descSetLayout->bindingCount; ++k)
      descSetLayout->_info[k] = descSetLayout->bindings[k];

    if(descSetLayout->bindingFlags != NULL)
    {
      descSetLayout->_bindingFlags = (VkDescriptorBindingFlags *)
          bkpArrayCreate(VkDescriptorBindingFlags);
      bkpArrayResize(&descSetLayout->_bindingFlags, descSetLayout->bindingCount);
      for(uint32_t k = 0; k < descSetLayout->bindingCount; ++k)
        descSetLayout->_bindingFlags[k] = descSetLayout->bindingFlags[k];
    }
  }

  uint32_t count = 0;
  const VkDescriptorSetLayoutBinding * pBindings = VK_NULL_HANDLE;
  const VkDescriptorBindingFlags *     pFlags    = NULL;

  if(descSetLayout->_info != NULL)
  {
    count     = (uint32_t)bkpArraySize(descSetLayout->_info);
    pBindings = descSetLayout->_info;
    pFlags    = descSetLayout->_bindingFlags;
  }

  VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCI = {};
  BkpBool hasVariableFlags = BKP_FALSE;

  if(pFlags != NULL && count > 0)
  {
    /* pBindings is const — copy to a mutable temp to patch descriptorCount */
#ifdef _WIN32
    VkDescriptorSetLayoutBinding * mutableBindings =
        (VkDescriptorSetLayoutBinding *)_alloca(sizeof(VkDescriptorSetLayoutBinding) * count);
#else
    VkDescriptorSetLayoutBinding * mutableBindings =
        (VkDescriptorSetLayoutBinding *)alloca(sizeof(VkDescriptorSetLayoutBinding) * count);
#endif
    for(uint32_t i = 0; i < count; ++i) mutableBindings[i] = pBindings[i];

    for(uint32_t i = 0; i < count; ++i)
    {
      if(pFlags[i] & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT)
      {
        uint32_t maxDesc = descSetLayout->variableDescriptorMax > 0
                           ? descSetLayout->variableDescriptorMax : 1024;
        mutableBindings[i].descriptorCount = maxDesc;
        hasVariableFlags = BKP_TRUE;
      }
    }
    pBindings = mutableBindings;
    if(hasVariableFlags)
    {
      flagsCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
      flagsCI.bindingCount  = count;
      flagsCI.pBindingFlags = pFlags;
    }
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.pNext        = hasVariableFlags ? &flagsCI : NULL;
  layoutInfo.flags        = 0;
  layoutInfo.bindingCount = (uint32_t)count;
  layoutInfo.pBindings    = pBindings;

  if(vkCreateDescriptorSetLayout(adapter->device, &layoutInfo, adapter->allocator, &descSetLayout->layout) != VK_SUCCESS)
  {
    LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create descriptor set layout!\n");
  }
}

/*____________________________________________________________________________________*/
void bkpCreateDescriptorSet(BkpGpuAdapter  adapter, BkpDescriptorSet * dset)
{
  dset->descriptorSets = (VkDescriptorSet *) bkpAlloc(sizeof(VkDescriptorSet) * dset->count);
  for(size_t i = 0; i < dset->count; ++i)
    bkpAllocDescriptorSet(adapter, dset->descPool, dset->descLayout->layout, &dset->descriptorSets[i]);
}

/*____________________________________________________________________________________*/
void bkpDestroyDescriptorSet(BkpGpuAdapter  adapter, BkpDescriptorSet * dset)
{
  //vkDestroyDe ;
  bkpFree(dset->descriptorSets);
}

/*____________________________________________________________________________________*/
void bkpAllocDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorPool * pool,
                            VkDescriptorSetLayout layout, VkDescriptorSet * outSet)
{
  VkDescriptorSetAllocateInfo ai = {};
  ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  ai.descriptorPool     = pool->pool;
  ai.descriptorSetCount = 1;
  ai.pSetLayouts        = &layout;
  VkResult res = vkAllocateDescriptorSets(adapter->device, &ai, outSet);
  if(res != VK_SUCCESS)
    LOGC(eFATAL, bkpTAG, bkpColor, "bkpAllocDescriptorSet failed [%s]", vkResullt2String(res, 0));
}

/*____________________________________________________________________________________*/
void bkpFreeDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorPool * pool, VkDescriptorSet set)
{
  vkFreeDescriptorSets(adapter->device, pool->pool, 1, &set);
}

/*____________________________________________________________________________________*/
void bkpWriteDescriptorBuffer(BkpGpuAdapter adapter, VkDescriptorSet set,
                               uint32_t binding, VkBuffer buffer,
                               VkDeviceSize offset, VkDeviceSize size)
{
  VkDescriptorBufferInfo bi = { buffer, offset, size };
  VkWriteDescriptorSet wr   = {};
  wr.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  wr.dstSet          = set;
  wr.dstBinding      = binding;
  wr.descriptorCount = 1;
  wr.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  wr.pBufferInfo     = &bi;
  vkUpdateDescriptorSets(adapter->device, 1, &wr, 0, NULL);
}

/*____________________________________________________________________________________*/
void bkpWriteDescriptorImage(BkpGpuAdapter adapter, VkDescriptorSet set,
                              uint32_t binding, VkSampler sampler,
                              VkImageView imageView, VkImageLayout imageLayout)
{
  VkDescriptorImageInfo ii = { sampler, imageView, imageLayout };
  VkWriteDescriptorSet wr  = {};
  wr.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  wr.dstSet          = set;
  wr.dstBinding      = binding;
  wr.descriptorCount = 1;
  wr.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  wr.pImageInfo      = &ii;
  vkUpdateDescriptorSets(adapter->device, 1, &wr, 0, NULL);
}

/*____________________________________________________________________________________*/
void bkpWriteDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorSet * dset,
                           uint32_t setIndex, BkpWriteDescriptorSetInfo * info)
{
  const BkpDescriptorSetLayout * dl = dset->descLayout;
  const VkDescriptorSetLayoutBinding * dlBindings = dl->_info ? dl->_info : dl->bindings;
  uint32_t dwCount = dl->_info ? (uint32_t)bkpArraySize(dl->_info) : dl->bindingCount;
#ifdef _WIN32
  VkWriteDescriptorSet * dw = (VkWriteDescriptorSet *) _alloca(sizeof(VkWriteDescriptorSet) * dwCount);
#else
  VkWriteDescriptorSet * dw = (VkWriteDescriptorSet *) alloca(sizeof(VkWriteDescriptorSet) * dwCount);
#endif
  uint32_t j = 0;
  for(uint32_t c = 0; c < dwCount; c++)
  {
    VkDescriptorSetLayoutBinding binding = dlBindings[c];
    uint32_t cnt = info->setBindings[binding.binding].count;
    if(cnt == 0)
    {
      continue;
    }

    dw[j].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dw[j].pNext           = NULL;
    dw[j].dstSet          = dset->descriptorSets[setIndex];
    dw[j].dstBinding      = binding.binding;
    dw[j].dstArrayElement = 0;
    dw[j].descriptorType  = binding.descriptorType;
    dw[j].descriptorCount = cnt;

    if(dw[j].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
       dw[j].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
       dw[j].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
      dw[j].pBufferInfo      = info->setBindings[binding.binding].ubos;
      dw[j].pImageInfo       = NULL;
      dw[j].pTexelBufferView = NULL;
    }
    else if(dw[j].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            dw[j].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
            dw[j].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
    {
      dw[j].pBufferInfo      = NULL;
      dw[j].pImageInfo       = info->setBindings[binding.binding].imgs;
      dw[j].pTexelBufferView = NULL;
    }
    ++j;
  }
  vkUpdateDescriptorSets(adapter->device, j, dw, 0, NULL);
}

void bkpWriteDescriptorSets(BkpGpuAdapter adapter, BkpDescriptorSet * dset,
                             BkpWriteDescriptorSetInfo * infos)
{
  for(uint32_t i = 0; i < dset->count; ++i)
  {
    bkpWriteDescriptorSet(adapter, dset, i, &infos[i]);
  }
}


/*____________________________________________________________________________________*/
void bkpDestroyPipelineCache(BkpGpuAdapter adapter, BkpPipelineCache * cache)
{
  if(cache->pipelineCache != VK_NULL_HANDLE)
    vkDestroyPipelineCache(adapter->device, cache->pipelineCache, NULL);
}

/*____________________________________________________________________________________*/
void bkpDestroyDescriptorPool(BkpGpuAdapter  adapter, BkpDescriptorPool * dpool)
{
  vkDestroyDescriptorPool(adapter->device, dpool->pool, NULL);
}

/*____________________________________________________________________________________*/
void bkpDestroyDescriptorSetLayout(BkpGpuAdapter  adapter, BkpDescriptorSetLayout * llayout)
{
  vkDestroyDescriptorSetLayout(adapter->device, llayout->layout, NULL);
  if(llayout->_info != NULL)
    bkpArrayDestroy(&llayout->_info);
  if(llayout->_bindingFlags != NULL)
    bkpArrayDestroy(&llayout->_bindingFlags);
}

/*___________________________________________________________________*/
void bkpDestroyGraphicPipeline(BkpGpuAdapter  adapter, BkpPipelineGraphic * pipeline)
{
  vkDestroyPipeline(adapter->device, pipeline->pipeline, NULL);
}

/*___________________________________________________________________*/
void bkpDestroyComputePipeline(BkpGpuAdapter  adapter, BkpPipelineCompute * pipeline)
{
  vkDestroyPipeline(adapter->device, pipeline->pipeline, NULL);
}

/*_________________________________________________________________________________*/
void bkpDestroyPipelineLayout(BkpGpuAdapter  adapter, BkpPipelineLayout * ppll)
{
  vkDestroyPipelineLayout(adapter->device, ppll->layout, NULL);
}

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


/******************** Helper functions ********************************/
/******************** static functions ********************************/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

#ifdef __cplusplus
}
#endif
