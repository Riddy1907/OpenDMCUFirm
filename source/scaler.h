/*
 *   This file is part of OpenDMCUFirm
 *
 *   Sharp scaling for the TV output in vWii mode.
 */
#ifndef _DMCU_SCALER_H_
#define _DMCU_SCALER_H_

#include <stdint.h>
#include "types.h"

/*
 * SHARP_MODE (set at build time, e.g. `make SHARP_MODE=1`):
 *   0 = stock behaviour (bilinear scaling to the full screen)
 *   1 = integer 2x vertical line doubling (nearest neighbour), horizontal stays smooth,
 *       aspect ratio is preserved by shrinking the horizontal size accordingly
 *   2 = like 1, but horizontal scaling also uses nearest neighbour
 */
#ifndef SHARP_MODE
#define SHARP_MODE 1
#endif

/* Remember the overscan (pillarbox) values the system requested for the TV. */
void SCALER_SetTVBaseOverscan(uint16_t left, uint16_t right);

/* (Re)apply the sharp scaling configuration for the TV output. Safe to call repeatedly. */
void SCALER_ApplyTV(void);

#endif
