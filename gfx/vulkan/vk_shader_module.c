#include "system/include/bkp_string.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/vk_info.h"
#include "include/vk_shader_module.h"
#include "include/vk_pipeline.h"
#include <system/include/macro.h>
#include <system/include/bkp_fs.h>
#include <system/include/bkp_log.h>
#include <thirdparty/SPIRV/include/spirv_reflect.h>
#include <system/include/bkp_allocator.h>

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
static const char * bkpTAG = "bkp_GFX";

/***************************************************************************
*	Prototypes
**************************************************************************/

//void initializeShaderProgram(SShaderProgram * prog);
static void getDescriptorSet(SpvReflectShaderModule * module, BkpShaderModule * sModule);
void generatePushConstantInfo(BkpShaderProgram * shaderProgram, BkpShaderModule ** modules, uint32_t module_count);

static VkShaderStageFlagBits bkpSpvToVkShaderStage(SpvReflectShaderStageFlagBits bit);
static VkDescriptorType bkpSpvToVkDescriptorType(SpvReflectDescriptorType type);

static uint8_t initializeLayoutArray(BkpShaderProgram * shaderProgram, BkpShaderModule ** modules, uint32_t count)
{
	uint8_t hasSet = 0;
	size_t big = 0;

	shaderProgram->layouts = NULL;

	for(uint32_t i = 0; i < count; ++i)
	{
		size_t setCount = bkpArraySize(modules[i]->sets);
		for(size_t j = 0; j < setCount; ++j)
		{
			hasSet = 1;
			if(modules[i]->sets[j].set > big)
			{
				big = modules[i]->sets[j].set;
			}
		}
	}

	if(hasSet)
	{
		++big;
		shaderProgram->layouts = (BkpDescriptorSetLayout *) bkpArrayCreate(BkpDescriptorSetLayout);
		bkpArrayResize(&shaderProgram->layouts, big);
		for(size_t i = 0; i < big; ++i)
		{
			shaderProgram->layouts[i]._info = NULL;
			shaderProgram->layouts[i]._bindingFlags = NULL;
			shaderProgram->layouts[i].variableDescriptorMax = 0;
			shaderProgram->layouts[i].layout = VK_NULL_HANDLE;
		}
		return 1;
	}
	return 0;
}

/**************************************************************************
*	Implementations
**************************************************************************/
static void reflectDescriptorSetLayouts(BkpShaderProgram * shaderProgram, BkpShaderModule ** modules, uint32_t count)
{
	initializeLayoutArray(shaderProgram, modules, count);

	for(uint32_t i = 0; i < count; ++i)
	{
		size_t setCount = bkpArraySize(modules[i]->sets);
		for(size_t j = 0; j < setCount; ++j)
		{
			BkpDescriptorSetRefletion * set = &modules[i]->sets[j];
			BkpDescriptorSetLayout * layout = &shaderProgram->layouts[set->set];

			size_t binCount = bkpArraySize(set->bindings);
			for(size_t k = 0; k < binCount; ++k)
			{
				BkpSetBindingReflection * binding = &set->bindings[k];

				VkDescriptorBindingFlags bFlags = 0;
				if(binding->isRuntimeArray)
					bFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
					         VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

				/* merge stageFlags if binding already exists */
				BkpBool found = BKP_FALSE;
				if(layout->_info != NULL)
				{
					size_t existing = bkpArraySize(layout->_info);
					for(size_t e = 0; e < existing; ++e)
					{
						if(layout->_info[e].binding == binding->binding)
						{
							layout->_info[e].stageFlags |= modules[i]->stage;
							if(layout->_bindingFlags != NULL)
								layout->_bindingFlags[e] |= bFlags;
							found = BKP_TRUE;
							break;
						}
					}
				}
				if(found) continue;

				VkDescriptorSetLayoutBinding desc = (VkDescriptorSetLayoutBinding){0};
				desc.binding            = binding->binding;
				desc.descriptorType     = binding->type;
				desc.stageFlags         = modules[i]->stage;
				desc.descriptorCount    = binding->count;
				desc.pImmutableSamplers = binding->pSamplers;
				if(layout->_info == NULL)
					layout->_info = (VkDescriptorSetLayoutBinding *) bkpArrayCreate(VkDescriptorSetLayoutBinding);
				bkpArrayPush(&layout->_info, desc);

				if(layout->_bindingFlags == NULL)
					layout->_bindingFlags = (VkDescriptorBindingFlags *) bkpArrayCreate(VkDescriptorBindingFlags);
				bkpArrayPush(&layout->_bindingFlags, bFlags);
			}
		}
	}

	if(shaderProgram->layouts != NULL)
		shaderProgram->layoutCount = (uint16_t)bkpArraySize(shaderProgram->layouts);
}

