#include "math/include/bkp_vect.h"
#include "system/include/bkp_array_f.h"
#include "system/include/bkp_bool.h"
#include "system/include/bkp_string.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>


#include "include/bkp_font.h"
#include <include/type.h>
#include <system/include/bkp_allocator.h>
#include <system/include/bkp_array.h>
#include <system/include/bkp_map.h>
#include <system/include/bkp_fs.h>
#include <system/include/bkp_log.h>
#include <gfx/vulkan/include/vkAllocator.h>
#include <gfx/vulkan/include/vk_pipeline.h>
#include <system/include/bkp_utf8.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include <thirdparty/stb/stb_truetype.h>
#pragma GCC diagnostic pop

/**************************************************************************
*	Defines & Maro
**************************************************************************/

#define DEFAULT_BKFT_NAME "InGa Engine 1.0, wypifu"
#define BKFT_MAGIC_NUMBER 0xABABA
#define DEFAULT_POINT_COUNT 96
#define DEFAULT_FONT_SIZE 32
/* Extra empty atlas pages pre-allocated for on-demand TTF glyph growth. */
#define FONT_RESERVE_PAGES 2

#define CHECK_FNT(returnValue, line, linenum)\
{\
	if(returnValue != BKP_TRUE)\
	{\
		LOGC(eERROR, gTAG, gColor, "Error in fnt line %lu : '%s'", linenum, line);\
		goto error_exit;\
	}\
}

#define CHECK_WRITE_FILE(expected, actual)\
{\
	if(actual != expected)\
	{\
		LOGC(eERROR, gTAG, gColor, "Error writing Expected %d element(s) but found %d", expected, actual);\
		goto clean_exit;\
		return BKP_FALSE;\
	}\
}

/**************************************************************************
*	Structs, Enum, Unio and Typesdef
**************************************************************************/

typedef enum {eFONT_BITMAP, eFONT_TTF, eFONT_COUNT} BkpFontType;

typedef struct
{
	char name[64];
	uint32_t magic;
	uint8_t type;
	uint8_t major;
	uint8_t minor;
	uint8_t build;
}BkftHeader;

typedef struct
{
	int codepoint;
	int16_t x;
	int16_t y;
	int16_t width;
	int16_t height;
	int16_t xOffset;
	int16_t yOffset;
	int16_t xAdvance;
	uint8_t pageId;
	uint8_t channel;
}SFontGlyph;

typedef struct
{
	int32_t codepoint_0;
	int32_t codepoint_1;
	int8_t  amount;
}SFontKerning;

struct BkpFont_
{
	BkpString face;
	BkpString charset;
	uint32_t size;
	uint32_t textureWidth;
	uint32_t textureHeight;
	int32_t lineHeight;
	int32_t baseline;
	int32_t tabXadance;
	int32_t stretchH;
	uint8_t smooth;
	uint8_t aa;
	int8_t padding[4];
	uint8_t spacing[2];
	uint8_t outline;
	uint8_t packed;
	uint8_t alpha;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t bold;
	uint8_t italic;
	uint8_t unicode;
	uint8_t useBaseLine;
	BkpBool hasTexture;
	BkpFontType eType;
	BkpImageResource textures;
	BkpSampler * sampler;
    BkpGpuAdapter adapter;
	BkpString sourceDir;
	BkpMap texturesPath;
	BkpMap glyphs;
	BkpArray(SFontKerning) kernings;

	/* TTF on-demand glyph growth (all zeroed for bitmap fonts).
	 * ttfBuffer is kept alive after creation so missing glyphs can be packed
	 * into pre-allocated reserve pages without recreating the GPU texture. */
	unsigned char  * ttfBuffer;
	stbtt_fontinfo   ttfInfo;
	int              ttfFontIndex;
	int              ttfFontSize;
	int              ttfPageCap;        /* max glyphs per page      */
	int              ttfUsedPages;      /* pages with glyph data    */
	int              ttfReservedPages;  /* empty pages still usable */
};




/**************************************************************************
*	Globals
**************************************************************************/
static const char * gTAG 	 = "bkp_font";
static const char * gColor = "\x1b[0; 31m";
static const int32_t gDefaultCodePoint[DEFAULT_POINT_COUNT] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};
static int gFontWidth = 512;
static int gFontHeight = 512;

/***************************************************************************
*	Prototypes
**************************************************************************/
static BkpFont import_bkft(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler);
static BkpFont import_fnt(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler);
static BkpBool growFontAtlas(BkpFont font, int codepoint);
static char * skipWord(char * p);
static BkpBool getKeyValue(char * p, char * key, char * value);
static BkpBool get_fnt_char_line(char * line, BkpFont font);
static BkpBool get_fnt_common_line(char * line, BkpFont font, uint8_t * pages);
static BkpBool get_fnt_info_line(char * line, BkpFont font);
static BkpBool get_fnt_kerning_line(char * line, BkpFont font);
static char * readBuffer(char * buff, char * result, size_t size);
static char * readBufferString(char * buff, char * word, size_t count);
//static void print_glyph(SFontGlyph gly);

/**************************************************************************
*	Implementations
**************************************************************************/
void bkpSetDefaultFontSize(int w, int h)
{
    gFontWidth  = w;
    gFontHeight = h;
}

/*_________________________________________________________________________________*/
BkpArray(VkImageView) bkpGetFontImageView(BkpFont font)
{
    return font->textures.imageViews;
}

/*_________________________________________________________________________________*/
BkpFont bkpCreateFontFromBitmap(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler)
{
	LOGC(eDEBUG, gTAG, gColor, "Loading font '%s'", path);

	const char * szExt = bkp_getFileExtension(path);

	if(strcmp(szExt, "bkft") == 0)
	{
		return import_bkft(adapter, path, sampler);
	}
	else if(strcmp(szExt, "fnt") == 0)
	{
		 return import_fnt(adapter, path, sampler);
	}

	return NULL;
}

