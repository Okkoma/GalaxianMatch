#if defined(__ANDROID__)

#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include "GameStatics.h"

#include "DefsJNI.h"

// Export C++ Functions To Java

JNIEXPORT void JNICALL GAME_JAVA_INTERFACE(AddReward)(JNIEnv* env, jclass jcls, jint rewardid)
{
    GameStatics::rewardEventStack_.Resize(GameStatics::rewardEventStack_.Size()+1);
    RewardEventItem& item = GameStatics::rewardEventStack_.Back();
    item.rewardid_ = rewardid > 8 ? 0 : rewardid;
    item.interactive_ = false;
    URHO3D_LOGINFOF("Game() - AddReward : rewardid=%d !", rewardid);
}

// Call Java Class Inside C++

bool Android_ShowAds()
{
    bool ok = false;

    URHO3D_LOGINFOF("Android_ShowAds ...");

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject mActivityInstance = (jobject)SDL_AndroidGetActivity();
    jclass mActivityClass = env->GetObjectClass(mActivityInstance);
    jmethodID midshowAds = env->GetMethodID(mActivityClass, "showAds", "(I)Z");

    URHO3D_LOGINFOF("Android_ShowAds : Launch : jobject=%u jclass=%u jmethod=%u", (void*)mActivityInstance, (void*)mActivityClass, (void*)midshowAds);

    if (midshowAds != NULL)
        ok = env->CallNonvirtualBooleanMethod(mActivityInstance, mActivityClass, midshowAds, 0);
    else
        URHO3D_LOGERROR("Android_ShowAds : no showRewardedVideo found !");

    env->DeleteLocalRef(mActivityInstance);

    return ok;
}

// Call From Java

#endif
