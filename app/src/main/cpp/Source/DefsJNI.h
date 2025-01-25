#pragma once

#if defined(__ANDROID__)

#include <SDL/SDL.h>

#include <jni.h>

#include "DefsGame.h"

#ifdef __cplusplus
extern "C" {
#endif

// Export C++ Functions To Java

JNIEXPORT void GAME_JAVA_INTERFACE(PlayTest)(JNIEnv* env, jclass jcls, jint test);
JNIEXPORT void GAME_JAVA_INTERFACE(AddReward)(JNIEnv* env, jclass jcls, jint rewardid);

#ifdef __cplusplus
}
#endif

// Call Java Class Inside C++
void Android_FinishTest();
bool Android_ShowAds();

#endif
