#include <jni.h>
#include <string.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "Common.h"
#include "Enums.h"
#include "FixP.h"
#include "Vec.h"
#include "Vec.h"
#include "LoadBitmap.h"
#include "CActor.h"
#include "Core.h"
#include "Engine.h"
#include "Dungeon.h"
#include "MapWithCharKey.h"
#include "CTile3DProperties.h"
#include "Renderer.h"
#include "FixP.h"
#include "Vec.h"
#include "Enums.h"
#include "CActor.h"
#include "MapWithCharKey.h"
#include "Common.h"
#include "LoadBitmap.h"
#include "Engine.h"
#include "CTile3DProperties.h"
#include "Renderer.h"
#include "VisibilityStrategy.h"
#include "PackedFileReader.h"
#include "SoundSystem.h"
#include "Matrices.h"

int width = 640;
int height = 480;
#define ANGLE_TURN_THRESHOLD 40
#define ANGLE_TURN_STEP 5

extern t_mat4x4 projection_matrix;
extern t_mat4x4 viewMatrix;
extern t_mat4x4 transformMatrix;
extern t_mat4x4 rotateXMatrix;
extern t_mat4x4 rotateYMatrix;
extern t_mat4x4 rotateZMatrix;


extern int turning;

enum ESoundDriver soundDriver = kNoSound;

int isInstantApp = FALSE;

void enterState(enum EGameMenuState newState);

void mainLoop(void);

extern int currentGameMenuState;

int enable3DRendering = FALSE;

AAssetManager *defaultAssetManager = NULL;

void graphicsInit(void) {

    enableSmoothMovement = TRUE;
    defaultFont = NULL;
}

void handleSystemEvents(void) {

}

void graphicsShutdown(void) {
    releaseBitmap(defaultFont);
    texturesUsed = 0;
}

void flipRenderer(void) {

}


JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_initAssets(JNIEnv *env, jclass clazz,
                                                       jobject assetManager) {

    AAssetManager *asset_manager = AAssetManager_fromJava(env, assetManager);
    defaultAssetManager = asset_manager;
    initHW(0, NULL);
    enableSmoothMovement = TRUE;
    enterState(kPlayGame);
}

int soundToPlay = -1;

void stopSounds(void) {}

void playSound(const uint8_t action) {
    soundToPlay = action;
}

void soundTick(void) {}

void muteSound(void) {}

JNIEXPORT jint JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_getSoundToPlay(JNIEnv *env, jclass clazz) {
    int toReturn = soundToPlay;
    soundToPlay = -1;
    return toReturn;
}

JNIEXPORT jint JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_isOnMainMenu(JNIEnv *env, jclass clazz) {
    return currentGameMenuState == (kMainMenu);
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_sendCommand(JNIEnv *env, jclass clazz, jchar cmd) {
    switch (cmd) {
        case 'w':
            mBufferedCommand = kCommandUp;
            break;

        case 's':
            mBufferedCommand = kCommandDown;
            break;

        case 'z':
            mBufferedCommand = kCommandFire1;
            break;


        case 'x':
            mBufferedCommand = kCommandFire2;
            break;

        case 'c':
            mBufferedCommand = kCommandFire3;
            break;

        case 'v':
            mBufferedCommand = kCommandFire4;
            break;

        case 'a':
            turning = 1;
            leanX = -ANGLE_TURN_STEP;
            break;

        case 'd':
            turning = 1;
            leanX = ANGLE_TURN_STEP;
            break;

        case 'q':
            mBufferedCommand = kCommandBack;
            break;

        case 'n':
            mBufferedCommand = kCommandStrafeLeft;
            break;

        case 'm':
            mBufferedCommand = kCommandStrafeRight;
            break;
    }
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_init(JNIEnv *env, jclass clazz, jint jwidth, jint jheight) {
    width = jwidth;
    height = jheight;

    initGL();

    enableSmoothMovement = TRUE;
    enable3DRendering = TRUE;
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_drawFrame(JNIEnv *env, jclass clazz) {
    if (enable3DRendering) {
        isRunning = isRunning && menuTick(10);
    }
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_onDestroy(JNIEnv *env, jclass clazz) {
    enable3DRendering = FALSE;
    unloadTextures();
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_startFrame(JNIEnv *env, jclass clazz) {
    if (enable3DRendering) {
        startFrame(0, 0, width, height);
    }
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_setPerspective(JNIEnv *env, jclass clazz,
                                                      jfloatArray perspective) {
    jfloat *raw = (*env)->GetFloatArrayElements(env, perspective, NULL);

    memcpy(projection_matrix, raw, sizeof(float) * 16);

    (*env)->ReleaseFloatArrayElements(env, perspective, raw, 0);
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_setLookAtMatrix(JNIEnv *env, jclass clazz,
                                                       jfloatArray look_at) {
    jfloat *raw = (*env)->GetFloatArrayElements(env, look_at, NULL);

    memcpy(viewMatrix, raw, sizeof(float) * 16);

    (*env)->ReleaseFloatArrayElements(env, look_at, raw, 0);

}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_setXZAngle(JNIEnv *env, jclass clazz, jfloat xz_angle) {
    // TODO: implement setXZAngle()
}

JNIEXPORT void JNICALL
Java_pt_b13h_equivalencevr_DerelictJNI_endFrame(JNIEnv *env, jclass clazz) {
    endFrame();
}