void bkpFinalizeShaderLayouts(BkpGpuAdapter adapter, BkpShaderProgram * shaderProgram)
{
	if(shaderProgram->layouts == NULL) return;
	size_t layoutCount = bkpArraySize(shaderProgram->layouts);
	for(size_t i = 0; i < layoutCount; ++i)
		bkpCreateDescriptorSetLayout(adapter, &shaderProgram->layouts[i]);
}

void bkpReflectShader(BkpGpuAdapter adapter, BkpShaderModule ** modules, uint32_t module_count, BkpShaderProgram * shaderProgram)
{
	(void)adapter;
	shaderProgram->stages = (VkPipelineShaderStageCreateInfo *) bkpArrayCreate(VkPipelineShaderStageCreateInfo);
	bkpArrayReserve(&shaderProgram->stages, module_count);
	for(size_t i = 0; i < module_count; ++i)
	{
		VkPipelineShaderStageCreateInfo stage = {};
		stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.pName  = modules[i]->entryPoint;
		stage.stage  = modules[i]->stage;
		stage.module = modules[i]->shaderModule;
		bkpArrayPush(&shaderProgram->stages, stage);
	}
	shaderProgram->autoDestroyModules = BKP_FALSE;
	reflectDescriptorSetLayouts(shaderProgram, modules, module_count);
	generatePushConstantInfo(shaderProgram, modules, module_count);
}

void bkpShaderSetImmutableSamplers(BkpShaderProgram * prog, uint32_t set, uint32_t binding, const VkSampler * samplers)
{
	if(prog->layouts == NULL || set >= prog->layoutCount) return;
	BkpDescriptorSetLayout * layout = &prog->layouts[set];
	if(layout->_info == NULL) return;
	size_t count = bkpArraySize(layout->_info);
	for(size_t i = 0; i < count; ++i)
	{
		if(layout->_info[i].binding == binding)
		{
			layout->_info[i].pImmutableSamplers = samplers;
			return;
		}
	}
}

void bkpShaderSetVariableDescriptorMax(BkpShaderProgram * prog, uint32_t set, uint32_t maxCount)
{
	if(prog->layouts == NULL || set >= prog->layoutCount) return;
	prog->layouts[set].variableDescriptorMax = maxCount;
}
/*_________________________________________________________________________________*/
void generatePushConstantInfo(BkpShaderProgram * shaderProgram, BkpShaderModule ** modules, uint32_t module_count)
{
  shaderProgram->pushConstants = NULL;
  for(size_t i = 0; i < module_count; ++i)
  {
    if(modules[i]->pushConstants != NULL)
    {
      if(shaderProgram->pushConstants == NULL)
      {
        shaderProgram->pushConstants = (BkpPushConstantInfo *) bkpArrayCreate(BkpPushConstantInfo);
      }
      for(size_t constIdx = 0; constIdx < bkpArraySize(modules[i]->pushConstants); ++constIdx)
      {
        bkpArrayPush(&shaderProgram->pushConstants, modules[i]->pushConstants[constIdx]);
      }
    }
    
  }

}
/*_________________________________________________________________________________*/
void bkpDebugShaderConstants(BkpShaderProgram * prog)
{
  if(prog->pushConstants == NULL) return;

  size_t t = bkpArraySize(prog->pushConstants);
  fprintf(stdout, "Constant : %zu{\n", t);
  for(size_t i = 0; i < t; ++i)
  {
    fprintf(stdout, "\t{%s, %s, size : %zu, offset : %zu}\n",
					bkpVkShaderStage2String(prog->pushConstants[i].stage),
          prog->pushConstants[i].name,
          prog->pushConstants[i].size,
          prog->pushConstants[i].offset);
  }

}

