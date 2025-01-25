#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "GameEvents.h"

HashMap<StringHash, String> GOE::events_;

void GOE::InitEventTable()
{
    events_.Clear();
    events_ += Pair<StringHash,String>(StringHash("World_EntityCreate"), "World_EntityCreate");
    events_ += Pair<StringHash,String>(StringHash("World_EntityDestroy"), "World_EntityDestroy");

    events_ += Pair<StringHash,String>(StringHash("Go_Selected"), "Go_Selected");
    events_ += Pair<StringHash,String>(StringHash("Game_LevelWin"), "Game_LevelWin");
    events_ += Pair<StringHash,String>(StringHash("Game_NextLevel"), "Game_NextLevel");
    events_ += Pair<StringHash,String>(StringHash("Game_MoveRemoved"), "Game_MoveRemoved");
    events_ += Pair<StringHash,String>(StringHash("Game_MoveAdded"), "Game_MoveAdded");
    events_ += Pair<StringHash,String>(StringHash("Game_TryRemoved"), "Game_TryRemoved");
    events_ += Pair<StringHash,String>(StringHash("Game_TryAdded"), "Game_TryAdded");
    events_ += Pair<StringHash,String>(StringHash("Game_ScoreChange"), "Game_ScoreChange");
    events_ += Pair<StringHash,String>(StringHash("Game_ObjectiveChange"), "Game_ObjectiveChange");
    events_ += Pair<StringHash,String>(StringHash("Game_MatchStateChange"), "Game_MatchStateChange");
    events_ += Pair<StringHash,String>(StringHash("Game_BossAppears"), "Game_BossAppears");
    events_ += Pair<StringHash,String>(StringHash("Game_BossStateChange"), "Game_BossStateChange");
    events_ += Pair<StringHash,String>(StringHash("Game_AbilityReseted"), "Game_AbilityReseted");

    events_ += Pair<StringHash,String>(StringHash("Net_ModeChanged"), "Net_ModeChanged");
    events_ += Pair<StringHash,String>(StringHash("Net_ServerSeedTime"), "Net_ServerSeedTime");
    events_ += Pair<StringHash,String>(StringHash("Net_GameStatusChanged"), "Net_GameStatusChanged");
    events_ += Pair<StringHash,String>(StringHash("Net_ObjectCommand"), "Net_ObjectCommand");

    events_ += Pair<StringHash,String>(StringHash("Go_ChangeState"), "Go_ChangeState");
    events_ += Pair<StringHash,String>(StringHash("Go_Appear"), "Go_Appear");
    events_ += Pair<StringHash,String>(StringHash("Go_ChangeMap"), "Go_ChangeMap");
    events_ += Pair<StringHash,String>(StringHash("Go_Destroy"), "Go_Destroy");
    events_ += Pair<StringHash,String>(StringHash("Go_Killer"), "Go_Killer");
    events_ += Pair<StringHash,String>(StringHash("ControlUpdate"), "ControlUpdate");
    events_ += Pair<StringHash,String>(StringHash("ControlMove"), "ControlMove");
    events_ += Pair<StringHash,String>(StringHash("ControlAction0"), "ControlAction0");
    events_ += Pair<StringHash,String>(StringHash("ControlAction1"), "ControlAction1");
    events_ += Pair<StringHash,String>(StringHash("ControlAction2"), "ControlAction2");
    events_ += Pair<StringHash,String>(StringHash("ControlAction3"), "ControlAction3");
    events_ += Pair<StringHash,String>(StringHash("ControllerChange"), "ControllerChange");

    events_ += Pair<StringHash,String>(StringHash("GOC_Life_Decrease"), "GOC_Life_Decrease");
    events_ += Pair<StringHash,String>(StringHash("GOC_Life_Restore"), "GOC_Life_Restore");
    events_ += Pair<StringHash,String>(StringHash("GOC_Life_Dead"), "GOC_Life_Dead");
    events_ += Pair<StringHash,String>(StringHash("GOC_Life_Update"), "GOC_Life_Update");

    events_ += Pair<StringHash,String>(StringHash("Go_CollideGround"), "Go_CollideGround");
    events_ += Pair<StringHash,String>(StringHash("Go_CollideAttack"), "Go_CollideAttack");
    events_ += Pair<StringHash,String>(StringHash("Go_ReceiveEffect"), "Go_ReceiveEffect");
    events_ += Pair<StringHash,String>(StringHash("Go_ChangeDirection"), "Go_ChangeDirection");

    events_ += Pair<StringHash,String>(StringHash("Go_DetectorPlayerIn"), "Go_DetectorPlayerIn");
    events_ += Pair<StringHash,String>(StringHash("Go_DetectorPlayerOff"), "Go_DetectorPlayerOff");
    events_ += Pair<StringHash,String>(StringHash("Go_DetectorSwitchViewIn"), "Go_DetectorSwitchViewIn");
    events_ += Pair<StringHash,String>(StringHash("Go_DetectorSwitchViewOut"), "Go_DetectorSwitchViewOut");

    events_ += Pair<StringHash,String>(StringHash("Go_InventoryGet"), "Go_InventoryGet");
    events_ += Pair<StringHash,String>(StringHash("Go_InventoryEmpty"), "Go_InventoryEmpty");
    events_ += Pair<StringHash,String>(StringHash("Go_InventoryFull"), "Go_InventoryFull");
    events_ += Pair<StringHash,String>(StringHash("Go_InventoryReceive"), "Go_InventoryReceive");
    events_ += Pair<StringHash,String>(StringHash("Go_InventoryGive"), "Go_InventoryGive");

    events_ += Pair<StringHash,String>(StringHash("Go_ItemLoopUse"), "Go_ItemLoopUse");
    events_ += Pair<StringHash,String>(StringHash("Go_ItemEndUse"), "Go_ItemEndUse");
    events_ += Pair<StringHash,String>(StringHash("Go_EquipmentUpdated"), "Go_EquipmentUpdated");
    events_ += Pair<StringHash,String>(StringHash("Go_EquipmentHolderChanged"), "Go_EquipmentHolderChanged");

    events_ += Pair<StringHash,String>(StringHash("ANIM_Event"), "ANIM_Event");
    events_ += Pair<StringHash,String>(StringHash("Event_OnFall"), "Event_OnFall");
    events_ += Pair<StringHash,String>(StringHash("Event_OnJump"), "Event_OnJump");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_Part"), "SPRITER_Part");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_PartFeet"), "SPRITER_PartFeet");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_Particule"), "SPRITER_Particule");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_Light"), "SPRITER_Light");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_SoundFeet"), "SPRITER_SoundFeet");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_SoundAttack"), "SPRITER_SoundAttack");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_Entity"), "SPRITER_Entity");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_Animation"), "SPRITER_Animation");
    events_ += Pair<StringHash,String>(StringHash("SPRITER_AnimationInside"), "SPRITER_AnimationInside");

    events_ += Pair<StringHash,String>(StringHash("Go_CollectableDrop"), "Go_CollectableDrop");
    events_ += Pair<StringHash,String>(StringHash("Go_AbilityAdded"), "Go_AbilityAdded");
    events_ += Pair<StringHash,String>(StringHash("Go_AbilityRemoved"), "Go_AbilityRemoved");
    events_ += Pair<StringHash,String>(StringHash("Go_PlayerMissionManagerStart"), "Go_PlayerMissionManagerStart");
    events_ += Pair<StringHash,String>(StringHash("Go_PlayerTransferCollectableStart"), "Go_PlayerTransferCollectableStart");
    events_ += Pair<StringHash,String>(StringHash("Go_PlayerTransferCollectableFinish"), "Go_PlayerTransferCollectableFinish");
    events_ += Pair<StringHash,String>(StringHash("Go_MissionAdded"), "Go_MissionAdded");
    events_ += Pair<StringHash,String>(StringHash("Go_MissionUpdated"), "Go_MissionUpdated");
    events_ += Pair<StringHash,String>(StringHash("Go_ObjectiveUpdated"), "Go_ObjectiveUpdated");
    events_ += Pair<StringHash,String>(StringHash("Panel_SlotUpdated"), "Panel_SlotUpdated");
    events_ += Pair<StringHash,String>(StringHash("Panel_SwitchVisibleSubscribers"), "Panel_SwitchVisibleSubscribers");

    events_ += Pair<StringHash,String>(StringHash("Ai_PostUpdate"), "Ai_PostUpdate");
    events_ += Pair<StringHash,String>(StringHash("Go_StartTimer"), "Go_StartTimer");
    events_ += Pair<StringHash,String>(StringHash("Go_EndTimer"), "Go_EndTimer");
    events_ += Pair<StringHash,String>(StringHash("Go_TimerInformer"), "Go_TimerInformer");

