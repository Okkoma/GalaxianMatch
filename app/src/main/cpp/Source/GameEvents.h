#pragma once

#include <Urho3D/Core/Object.h>

//#define URHO3D_EVENT(eventID, eventName) static const Urho3D::StringHash eventID(#eventName); namespace eventName
//#define URHO3D_PARAM(paramID, paramName) static const Urho3D::StringHash paramID(#paramName)
//#define URHO3D_HANDLER(className, function) (new Urho3D::EventHandlerImpl<className>(this, &className::function))
//#define URHO3D_HANDLER_USERDATA(className, function, userData) (new Urho3D::EventHandlerImpl<className>(this, &className::function, userData))

namespace Urho3D
{
/// Game
    URHO3D_EVENT(GAME_SCREENRESIZED, Game_ScreenResized) { }
    URHO3D_EVENT(GAME_SCREENDELAYED, Game_ScreenDelayed) { }
    URHO3D_EVENT(GAME_PRELOADINGFINISHED, Game_PreloadingFinished) { }

/// World2D
    /// => Sender : ObjectTiled
    /// => Subscribers : World2D => reload all chunks
    URHO3D_EVENT(WORLD_DIRTY, World_Dirty) { }
    /// => Sender : Map, BodyExploder, SimpleActor, Collectable, Animator2D
    /// => Subscribers : GOC_Destroyer
    URHO3D_EVENT(WORLD_ENTITYCREATE, World_EntityCreate) { }
    /// => Sender : Collectable, Animator2D
    /// => Subscribers : GOC_Destroyer
    URHO3D_EVENT(WORLD_ENTITYDESTROY, World_EntityDestroy) { }
/// sPLAY
    URHO3D_EVENT(GO_SELECTED, Go_Selected)
    {
        URHO3D_PARAM(GO_ID, GoID);             // ID of the GO
        URHO3D_PARAM(GO_TYPE, GoType);         // Type of controller (player,AI,none)
        URHO3D_PARAM(GO_ACTION, GoAction);     // Type of Action to Release by AI
    }

    URHO3D_EVENT(GAME_SWITCHSCENEMODE, Game_SwitchSceneMode) { }
    URHO3D_EVENT(GAME_SWITCHPLANETMODE, Game_SwitchPlanetMode) { }
    URHO3D_EVENT(GAME_LEVELTOLOAD, Game_LevelLoaded) { }
    URHO3D_EVENT(GAME_LEVELREADY, Game_LevelReady) { }
    URHO3D_EVENT(GAME_LEVELWIN, Game_LevelWin) { }
    URHO3D_EVENT(GAME_OVER, Game_Over) { }
    URHO3D_EVENT(GAME_UNPAUSE, Game_UnPause) { }
    URHO3D_EVENT(GAME_NEXTLEVEL, Game_NextLevel) { }

    URHO3D_EVENT(GAME_EARNSTARS, Game_EarnStars)
    {
        URHO3D_PARAM(NUMSTARS, NumStars); // int
    }
    URHO3D_EVENT(GAME_MOVEREMOVED, Game_MoveRemoved) { }
    URHO3D_EVENT(GAME_MOVEADDED, Game_MoveAdded) { }
    URHO3D_EVENT(GAME_MOVERESTORED, Game_MoveRestored) { }
    URHO3D_EVENT(GAME_TRYREMOVED, Game_TryRemoved) { }
    URHO3D_EVENT(GAME_TRYADDED, Game_TryAdded) { }
    URHO3D_EVENT(GAME_TRYRESTORED, Game_TryRestored) { }
    URHO3D_EVENT(GAME_COINUPDATED, Game_CoinUpdated) { }
    URHO3D_EVENT(GAME_STORYITEMSUPDATED, GameStoryItemsUpdated) { }