/*_________________________________________________________________________________*/
void bkpDebugShaderLayouts(BkpShaderProgram * shaderProgram)
{
	size_t layoutCount = bkpArraySize(shaderProgram->layouts);
	LOGC(eDEBUG, bkpTAG, bkpColor, "\tlayoutCount(%zu)", layoutCount);
	for(size_t i = 0; i < layoutCount; ++i)
	{
		BkpDescriptorSetLayout * layout = &shaderProgram->layouts[i];
		fprintf(stdout, "Set : %zu{\n", i);
		for(size_t j = 0; j < bkpArraySize(layout->_info); ++j)
		{
			VkDescriptorSetLayoutBinding * binding = &layout->_info[j];
			fprintf(stdout, "\t{\n\t\tbinding: %u\n\t\ttype: %s\n\t\tcount: %u\n\t\tstage: %u\n\t\t%s\n\t\tpSample:%p\n\t}\n",
					binding->binding,
					bkpVkDescriptorType2String(binding->descriptorType),
					binding->descriptorCount,
					binding->stageFlags,
					bkpVkShaderStage2String(binding->stageFlags),
					binding->pImmutableSamplers);
		}
		fprintf(stdout, "}\n");
	}
}
/*_________________________________________________________________________________*/
void bkpDebugDescriptorLayout(BkpDescriptorSetLayout * layout)
{
	for(size_t j = 0; j < bkpArraySize(layout->_info); ++j)
	{
		VkDescriptorSetLayoutBinding * binding = &layout->_info[j];
		fprintf(stdout, "\t{\n\t\tbinding: %u\n\t\ttype: %s\n\t\tcount: %u\n\t\tstage: %u\n\t\t%s\n\t\tpSample:%p\n\t}\n",
				binding->binding,
				bkpVkDescriptorType2String(binding->descriptorType),
				binding->descriptorCount,
				binding->stageFlags,
				bkpVkShaderStage2String(binding->stageFlags),
				binding->pImmutableSamplers);
	}
	fprintf(stdout, "}\n");
}

/*_________________________________________________________________________________*/
BkpBool bkpCreateShaderModule(BkpGpuAdapter  adapter, const char * szPath, BkpShaderModule * sModule)
{
	size_t len;

	LOGC(eDEBUG, bkpTAG, bkpColor, "loading '%s'!", szPath);
	char * code_src = bkp_loadBinary(szPath, &len);

	if(NULL == code_src)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to load shaders bytecode at '%s'!\n", szPath);
	}

	bkpCreateShaderModuleFromMemory(adapter, code_src, (uint32_t)len, sModule);
	bkpFree(code_src);

	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
