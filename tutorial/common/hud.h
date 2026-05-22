#ifndef TUTORIAL_HUD_H_
#define TUTORIAL_HUD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <include/blukpast.h>
#include <gfx/include/bkp_font.h>
#include "scene.h"

#define HUD_FONT_SIZE  24
#define HUD_TEXT_COUNT  3  /* fps, hint, help */

/* Push constant for the rect pipeline (40 bytes). */
typedef struct
{
    float rect[4];    /* x, y, w, h in pixels */
    float color[4];
    float screen[2];
    float pad[2];
} RectPC;

typedef struct
{
    BkpFont         font;
    BkpSampler      fontSampler;

    /* text pipeline: BkpVertexUI → font atlas sampler2DArray */
    BkpShaderModule    hudVert, hudFrag;
    BkpShaderProgram   hudProg;
    BkpPipelineGraphic hudPipeline;

    /* rect pipeline: procedural quad, push-constant color */
    BkpShaderModule    rectVert, rectFrag;
    BkpShaderProgram   rectProg;
    BkpPipelineGraphic rectPipeline;

    /* shared descriptor pool for all BkpText objects */
    BkpDescriptorPool descPool;

    BkpText fpsText;   /* "FPS: 60"                        */
    BkpText hintText;  /* "press H to toggle help"         */
    BkpText helpText;  /* three-line help content          */

    int helpVisible;   /* 1 when help overlay is shown     */
    int hPressed;      /* set by bkp_input callback        */

    float fpsTimer;    /* time accumulated since last update */
    int   fpsCount;
    float fpsValue;

    int lineHeight;    /* font line height in pixels        */
    int width, height;
} Hud;

/* shaderDir: directory containing hud.vert.spv, hud.frag.spv,
 *            rect.vert.spv, rect.frag.spv at runtime.
 * fontPath:  path to .ttf font file at runtime. */
void hudInit(BkpGpuAdapter adp, Hud * h,
             VkFormat colorFmt, VkFormat depthFmt, int w, int ht,
             const char * shaderDir, const char * fontPath);
void hudUpdate(Hud * h, float dt);
void hudDraw(BkpGpuAdapter adp, VkCommandBuffer cmd, Hud * h, uint32_t frame);
void hudCleanup(BkpGpuAdapter adp, Hud * h);

#ifdef __cplusplus
}
#endif

#endif /* TUTORIAL_HUD_H_ */