//    events_ += Pair<StringHash,String>(StringHash("Go_RemoveFinish"), "Go_RemoveFinish");

    events_ += Pair<StringHash,String>(StringHash("SplashScreen_Finish"), "SplashScreen_Finish");

    events_ += Pair<StringHash,String>(StringHash("World_MapUpdate"), "World_MapUpdate");
    events_ += Pair<StringHash,String>(StringHash("World_MapUpdate"), "World_MapUpdate");
    events_ += Pair<StringHash,String>(StringHash("World_EntityUpdate"), "World_EntityUpdate");

    events_ += Pair<StringHash,String>(StringHash("ViewManager_SwitchView"), "ViewManager_SwitchView");

}

StringHash GOE::Register(const String& eventName)
{
    StringHash event = StringHash(eventName);

    if (events_.Contains(event))
    {
//        URHO3D_LOGINFOF("GOE() : Register eventName=%s(%u) (Warning:events_ contains already the key)",eventName.CString(),event.Value());
        return event;
    }

    events_[event] = eventName;
    return event;
}

const String& GOE::GetEventName(const StringHash& hashevent)
{
    HashMap<StringHash, String>::ConstIterator it = events_.Find(hashevent);
    return it != events_.End() ? it->second_ : String::EMPTY;
}

void GOE::DumpAll()
{
    URHO3D_LOGINFOF("GOE() - DumpAll : events_ Size =", events_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, String>::ConstIterator it=events_.Begin();it!=events_.End();++it)
    {
        URHO3D_LOGINFOF("     => events_[%u] = (hash:%u (dec:%d) - name:%s)", index, it->first_.Value(), it->first_.Value(), it->second_.CString());
        ++index;
    }
}
