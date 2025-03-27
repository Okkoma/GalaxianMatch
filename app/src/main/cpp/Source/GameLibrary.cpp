#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "GameOptions.h"

#include "Graphics2D.h"

#include "GameRand.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameStatics.h"
#include "GameUI.h"

#include "SplinePath2D.h"

#include "BossLogic.h"
#include "TicTacToeLogic.h"
#include "BlastLogic.h"

#include "LevelGraph.h"

#include "MAN_Matches.h"
#include "Tutorial.h"

#if defined(TEST_NETWORK)
#include "Network.h"
#endif

#include "GameTest.h"

#include "GameLibrary.h"



void RegisterGameLibrary(Context* context)
{
    URHO3D_LOGINFO("RegisterGameLibrary : ... ");

#if defined(ACTIVE_CUSTOM_URHO)
    RegisterGraphics2DLibrary(context);
#endif
#if defined(TEST_NETWORK)
    Network::RegisterLibrary(context);
#endif

    GameRand::InitTable();

    GOA::InitAttributeTable();
    GOA::RegisterToContext(context);
    GOS::InitStateTable();
    COT::InitCategoryTables();
    GOT::InitDefaultTables();
    GOE::InitEventTable();

    SplinePath2D::RegisterObject(context);

    BossLogic::RegisterObject(context);
    TicTacToeLogic::RegisterObject(context);
    BlastLogic::RegisterObject(context);

    UIMenu::RegisterObject(context);
    LevelGraph::RegisterObject(context);

    MatchesManager::RegisterObject(context);
    Tutorial::RegisterObject(context);

#ifdef DUMP_ATTRIBUTES
    GOA::DumpAll();
    GOE::DumpAll();
    GOS::DumpAll();
#endif

    URHO3D_LOGINFO("RegisterGameLibrary : ... OK !");
}

void UnRegisterGameLibrary(Context* context)
{
	URHO3D_LOGINFO("UnRegisterGameLibrary : ... ");

	MatchesManager::Reset();
    Tutorial::Reset();

#ifdef ACTIVE_GAMELOOPTESTING
    InputPlayer::Release();
    InputRecorder::Release();
#endif
	URHO3D_LOGINFO("UnRegisterGameLibrary : ... OK !");
}

void DumpGameLibrary()
{
    URHO3D_LOGINFO("DumpGameLibrary : ...");

#ifdef DUMP_ATTRIBUTES
    GOA::DumpAll();
    GOE::DumpAll();
    GOS::DumpAll();
    GOT::DumpAll();
    COT::DumpAll();
#endif // DUMP_ATTRIBUTES

#ifdef DUMP_COMPONENTTEMPLATES
//    GOC_SoundEmitter_Template::DumpAll();
//    GOC_Inventory_Template::DumpAll();
//    EffectType::DumpAll();
#endif

    URHO3D_LOGINFO("DumpGameLibrary : ... OK !");
}