/*_________________________________________________________________________________*/
BkpFont bkpCreateFontFromTrueType(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler, BkpTrueTypeFontConfig * config)
{
    stbtt_fontinfo font_info;
    int ascent, descent, linegap, baseline, lineheight;
    float scale; // leave a little padding in case the character extends left
    int fontsize = DEFAULT_FONT_SIZE;
    int kerningCount;
    stbtt_kerningentry * kernings = NULL;
    const char * faceName;
    const int * codepoints = gDefaultCodePoint;
    int pointCount = DEFAULT_POINT_COUNT;
    BkpFont font = NULL;
    int * pCodePoints;

    LOGC(eDEBUG, gTAG, gColor, "loading '%s'", path);
    if(config != NULL)
    {
        faceName = config->faceName;
        if(config->glySamples != NULL)
        {
            codepoints = config->glySamples;
            pointCount = config->glyCount;
        }
        if(fontsize > 0)
        {
            fontsize = config->fontSize;
        }
    }

    pCodePoints = (int *)codepoints;
    int glyPerLine = gFontWidth / fontsize;
    int glyLines = gFontHeight / fontsize;
    if(glyPerLine < 1 || glyLines < 1)
    {
        LOGC(eERROR, gTAG, gColor, "Font size (%d) too big for image dimension (%d, %d)", fontsize, gFontWidth, gFontHeight);
        return NULL;
    }

    int pageGlyCount = glyPerLine * glyLines;
    int currentGlyCount = 0;


    size_t s_size = 0;
    unsigned char * buffer = (unsigned char *) bkp_loadBinary(path, &s_size);
    int fontCount = stbtt_GetNumberOfFonts(buffer);
    int faceOffset = -1;
	int faceIndex = 0;
    unsigned char * bitmap = NULL;
    size_t bitmap1_size = gFontWidth * gFontHeight * sizeof(unsigned char);
    size_t bitmap4_size = gFontWidth * gFontHeight * sizeof(unsigned char) * 4;
    stbtt_packedchar * packed = NULL;
    int total_pages = pointCount / pageGlyCount + (pointCount % pageGlyCount == 0 ? 0 : 1) ;
    uint8_t page = 0;
    BkpArray(unsigned char *) finalBmp = NULL;
    BkpArray(BkpMemInfo) buffMem = NULL;


    char szPage[16] = {0}; // TO DELETE
    if(faceName != NULL)
    {
        faceOffset = stbtt_FindMatchingFont(buffer, config->faceName, STBTT_MACSTYLE_NONE);
        if(faceOffset == -1)
        {
            LOGC(eERROR, gTAG, gColor, "Could not find face '%s'", faceName);
            bkpFree(buffer);
            goto BadExit;
        }
        for(int i = 0; i < fontCount; ++i)
        {
            if(stbtt_GetFontOffsetForIndex(buffer, i) == faceOffset)
            {
                faceIndex = i;
                break;
            }
        }
    }
    else
    {
        faceOffset = stbtt_GetFontOffsetForIndex(buffer, 0);
    }

    if(stbtt_InitFont(&font_info, buffer, faceOffset) == 0)
    {
        LOGC(eERROR, gTAG, gColor, "stbtt_initfont failed");
        bkpFree(buffer);
        goto BadExit;
    }

    scale = stbtt_ScaleForPixelHeight(&font_info, fontsize);
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &linegap);
	baseline = (int) ascent * scale;
    lineheight = (int) (ascent - descent + linegap) * scale;

    bitmap = bkpAlloc(bitmap1_size);
    packed = bkpAlloc(sizeof(stbtt_packedchar) * pointCount);

	buffMem = bkpArrayCreate(BkpMemInfo);
    bkpArrayResize(&buffMem, total_pages);

	finalBmp = bkpArrayCreate(char *);
    bkpArrayResize(&finalBmp, total_pages);

	font = bkpAlloc(sizeof(BkpFont_));
	font->glyphs = bkpMapCreate(int32_t, SFontGlyph, pointCount);
	font->kernings = bkpArrayCreate(SFontKerning);
    font->face = NULL;
    font->charset = NULL;
    kerningCount = stbtt_GetKerningTableLength(&font_info);

    if(faceName != NULL)
    {
		font->face = bkpStringCreate(faceName);
    }

    if(kerningCount > 0)
    {
        bkpArrayReserve(&font->kernings, kerningCount);
        kernings = bkpAlloc(sizeof(stbtt_kerningentry) * kerningCount);
        if(stbtt_GetKerningTable(&font_info, kernings, kerningCount) != kerningCount)
        {
            LOGC(eERROR, gTAG, gColor, "kernings numbers missmatch");\
            goto BadExit;
        }
        for(size_t i = 0; i < kerningCount; ++i)
        {
            SFontKerning kerning;
            kerning.amount = kernings[i].advance;
            kerning.codepoint_0 = kernings[i].glyph1;
            kerning.codepoint_1 = kernings[i].glyph2;
            bkpArrayPush(&font->kernings, kerning);
        }

        bkpFree(kernings);
    }

	font->texturesPath = bkpMapCreate(uint8_t, BkpString, total_pages);
	font->useBaseLine = 0;

    for(page = 0; page < total_pages; ++page)
    {
        pCodePoints += currentGlyCount;
        currentGlyCount = pointCount > pageGlyCount ? pageGlyCount : pointCount;
        pointCount -= currentGlyCount;
        LOGC(eWARNING, gTAG, gColor, "\tpage : %i, pointCount '%i'", page, currentGlyCount);

        stbtt_pack_context context;
        if(!stbtt_PackBegin(&context, bitmap, gFontWidth, gFontHeight, 0, 1, 0))
        {
            LOGC(eERROR, gTAG, gColor, "packing font face failed");
            goto BadExit;
        }

        stbtt_pack_range range;
        range.first_unicode_codepoint_in_range = 0;
        range.font_size = fontsize;
        range.num_chars = currentGlyCount;
        range.chardata_for_range = packed;
        range.array_of_unicode_codepoints = pCodePoints;

        if(stbtt_PackFontRanges(&context, buffer, faceIndex, &range, 1) == 0)
        {
            LOGC(eERROR, gTAG, gColor, "pack font range failed");\
            goto BadExit;
        }
        stbtt_PackEnd(&context);
        finalBmp[page] = bkpAlloc(bitmap4_size);

        for(size_t k = 0; k < bitmap1_size; ++k)
        {
            finalBmp[page][(k * 4) + 0] = bitmap[k];
            finalBmp[page][(k * 4) + 1] = bitmap[k];
            finalBmp[page][(k * 4) + 2] = bitmap[k];
            finalBmp[page][(k * 4) + 3] = bitmap[k];
        }

		BkpMemInfo tmp = {finalBmp[page], 0, bitmap4_size};
        buffMem[page] = tmp;

		int i = 0;
        for(int *pp = pCodePoints; i < currentGlyCount; ++i, ++pp)
        {
            SFontGlyph glyph;
            glyph.x = packed[i].x0;
            glyph.y = packed[i].y0;
            glyph.xOffset = packed[i].xoff;
            glyph.yOffset = packed[i].yoff;
            glyph.width = packed[i].x1 - packed[i].x0;
            glyph.height = packed[i].y1 - packed[i].y0;
            glyph.xAdvance = packed[i].xadvance;
            glyph.channel = 4;
            glyph.pageId = page;
            glyph.codepoint = *pp;
            bkpMapInsert(&font->glyphs, *pp, glyph);
			if(glyph.yOffset < 0)
			{
				font->useBaseLine = BKP_TRUE;
			}
			//fprintf(stderr, "#%d  : %d\n", *pp, glyph.yOffset);
        }

        sprintf(szPage, "_%d.png", page);
		BkpString str = bkpStringCreate(szPage);
		bkpMapInsert(&font->texturesPath, page, str);
    }

	/* Append FONT_RESERVE_PAGES transparent empty layers so glyphs can be
	 * added later without recreating the GPU texture array. */
	{
		uint8_t * emptyPage = (uint8_t *)bkpAlloc(bitmap4_size);
		memset(emptyPage, 0, bitmap4_size);
		for(int rp = 0; rp < FONT_RESERVE_PAGES; ++rp)
		{
			size_t ri = (size_t)(total_pages + rp);
			bkpArrayResize(&finalBmp, ri + 1);
			bkpArrayResize(&buffMem,  ri + 1);
			finalBmp[ri] = (uint8_t *)bkpAlloc(bitmap4_size);
			memset(finalBmp[ri], 0, bitmap4_size);
			BkpMemInfo tmp2 = {finalBmp[ri], 0, bitmap4_size};
			buffMem[ri] = tmp2;
		}
		bkpFree(emptyPage);
	}

	bkpSetDefaultTextureInfo(&font->textures);
	font->textures.mipType = eMIPMAP_NONE;
	font->textures.viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	bkpCreateTextureLayersFromBitmap(adapter, buffMem, bkpArraySize(buffMem), gFontWidth, gFontHeight, BKP_TRUE, &font->textures);
	font->sampler = sampler;
	font->hasTexture = BKP_TRUE;
    font->size = fontsize;
    font->textureWidth = gFontWidth;
    font->textureHeight= gFontHeight;
    font->lineHeight = lineheight;
    font->baseline = baseline;
    font->tabXadance = 4;

	font->stretchH = 100;
	font->smooth = 1;
	font->aa = 1;
	font->padding[0] = font->padding[1] = font->padding[2] = font->padding[3] = 0;
	font->spacing[0] = font->spacing[1] = 1;
	font->outline = 0;
	font->bold = 0;
	font->italic = 0;
	font->unicode = 1;
	font->packed = 0;
	font->red = 0;
	font->green = 0;
	font->blue = 0;
	font->alpha = 0;
	font->eType = eFONT_TTF;
    font->adapter = adapter;

	/* Store TTF state for on-demand glyph growth. */
	font->ttfBuffer        = buffer;   /* kept alive — do NOT free here */
	memcpy(&font->ttfInfo, &font_info, sizeof(font_info));
	font->ttfFontIndex     = faceIndex;
	font->ttfFontSize      = fontsize;
	font->ttfPageCap       = pageGlyCount;
	font->ttfUsedPages     = total_pages;
	font->ttfReservedPages = FONT_RESERVE_PAGES;

	bkpArrayDestroy(&buffMem);
    for(size_t i = 0; i < bkpArraySize(finalBmp); ++i)
    {
        bkpFree(finalBmp[i]);
    }

	bkpArrayDestroy(&finalBmp);
    bkpFree(packed);
    bkpFree(bitmap);
    /* buffer is NOT freed — stored in font->ttfBuffer for on-demand packing */


    bkpPrintFont(font);

    return font;

    /*
        return NULL;
        */

