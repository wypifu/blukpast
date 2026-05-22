#ifndef BKP_FONT_H
#define BKP_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math/include/bkpmath.h>
#include <gfx/vulkan/include/vulkan.h>
#include "system/include/bkp_bool.h"
#include "system/include/bkp_export.h"
#include <system/include/bkp_string.h>

/*********************************************************************
 * Defines
*********************************************************************/

/*********************************************************************
 * Type def & enum
*********************************************************************/
typedef struct BkpFont_ BkpFont_;
typedef BkpFont_ * BkpFont;

typedef struct BkpText_ BkpText_;
typedef BkpText_ * BkpText;

typedef struct
{
	BkpMat4 transformation;
	BkpVec4 color;
}BkpUniformTextFont;

typedef struct
{
    const char * faceName;
    const int * glySamples;
    size_t glyCount;
    int16_t fontSize;
}BkpTrueTypeFontConfig;

struct BkpText_
{
	BkpFont font;
	BkpBuffer vertexBuffer;
	BkpString text;

	BkpBufferGPU ubo;
	BkpDescriptorSet set;
	BkpVec3 scale;
	BkpVec3 position;

	BkpUniformTextFont data;
	uint32_t vertexCount;
	BkpBool needUpdate;
};


/*********************************************************************
 * Macro
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/

BKP_EXPORTED void bkpSetDefaultFontSize(int w, int h);
BKP_EXPORTED BkpArray(VkImageView) bkpGetFontImageView(BkpFont font);
BKP_EXPORTED BkpFont bkpCreateFontFromBitmap(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler);
BKP_EXPORTED BkpFont bkpCreateFontFromTrueType(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler, BkpTrueTypeFontConfig * config);
BKP_EXPORTED void bkpDestroyFont(BkpFont font);
BKP_EXPORTED BkpBool bkpSaveFont(BkpFont font, const char * path);
BKP_EXPORTED void bkpPrintFont(BkpFont font);

BKP_EXPORTED int     bkpGetFontLineHeight(BkpFont font);
BKP_EXPORTED BkpText bkpCreateText(BkpFont font, const char * txt, BkpDescriptorPool * descriptorPool, BkpDescriptorSetLayout * layout);
BKP_EXPORTED BkpBool bkpSetText(BkpText text, const char * content);
BKP_EXPORTED void bkpSetTextSize(BkpText text, BkpVec3 value);
BKP_EXPORTED void bkpSetTextColor(BkpText text, BkpVec4 value);
BKP_EXPORTED void bkpSetTextPosition(BkpText text, BkpVec3 value);
BKP_EXPORTED void bkpDestroyText(BkpText text);

#ifdef __cplusplus
}
#endif

#endif

