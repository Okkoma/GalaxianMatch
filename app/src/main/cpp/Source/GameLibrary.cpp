#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "GameOptions.h"

#include "GameRand.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameStatics.h"
#include "GameUI.h"

#include "SplinePath2D.h"
#include "SceneAnimation2D.h"

#include "AnimatedSprite.h"
#include "BossLogic.h"
#include "TicTacToeLogic.h"
#include "BlastLogic.h"

#include "ObjectPool.h"

#include "LevelGraph.h"

#include "MAN_Matches.h"
#include "Tutorial.h"

#include "Network.h"

#include "GameTest.h"

#include "GameLibrary.h"



void RegisterGameLibrary(Context* context)
{
    URHO3D_LOGINFO("RegisterGameLibrary : ... ");

    Network::RegisterLibrary(context);

    GameRand::InitTable();

    GOA::InitAttributeTable();
    GOA::RegisterToContext(context);
    GOS::InitStateTable();
    COT::InitCategoryTables();
    GOT::InitDefaultTables();
    GOE::InitEventTable();

    SplinePath2D::RegisterObject(context);
    SceneAnimation2D::RegisterObject(context);

    AnimatedSprite::RegisterObject(context);
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

	ObjectPool::Reset();
	MatchesManager::Reset();
    Tutorial::Reset();

    InputPlayer::Release();
    InputRecorder::Release();

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