BadExit:
    return NULL;
}

/*_________________________________________________________________________________*/
void bkpDestroyFont(BkpFont font)
{
	BkpMapIterator it;
	for(bkpMapInitIterator(font->texturesPath, &it); it != NULL; bkpMapNextEntry(font->texturesPath, &it))
	{
		bkpStringDestroy(it->value);
	}

	bkpStringDestroy(&font->face);
	bkpStringDestroy(&font->charset);
	bkpStringDestroy(&font->sourceDir);
	bkpArrayDestroy(&font->kernings);
	bkpMapDestroy(&font->glyphs);
	bkpMapDestroy(&font->texturesPath);
	if(font->hasTexture == BKP_TRUE)
	{
		bkpDestroyImageResource(font->adapter, &font->textures);
	}
	if(font->ttfBuffer != NULL)
	{
		bkpFree(font->ttfBuffer);
	}
	bkpFree(font);
}

/*_________________________________________________________________________________*/
static BkpFont import_bkft(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler)
{
	char szPath[BKP_MAX_FULL_PATH_SIZE] = {0};
	size_t s_size = 0;
	char * content = bkp_loadBinary(path, &s_size);

	if(content == NULL)
	{
		LOGC(eERROR, gTAG, gColor, "Could not open '%s'", path);
		return NULL;
	}

	char * p = content;
	BkftHeader header;

	BkpFont font = bkpAlloc(sizeof(BkpFont_));
	font->glyphs = bkpMapCreate(int32_t, SFontGlyph, 23);
	font->texturesPath = bkpMapCreate(uint8_t, BkpString, 3);
	font->kernings = bkpArrayCreate(SFontKerning);
	font->hasTexture = BKP_FALSE;
	font->sourceDir = NULL;

	p = readBuffer(p, (char *) &header, sizeof(BkftHeader));

	p = readBuffer(p, (char *) &s_size, sizeof(size_t));
	p = readBufferString(p, szPath, s_size);
	font->face = bkpStringCreate(szPath);
	p = readBuffer(p, (char *) &s_size, sizeof(size_t));
	p = readBufferString(p, szPath, s_size);
	font->charset = bkpStringCreate(szPath);

	p = readBuffer(p, (char *)&font->size, sizeof(font->size));
	p = readBuffer(p, (char *)&font->textureWidth, sizeof(font->textureWidth));
	p = readBuffer(p, (char *)&font->textureHeight, sizeof(font->textureHeight));

	p = readBuffer(p, (char *)&font->lineHeight, sizeof(font->lineHeight));
	p = readBuffer(p, (char *)&font->baseline, sizeof(font->baseline));
	p = readBuffer(p, (char *)&font->tabXadance, sizeof(font->tabXadance));

	p = readBuffer(p, (char *)&font->stretchH, sizeof(font->stretchH));
	p = readBuffer(p, (char *)&font->smooth, sizeof(font->smooth));
	p = readBuffer(p, (char *)&font->aa, sizeof(font->aa));
	p = readBuffer(p, (char *)font->padding , sizeof(font->padding[0]) * 4);
	p = readBuffer(p, (char *)font->spacing, sizeof(font->spacing[0]) * 2);
	p = readBuffer(p, (char *)&font->outline, sizeof(font->outline));
	p = readBuffer(p, (char *)&font->bold, sizeof(font->bold));
	p = readBuffer(p, (char *)&font->italic, sizeof(font->italic));
	p = readBuffer(p, (char *)&font->unicode, sizeof(font->unicode));

	p = readBuffer(p, (char *)&font->packed, sizeof(font->packed));
	p = readBuffer(p, (char *)&font->red, sizeof(font->red));
	p = readBuffer(p, (char *)&font->green, sizeof(font->green));
	p = readBuffer(p, (char *)&font->blue, sizeof(font->blue));
	p = readBuffer(p, (char *)&font->alpha, sizeof(font->alpha));

	p = readBuffer(p, (char *)&font->eType, sizeof(font->eType));

	p = readBuffer(p, (char *)&s_size, sizeof(s_size));

	for(size_t i = 0; i < s_size; ++i)
	{
		int32_t id;
		SFontGlyph glyph;
		p = readBuffer(p, (char *)&id, sizeof(id));
		p = readBuffer(p, (char *)&glyph, sizeof(glyph));
		bkpMapInsert(&font->glyphs, id, glyph);
	}

	p = readBuffer(p, (char *)&s_size, sizeof(s_size));
	if(s_size > 0)
	{
		bkpArrayReserve(&font->kernings, s_size);
	}
	for(size_t i = 0; i < s_size; ++i)
	{
		SFontKerning kerning;
		p = readBuffer(p, (char *)&kerning, sizeof(kerning));
		bkpArrayPush(&font->kernings, kerning);
	}

	p = readBuffer(p, (char *)&s_size, sizeof(s_size));
	BkpArray(BkpMemInfo) buffMem = bkpArrayCreate(BkpMemInfo);
	if(s_size > 0)
	{
		bkpArrayResize(&buffMem, s_size);
	}
	for(size_t i = 0; i < s_size; ++i)
	{
		size_t ss;
		uint8_t id;
		p = readBuffer(p, (char *)&id, sizeof(id));
		p = readBuffer(p, (char *)&ss, sizeof(ss));
		p = readBufferString(p, szPath, ss);
		BkpString str = bkpStringCreate(szPath);
		bkpMapInsert(&font->texturesPath, id, str);

		p = readBuffer(p, (char *)&ss, sizeof(ss));
		BkpMemInfo tmp = {p, 0, ss};
		buffMem[id] = tmp;
		p += sizeof(char) * ss;
	}

	bkpSetDefaultTextureInfo(&font->textures);
	font->textures.mipType = eMIPMAP_NONE;
	font->textures.viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	bkpCreateTextureLayersFromMemory(adapter, buffMem, bkpArraySize(buffMem), &font->textures);
	font->sampler = sampler;
	font->hasTexture = BKP_TRUE;
    font->adapter = adapter;

	bkpArrayDestroy(&buffMem);
	bkpFree(content);

	return font;
}