    URHO3D_EVENT(GAME_ABILITYADDED, Game_AbilityAdded)
    {
        URHO3D_PARAM(ABILITY_ID, AbilityId);
    }
    URHO3D_EVENT(GAME_ABILITYREMOVED, Game_AbilityRemoved)
    {
        URHO3D_PARAM(ABILITY_ID, AbilityId);
    }
    URHO3D_EVENT(GAME_SCORECHANGE, Game_ScoreChange) { }
    URHO3D_EVENT(GAME_OBJECTIVECHANGE, Game_ObjectiveChange) { }
    URHO3D_EVENT(GAME_MATCHSTATECHANGE, Game_MatchStateChange) { }
    URHO3D_EVENT(GAME_NOMATCHSTATE, Game_NoMatchState) { }
    URHO3D_EVENT(GAME_BOSSAPPEARS, Game_BossAppears) { }
    URHO3D_EVENT(GAME_BOSSSTATECHANGE, Game_BossStateChange) { }
    URHO3D_EVENT(GAME_UIFRAME_ADD, Game_UIFrameAdd)
    {
        URHO3D_PARAM(FRAME, Frame);
    }
    URHO3D_EVENT(GAME_UIFRAME_START, Game_UIFrameStart)
    {
        URHO3D_PARAM(FRAME, Frame);
    }
    URHO3D_EVENT(GAME_UIFRAME_STOP, Game_UIFrameStop)
    {
        URHO3D_PARAM(FRAME, Frame);
    }
    URHO3D_EVENT(GAME_UIFRAME_REMOVE, Game_UIFrameRemove)
    {
        URHO3D_PARAM(FRAME, Frame);
    }
    URHO3D_EVENT(GAME_DESTROYALLUIFRAMES, Game_DestroyAllUIFrames) { }
    URHO3D_EVENT(GAME_STATEBEGIN, Game_StateBegin) { }
    URHO3D_EVENT(GAME_STATEBACK, Game_StateBack) { }
    URHO3D_EVENT(GAME_STATEREADY, Game_StateReady) { }
    URHO3D_EVENT(GAME_STATEEND, Game_StateEnd) { }
    URHO3D_EVENT(GAME_PLAYEXIT, Game_PlayExit) { }
    URHO3D_EVENT(GAME_PLAYTEST, Game_Playtest) { }

/// MAN_MATCHES
    URHO3D_EVENT(GAME_ABILITYRESETED, Game_AbilityReseted) { }
    URHO3D_EVENT(GAME_POWERUNLOCKED, Game_PowerUnlocked)
    {
        URHO3D_PARAM(POWER_ID, PowerId);  // int
    }
    URHO3D_EVENT(GAME_POWERADDED, Game_PowerAdded)
    {
        URHO3D_PARAM(MATCHPROPERTY, MProperty);
        URHO3D_PARAM(NODE, Node);         // node*
    }

/// SPLASHSCREEN Events
    URHO3D_EVENT(SPLASHSCREEN_EVENTS, SplashScreen_Events)
    {
        URHO3D_PARAM(KEEPSTATEDELAY, SplashScreen_KeepStateDelay); // float
    }
    URHO3D_EVENT(SPLASHSCREEN_STOP, SplashScreen_Stop) { }
    URHO3D_EVENT(SPLASHSCREEN_STARTCLOSE, SplashScreen_Begin) { }
    URHO3D_EVENT(SPLASHSCREEN_FINISHCLOSE, SplashScreen_FinishClose) { }
    URHO3D_EVENT(SPLASHSCREEN_STARTOPEN, SplashScreen_StartFinish) { }
    URHO3D_EVENT(SPLASHSCREEN_FINISHOPEN, SplashScreen_EndFinish) { }

/// UIMENU
    URHO3D_EVENT(UIMENU_SHOWCONTENT, UIMenu_ShowContent) { }
    URHO3D_EVENT(UIMENU_HIDECONTENT, UIMenu_HideContent) { }
/// UIDIALOG
    URHO3D_EVENT(UIDIALOG_SHOWCONTENT, UIDialog_ShowContent) { }
    URHO3D_EVENT(UIDIALOG_HIDECONTENT, UIDialog_HideContent) { }
/// TUTORIAL
    URHO3D_EVENT(TUTORIAL_LAUNCH, Tutorial_Launch) { }
    URHO3D_EVENT(TUTORIAL_NEXT, Tutorial_Next) { }
/// COMPANION
    URHO3D_EVENT(MESSAGE_START, Message_Start) { }
    URHO3D_EVENT(MESSAGE_STOP, Message_Stop) { }
    URHO3D_EVENT(COMPANION_SHOW, Companion_Show)
    {
        URHO3D_PARAM(CONTENT, Content);
    }
    URHO3D_EVENT(COMPANION_HIDE, Companion_Hide) { }
/// DELAYEDACTION
    URHO3D_EVENT(DELAYEDACTION_EXECUTE, DelayedAction_Execute) { }

/// Network
    URHO3D_EVENT(NET_MODECHANGED, Net_ModeChanged) { }
    URHO3D_EVENT(NET_SERVERSEEDTIME, Net_ServerSeedTime)
    {
        URHO3D_PARAM(P_SEEDTIME, SeedTime);        // Server Seed Time (Unsigned)
    }
    URHO3D_EVENT(NET_GAMESTATUSCHANGED, Net_GameStatusChanged)
    {
        URHO3D_PARAM(P_TIMESTAMP, TimeStamp);           // TimeStamp
        URHO3D_PARAM(P_STATUS, PlayStatus);                  // PlayStatus (Int)
        URHO3D_PARAM(P_NUMPLAYERS, NumPlayers);         // Num Players (Int)
    }
    URHO3D_EVENT(NET_OBJECTCOMMAND, Net_ObjectCommand)
    {
        URHO3D_PARAM(P_COMMAND, NetCommand);                                     // ServerCommand (Int)
        URHO3D_PARAM(P_NODEID, NetNodeID);                                             // ServerNodeID (Unsigned)
        URHO3D_PARAM(P_NODEIDFROM, NetNodeIDFrom);                              // ServerNodeFromID (Unsigned)
        URHO3D_PARAM(P_NODEPTRFROM, NetNodePtrFrom);                          // Node Ptr From (Pointer)
        URHO3D_PARAM(P_NODEISENABLED, NetNodeIsEnabled);                       // Bool
        URHO3D_PARAM(P_CLIENTOBJECTTYPE, ClientObjectType);                   // Unsigned
        URHO3D_PARAM(P_CLIENTOBJECTVIEWZ, ClientObjectViewZ);                // Unsigned
        URHO3D_PARAM(P_CLIENTOBJECTFACTION, ClientObjectFaction);           // Unsigned
        URHO3D_PARAM(P_CLIENTOBJECTPOSITION, ClientObjectPosition);         // Vector2
        URHO3D_PARAM(P_CLIENTOBJECTVELOCITY, ClientObjectVelocity);         // Vector2
        URHO3D_PARAM(P_CLIENTOBJECTDIRECTION, ClientObjectDirection);      // Float
        URHO3D_PARAM(P_SLOTPARTFROMTYPE, ClientSlotPartFromType);         // uint
        URHO3D_PARAM(P_SLOTQUANTITY, ClientSlotQuantity);                         // long : Slot Quantity
        URHO3D_PARAM(P_SLOTSPRITE, ClientSlotSprite);                                 // uint : Slot Hash of the sprite
        URHO3D_PARAM(P_SLOTSCALE, ClientSlotScale);                                   // float : Slot scale x
        URHO3D_PARAM(P_SLOTCOLOR, ClientSlotColor);                                   // uint : Slot color
        URHO3D_PARAM(P_INVENTORYTEMPLATE, ClientInventoryTemplate);       // uint : Inventory Template HashName
        URHO3D_PARAM(P_INVENTORYITEMTYPE, ClientItemType);                     // uint : TYPE of the Item
        URHO3D_PARAM(P_INVENTORYIDSLOT, ClientInventoryIdSlot);                 // uint : ID of the slot
    }

/// GOC_Animator
    /// CHANGE STATE Event
    URHO3D_EVENT(GO_CHANGESTATE, Go_ChangeState)
    {
        URHO3D_PARAM(GO_ID, GoID);         // ID of the GO that animState change
        URHO3D_PARAM(GO_STATE, GoState);   // unsigned : new State
    }
    /// APPEAR Event
    /// => Sender : GOC_Destroyer
    /// => Subscribers : GOManager, World2D
    URHO3D_EVENT(GO_APPEAR, Go_Appear)
    {
        URHO3D_PARAM(GO_ID, GoID);         // ID of the node GO appeared
        URHO3D_PARAM(GO_TYPE, GoType);     // Type of controller (player,AI,none)
        URHO3D_PARAM(GO_MAINCONTROL, GoMainControl);     // AI mainControl
        URHO3D_PARAM(GO_MAP, GoMap);
    }
    /// CHANGEMAP Event
    /// => Sender : GOC_Destroyer
    /// => Subscribers : World2D
    URHO3D_EVENT(GO_CHANGEMAP, Go_ChangeMap)
    {
        URHO3D_PARAM(GO_ID, GoID);         // ID of the node GO appeared
        URHO3D_PARAM(GO_TYPE, GoType);     // Type of controller (player,AI,none)
        URHO3D_PARAM(GO_MAPFROM, MapFrom);
        URHO3D_PARAM(GO_MAPTO, MapTo);
    }
    /// DESTROY Event
    /// => Sender : GOC_Animator2D, GOC_Destroyer, GOC_Collectable,
    /// => Subscribers : GOManager, World2D, GOC_Destroyer
    URHO3D_EVENT(GO_DESTROY, Go_Destroy)
    {
        URHO3D_PARAM(GO_ID, GoID);         // ID of the GO To Destroy
        URHO3D_PARAM(GO_TYPE, GoType);     // Type of controller (player,AI,none)
        URHO3D_PARAM(GO_MAP, GoMap);
        URHO3D_PARAM(GO_PTR, GoPtr);       // Node*
    }

/// GOC_Controller
    /// Control Update Event
    /// => Subscribers : GOC_Move2D
    URHO3D_EVENT(GOC_CONTROLUPDATE, ControlUpdate) { }
    /// Move Event
    URHO3D_EVENT(GOC_CONTROLMOVE, ControlMove) { }
    /// Action0 Press Event : Jump
    URHO3D_EVENT(GOC_CONTROLACTION0, ControlAction0) { }
    /// Action1 Press Event : Fire
    /// => Subscribers : GOC_Attack
    URHO3D_EVENT(GOC_CONTROLACTION1, ControlAction1) { }
    /// Action2 Press Event : Fire2
    URHO3D_EVENT(GOC_CONTROLACTION2, ControlAction2) { }
    /// Action3 Press Event : Fire3
    URHO3D_EVENT(GOC_CONTROLACTION3, ControlAction3) { }