BkpBool bkpCreateShaderModuleFromMemory(BkpGpuAdapter adapter, const char * src, size_t src_len, BkpShaderModule * sModule)
{
	SpvReflectShaderModule module = {};
	SpvReflectResult result = spvReflectCreateShaderModule(src_len, src, &module);
	if(result != SPV_REFLECT_RESULT_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create reflection\n");
	}

	getDescriptorSet(&module, sModule);
	spvReflectDestroyShaderModule(&module);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = src_len;
	createInfo.pCode = (const uint32_t *) src;

	//VkShaderModule shaderModule;

	if(vkCreateShaderModule(adapter->device, &createInfo, adapter->allocator, &sModule->shaderModule) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create shader module!\n");
	}

	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
BkpBool bkpCreateShader(BkpGpuAdapter  adapter, BkpShaderModule ** modules, uint32_t module_count, BkpShaderProgram * shaderProgram)
{
	bkpReflectShader(adapter, modules, module_count, shaderProgram);
	bkpFinalizeShaderLayouts(adapter, shaderProgram);
	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
void bkpDestroyConstantInfo(BkpArray(BkpPushConstantInfo) * info) 
{
  BkpArray(BkpPushConstantInfo) pCons = *info;

  if(pCons == NULL) return;

  size_t constantCount = bkpArraySize(pCons);
  if(constantCount > 0)
  {
    for(size_t i = 0; i < constantCount; ++i)
    {
      bkpStringDestroy(&pCons[i].name);
    }
    bkpArrayDestroy(&pCons);
    *info = NULL;
  }

}

/*_________________________________________________________________________________*/
void bkpDestroyShaderModule(BkpGpuAdapter  adapter, BkpShaderModule * smodule)
{
	vkDestroyShaderModule(adapter->device, smodule->shaderModule, adapter->allocator);
	size_t s = bkpArraySize(smodule->sets);
	for(size_t i = 0; i < s; ++i)
	{
		if(smodule->sets[i].bindings != NULL)
		{
			bkpArrayDestroy(&smodule->sets[i].bindings);
		}
	}
	bkpStringDestroy(&smodule->entryPoint);
	bkpArrayDestroy(&smodule->sets);

  bkpDestroyConstantInfo(&smodule->pushConstants);
}

/*_________________________________________________________________________________*/
void bkpDestroyShader(BkpGpuAdapter  adapter,  BkpShaderProgram * shaderProgram)
{
	bkpArrayDestroy(&shaderProgram->stages);
	if(shaderProgram->layouts != NULL)
	{
		size_t layoutCount = bkpArraySize(shaderProgram->layouts);
		for(size_t i = 0; i < layoutCount; ++i)
		{
			bkpDestroyDescriptorSetLayout(adapter, &shaderProgram->layouts[i]);
		}
		bkpArrayDestroy(&shaderProgram->layouts);
	}
  if(shaderProgram->pushConstants)
  {
    bkpArrayDestroy(&shaderProgram->pushConstants);
  }
}

/*_________________________________________________________________________________*/
VkSpecializationInfo bkpCreateSpecializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
{
	VkSpecializationInfo specializationInfo = {};
	specializationInfo.mapEntryCount = mapEntryCount;
	specializationInfo.pMapEntries = mapEntries;
	specializationInfo.dataSize = dataSize;
	specializationInfo.pData = data;
	return specializationInfo;
}

/*_________________________________________________________________________________*/
VkSpecializationMapEntry bkpCreateSpecializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
{
	VkSpecializationMapEntry specializationMapEntry = {};
	specializationMapEntry.constantID = constantID;
	specializationMapEntry.offset = offset;
	specializationMapEntry.size = size;
	return specializationMapEntry;
}

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


/******************** static functions ********************************/


/*_________________________________________________________________________________*/
static VkShaderStageFlagBits bkpSpvToVkShaderStage(SpvReflectShaderStageFlagBits bit)
{
	switch(bit)
	{
		case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT : return VK_SHADER_STAGE_VERTEX_BIT;
		case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT : return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT : return VK_SHADER_STAGE_FRAGMENT_BIT;
		case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT : return VK_SHADER_STAGE_COMPUTE_BIT;
		case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV : return VK_SHADER_STAGE_TASK_BIT_NV;
		case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV : return VK_SHADER_STAGE_MESH_BIT_NV;
		case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR : return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR : return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR : return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR : return VK_SHADER_STAGE_MISS_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR : return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR : return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
	}

	return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

/*_________________________________________________________________________________*/
static VkDescriptorType bkpSpvToVkDescriptorType(SpvReflectDescriptorType type)
{
	switch(type)
	{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER                    : return VK_DESCRIPTOR_TYPE_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER     : return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE              : return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE              : return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER       : return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER       : return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER             : return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER             : return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC     : return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC     : return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT           : return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR : return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	}

	return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

/*_________________________________________________________________________________*/
static void getDescriptorSet(SpvReflectShaderModule * module, BkpShaderModule * sModule)
{
	uint32_t count = 0;
	SpvReflectResult result = spvReflectEnumerateDescriptorSets(module, &count, NULL);

/*
	{
		uint32_t vCount = 0;
		spvReflectEnumerateInputVariables(module, &vCount, nullptr);
		BkpArray(SpvReflectInterfaceVariable *) vars= (SpvReflectInterfaceVariable**)bkpArrayCreate(SpvReflectInterfaceVariable*);
		bkpArrayResize(&vars, vCount);
		spvReflectEnumerateInputVariables(module, &vCount, vars);

		for(size_t i = 0; i < vCount; ++i)
		{
			LOGC(eERROR, bkpTAG, bkpColor,
				"\t#%lu, '%s', location %lu",
				 i, vars[i]->name, vars[i]->location);
		}

	}
	*/


	if(result != SPV_REFLECT_RESULT_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to Enumerate DescriptorSet reflection\n");
	}

	BkpArray(SpvReflectDescriptorSet *) sets = (SpvReflectDescriptorSet **)bkpArrayCreate(SpvReflectDescriptorSet *);
	bkpArrayResize(&sets, count);
	result = spvReflectEnumerateDescriptorSets(module, &count, sets);
	if(result != SPV_REFLECT_RESULT_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to get DescriptorSet reflection\n");
	}

	size_t setCount = bkpArraySize(sets);
#if defined(SPIRV_REFLECT_HAS_VULKAN_H)
	BkpArray(DescriptorSetLayoutData) set_layouts = (DescriptorSetLayoutData*)bkpArrayCreate(DescriptorSetLayoutData);
	bkpArrayResize(&set_layouts, setCount);
	for (size_t i_set = 0; i_set < setCount; ++i_set)
	{
		const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
		DescriptorSetLayoutData& layout = set_layouts[i_set];
		layout.bindings.resize(refl_set.binding_count);
		for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding)
		{
			const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
			VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
			layout_binding.binding = refl_binding.binding;
			layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
			layout_binding.descriptorCount = 1;
			for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim)
			{
				layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
			}
			layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(module->shader_stage);
		}
		layout.set_number = refl_set.set;
		layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout.create_info.bindingCount = refl_set.binding_count;
		layout.create_info.pBindings = layout.bindings.data();
	}
	bkpArrayDestroy(&setLayouts);
#endif

	sModule->stage = bkpSpvToVkShaderStage(module->shader_stage);
  sModule->pushConstants = NULL;
  if(module->push_constant_block_count > 0)
  {
    sModule->pushConstants = (BkpPushConstantInfo *) bkpArrayCreate(BkpPushConstantInfo);
    bkpArrayResize(&sModule->pushConstants, module->push_constant_block_count);

    for(int i = 0; i < module->push_constant_block_count; ++i)
    {
      sModule->pushConstants[i].offset = module->push_constant_blocks[i].offset;
      sModule->pushConstants[i].size = module->push_constant_blocks[i].size;
      sModule->pushConstants[i].name = bkpStringCreate(module->push_constant_blocks[i].name);
      sModule->pushConstants[i].stage = sModule->stage;
      /*
      for(int j = 0; j < module->push_constant_blocks[0].member_count; ++j)
      {
        SpvReflectBlockVariable * var = module->push_constant_blocks[i].members;

        BkpString typename = getTypeOfShaderVar(var[j]);
        fprintf(stderr, "\t%s : %s\n", typename, var[j].name);
        bkpStringDestroy(&typename);
      }
      */
    }
  }
  


	sModule->entryPoint = bkpStringCreate(module->entry_point_name);
	sModule->sets = (BkpDescriptorSetRefletion *) bkpArrayCreate(BkpDescriptorSetRefletion);
	bkpArrayResize(&sModule->sets, setCount);

	for (size_t index = 0; index < setCount; ++index)
	{
		SpvReflectDescriptorSet * p_set = sets[index];

		// descriptor sets can also be retrieved directly from the module, by set index
		const SpvReflectDescriptorSet * p_set2 = spvReflectGetDescriptorSet(module, p_set->set, &result);
		if(result != SPV_REFLECT_RESULT_SUCCESS)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Unable to get DescriptorSet after reflection\n");
		}

		if(p_set != p_set2)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Pset not == Pset2\n");
		}
		(void)p_set2;

		sModule->sets[index] = (BkpDescriptorSetRefletion){};
		BkpDescriptorSetRefletion * setReflection = &sModule->sets[index];
		setReflection->set = p_set->set;
		setReflection->bindings = (BkpSetBindingReflection*) bkpArrayCreate(BkpSetBindingReflection);
		bkpArrayResize(&setReflection->bindings, p_set->binding_count);

		for (uint32_t i = 0; i < p_set->binding_count; ++i)
		{
			BkpSetBindingReflection bindReflection = {};
			SpvReflectDescriptorBinding * binding = p_set->bindings[i];

			/*
			fprintf(stderr, "\n*** : set(%u, %u, %u) \n", binding->set, binding->binding, binding->count);
			if(binding->count == 0xFFFFFFFF)
			{
				fprintf(stderr, "\t\tArray : %u, '%u', '%u'\n", binding->array.dims_count,
				binding->block.array.dims[0], binding->uav_counter_id);
			}
			*/
			bindReflection.binding = binding->binding;
			bindReflection.type = bkpSpvToVkDescriptorType(binding->descriptor_type);
			bindReflection.pSamplers = NULL;

			/* detect runtime (unsized) arrays — SpvReflect sets count=0 for them */
			BkpBool isRuntime = (binding->count == 0);
			if(!isRuntime && binding->array.dims_count > 0)
			{
				for(uint32_t d = 0; d < binding->array.dims_count; ++d)
					if(binding->array.dims[d] == 0) { isRuntime = BKP_TRUE; break; }
			}
			bindReflection.isRuntimeArray = isRuntime;
			bindReflection.count = isRuntime ? 0 : binding->count;

			setReflection->bindings[i] = bindReflection;

			/*
			if (binding->array.dims_count > 0)
			{
				for (uint32_t dim_index = 0; dim_index < binding->array.dims_count; ++dim_index) {
					fprintf(stdout, "[%u]", binding->array.dims[dim_index]);
				}
			}

			// counter
			if (binding->uav_counter_binding != NULL) {
				fprintf(stdout, "Counter : ");
				fprintf(stdout, "(");
				fprintf(stdout, "set=%u,", binding->uav_counter_binding->set);
				fprintf(stdout, "binding=%u,", binding->uav_counter_binding->binding);
				fprintf(stdout, "name=%s);\n", binding->uav_counter_binding->name);
			}

			fprintf(stdout, "set %lu binding %lu variable name : %s", p_set->set, binding->binding, binding->name);
			if ((binding->type_description->type_name != NULL) && (strlen(binding->type_description->type_name) > 0)) {
				fprintf(stdout, " ( %s)", binding->type_description->type_name );
			}
			fprintf(stdout, "\n");
			*/

		}
	}
	bkpArrayDestroy(&sets);
}