/*_________________________________________________________________________________*/
static BkpFont import_fnt(BkpGpuAdapter adapter, const char * path, BkpSampler * sampler)
{
	char * content = bkp_loadTxtFile(path);
	if(content == NULL)
	{
		LOGC(eERROR, gTAG, gColor, "Could not open '%s'", path);
		return NULL;
	}

	BkpFont font = bkpAlloc(sizeof(BkpFont_));
	font->glyphs = bkpMapCreate(int32_t, SFontGlyph, 23);
	font->texturesPath = bkpMapCreate(uint8_t, BkpString, 3);
	font->kernings = bkpArrayCreate(SFontKerning);
	font->hasTexture = BKP_FALSE;
	font->useBaseLine = BKP_FALSE;

	char * pContent = content;
	char line[512] = {0};
	char fname[512];
	uint64_t line_num	= 1;
	uint32_t line_length = 0;
	uint32_t glyphsCount = 0;
	uint32_t  kerningsCount = 0;
	uint8_t pages = 0;
	int8_t elt_read = 0;
	uint8_t id;
	font->tabXadance = 0;

	while(1)
	{
		pContent = bkp_readBufferLine(pContent, 511, line, &line_length);
		if(!pContent) break;
		if(line_length < 2) continue;
		switch(line[0])
		{
			case 'c' :
				if(line[1] == 'h')
				{
					if(line[4] == ' ')
					{
						CHECK_FNT(get_fnt_char_line(line, font), line, line_num);
					}
					else if(line[4] == 's')
					{
						elt_read = sscanf(line, "chars count=%d", &glyphsCount);
						if(1 != elt_read || glyphsCount < 1)
						{
							LOGC(eERROR, gTAG, gColor, "Font has no glypth(s)'");
							goto error_exit;
						}
					}
				}
				else if(line[1] == 'o')
				{
					CHECK_FNT(get_fnt_common_line(line, font, &pages), line, line_num);
					if(pages < 1)
					{
						LOGC(eERROR, gTAG, gColor, "Font has no texture file(s)'");
						goto error_exit;
					}
				}
				break;
			case 'i' :
					CHECK_FNT(get_fnt_info_line(line, font), line, line_num);
				break;
			case 'k' :
				if(line[7] == ' ')
				{
					CHECK_FNT(get_fnt_kerning_line(line, font), line, line_num);
				}
				else if(line[7] == 's')
				{
					if(sscanf(line, "kernings count=%d", &kerningsCount) != 1)
					{
						goto error_exit;
					}
				}
				break;
			case 'p' :
				memset(fname, 0, 512);
				if(sscanf(line, "page id=%hhi file=\"%[^\"]\"", &id, fname) != 2)
				{
					goto error_exit;
				}

				BkpString fn = bkpStringCreate(fname);
				bkpMapInsert(&font->texturesPath, id, fn);
				break;
			default:
				break;
		}
		++line_num;
	}
	//bkpPrintFont(font);

	if(glyphsCount != bkpMapSize(font->glyphs))
	{
		LOGC(eERROR, gTAG, gColor, "Glyphs count (%u) and glyphs read (%u) miss match", glyphsCount, bkpMapSize(font->glyphs));
		goto error_exit;
	}

	if(pages != bkpMapSize(font->texturesPath))
	{
		LOGC(eERROR, gTAG, gColor, "Pages and pages path miss match");
		goto error_exit;
	}

	if(kerningsCount != bkpArraySize(font->kernings))
	{
		LOGC(eERROR, gTAG, gColor, "Kernings count (%u) and kerning read (%u) miss match", kerningsCount, bkpArraySize(font->kernings));
		goto error_exit;
	}

	SFontGlyph gly;
	if(bkpMapGet(font->glyphs, '\t', &gly) != 0)
	{
		font->tabXadance = gly.xAdvance;
	}
	else
	{
		if(bkpMapGet(font->glyphs, 0x20, &gly) != 0)
		{
			font->tabXadance = gly.xAdvance * 4;
		}
		else
		{
			font->tabXadance = font->size * 4;
		}
	}

	BkpMapIterator itMap;
	char ** paths;
	paths = (char **)bkpAlloc(sizeof(char *) * pages);

	BkpPath bkpPath;
	bkpSplitPath(&bkpPath, path);
	size_t dirLen = strlen(bkpPath.directory);

	for(bkpMapInitIterator(font->texturesPath, &itMap); itMap != NULL; bkpMapNextEntry(font->texturesPath, &itMap))
	{
		id = *(uint8_t *) itMap->key;
		BkpString pStr = *(BkpString *) itMap->value;
		size_t len = dirLen + bkpStringLength(pStr) + 1;
		paths[id] = (char *)bkpAlloc(sizeof(char ) * len);
		strcpy(paths[id], bkpPath.directory);
		strcat(paths[id], pStr);
		paths[id][len] = '\0';
	}

	bkpSetDefaultTextureInfo(&font->textures);
	font->textures.mipType = eMIPMAP_NONE;
	font->textures.viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	bkpCreateTextureLayersFromFile(adapter, (const char **)paths, pages, &font->textures);
	font->sampler = sampler;
	font->hasTexture = BKP_TRUE;
	font->sourceDir = bkpStringCreate(bkpPath.directory);
    font->adapter = adapter;

	for(size_t i = 0; i < pages; ++i)
	{
		bkpFree(paths[i]);
	}
	bkpFree(paths);
	bkpFree(content);

	return font;

error_exit:
	bkpFree(content);
	bkpDestroyFont(font);
	return NULL;
}

/*_________________________________________________________________________________*/
static char * skipWord(char * p)
{
	unsigned char * q = (unsigned char *)p;
	while(*q > 0x20) ++q;

	if(*q == '\0' || *q == '\n' || *q == '\r') return NULL;

	while(*q <= 0x20) ++q;

	return (char *)q;
}