    /// Change Controller Event
    URHO3D_EVENT(GOC_CONTROLLERCHANGE, ControllerChange)
    {
        URHO3D_PARAM(NODE_ID, Node_ID);             // Node ID
        URHO3D_PARAM(GO_TYPE, GoType);              // Type of controller (player,AI,none)
        URHO3D_PARAM(GO_MAINCONTROL, GoMainControl);     // AI mainControl
        URHO3D_PARAM(NEWCTRL_PTR, NewControl);      // Pointer of new GOC_Controller
    }
/// GOC_Life
    /// LIFE RESTORE Event
    URHO3D_EVENT(GOC_LIFERESTORE, GOC_Life_Restore) { }
    /// ENERGY DECREASE Event
    URHO3D_EVENT(GOC_LIFEDEC, GOC_Life_Decrease) { }
    /// ENERGY UPDATE Event
    URHO3D_EVENT(GOC_LIFEUPDATE, GOC_Life_Update) { }
    /// DEAD Event
    /// => Sender : GOC_Life, GOC_Destroyer
    /// => Subscribers : GOManager, GOC_Inventory(Drop), GOC_Collectable(Drop), Player(updateUI),
    /// ==> sPlay(updateScore), GOC_PlayerController(StopUpdateInput), GOC_AIController(StopUpdateAI),
    /// ==> GOC_BodyExploder(Explode), GOC_Animator2D(ChangeAnimation)
    URHO3D_EVENT(GOC_LIFEDEAD, GOC_Life_Dead) { }
    URHO3D_EVENT(GOC_LIFEEVENTS, GOC_Life_Events)
    {
        URHO3D_PARAM(GO_ID, GoID);                  // ID of the Node
        URHO3D_PARAM(GO_TYPE, GoType);         // Type of controller (player,AI,none)
        URHO3D_PARAM(GO_KILLER, GoKiller);      // ID of the GO killer
        URHO3D_PARAM(GO_WORLDCONTACTPOINT, GoWorldContactPoint);   // Vector2 (Last World contact point)
    }
    /// KILLER Entity
    /// is sent to the killer node
    /// => Sender : GOC_Life
    /// => Subscribers : class Actor, Player
    URHO3D_EVENT(GO_KILLER, Go_Killer)
    {
        URHO3D_PARAM(GO_ID, GoID);         // ID of the GO Killer
        URHO3D_PARAM(GO_DEAD, GoDead);     // ID of the GO Dead
    }
/// GOC_Collide2D
    /// GO Touch Walls Event
    URHO3D_EVENT(GO_COLLIDEGROUND, Go_CollideGround) { }

