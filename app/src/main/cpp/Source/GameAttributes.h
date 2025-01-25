#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

#include "DefsGame.h"

using namespace Urho3D;


/// GO Attributes
struct GOA
{
    // GameObjectType handle by GOT and GO_Pools
    static const StringHash GOT;
    static const StringHash ENTITYCLONE;

    static const StringHash GRIDCOORD;
    static const StringHash MATCHTYPE;
    static const StringHash BONUS;

    static const StringHash FACTION;
    static const StringHash DIRECTION;
    static const StringHash MOVESTATE;
    static const StringHash KEEPDIRECTION;
    static const StringHash VELOCITY;

    static const StringHash LIFE;
    static const StringHash ISDEAD;
    static const StringHash ENERGY;
    static const StringHash ABILITY;
    static const StringHash LVLID;
    static const StringHash LVLMAPNODES;
    static const StringHash MISSIONGAINS;

    static const StringHash DURATION;

    static void InitAttributeTable();
    static void RegisterToContext(Context* context);
    static StringHash Register(const String& AttributeName);
    static String GetAttributeName(StringHash hashattrib);
    static void DumpAll();

private :
    static HashMap<StringHash, String> attributes_;
};

/// GO Base Types
enum GOControllerType
{
    GO_None = 0,
    GO_Player = 0x01,       // 000001
    GO_NetPlayer = 0x03,  // 000011
    GO_AI = 0x04,             // 000100
    GO_AI_None = 0x08,    // 001000
    GO_AI_Ally = 0x10,      // 010000
    GO_AI_Enemy = 0x20,  // 100000
};

enum GOTypeProperties
{
    GOT_None            = 0x0000,
    GOT_Collectable     = 0x0001,
    GOT_Usable_DropOn   = 0x0002,
    GOT_Usable_DropOut  = 0x0004,
    GOT_Buildable       = 0x0008,
    GOT_Part        	= 0x0010,
    GOT_Droppable    	= 0x0020,
    GOT_Wearable		= 0x0040,     // equipement
    GOT_Ability         = 0x0080,     // ability getter
    GOT_Controllable    = 0x0100,     // ControllerAI_Ally possible ?
    GOT_Spawnable       = 0x0200,     // via DropZone, Random spawn etc ...
    GOT_CanCollect      = 0x0400,     // can collect collectables
    GOT_Effect          = 0x0800      // local animation
    // wearable + usable = slot arme ?
};

/// Table for GOT (Game Object Types)
struct GOT
{
    static StringHash Register(const String& type, const StringHash& category, const String& objectFile,
                               unsigned typeProperty = 0, bool replicateMode=false, int qtyInPool = 0, int defaultValue = 0, int maxDropQty = 0);
    static void RegisterBuildablePartsForType(const String& typeName, const String& typePartName, const String& partNames);
    static void Clear();
    static void InitDefaultTables();
    static void LoadJSONFile(Context* context, const String& name);
//    static void PreLoadObjects(Context* context, Node* node, bool useObjectPool=false);
    static bool PreLoadObjects(int& state, HiresTimer* timer, const long long& delay, Node* preloaderGOT, bool useObjectPool=false);
    static void UnLoadObjects(Node* node);

    static bool IsRegistered(const StringHash& type);
    static const GOTInfo& GetConstInfo(const StringHash& type);
    static bool HasObject(const StringHash& type);
    static Node* GetObject(const StringHash& type);
    static bool HasObjectFile(const StringHash& type);
    static const String& GetObjectFile(const StringHash& type);
    static const String& GetType(const StringHash& type);
    static unsigned GetTypeProperties(const StringHash& type);
    static bool HasReplicatedMode(const StringHash& type);
    static const StringHash& GetCategory(const StringHash& type);
    static int GetMaxDropQuantityFor(const StringHash& type);
    static int GetDefaultValue(const StringHash& type);

    static void DumpAll();

    static const StringHash COLLECTABLE_ALL;
    static const StringHash COLLECTABLEPART;

private :
    static HashMap<StringHash, GOTInfo > infos_;
    static HashMap<StringHash, WeakPtr<Node> > objects_;
};

/// Tables for COT (Category Object Type)
// a COT is a set of GOT
// a GOT have a unique Category register in GOT::typeCategories_
// example : a Leather Cuirass (GOT) is an Armor (COT)
struct COT
{
public :
    static const StringHash OBJECTS;
    static const StringHash ENEMIES;
    static const StringHash BOSSES;
    static const StringHash FRIENDS;

