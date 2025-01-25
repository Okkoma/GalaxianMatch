#pragma once

using namespace Urho3D;

#include "BossLogic.h"

class TicTacToeLogic : public BossLogic
{
    URHO3D_OBJECT(TicTacToeLogic, BossLogic);

    public:
        TicTacToeLogic(Context* context);
        virtual ~TicTacToeLogic();

        static void RegisterObject(Context* context);

        virtual void OnSetEnabled();

    protected :
        void StartGame(char firstplayer);

        void OnBossStateChange(StringHash eventType, VariantMap& eventData);
        void HandleGame(StringHash eventType, VariantMap& eventData);

        virtual void SubscribeToEvents();
        virtual void UnsubscribeFromEvents();

    private :
        bool ticTacToeMode_;
        char firstplayer_, currentplayer_, winner_;
        int turn_;
        int case_;
        int difficulty_;
        float elapedTime_, turnTime_;
};