    URHO3D_EVENT(COLLIDEWALLBEGIN, CollideWallBegin)
    {
        URHO3D_PARAM(WALLCONTACTTYPE, WallContactType);         // int WallType (Ground,Border,Roof)
    }
    URHO3D_EVENT(COLLIDEWALLEND, CollideWallEnd)
    {
        URHO3D_PARAM(WALLCONTACTTYPE, WallContactType);         // int WallType (Ground,Border,Roof)
    }
    /// GO Touch Enemy Event
    URHO3D_EVENT(GO_COLLIDEATTACK, Go_CollideAttack)
    {
        URHO3D_PARAM(GO_ID, GoID);         // ID of the node GO
        URHO3D_PARAM(GO_ENEMY, GoEnemy);   // ID of the node GO Enemy touched
        URHO3D_PARAM(GO_WORLDCONTACTPOINT, GoWorldContactPoint);   // Vector2 (World contact point)
    }
/// GOC_Collide2D, GOC_ZoneEffect
    /// GO Receive Effect Event
    URHO3D_EVENT(GO_RECEIVEEFFECT, Go_ReceiveEffect)
    {
        URHO3D_PARAM(GO_RECEIVERID, GoReceiverID);         // UInt : ID of the node GO Receiver
        URHO3D_PARAM(GO_SENDERID, GoSenderID);             // UInt : ID of the node GO Sender
        URHO3D_PARAM(GO_SENDERFACTION, GoSenderFaction);   // UInt : Faction of the node GO Sender
        URHO3D_PARAM(GO_EFFECTID, GoEffectID);             // UInt : ID of the effectType
        URHO3D_PARAM(GO_VARAFFECTED, GoVar);               // UInt : Affected Variable
        URHO3D_PARAM(GO_QTY, GoQty);                       // float : Quantity to Apply on Variable affected
        URHO3D_PARAM(GO_WORLDCONTACTPOINT, GoWorldContactPoint);   // Vector2 (World contact point)
    }

/// GOC_Move2D
    /// GO Change Direction
    URHO3D_EVENT(GO_CHANGEDIRECTION, Go_ChangeDirection) { }
/// GOC_Detector
//    URHO3D_EVENT(GO_DETECTORPLAYERIN, Go_DetectorPlayerIn)
//    {
//        URHO3D_PARAM(GO_GETTER, GoGetter);
//    }
//    URHO3D_EVENT(GO_DETECTORPLAYEROFF, Go_DetectorPlayerOff)
//    {
//        URHO3D_PARAM(GO_GETTER, GoGetter);
//    }
//    URHO3D_EVENT(GO_DETECTORSWITCHVIEWIN, Go_DetectorSwitchViewIn)
//    {
//        URHO3D_PARAM(GO_GETTER, GoGetter);                              // ID of the node GO Getter
//    }
//    URHO3D_EVENT(GO_DETECTORSWITCHVIEWOUT, Go_DetectorSwitchViewOut)
//    {
//        URHO3D_PARAM(GO_GETTER, GoGetter);                              // ID of the node GO Getter
//    }
    URHO3D_EVENT(GO_DETECTOR, Go_Detector)
    {
        URHO3D_PARAM(GO_GETTER, GoGetter);
    }
    URHO3D_EVENT(GO_DETECTORPLAYERIN, Go_DetectorPlayerIn) { }
    URHO3D_EVENT(GO_DETECTORPLAYEROFF, Go_DetectorPlayerOff) { }
    URHO3D_EVENT(GO_DETECTORSWITCHVIEWIN, Go_DetectorSwitchViewIn) { }
    URHO3D_EVENT(GO_DETECTORSWITCHVIEWOUT, Go_DetectorSwitchViewOut) { }

/// GOC_Inventory
    /// GO_INVENTORYGET
    /// => Sender : GOC_Inventory, GOC_Collectable
    /// => Subscribers : Player
    URHO3D_EVENT(GO_INVENTORYGET, Go_InventoryGet)
    {
        URHO3D_PARAM(GO_GIVER, GoGiver);                                // ID of the node GO Giver (which has a GOC_Collectable or GOC_Inventory component)
        URHO3D_PARAM(GO_GETTER, GoGetter);                              // ID of the node GO Getter (which has a GOC_Inventory component)
        URHO3D_PARAM(GO_POSITION, GoPosition);                          // Variant (Vector3 or IntVector2) world position of item
        URHO3D_PARAM(GO_RESOURCEREF, GoResourceRef);                    // sprite resource ref
        URHO3D_PARAM(GO_IDSLOTSRC, Go_IdSlotSrc);                    // ID of the slot source (use in network AddItem)
        URHO3D_PARAM(GO_IDSLOT, Go_IdSlot);                             // ID of the slot destination (use in player for transfertoUI)
        URHO3D_PARAM(GO_QUANTITY, GoQuantity);                          // unsigned quantity of collectable
    }
    URHO3D_EVENT(GO_INVENTORYEMPTY, Go_InventoryEmpty) { }
    URHO3D_EVENT(GO_INVENTORYFULL, Go_InventoryFull) { }
    URHO3D_EVENT(GO_INVENTORYRECEIVE, Go_InventoryReceive) { }
    URHO3D_EVENT(GO_INVENTORYGIVE, Go_InventoryGive) { }
    URHO3D_EVENT(GO_INVENTORYSLOTEQUIP, Go_InventorySlotEquip) { }

/// GOC_Collectable
    /// GO_COLLECTABLEDROP
    /// => Sender : GOC_Collectable (DropSlotFrom)
    /// => Subscribers : Player
    URHO3D_EVENT(GO_COLLECTABLEDROP, Go_CollectableDrop)
    {
        URHO3D_PARAM(GO_TYPE, GoType);           // type of the node Collectable, (use in drop Collectable)
        URHO3D_PARAM(GO_QUANTITY, GoQuantity);   // unsigned quantity of collectable, (use in drop Collectable)
    }
    URHO3D_EVENT(GO_ITEMLOOPUSE, Go_ItemLoopUse) { }
    URHO3D_EVENT(GO_ITEMENDUSE, Go_ItemEndUse) { }
/// Player, Equipment
    URHO3D_EVENT(GO_HOLDERNODECHANGED, Go_ChangeHolderNode) { }
    URHO3D_EVENT(GO_EQUIPMENTUPDATED, Go_EquipmentUpdated) { }
    URHO3D_EVENT(GO_ABILITYADDED, Go_AbilityAdded) { }
    URHO3D_EVENT(GO_ABILITYREMOVED, Go_AbilityRemoved) { }
    URHO3D_EVENT(GO_PLAYERMISSIONMANAGERSTART, Go_PlayerMissionManagerStart) { }
    URHO3D_EVENT(GO_PLAYERTRANSFERCOLLECTABLESTART, Go_PlayerTransferCollectableStart) { }
    URHO3D_EVENT(GO_PLAYERTRANSFERCOLLECTABLEFINISH, Go_PlayerTransferCollectableFinish) { }
/// Mission Manager, Mission
    URHO3D_EVENT(GO_MISSIONADDED, Go_MissionAdded)
    {
        URHO3D_PARAM(GO_MISSIONID, GoMissionID);           // UInt : ID Mission
        URHO3D_PARAM(GO_RECEIVERID, GoReceiverID);         // UInt : ID Actor Receiver
        URHO3D_PARAM(GO_SENDERID, GoSenderID);             // UInt : ID Actor Sender
    }
    URHO3D_EVENT(GO_MISSIONUPDATED, Go_MissionUpdated)
    {
        URHO3D_PARAM(GO_MISSIONID, GoMissionID);           // UInt : ID Mission
        URHO3D_PARAM(GO_STATE, GoState);                   // Int : Mission State
    }
    URHO3D_EVENT(GO_OBJECTIVEUPDATED, Go_ObjectiveUpdated)
    {
        URHO3D_PARAM(GO_MISSIONID, GoMissionID);           // UInt : ID Mission
        URHO3D_PARAM(GO_OBJECTIVEID, GoObjectiveID);       // UInt : ID Objective
        URHO3D_PARAM(GO_STATE, GoState);                   // Int :  Objective State
    }
/// UISlotPanel
    URHO3D_EVENT(PANEL_SWITCHVISIBLESUBSCRIBERS, Panel_SwitchVisibleSubscribers) { }
    URHO3D_EVENT(PANEL_SLOTUPDATED, Panel_SlotUpdated) { }
//    {
//        URHO3D_PARAM(GO_IDSLOT, Go_IdSlot);
//    }
/// AIManager
    URHO3D_EVENT(AI_POSTUPDATE, Ai_PostUpdate)
    {
        URHO3D_PARAM(GO_UPDATEORWAIT, GoUpdateOrWait);  // bool = 0 = update ; 1 = wait state
    }
//    URHO3D_EVENT(AI_CALLBACKORDER, Ai_CallBackOrder)
//    {
//        URHO3D_PARAM(NODE_ID, Node_ID);      // Node ID : UInt
//        URHO3D_PARAM(ORDER, Order);          // UInt
//    }
/// Generic Animation Event
    URHO3D_EVENT(ANIM_EVENT, ANIM_Event)
    {
        URHO3D_PARAM(DATAS, datas);
    }
    URHO3D_EVENT(EVENT_DEFAULT, Event_Default) { }
    URHO3D_EVENT(EVENT_DEFAULT_GROUND, Event_Default_Ground) { }
    URHO3D_EVENT(EVENT_DEFAULT_AIR, Event_Default_Air) { }
    URHO3D_EVENT(EVENT_DEFAULT_CLIMB, Event_Default_Climb) { }
    URHO3D_EVENT(EVENT_DEFAULT_FLUID, Event_Default_Fluid) { }
    URHO3D_EVENT(EVENT_MOVE, Event_OnWalk) { }
    URHO3D_EVENT(EVENT_MOVE_GROUND, Event_OnWalk) { }
    URHO3D_EVENT(EVENT_MOVE_AIR, Event_OnFlyUp) { }
    URHO3D_EVENT(EVENT_MOVE_FLUID, Event_Move_Fluid) { }
    URHO3D_EVENT(EVENT_FALL, Event_OnFall) { }
    URHO3D_EVENT(EVENT_JUMP, Event_OnJump) { }
    URHO3D_EVENT(EVENT_CLIMB, Event_OnClimb) { }

/// SPRITER Animation Event
    URHO3D_EVENT(SPRITER_PARTICULE, SPRITER_Particule) { }
    URHO3D_EVENT(SPRITER_PARTFEET, SPRITER_PartFeet) { }
    URHO3D_EVENT(SPRITER_PART, SPRITER_Part) { }
    URHO3D_EVENT(SPRITER_LIGHT, SPRITER_Light) { }
    URHO3D_EVENT(SPRITER_SOUNDFEET, SPRITER_SoundFeet) { }
    URHO3D_EVENT(SPRITER_SOUNDATTACK, SPRITER_SoundAttack) { }
    URHO3D_EVENT(SPRITER_ENTITY, SPRITER_Entity) { }
    URHO3D_EVENT(SPRITER_ANIMATION, SPRITER_Animation) { }
    URHO3D_EVENT(SPRITER_ANIMATIONINSIDE, SPRITER_AnimationInside) { }
}

using namespace Urho3D;

/// GO Events
struct GOE
{
    static void InitEventTable();
    static StringHash Register(const String& eventName);
    static const String& GetEventName(const StringHash& hashevent);
    static void DumpAll();

private :
    static HashMap<StringHash, String> events_;
};