/*_________________________________________________________________________________*/
static BkpBool getKeyValue(char * p, char * key, char * value)
{
	char * q = key;

	while(*p != '=')
	{
		*q = *p;
		++p;
		++q;
	}

	++p;
	q = value;

	while(*p > 0x20)
	{
		*q = *p;
		++p;
		++q;
	}

	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
static BkpBool get_fnt_char_line(char * line, BkpFont font)
{
	char * p = line;
	p = skipWord(p);
	uint8_t required[10] = {0};
	SFontGlyph gly;

	while(p != NULL)
	{
		char key[32] = {0};
		char valueStr[32] = {0};
		int value;

		getKeyValue(p, key, valueStr);

		if(sscanf(valueStr, "%d", &value) != 1)
		{
			return BKP_FALSE;
		}

		if(strcmp(key, "id") == 0)
		{
			required[0] = 1;
			gly.codepoint = value;
		}
		else if(strcmp(key, "x") == 0)
		{
			required[1] = 1;
			gly.x = value;
		}
		else if(strcmp(key, "y") == 0)
		{
			required[2] = 1;
			gly.y = value;
		}
		else if(strcmp(key, "width") == 0)
		{
			required[3] = 1;
			gly.width = value;
		}
		else if(strcmp(key, "height") == 0)
		{
			required[4] = 1;
			gly.height = value;
		}
		else if(strcmp(key, "xoffset") == 0)
		{
			required[5] = 1;
			gly.xOffset = value;
		}
		else if(strcmp(key, "yoffset") == 0)
		{
			required[6] = 1;
			gly.yOffset = value;
		}
		else if(strcmp(key, "xadvance") == 0)
		{
			required[7] = 1;
			gly.xAdvance = value;
		}
		else if(strcmp(key, "page") == 0)
		{
			required[8] = 1;
			gly.pageId = value;
		}
		else if(strcmp(key, "chnl") == 0)
		{
			required[9] = 1;
			gly.channel = value;
		}

		p = skipWord(p);
	}

	uint8_t require = 0;
	for(int i = 0; i < 10; ++i)
	{
		require += required[i];
	}

	if(require != 10) return BKP_FALSE;

	bkpMapInsert(&font->glyphs, gly.codepoint, gly);

	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
static BkpBool get_fnt_common_line(char * line, BkpFont font, uint8_t * pages)
{
	char * p = line;
	p = skipWord(p);
	font->packed = 0;
	font->alpha = 1;
	font->red = 0;
	font->green = 0;
	font->blue = 0;

	uint8_t required[5] = {0};

	while(p != NULL)
	{
		char key[32] = {0};
		char valueStr[32] = {0};
		int32_t value;

		getKeyValue(p, key, valueStr);

		if(sscanf(valueStr, "%d", &value) != 1)
		{
			return BKP_FALSE;
		}

		if(strcmp(key, "lineHeight") == 0)
		{
			required[0] = 1;
			font->lineHeight = value;
		}
		else if(strcmp(key, "base") == 0)
		{
			required[1] = 1;
			font->baseline = value;
		}
		else if(strcmp(key, "scaleW") == 0)
		{
			required[2] = 1;
			font->textureWidth = value;
		}
		else if(strcmp(key, "scaleH") == 0)
		{
			required[3] = 1;
			font->textureHeight = value;
		}
		else if(strcmp(key, "pages") == 0)
		{
			required[4] = 1;
			*pages = value;
		}
		else if(strcmp(key, "packed") == 0)
		{
			font->packed = value;
		}
		else if(strcmp(key, "alphaChnl") == 0)
		{
			font->alpha = value;
		}
		else if(strcmp(key, "redChnl") == 0)
		{
			font->red = value;
		}
		else if(strcmp(key, "greenChnl") == 0)
		{
			font->green = value;
		}
		else if(strcmp(key, "blueChnl") == 0)
		{
			font->blue = value;
		}
		p = skipWord(p);
	}

	uint8_t require = 0;
	for(int i = 0; i < 5; ++i)
	{
		require += required[i];
	}

	return require == 5;
}

/*_________________________________________________________________________________*/
static BkpBool get_fnt_info_line(char * line, BkpFont font)
{
	char * p = line;
	p = skipWord(p);
	font->padding[0] = font->padding[1] = font->padding[2] = font->padding[3] = 4;
	font->spacing[0] = font->spacing[1] = 1;
	font->bold = 0;
	font->italic = 0;
	font->outline = 0;

	int32_t value;
	uint8_t required[7] = {0};

	while(p != NULL)
	{
		char key[32] = {0};
		char valueStr[32] = {0};

		getKeyValue(p, key, valueStr);

		if(strcmp(key, "face") == 0 || strcmp(key, "charset") == 0)
		{
			if(strcmp(key, "face") == 0)
			{
				required[0] = 1;
				char qR[32] = {0};
				char * p0 = valueStr + 1;
				char * q = qR;
				while(*p0 != '"')
				{
					*q = *p0;
					++p0;
					++q;
				};
				font->face = bkpStringCreate(qR);
			}
			else
			{
				required[2] = 1;
				char qR[32] = {0};
				char * p0 = valueStr + 1;
				char * q = qR;
				while(*p0 != '"')
				{
					*q = *p0;
					++p0;
					++q;
				};
				font->charset = bkpStringCreate(qR);
			}
		}
		else
		{
			if(strcmp(key, "padding") == 0)
			{
				sscanf(valueStr, "%hhd,%hhd,%hhd,%hhd", &font->padding[0], &font->padding[1], &font->padding[2], &font->padding[3]);
			}
			else if(strcmp(key, "spacing") == 0)
			{
				sscanf(valueStr, "%hhd,%hhd", &font->spacing[0], &font->spacing[1]);
			}
			else
			{
				if(sscanf(valueStr, "%d", &value) != 1)
				{
					return BKP_FALSE;
				}

				if(strcmp(key, "size") == 0)
				{
					required[1] = 1;
					font->size = value;
				}
				else if(strcmp(key, "bold") == 0)
				{
					font->bold= value;
				}
				else if(strcmp(key, "italic") == 0)
				{
					font->italic = value;
				}
				else if(strcmp(key, "unicode") == 0)
				{
					required[3] = 1;
					font->unicode = value;
				}
				else if(strcmp(key, "stretchH") == 0)
				{
					required[4] = 1;
					font->stretchH = value;
				}
				else if(strcmp(key, "smooth") == 0)
				{
					required[5] = 1;
					font->smooth = value;
				}
				else if(strcmp(key, "aa") == 0)
				{
					required[6] = 1;
					font->aa = value;
				}
				else if(strcmp(key, "outline") == 0)
				{
					font->outline = value;
				}
			}
		}
		p = skipWord(p);
	}

	uint8_t require = 0;
	for(int i = 0; i < 7; ++i)
	{
		require += required[i];
	}

	return require == 7;
}

/*_________________________________________________________________________________*/
static BkpBool get_fnt_kerning_line(char * line, BkpFont font)
{
	char * p = line;
	p = skipWord(p);

	int32_t value;
	uint8_t required[3] = {0};

	SFontKerning ker;
	while(p != NULL)
	{
		char key[32] = {0};
		char valueStr[32] = {0};

		getKeyValue(p, key, valueStr);

		if(sscanf(valueStr, "%d", &value) != 1)
		{
			return BKP_FALSE;
		}

		if(strcmp(key, "first") == 0)
		{
			required[0] = 1;
			ker.codepoint_0 = value;
		}
		else if(strcmp(key, "second") == 0)
		{
			required[1] = 1;
			ker.codepoint_1 = value;
		}
		else if(strcmp(key, "amount") == 0)
		{
			required[2] = 1;
			ker.amount = value;
		}
		p = skipWord(p);
	}

	uint8_t require = 0;
	for(int i = 0; i < 3; ++i)
	{
		require += required[i];
	}

	if(require != 3) return BKP_FALSE;

	bkpArrayPush(&font->kernings, ker);

	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
BkpBool bkpSaveFont(BkpFont font, const char * path)
{
    FILE *inf = NULL;
    size_t s_file;

	BkpPath bkpPath;
	bkpSplitPath(&bkpPath, path);
	if(bkpPathExists(bkpPath.directory) == 0)
	{
	   	if(bkpCreateDirectory(bkpPath.directory) == 0)
		{
			LOGC(eERROR, gTAG, gColor, "Could not create directory '%s'", bkpPath.directory);
			return BKP_FALSE;
		}
	}


    inf = fopen(path,"wb");
	if(inf == NULL)
	{
		LOGC(eERROR, gTAG, gColor, "Could not save '%s'", path);
		return BKP_FALSE;
	}

	BkftHeader header;
	header.magic = BKFT_MAGIC_NUMBER;
	header.type =  eFONT_BITMAP;
	header.major = 1;
	header.minor = 0;
	header.build = 0;
	strcpy(header.name, DEFAULT_BKFT_NAME);

    size_t val=fwrite(&header, sizeof(BkftHeader), 1, inf);
	CHECK_WRITE_FILE(1, val);
	s_file = bkpStringLength(font->face);
    val=fwrite(&s_file, sizeof(size_t), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(font->face, sizeof(char), s_file, inf);
	CHECK_WRITE_FILE(s_file, val);

	s_file = bkpStringLength(font->charset);
    val=fwrite(&s_file, sizeof(size_t), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(font->charset, sizeof(char), s_file, inf);
	CHECK_WRITE_FILE(s_file, val);

    val=fwrite(&font->size, sizeof(font->size), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->textureWidth, sizeof(font->textureWidth), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->textureHeight, sizeof(font->textureHeight), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->lineHeight, sizeof(font->lineHeight), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->baseline, sizeof(font->baseline), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->tabXadance, sizeof(font->tabXadance), 1, inf);
	CHECK_WRITE_FILE(1, val);

    val=fwrite(&font->stretchH, sizeof(font->stretchH), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->smooth, sizeof(font->smooth), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->aa, sizeof(font->aa), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(font->padding, sizeof(font->padding[0]), 4, inf);
	CHECK_WRITE_FILE(4, val);
    val=fwrite(font->spacing, sizeof(font->spacing[0]), 2, inf);
	CHECK_WRITE_FILE(2, val);
    val=fwrite(&font->outline, sizeof(font->outline), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->bold, sizeof(font->bold), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->italic, sizeof(font->italic), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->unicode, sizeof(font->unicode), 1, inf);
	CHECK_WRITE_FILE(1, val);

    val=fwrite(&font->packed, sizeof(font->packed), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->red, sizeof(font->red), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->green, sizeof(font->green), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->blue, sizeof(font->blue), 1, inf);
	CHECK_WRITE_FILE(1, val);
    val=fwrite(&font->alpha, sizeof(font->alpha), 1, inf);
	CHECK_WRITE_FILE(1, val);

    val=fwrite(&font->eType, sizeof(font->eType), 1, inf);
	CHECK_WRITE_FILE(1, val);

	s_file = bkpMapSize(font->glyphs);
	val = fwrite(&s_file, sizeof(size_t), 1, inf);
	CHECK_WRITE_FILE(1, val);

	BkpMapIterator it;
	for(bkpMapInitIterator(font->glyphs, &it); it != NULL; bkpMapNextEntry(font->glyphs, &it))
	{
		int32_t id = *(int32_t *)it->key;
		SFontGlyph gly = *(SFontGlyph *) it->value;
		val=fwrite(&id, sizeof(id), 1, inf);
		CHECK_WRITE_FILE(1, val);
		val=fwrite(&gly, sizeof(gly), 1, inf);
		CHECK_WRITE_FILE(1, val);
	}

	s_file = bkpArraySize(font->kernings);
	val=fwrite(&s_file, sizeof(size_t), 1, inf);
	CHECK_WRITE_FILE(1, val);
	for(size_t i = 0; i < s_file; ++i)
	{
		val = fwrite(&font->kernings[i], sizeof(font->kernings[0]), 1, inf);
		CHECK_WRITE_FILE(1, val);
	}

	s_file = bkpMapSize(font->texturesPath);
    val=fwrite(&s_file, sizeof(size_t), 1, inf);
	CHECK_WRITE_FILE(1, val);
	for(bkpMapInitIterator(font->texturesPath, &it); it != NULL; bkpMapNextEntry(font->texturesPath, &it))
	{
		uint8_t id = *(uint8_t *)it->key;
		val=fwrite(&id, sizeof(id), 1, inf);
		CHECK_WRITE_FILE(1, val);
		BkpString str = *(BkpString *)it->value;
		s_file = strlen(str);
		val=fwrite(&s_file, sizeof(size_t), 1, inf);
		CHECK_WRITE_FILE(1, val);
		val=fwrite(str, sizeof(char), s_file, inf);
		CHECK_WRITE_FILE(s_file, val);
		//if we are here we assume that fnt was loaded correctly so no sanity check
		BkpString fpath = bkpStringCreate(font->sourceDir);
		bkpStringcat(&fpath, str);

		size_t fSize = 0;
		char * fData = bkp_loadBinary(fpath, &fSize);

		val=fwrite(&fSize, sizeof(size_t), 1, inf);
		CHECK_WRITE_FILE(1, val);
		val=fwrite(fData, sizeof(char), fSize, inf);
		CHECK_WRITE_FILE(fSize, val);

		bkpFree(fData);
		bkpStringDestroy(&fpath);
	}

	fclose(inf);
	return BKP_TRUE;

clean_exit:
	fclose(inf);
	//ifexist destroy file
	return BKP_FALSE;
}

/*____________________________________________________________________________________*/
void bkpPrintFont(BkpFont font)
{
	LOGC(eDEBUG, gTAG, gColor, "font : \n\tface : '%s'\n\ttype : %u\n\tsize : %u\n\tglyths : %lu\n\tpages : %lu\n\t"\
			"kernings : %lu\n\tbase : %u\n\tlineHeight %u\n\ttextureWH(%u, %u)\n\tpacked : %u\n\t"\
			"rgba(%u,%u,%u,%u)\n\tstretchH : %d\n\tsmooth : %u\n\tAntiAliasing : %u\n\t"\
			"spacing(%u, %u, %u, %u), padding(%u, %u), outline : %u\n\tbold : %u\n\tunicode : %u\n\t"\
			"charset '%s'\n\ttab : %u",
			font->face, (uint8_t)font->eType, font->size, bkpMapSize(font->glyphs), bkpMapSize(font->texturesPath),
			bkpArraySize(font->kernings), font->baseline, font->lineHeight, font->textureWidth, font->textureHeight,
			font->packed, font->red, font->green, font->blue, font->alpha, font->stretchH, font->smooth,
			font->aa, font->padding[0], font->padding[1], font->padding[2], font->padding[3],
			font->spacing[0], font->spacing[1], font->outline, font->bold, font->unicode,
			font->charset, font->tabXadance);
}

/*_________________________________________________________________________________*/
int bkpGetFontLineHeight(BkpFont font)
{
    return font->lineHeight;
}

/*_________________________________________________________________________________*/
BkpText bkpCreateText(BkpFont font, const char * txt, BkpDescriptorPool * descriptorPool, BkpDescriptorSetLayout * layout)
{
	BkpText text = bkpAlloc(sizeof(BkpText_));

	BkpBufferInfo info = {};
	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size = sizeof(BkpVertexUI) * VERTEX_QUAD_NO_INDICE_COUNT * 32;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(font->adapter, &text->vertexBuffer, &info);

	text->font = font;
    text->text = bkpStringCreate("");

	BkpArray(BkpWriteDescriptorSetInfo) dwInfo = bkpArrayCreate(BkpWriteDescriptorSetInfo);

	text->ubo.size  = sizeof(BkpUniformTextFont);
	text->ubo.count = font->adapter->frameInfo.maxFrameInFlight;
	bkpCreateBuffersGPU(font->adapter, &text->ubo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, eBUFFER_CPU_GPU, BKP_DO_MAP);
	text->set.descPool   = descriptorPool;
	text->set.descLayout = layout;
	text->set.count = font->adapter->frameInfo.maxFrameInFlight;

	//text->set.descPool   = &gDescriptorPool;

	BkpWriteDescriptorSetInfo inf[4] = {};
	VkDescriptorImageInfo pImgs[1];
	pImgs[0] = (VkDescriptorImageInfo){font->sampler->sampler,
            *font->textures.imageViews,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	BkpArray(VkDescriptorBufferInfo) pBuff = bkpArrayCreate(VkDescriptorBufferInfo);
	bkpArrayResize(&pBuff, text->set.count);

	for(size_t i = 0; i <text->set.count; ++i)
	{
		pBuff[i] = bkpMakeUboInfo(text->ubo.buffer, i, text->ubo.size);
		inf[i].setBindings[0].type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		inf[i].setBindings[0].ubos  = &pBuff[i];
		inf[i].setBindings[0].count = 1;
		inf[i].setBindings[1].type  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		inf[i].setBindings[1].imgs  = pImgs;
		inf[i].setBindings[1].count = 1;
		inf[i].setBindingCount = 2;
		bkpArrayPush(&dwInfo, inf[i]);
	}

	bkpCreateDescriptorSet(font->adapter, &text->set);
	bkpWriteDescriptorSets(font->adapter, &text->set, dwInfo);

	bkpArrayDestroy(&pBuff);
	bkpArrayDestroy(&dwInfo);

   	text->data.color = bkpVec4(1.0f, 1.0f, 1.0f, 1.0f);
	text->scale = bkpVec3(1.0f, 1.0f, 0.0f);
   	text->data.transformation = bkpIdentityMat4();

	for (size_t j = 0; j < font->adapter->frameInfo.maxFrameInFlight; ++j)
	{
		bkpUploadBufferData(font->adapter, text->ubo.buffer, &text->data, j * text->ubo.size, sizeof(text->data));
	}

    bkpSetText(text, txt);

    return text;
}


/*_________________________________________________________________________________*/
BkpBool bkpSetText(BkpText text, const char * content)
{
	if(strcmp(text->text, content) == 0)
    {
        return BKP_FALSE;
    };

    bkpStringcpy(&text->text, content);

	size_t textLenUtf8 = bkpUtf8Strlen(text->text);
	text->vertexCount = VERTEX_QUAD_NO_INDICE_COUNT * textLenUtf8;
	VkDeviceSize vertexSizeOld = 0;
	VkDeviceSize vertexSize = sizeof(BkpVertexUI) * text->vertexCount;
	bkpGetBufferSize(text->vertexBuffer, &vertexSizeOld);

	if(vertexSize > vertexSizeOld)
	{
		bkpFreeBuffer(text->font->adapter, &text->vertexBuffer);

		BkpBufferInfo info = {};
		info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		info.type = eBUFFER_GPU;
		info.size = vertexSize;
		info.isImage = BKP_FALSE;
		bkpAllocateBuffer(text->font->adapter, &text->vertexBuffer, &info);
	}


	BkpArray(BkpVertexUI) vertices = bkpArrayCreate(BkpVertexUI);

	const char * p = text->text;
	int32_t xPointer = 0;
	int32_t yPointer = text->font->useBaseLine == BKP_TRUE ? text->font->baseline : 0;
    size_t textLen = strlen(text->text);
	for(size_t chr = 0; chr < textLen; ++chr)
	{
		int32_t codepoint = p[chr];
		if(codepoint == 0x0A)
		{
			xPointer = 0;
			yPointer += text->font->lineHeight;
			continue;
		}

		if(codepoint == 0x09)
		{
			xPointer += text->font->tabXadance;
			continue;
		}

		uint8_t steps = 0;
		if(!bkpUtf8GetCodepoint(p, chr, &codepoint, &steps))
		{
			//codepoint = -1;
			codepoint = 127;
		}

		if(bkpMapHasKey(text->font->glyphs, codepoint) == BKP_FALSE)
		{
			/* For TTF fonts: try to pack the missing glyph into a reserve page. */
			if(text->font->eType != eFONT_TTF || growFontAtlas(text->font, codepoint) == BKP_FALSE)
			{
				LOGC(eWARNING, gTAG, gColor, "Glyph '%d' not found and cannot be added", codepoint);
				codepoint = 127;
			}
		}

		SFontGlyph g;
		bkpMapGet(text->font->glyphs, codepoint, &g);
        //add not found glyph

		float minx = xPointer + g.xOffset;
		float miny = yPointer + g.yOffset;
		float maxx = minx + g.width;
		float maxy = miny + g.height;

		float tminx = (float)g.x / text->font->textureWidth;
		float tminy = (float)g.y / text->font->textureHeight;
		float tmaxx = (float)(g.x + g.width) / text->font->textureWidth;
		float tmaxy = (float)(g.y + g.height) / text->font->textureHeight;

		/*
		if(text->font->eType  == eFONT_TTF)
		{
			tminy = 1.0f - tminy;
			tmaxy = 1.0f - tmaxy;
		}
		*/

		BkpVertexUI vertex;
		/* CCW winding in Vulkan NDC (y-down): TL→BR→TR, TL→BL→BR */
		vertex = (BkpVertexUI){bkpVec2(minx, miny), bkpVec3(tminx, tminy, g.pageId)};
		bkpArrayPush(&vertices, vertex);

		vertex = (BkpVertexUI){bkpVec2(maxx, maxy), bkpVec3(tmaxx, tmaxy, g.pageId)};
		bkpArrayPush(&vertices, vertex);

		vertex = (BkpVertexUI){bkpVec2(maxx, miny), bkpVec3(tmaxx, tminy, g.pageId)};
		bkpArrayPush(&vertices, vertex);

		vertex = (BkpVertexUI){bkpVec2(minx, miny), bkpVec3(tminx, tminy, g.pageId)};
		bkpArrayPush(&vertices, vertex);

		vertex = (BkpVertexUI){bkpVec2(minx, maxy), bkpVec3(tminx, tmaxy, g.pageId)};
		bkpArrayPush(&vertices, vertex);

		vertex = (BkpVertexUI){bkpVec2(maxx, maxy), bkpVec3(tmaxx, tmaxy, g.pageId)};
		bkpArrayPush(&vertices, vertex);

		int32_t kerning = 0;
		size_t offset = chr + steps;
		if(offset < textLenUtf8 - 1)
		{
			int32_t next_codepoint = 0;
			uint8_t steps_next = 0;
			//need to put a codepoint -1 here;
			if(bkpUtf8GetCodepoint(p, offset, &next_codepoint, &steps_next))
			{
				for(size_t i = 0; i < bkpArraySize(text->font->kernings); ++i)
				{
					SFontKerning * fK = &text->font->kernings[i];
					if(fK->codepoint_0 == codepoint && fK->codepoint_1 == next_codepoint)
					{
						kerning = fK->amount;
						break;
					}
				}
			}
		}
		xPointer += g.xAdvance + kerning;

		chr += steps - 1;
	}

	if(bkpArraySize(vertices) < 1)
	{
		return BKP_FALSE;
	}

	bkpUploadBufferData(text->font->adapter, text->vertexBuffer, vertices, 0, bkpArraySize(vertices) * sizeof(BkpVertexUI));
    bkpArrayDestroy(&vertices);
	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
void bkpSetTextSize(BkpText text, BkpVec3 value)
{
	text->scale = bkpVec3DivideScalar(&value, text->font->size);
    text->needUpdate = BKP_TRUE;
}

/*_________________________________________________________________________________*/
void bkpSetTextColor(BkpText text, BkpVec4 value)
{
    text->data.color = value;
    text->needUpdate = BKP_TRUE;
}

/*_________________________________________________________________________________*/
void bkpSetTextPosition(BkpText text, BkpVec3 value)
{
	text->position = value;
    text->needUpdate = BKP_TRUE;
}


/*_________________________________________________________________________________*/
void bkpDestroyText(BkpText text)
{
	bkpDestroyBuffersGPU(text->font->adapter, &text->ubo);
	bkpFreeBuffer(text->font->adapter, &text->vertexBuffer);
    bkpStringDestroy(&text->text);
    bkpDestroyDescriptorSet(text->font->adapter, &text->set);
    bkpFree(text);
	//remove from textList;
}



/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/


/******************** static functions ********************************/

/*____________________________________________________________________________________*/
/* Pack one missing codepoint into the next pre-allocated reserve page and
 * upload the new glyph bitmap to the GPU texture array.
 * Returns BKP_TRUE and registers the glyph in font->glyphs on success. */
static BkpBool growFontAtlas(BkpFont font, int codepoint)
{
	if(font->ttfBuffer == NULL)
	{
		return BKP_FALSE;   /* bitmap font — cannot grow */
	}
	if(font->ttfReservedPages < 1)
	{
		LOGC(eERROR, gTAG, gColor, "Font atlas full: no reserve pages left for codepoint %d", codepoint);
		return BKP_FALSE;
	}

	uint8_t targetPage = (uint8_t)font->ttfUsedPages;
	int w = (int)font->textureWidth;
	int h = (int)font->textureHeight;
	size_t bmp1sz = (size_t)(w * h);
	size_t bmp4sz = bmp1sz * 4;

	uint8_t * bmp1 = (uint8_t *)bkpAlloc(bmp1sz);
	memset(bmp1, 0, bmp1sz);

	stbtt_packedchar packed = {0};
	stbtt_pack_context ctx;
	if(!stbtt_PackBegin(&ctx, bmp1, w, h, 0, 1, 0))
	{
		LOGC(eERROR, gTAG, gColor, "stbtt_PackBegin failed for reserve page %d", targetPage);
		bkpFree(bmp1);
		return BKP_FALSE;
	}

	stbtt_pack_range range = {0};
	range.font_size                   = (float)font->ttfFontSize;
	range.num_chars                   = 1;
	range.chardata_for_range          = &packed;
	int cp = codepoint;
	range.array_of_unicode_codepoints = &cp;

	int ok = stbtt_PackFontRanges(&ctx, font->ttfBuffer, font->ttfFontIndex, &range, 1);
	stbtt_PackEnd(&ctx);

	if(!ok)
	{
		LOGC(eERROR, gTAG, gColor, "Failed to pack glyph %d into reserve page %d", codepoint, targetPage);
		bkpFree(bmp1);
		return BKP_FALSE;
	}

	/* Expand single-channel coverage bitmap to RGBA */
	uint8_t * bmp4 = (uint8_t *)bkpAlloc(bmp4sz);
	for(size_t k = 0; k < bmp1sz; ++k)
	{
		bmp4[k*4+0] = bmp1[k]; bmp4[k*4+1] = bmp1[k];
		bmp4[k*4+2] = bmp1[k]; bmp4[k*4+3] = bmp1[k];
	}
	bkpFree(bmp1);

	/* Upload to the pre-allocated transparent layer */
	bkpUploadTextureLayer(font->adapter, &font->textures, targetPage, bmp4, (VkDeviceSize)bmp4sz);
	bkpFree(bmp4);

	/* Register the new glyph in the hashmap */
	SFontGlyph glyph = {0};
	glyph.codepoint = codepoint;
	glyph.x         = packed.x0;
	glyph.y         = packed.y0;
	glyph.xOffset   = (int16_t)packed.xoff;
	glyph.yOffset   = (int16_t)packed.yoff;
	glyph.width     = (int16_t)(packed.x1 - packed.x0);
	glyph.height    = (int16_t)(packed.y1 - packed.y0);
	glyph.xAdvance  = (int16_t)packed.xadvance;
	glyph.channel   = 4;
	glyph.pageId    = targetPage;
	bkpMapInsert(&font->glyphs, codepoint, glyph);

	font->ttfUsedPages++;
	font->ttfReservedPages--;
	if(glyph.yOffset < 0) { font->useBaseLine = BKP_TRUE; }

	LOGC(eDEBUG, gTAG, gColor, "Glyph %d added to atlas page %d (%d reserve pages left)",
	     codepoint, targetPage, font->ttfReservedPages);
	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
static char * readBuffer(char * buff, char * result, size_t size)
{
	memcpy(result, buff, size);
	buff += size;
	return buff;
}

/*____________________________________________________________________________________*/
static char * readBufferString(char * buff, char * word, size_t count)
{
	for(size_t i = 0; i < count; ++i, ++buff)
	{
		word[i] = *buff;
	}
	word[count] = '\0';

	return buff;
}

/*____________________________________________________________________________________*/
/*
static void print_glyph(SFontGlyph gly)
{
	LOGC(eDEBUG, gTAG, gColor, "code[%d] x=%u y=%u width=%u height=%u xoffset=%u yoffset=%u xadvance=%u page=%u chnl=%u\n",
			gly.codepoint, gly.x, gly.y, gly.width, gly.height,
			gly.xOffset, gly.yOffset, gly.xAdvance, gly.pageId, gly.channel);
}
*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/

/*_________________________________________________________________________________*/



#ifdef __cplusplus
}
#endif
