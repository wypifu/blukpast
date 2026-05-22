#ifdef __cplusplus
extern "C" {
#endif

#include "include/pipelines_helper.h"
/*Copyright*/
/*
 * AUTHOR: lilington
 * addr	: lilington@yahoo.fr
 *
 */

/**************************************************************************
 * *	Defines & Maro
**************************************************************************/

/**************************************************************************
*	Structs, Enum, Union and Typesdef
**************************************************************************/

/**************************************************************************
*	Globals
**************************************************************************/

/**************************************************************************
*	Classes
**************************************************************************/

/***************************************************************************
*	Prototypes
**************************************************************************/
/**************************************************************************
*	Implementations
**************************************************************************/

VkPipelineInputAssemblyStateCreateInfo bkpDefaultPipelineInputAssembly(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType                     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.flags						= 0;
	inputAssembly.topology                  = topology;
	inputAssembly.primitiveRestartEnable    = VK_FALSE;

	return inputAssembly;
}

/*_________________________________________________________________________________*/
VkViewport bkpDefaultViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
{
    VkViewport viewport = {};
    viewport.x = x;
    viewport.y = y;
    viewport.width = w;
    viewport.height= h;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;

	return viewport;
}

/*_________________________________________________________________________________*/
VkRect2D bkpDefaultRect2D(VkOffset2D offset, VkExtent2D dim)
{
    VkRect2D scissor = {};
    scissor.offset  =  offset;
    scissor.extent  =  dim;

	return scissor;
}

/*_________________________________________________________________________________*/
VkPipelineViewportStateCreateInfo bkpDefaultPipelineViewport(const VkViewport * viewports , uint32_t vCount, const VkRect2D * scissors, uint32_t sCount)
{

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = vCount;
    viewportState.pViewports = viewports;
    viewportState.scissorCount = sCount;
    viewportState.pScissors = scissors;

	return viewportState;
}

/*_________________________________________________________________________________*/
VkPipelineRasterizationStateCreateInfo bkpDefaultPipelineRasterization(VkPolygonMode poly, VkCullModeFlags cull, VkFrontFace face, VkPipelineRasterizationStateCreateFlags flags)
{
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext                   = NULL;
    rasterizer.flags                   = flags;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = poly;
    rasterizer.cullMode                = cull;
    rasterizer.frontFace               = face;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp          = 0.0f;
    rasterizer.depthBiasSlopeFactor    = 0.0f;
    rasterizer.lineWidth               = 1.0f;

	return rasterizer;
}

/*_________________________________________________________________________________*/
VkPipelineMultisampleStateCreateInfo bkpDefaultPipelineMultisample(VkBool32 sampleShadingEnabled, VkSampleCountFlagBits samples, float minSampleShading)
{
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext                                = NULL;
	multisampling.flags                                = 0;
	multisampling.sampleShadingEnable                  = sampleShadingEnabled;
	multisampling.rasterizationSamples                 = samples;
	multisampling.minSampleShading                     = minSampleShading;
	multisampling.pSampleMask                          = NULL;
	multisampling.alphaToCoverageEnable                = VK_FALSE;
	multisampling.alphaToOneEnable                     = VK_FALSE;

	return multisampling;
}

/*_________________________________________________________________________________*/
VkPipelineColorBlendAttachmentState bkpDefaultPipelineColorBlendAttachement()
{
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable         = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask    = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	return colorBlendAttachment;
}

/*_________________________________________________________________________________*/
VkPipelineColorBlendStateCreateInfo bkpDefaultPipelineColorBlend(const VkPipelineColorBlendAttachmentState * attachments, uint32_t count)
{
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext             = NULL;
    colorBlending.flags             = 0;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount   = count;
    colorBlending.pAttachments      = attachments;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

	return colorBlending;
}

/*_________________________________________________________________________________*/
VkPipelineDepthStencilStateCreateInfo bkpDefaultPipelineDepthStenscil(VkBool32 testEnabled, VkBool32 writeEnabled, VkCompareOp op)
{
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.pNext                 = NULL;
	depthStencil.flags                 = 0;
	depthStencil.depthTestEnable       = testEnabled;
	depthStencil.depthWriteEnable      = writeEnabled;
	depthStencil.depthCompareOp        = op;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.minDepthBounds        = 0.0f;
	depthStencil.maxDepthBounds        = 1.0f;
	depthStencil.front                 = (VkStencilOpState){};
	depthStencil.back                  = (VkStencilOpState){};

	return depthStencil;
}

/*_________________________________________________________________________________*/
VkPipelineDynamicStateCreateInfo bkpDefaultPipelineDynamicState(const VkDynamicState * dstates, uint32_t count, VkPipelineDynamicStateCreateFlags flags)
{
	VkPipelineDynamicStateCreateInfo dyn;
	dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dyn.pNext             = NULL;
	dyn.flags             = flags;
	dyn.dynamicStateCount = count;
	dyn.pDynamicStates    = dstates;

	return dyn;
}

/*_________________________________________________________________________________*/
VkPushConstantRange bkpDefaultPipelinePushConstant(VkShaderStageFlags stage, uint32_t offset, uint32_t size)
{
    VkPushConstantRange range;
    range.stageFlags = stage;
    range.offset     = offset;
    range.size       = size;

    return range;
}

/*_________________________________________________________________________________*/
VkPipelineLayoutCreateInfo bkpDefaultPipelineLayout(VkDescriptorSetLayout * layouts, uint32_t count)
{
	VkPipelineLayoutCreateInfo pplDesc = {};
	pplDesc.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pplDesc.pNext                  = NULL;
	pplDesc.flags                  = 0;
	pplDesc.setLayoutCount         = count;
	pplDesc.pSetLayouts            = layouts;
	pplDesc.pushConstantRangeCount = 0;
	pplDesc.pPushConstantRanges    = NULL;

	return pplDesc;
}

VkDescriptorPoolCreateInfo bkpDefaultDescriptorPool(VkDescriptorPoolSize * poolSizes, uint32_t poolSizeCount, uint32_t maxSet)
{
	VkDescriptorPoolCreateInfo desc = {};
	desc.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desc.pNext         = NULL;
	desc.flags         = 0;
	desc.poolSizeCount = poolSizeCount;
	desc.pPoolSizes    = poolSizes;
	desc.maxSets       = maxSet;

	return desc;
}

/*_________________________________________________________________________________*/
void bkpCreatePipelineMinimalInfo(BkpPipelineGraphicInfo * pplInfo, VkExtent2D extent)
{
	*pplInfo = (BkpPipelineGraphicInfo){};
	//pplInfo->viewports.push_back(bkp::gfx::bkpDefaultViewport(bkp::Vec2(0.0f, 0.0f), dimension, 0.0f, 1.0f));
	//pplInfo->scissors.push_back(bkp::gfx::bkpDefaultScissor(bkp::Vec2i(0, 0), dimension));
	//pplInfo->viewportState       = bkp::gfx::bkpDefaultPipelineViewport(pplInfo.viewports.data(), static_cast<uint32_t>(pplInfo.viewports.size()), pplInfo.scissors.data(), static_cast<uint32_t>(pplInfo.scissors.size()));
	pplInfo->inputAssembly       = bkpDefaultPipelineInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pplInfo->rasterizer          = bkpDefaultPipelineRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pplInfo->multisampler        = bkpDefaultPipelineMultisample(VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.0f);
	pplInfo->colorAttachments[0] = bkpDefaultPipelineColorBlendAttachement();
	pplInfo->colorAttachmentCount = 1;
	pplInfo->colorBlending       = bkpDefaultPipelineColorBlend(pplInfo->colorAttachments, pplInfo->colorAttachmentCount);
	pplInfo->depthStencil        = bkpDefaultPipelineDepthStenscil(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	pplInfo->depthStencilEnabled = VK_TRUE;
	pplInfo->dynamicStateEnabled = VK_FALSE;
}

/*_________________________________________________________________________________*/
VkPipelineDynamicStateCreateInfo bkpDefaultDynamicStates(VkDynamicState * st, size_t count)
{
	VkPipelineDynamicStateCreateInfo info = (VkPipelineDynamicStateCreateInfo){};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	info.dynamicStateCount = count;
	info.pDynamicStates = st;
	info.pNext = NULL;
	info.flags = 0;
	return info;
}

/*_________________________________________________________________________________*/


#ifdef __cplusplus
}
#endif

