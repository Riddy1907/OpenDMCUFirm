/*
 *   This file is part of OpenDMCUFirm
 *
 *   Sharp scaling for the TV output in vWii mode.
 *
 *   The stock firmware lets the D1 scaler stretch the ~474 line Wii image to the full
 *   1080 line output with the default (bilinear) filter. 1080 / 474 is not an integer,
 *   which makes the picture soft. In SHARP_MODE >= 1 we instead:
 *     - shrink the vertical destination to exactly 2x the source height using the
 *       vertical external overscan (black bars at top/bottom),
 *     - shrink the horizontal destination by the same factor so the aspect ratio stays
 *       the same (on top of the pillarbox the system already requested),
 *     - switch the vertical filter to nearest neighbour -> clean line doubling.
 *   If the output can't fit a 2x image (e.g. 720p or 576 line PAL), the stock
 *   configuration is restored.
 */
#include "scaler.h"
#include "dc.h"
#include "conf.h"

static uint16_t sBaseLeft = 0;
static uint16_t sBaseRight = 0;
static BOOL sBaseValid = FALSE;

static uint32_t sOrigHorzFilter = 0;
static uint32_t sOrigVertFilter = 0;
static BOOL sOrigValid = FALSE;

void SCALER_SetTVBaseOverscan(uint16_t left, uint16_t right)
{
    sBaseLeft = left;
    sBaseRight = right;
    sBaseValid = TRUE;
}

/* Active size from a CRTC *_BLANK_START_END register (blank start = end of active area). */
static uint16_t activeSpan(uint16_t reg)
{
    uint32_t v = DC_Read32(reg);
    uint16_t blankStart = (uint16_t) (v & 0x1FFF);
    uint16_t blankEnd = (uint16_t) ((v >> 16) & 0x1FFF);
    return blankStart > blankEnd ? (uint16_t) (blankStart - blankEnd) : 0;
}

static void writeOverscan(uint16_t left, uint16_t right, uint16_t top, uint16_t bottom)
{
    DC_Write32(D1MODE_EXT_OVERSCAN_LEFT_RIGHT, MODE_EXT_OVERSCAN_LEFT(left) | MODE_EXT_OVERSCAN_RIGHT(right));
    DC_Write32(D1MODE_EXT_OVERSCAN_TOP_BOTTOM, MODE_EXT_OVERSCAN_TOP(top) | MODE_EXT_OVERSCAN_BOTTOM(bottom));
}

void SCALER_ApplyTV(void)
{
#if SHARP_MODE == 0
    return;
#else
    uint16_t outW, outH, srcH, dstH, top, bottom, baseW, newW, extra;
    uint32_t lr;

    if (!sBaseValid) {
        lr = DC_Read32(D1MODE_EXT_OVERSCAN_LEFT_RIGHT);
        sBaseLeft = (uint16_t) ((lr >> MODE_EXT_OVERSCAN_LEFT_SHIFT) & MODE_EXT_OVERSCAN_LEFT_MASK);
        sBaseRight = (uint16_t) (lr & MODE_EXT_OVERSCAN_RIGHT_MASK);
        sBaseValid = TRUE;
    }

    if (!sOrigValid) {
        sOrigHorzFilter = DC_Read32(D1SCL_HORZ_FILTER_CONTROL);
        sOrigVertFilter = DC_Read32(D1SCL_VERT_FILTER_CONTROL);
        sOrigValid = TRUE;
    }

    outW = activeSpan(D1CRTC_H_BLANK_START_END);
    outH = activeSpan(D1CRTC_V_BLANK_START_END);
    srcH = (uint16_t) (DC_Read32(D1MODE_VIEWPORT_SIZE) & MODE_VIEWPORT_HEIGHT_MASK);

    if (!CONF_IsHD(DISPLAY_TV) || outH < 1000 || srcH == 0 || (uint16_t) (srcH * 2) > outH
        || (uint16_t) (sBaseLeft + sBaseRight) >= outW) {
        /* Not applicable -> stock behaviour */
        writeOverscan(sBaseLeft, sBaseRight, 0, 0);
        DC_Write32(D1SCL_HORZ_FILTER_CONTROL, sOrigHorzFilter);
        DC_Write32(D1SCL_VERT_FILTER_CONTROL, sOrigVertFilter);
        return;
    }

    /* vertical: exactly 2x, centered */
    dstH = (uint16_t) (srcH * 2);
    top = (uint16_t) ((outH - dstH) / 2);
    bottom = (uint16_t) (outH - dstH - top);

    /* horizontal: keep aspect ratio of the stock (full height) picture */
    baseW = (uint16_t) (outW - sBaseLeft - sBaseRight);
    newW = (uint16_t) (((uint32_t) baseW * dstH) / outH);
    extra = (uint16_t) (baseW - newW);

    writeOverscan((uint16_t) (sBaseLeft + extra / 2), (uint16_t) (sBaseRight + (extra - extra / 2)), top, bottom);

    DC_Write32(D1SCL_VERT_FILTER_CONTROL, SCL_V_FILTER_PICK_NEAREST);
#if SHARP_MODE >= 2
    DC_Write32(D1SCL_HORZ_FILTER_CONTROL, SCL_H_FILTER_PICK_NEAREST);
#else
    DC_Write32(D1SCL_HORZ_FILTER_CONTROL, sOrigHorzFilter);
#endif
#endif
}