    static const StringHash BONUS;
    static const StringHash POWERS;
    static const StringHash ITEMS;
    static const StringHash MOVES;
    static const StringHash STARS;
    static const StringHash COINS;
    static const StringHash SPECIALS;

    static const StringHash EXPLOSIONS;
    static const StringHash EFFECTS;
    static const StringHash TILES;
    static const StringHash GROUND1;
    static const StringHash GROUND2;
    static const StringHash GROUND3;
    static const StringHash ROCKS;
    static const StringHash WALLS;

    static const String& GetName(const StringHash& category);

    static StringHash Register(const String& category, const StringHash& base=StringHash::ZERO);
    static void AddTypeInCategory(const StringHash& category, const StringHash& type);
    static int IsInCategory(const StringHash& type, const StringHash& category);

    static void InitCategoryTables();

    static const Vector<StringHash>& GetObjectsInCategory(const StringHash& category) { return typesByCategory_[category]; }
    static const StringHash& GetCategoryIn(const StringHash& base, int index);
    static int GetCategoryIndexIn(const StringHash& base, const StringHash& category);
    static StringHash GetRandomCategoryFrom(const StringHash& base, int rand=0);
    static const StringHash& GetRandomTypeFrom(const StringHash& base, int rand=0);
    static const StringHash& GetTypeFromCategory(const StringHash& base, int index);
    static Node* GetObjectFromCategory(const StringHash& base, int index=0);

    static void DumpAll();

private :
    static HashMap<StringHash, String> categoryNames_;
    static HashMap<StringHash, Vector<StringHash> > typesByCategory_;
    static HashMap<StringHash, StringHash> categoryBases_;
    static HashMap<StringHash, Vector<StringHash> > categoriesByBase_;
};

/// GO States
struct GOS
{
    static void InitStateTable();
    static void RegisterToContext(Context* context);
    static StringHash Register(const String& stateName);
    static const String& GetStateName(StringHash hashstate);
    static void DumpAll();

private :
    static HashMap<StringHash, String> states_;
};

/// Animation States
//const unsigned STATEFINDER            = Urho3D::StringHash("STATEFINDER").Value();
const unsigned STATE_APPEAR           = Urho3D::StringHash("State_Appear").Value();
const unsigned STATE_DEFAULT          = Urho3D::StringHash("State_Default").Value();
const unsigned STATE_DEFAULT_GROUND   = Urho3D::StringHash("State_Default_Ground").Value();
const unsigned STATE_DEFAULT_AIR      = Urho3D::StringHash("State_Default_Air").Value();
const unsigned STATE_WALK             = Urho3D::StringHash("State_Walk").Value();
const unsigned STATE_RUN              = Urho3D::StringHash("State_Run").Value();
const unsigned STATE_ATTACK           = Urho3D::StringHash("State_Attack").Value();
const unsigned STATE_JUMP             = Urho3D::StringHash("State_Jump").Value();
const unsigned STATE_FALL             = Urho3D::StringHash("State_Fall").Value();
const unsigned STATE_HURT             = Urho3D::StringHash("State_Hurt").Value();
const unsigned STATE_DEAD             = Urho3D::StringHash("State_Dead").Value();
const unsigned STATE_DISAPPEAR        = Urho3D::StringHash("State_Disappear").Value();
const unsigned STATE_DESTROY          = Urho3D::StringHash("State_Destroy").Value();

const Urho3D::StringHash STATE_IDLE   = Urho3D::StringHash("State_Default");

/// Behaviors
const unsigned GOB_PATROL             = Urho3D::StringHash("GOB_Patrol").Value();
const unsigned GOB_FOLLOW             = Urho3D::StringHash("GOB_Follow").Value();
const unsigned GOB_FOLLOWATTACK       = Urho3D::StringHash("GOB_FollowAttack").Value();
const unsigned GOB_MOUNTON            = Urho3D::StringHash("GOB_MountOn").Value();

const unsigned BEHAVIOR_DEFAULT       = GOB_PATROL;


// Input Key
const int CTRL_UP    = 1;
const int CTRL_DOWN  = 2;
const int CTRL_LEFT  = 4;
const int CTRL_RIGHT = 8;
const int CTRL_JUMP  = 16;
const int CTRL_FIRE  = 32;
const int CTRL_FIRE2 = 64;
const int CTRL_FIRE3 = 128;

const int CTRL_ALL   = 255;


enum CSColliderType
{
    CS_None = -1,
    CS_UnStable = 0,
    CS_Stable = 1,
};


#include "DefsMove.h"

