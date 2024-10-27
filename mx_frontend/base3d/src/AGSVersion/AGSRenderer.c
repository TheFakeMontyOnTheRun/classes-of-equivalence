//
// Created by Daniel Monteiro on 04/01/2023.
//
#include "gba_video.h"
#include "gba_systemcalls.h"
#include "gba_input.h"
#include "gba_interrupt.h"
#include "fade.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "Common.h"
#include "Enums.h"
#include "FixP.h"
#include "Vec.h"
#include "Globals.h"
#include "Vec.h"
#include "LoadBitmap.h"
#include "CActor.h"
#include "Core.h"
#include "Engine.h"
#include "Dungeon.h"
#include "MapWithCharKey.h"
#include "CTile3DProperties.h"
#include "Renderer.h"

#define VRAM_PAGE_A ((uint8_t*)0x6000000)
#define VRAM_PAGE_B ((uint8_t*)0x600A000)


#define COOLDOWN 0x4
int snapshotSignal = '.';
int cooldown = 0;

/* DMG Sound registers */
#define REG_NR52 (*(volatile u8 *)0x4000084) /* Sound on/off */
#define REG_NR50 (*(volatile u8 *)0x4000080) /* Left/Right volume control */
#define REG_NR51 (*(volatile u8 *)0x4000081) /* Channel enable */
#define REG_NR10 (*(volatile u8 *)0x4000060) /* Channel 1 sweep */
#define REG_NR11 (*(volatile u8 *)0x4000062) /* Channel 1 duty and length */
#define REG_NR12 (*(volatile u8 *)0x4000063) /* Channel 1 envelope */
#define REG_NR13 (*(volatile u8 *)0x4000064) /* Channel 1 frequency low */
#define REG_NR14 (*(volatile u8 *)0x4000065) /* Channel 1 frequency high */

void init_sound(void) {
    REG_NR52 = 0x80;  /* Turn on sound */
    REG_NR51 = 0xFF;  /* Enable all sound channels */
    REG_NR50 = 0x77; /* Set volume for L&R channels */
}

void play_square_wave(void) {

    REG_NR10 = 0x00;  /* Sweep register (not used) */
    REG_NR11 = 0xBF;  /* Duty cycle and length (50% duty, 64 steps) */
    REG_NR12 = 0xF3;  /* Volume envelope (max volume) */

    REG_NR13 = 0x02;  /* Lower byte of frequency (set to 2 for C4) */
    REG_NR14 = 0x87;  /* Higher 3 bits of frequency (set to 0x80 for start and 0x07 for frequency) */
}


uint8_t getPaletteEntry(const uint32_t origin) {
	uint8_t shade;

	if (!(origin & 0xFF000000)) {
		return TRANSPARENCY_COLOR;
	}

	shade = 0;
	shade += (((((origin & 0x0000FF)) << 2) >> 8)) << 6;
	shade += (((((origin & 0x00FF00) >> 8) << 3) >> 8)) << 3;
	shade += (((((origin & 0xFF0000) >> 16) << 3) >> 8)) << 0;

	return shade;
}

void VblankInterrupt(void) {
    scanKeys();
}

void graphicsInit(void) {
	int r, g, b;
    uint16_t palette[256];
    // Set up the interrupt handlers
    irqInit();

    irqSet( IRQ_VBLANK, VblankInterrupt);

    // Enable Vblank Interrupt to allow VblankIntrWait
    irqEnable(IRQ_VBLANK);

    // Allow Interrupts
    REG_IME = 1;

    SetMode( MODE_4 | BG2_ON );		// screen mode & background to display

	framebuffer = (uint8_t*)allocMem(XRES_FRAMEBUFFER * YRES_FRAMEBUFFER, GENERAL_MEMORY, FALSE);
    memFill(palette, 0, sizeof(uint16_t) * 256);

    for (b = 0; b < 256; b += 16) {
        for (g = 0; g < 256; g += 8) {
            for (r = 0; r < 256; r += 8) {
                uint32_t pixel = 0xFF000000 + (b << 16) + (g << 8) + (r);
                uint8_t paletteEntry = getPaletteEntry(pixel);
                palette[paletteEntry] = RGB8(r - 0x38, g - 0x18, b - 0x10);
            }
        }
    }

    FadeToPalette( palette, 60);

	defaultFont = loadBitmap("font.img");
	enableSmoothMovement = TRUE;
    init_sound();
    play_square_wave();
}

void handleSystemEvents(void) {

    scanKeys();

    if (cooldown > 0) {
        cooldown--;
    } else {

        u16 keys = keysHeld();

        if ((keys & KEY_UP)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandUp;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }
        if ((keys & KEY_DOWN)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandDown;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }

        if ((keys & KEY_LEFT)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandLeft;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }
        if ((keys & KEY_RIGHT)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandRight;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }
        if ((keys & KEY_A)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandFire1;
            needsToRedrawHUD = TRUE;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }

        if ((keys & KEY_B)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandFire2;
            needsToRedrawHUD = TRUE;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }

        if ((keys & KEY_SELECT)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandFire4;
            needsToRedrawHUD = TRUE;
        }

        if ((keys & KEY_START)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandFire3;
            needsToRedrawHUD = TRUE;
        }

        if ((keys & KEY_L)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandStrafeLeft;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }

        if ((keys & KEY_R)) {
            cooldown = COOLDOWN;
            mBufferedCommand = kCommandStrafeRight;
            needsToRedrawVisibleMeshes = TRUE;
            visibilityCached = FALSE;
        }
    }
}

void graphicsShutdown(void) {
}

void flipRenderer(void) {

    renderPageFlip(VRAM_PAGE_A, framebuffer,
                   VRAM_PAGE_B, turnStep, turnTarget, 0);
    VBlankIntrWait();
}

void clearRenderer(void) {}