/*_________________________________________________________________________________*/
BkpString getTypeOfShaderVar(SpvReflectBlockVariable var)
{
  int f = var.type_description->type_flags;
  int f0 = f - SPV_REFLECT_TYPE_FLAG_VECTOR;
  int f1 = f - SPV_REFLECT_TYPE_FLAG_MATRIX;
  fprintf(stderr, "Type_flags %d [%d, %d] %d '%u'\n", f, f0, f1, var.decoration_flags, var.size);
  fprintf(stderr, "scalar %d, %d\n", var.numeric.scalar.width, var.numeric.scalar.signedness);
  fprintf(stderr, "vec%d - ", var.numeric.vector.component_count);
  fprintf(stderr, "mat %d %d %d\n", var.numeric.matrix.column_count, var.numeric.matrix.row_count, var.numeric.matrix.row_count);

  if(var.type_description->type_flags == SPV_REFLECT_TYPE_FLAG_VOID)   return bkpStringCreate("void");
  if(var.type_description->type_flags == SPV_REFLECT_TYPE_FLAG_BOOL)   return bkpStringCreate("bool");
  if(var.type_description->type_flags == SPV_REFLECT_TYPE_FLAG_INT)    return bkpStringCreate("int");
  if(var.type_description->type_flags == SPV_REFLECT_TYPE_FLAG_FLOAT)  return bkpStringCreate("float");

  if(var.type_description->type_flags == SPV_REFLECT_TYPE_FLAG_VECTOR)
  {
    char tn[5] = {0};
    snprintf(tn, 5, "vec%d", var.numeric.vector.component_count);
    return bkpStringCreate(tn);
  }
  if(var.type_description->type_flags == SPV_REFLECT_TYPE_FLAG_MATRIX)
  {
    char tn[5] = {0};
    snprintf(tn, 5, "mat%d", var.numeric.matrix.column_count);
    return bkpStringCreate(tn);
  }

  return bkpStringCreate("undefined");
}

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

#ifdef __cplusplus
}
#endif
