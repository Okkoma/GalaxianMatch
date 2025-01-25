#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>

#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameOptions.h"

#include "ObjectPool.h"

#include "GameAttributes.h"


using namespace Urho3D;

/// GO Attributes
HashMap<StringHash, String> GOA::attributes_;

const StringHash GOA::GOT               = StringHash("GOA_GOT");
const StringHash GOA::ENTITYCLONE       = StringHash("GOA_EntityClone");
const StringHash GOA::GRIDCOORD         = StringHash("GOA_GridCoord");
const StringHash GOA::MATCHTYPE         = StringHash("GOA_MatchType");
const StringHash GOA::BONUS             = StringHash("GOA_Bonus");

const StringHash GOA::FACTION           = StringHash("GOA_Faction");
const StringHash GOA::DIRECTION         = StringHash("GOA_Direction");
const StringHash GOA::MOVESTATE         = StringHash("GOA_MoveState");
const StringHash GOA::KEEPDIRECTION     = StringHash("GOA_KeepDirection");
const StringHash GOA::VELOCITY          = StringHash("GOA_Velocity");

const StringHash GOA::LIFE              = StringHash("GOA_Life");
const StringHash GOA::ISDEAD            = StringHash("GOA_IsDead");
const StringHash GOA::ENERGY            = StringHash("GOA_Energy");
const StringHash GOA::ABILITY           = StringHash("GOA_Ability");
const StringHash GOA::LVLID             = StringHash("GOA_LvlId");
const StringHash GOA::LVLMAPNODES       = StringHash("GOA_LvlMapNodes");
const StringHash GOA::MISSIONGAINS      = StringHash("GOA_MissionGains");

const StringHash GOA::DURATION          = StringHash("GOA_Duration");


void GOA::InitAttributeTable()
{
    attributes_.Clear();

    Register("GOA_GOT");
    Register("GOA_EntityClone");
    Register("GOA_GridCoord");
    Register("GOA_MatchType");
    Register("GOA_Bonus");

    Register("GOA_Faction");
    Register("GOA_Direction");
    Register("GOA_MoveState");
    Register("GOA_KeepDirection");
    Register("GOA_Velocity");

    Register("GOA_Life");
    Register("GOA_IsDead");
    Register("GOA_Energy");
    Register("GOA_Ability");
    Register("GOA_LvlMapNodes");
    Register("GOA_MissionGains");

    Register("GOA_Duration");
}

void GOA::RegisterToContext(Context* context)
{
    if (!context) return;

    URHO3D_LOGINFOF("GOA() - RegisterToContext");

    for (HashMap<StringHash, String>::ConstIterator it=attributes_.Begin();it!=attributes_.End();++it)
    {
        context->RegisterUserAttribute(it->second_);
    }
}

StringHash GOA::Register(const String& AttributeName)
{
    StringHash attrib = StringHash(AttributeName);

    if (attributes_.Contains(attrib))
    {
//        URHO3D_LOGINFOF("GOA() : Register AttributeName=%s(%u) (Warning:attributes_ contains already the key)", AttributeName.CString(), attrib.Value());
        return attrib;
    }

    attributes_[attrib] = AttributeName;
    return attrib;
}

String GOA::GetAttributeName(StringHash hashattrib)
{
    HashMap<StringHash, String>::ConstIterator it = attributes_.Find(hashattrib);
    return it != attributes_.End() ? it->second_ : String::EMPTY;
}

void GOA::DumpAll()
{
    URHO3D_LOGINFOF("GOA() - DumpAll : attributes_ Size =", attributes_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, String>::ConstIterator it=attributes_.Begin();it!=attributes_.End();++it)
    {
        URHO3D_LOGINFOF("     => attributes_[%u] = (hash:%u(dec:%d) - name:%s)", index, it->first_.Value(), it->first_.Value(), it->second_.CString());
        ++index;
    }
}


/// GO Types
#include "EnumList.hpp"

#define GOTYPEPROPERTIES \
    GOT_None, GOT_Collectable, GOT_Usable_DropOn, GOT_Usable_DropOut, \
    GOT_Buildable, GOT_Part, GOT_Droppable, GOT_Wearable, GOT_Ability, \
    GOT_Controllable, GOT_Spawnable, GOT_CanCollect, GOT_Effect

#define GOTYPEPROPERTIES_STRINGS \
    ENUMSTRING( GOT_None ), ENUMSTRING( GOT_Collectable ), ENUMSTRING( GOT_Usable_DropOn ), ENUMSTRING( GOT_Usable_DropOut ), \
    ENUMSTRING( GOT_Buildable ), ENUMSTRING( GOT_Part ), ENUMSTRING( GOT_Droppable ), ENUMSTRING( GOT_Wearable), ENUMSTRING( GOT_Ability ), \
    ENUMSTRING( GOT_Controllable ), ENUMSTRING( GOT_Spawnable ), ENUMSTRING( GOT_CanCollect), ENUMSTRING( GOT_Effect)

#define GOTYPEPROPERTIES_VALUES \
    0x0000, 0x0001, 0x0002, 0x0004, \
    0x0008, 0x0010, 0x0020, 0x0040, \
    0x0080, 0x0100, 0x0200, 0x0400, \
    0x0800

LISTVALUE(GOTypeProps, GOTYPEPROPERTIES, GOTYPEPROPERTIES_STRINGS, GOTYPEPROPERTIES_VALUES);
LISTVALUECONST(GOTypeProps, GOTYPEPROPERTIES_STRINGS, GOTYPEPROPERTIES_VALUES, 13);

//unsigned Convert2TypeProperties(const String& dataStr)
//{
//    int value = 0;
//    int propvalue;
//
//    Vector<String> properties = dataStr.Split('|');
//
//    for (unsigned i=0; i<properties.Size(); i++)
//    {
//        propvalue = GOTypeProps::GetValue(properties[i].CString());
//        if (propvalue != -1)
//            value = value | propvalue;
//    }
//
//    return (unsigned)value;
//}


GOTInfo::GOTInfo() :
    replicatedMode_(false)
{ }

const GOTInfo GOTInfo::EMPTY = GOTInfo();

const StringHash GOT::COLLECTABLE_ALL = StringHash("All");
const StringHash GOT::COLLECTABLEPART = StringHash("CollectablePart");

HashMap<StringHash, GOTInfo > GOT::infos_;
HashMap<StringHash, WeakPtr<Node> > GOT::objects_;


/// GOT Setters

void GOT::Clear()
{
    infos_.Clear();
    objects_.Clear();
}

void GOT::InitDefaultTables()
{
    Clear();

    Register("GOT_Default", COT::OBJECTS, String::EMPTY);
    Register("All", COT::OBJECTS, String::EMPTY, GOT_Collectable);

    // Add Here all Core Objects ...

}

StringHash GOT::Register(const String& typeName, const StringHash& category, const String& fileName,
                         unsigned typeProperty, bool replicatedMode, int qtyInPool, int defaultValue, int maxDropQty)
{
    StringHash hashName(typeName);

    if (infos_.Contains(hashName))
    {
//        URHO3D_LOGINFOF("GOT() : Register templateName=%s(%u) (Warning:infos_ contains already the key)", typeName.CString(), hashName.Value());
        return hashName;
    }

    GOTInfo& info = infos_[hashName];

    info.typename_ = typeName;
    info.filename_ = fileName;
    info.category_ = category;
    info.properties_ = typeProperty;
    info.replicatedMode_ = replicatedMode;
    info.poolqty_ = qtyInPool;
    info.maxdropqty_ = maxDropQty;
    info.defaultvalue_ = defaultValue;

    COT::AddTypeInCategory(category, hashName);

    URHO3D_LOGINFOF("GOT() : Register templateName=%s(%u) : cat=%u file=%s typeProps=%u replicatedMode=%s qtyInPool=%d value=%d maxDropQty=%d",
                    typeName.CString(), hashName.Value(), category.Value(), fileName.CString(), typeProperty, replicatedMode ? "true" : "false", qtyInPool, defaultValue, maxDropQty);

    return hashName;
}

void GOT::LoadJSONFile(Context* context, const String& name)
{
    FileSystem* fs = context->GetSubsystem<FileSystem>();
    JSONFile* jsonFile = context->GetSubsystem<ResourceCache>()->GetResource<JSONFile>(name);
    if (!jsonFile)
    {
        URHO3D_LOGWARNINGF("GOT() - LoadJSONFile : %s not exist !", name.CString());
        return;
    }

    const JSONValue& source = jsonFile->GetRoot();

    URHO3D_LOGINFOF("GOT() - LoadJSONFile : %s size=%u", name.CString(), source.Size());

    for (JSONObject::ConstIterator i = source.Begin(); i != source.End(); ++i)
    {
        const String& typeName = i->first_;
        if (typeName.Empty())
        {
            URHO3D_LOGWARNING("GOT() - LoadJSONFile : typeName is empty !");
            continue;
        }

        String category;
        String fileName;
        String typeProperty;
        bool replicateMode=false;
        int qtyInPool=0;
        int defaultValue=0;
        int maxDropQty=0;
        String typePartName;
        String partNames;

        const JSONValue& attributes = i->second_;
        for (JSONObject::ConstIterator j = attributes.Begin(); j != attributes.End(); ++j)
        {
            const String& attribute = j->first_;
            if (attribute == "category")
                category = j->second_.GetString();
            else if (attribute == "fileName")
                fileName = j->second_.GetString();
            else if (attribute == "typeProperty")
                typeProperty = j->second_.GetString();
            else if (attribute == "replicateMode")
                replicateMode = j->second_.GetInt() > 0;
            else if (attribute == "qtyInPool")
                qtyInPool = j->second_.GetInt();
            else if (attribute == "defaultValue")
                defaultValue = j->second_.GetInt();
            else if (attribute == "maxDropQty")
                maxDropQty = j->second_.GetInt();
            else if (attribute == "typePartName")
                typePartName = j->second_.GetString();
            else if (attribute == "partNames")
                partNames = j->second_.GetString();
            else
            {
                URHO3D_LOGWARNING("GOT() - LoadJSONFile : attribute is unknown, typeName=\"" + typeName + "\"");
                continue;
            }
        }

        fileName = "Data/Objects/" + fileName;

        if (fs->FileExists(GameStatics::gameConfig_.appDir_ + fileName))
            Register(typeName, StringHash(category), fileName, GOTypeProps::GetValue(typeProperty), replicateMode, qtyInPool, defaultValue, maxDropQty);
        else
            URHO3D_LOGWARNINGF("GOT() - LoadJSONFile : filename=%s not found !", fileName.CString());
    }
}

bool GOT::PreLoadObjects(int& state, HiresTimer* timer, const long long& delay, Node* preloaderGOT, bool useObjectPool)
{
    static HashMap<StringHash, GOTInfo >::ConstIterator gotinfosIt;
    static HashMap<StringHash, WeakPtr<Node> >::ConstIterator objectsIt;

    // Reset ObjectPool
    if (state == 3)
    {
        objects_.Clear();

        // Reset the object pool
        if (useObjectPool)
            ObjectPool::Reset(preloaderGOT);

        gotinfosIt = infos_.Begin();

#ifdef ACTIVE_PRELOADER_ASYNC
        state++;
#else
        state = 6;
#endif
        if (timer && timer->GetUSec(false) > delay)
            return false;
    }

#ifdef ACTIVE_PRELOADER_ASYNC
    // preloading Resources from template nodes
    if (state == 4)
    {
        if (gotinfosIt != infos_.End())
        {
            for (; gotinfosIt != infos_.End(); ++gotinfosIt)
            {
                const StringHash& got = gotinfosIt->first_;
                const GOTInfo& info = gotinfosIt->second_;

                if (info.filename_.Empty())
                    continue;

                if (objects_.Find(got) != objects_.End())
                    continue;

                URHO3D_LOGINFOF("GOT() - PreLoadObjects : Object %s(%u) preloading resources in %s", info.filename_.CString(), got.Value(), info.filename_.CString());

                if (!GameHelpers::PreloadXMLResourcesFrom(preloaderGOT->GetContext(), info.filename_))
                    continue;

                if (timer && timer->GetUSec(false) > delay)
                    return false;
            }
        }

        gotinfosIt = infos_.Begin();

        state++;
    }

    // Wait for Async Resources Loading
    if (state == 5)
    {
        if (!GameStatics::rootScene_->IsAsyncLoading())
            state++;
    }
#endif

    // Set Template nodes
    if (state == 6)
    {
        if (gotinfosIt != infos_.End())
        {
            for (; gotinfosIt != infos_.End(); ++gotinfosIt)
            {
                const StringHash& got = gotinfosIt->first_;
                const GOTInfo& info = gotinfosIt->second_;

                if (info.filename_.Empty())
                    continue;

                if (objects_.Find(got) != objects_.End())
                    continue;

                // Set Object Template File
                WeakPtr<Node> templateNode = WeakPtr<Node>(preloaderGOT->CreateChild("", LOCAL));

                URHO3D_LOGINFOF("GOT() - PreLoadObjects : Object %s(%u) templateNode=%u hasReplicateMode=%s",
                                info.filename_.CString(), got.Value(), templateNode->GetID(), info.replicatedMode_ ? "true" : "false");

                if (!GameHelpers::LoadNodeXML(preloaderGOT->GetContext(), templateNode, info.filename_.CString(), LOCAL, true))
                    continue;

                objects_[got] = templateNode;

                templateNode->SetEnabledRecursive(false);

                if (timer && timer->GetUSec(false) > delay)
                    return false;
            }
        }

        state++;
    }

    // Set ObjectPool Categories
    if (state == 7)
    {
        if (useObjectPool)
            if (!ObjectPool::Get()->CreateCategories(GameStatics::rootScene_, infos_, objects_, timer, delay))
                return false;

        URHO3D_LOGINFOF("GOT() - PreloadObjects ... OK !");

        return true;
    }


    return false;
}

void GOT::UnLoadObjects(Node* root)
{
    URHO3D_LOGINFOF("GOT() - UnloadObjects ...");

    Context* context = root->GetContext();

    objects_.Clear();

    // Erase Pools
	ObjectPool::Reset();

	root->Remove();

    URHO3D_LOGINFOF("GOT() - UnloadObjects ... OK !");
}


/// GOT Getters

bool GOT::IsRegistered(const StringHash& type)
{
    return infos_.Contains(type);
}

const GOTInfo& GOT::GetConstInfo(const StringHash& type)
{
    if (!infos_.Contains(type)) return GOTInfo::EMPTY;
    return infos_[type];
}

const String& GOT::GetType(const StringHash& type)
{
    if (!infos_.Contains(type)) return String::EMPTY;
    return infos_[type].typename_;
}

bool GOT::HasObjectFile(const StringHash& type)
{
    if (!infos_.Contains(type)) return false;
    return !infos_[type].filename_.Empty();
}

const String& GOT::GetObjectFile(const StringHash& type)
{
    if (!infos_.Contains(type)) return String::EMPTY;
    return infos_[type].filename_;
}

const StringHash& GOT::GetCategory(const StringHash& type)
{
    if (!infos_.Contains(type)) return StringHash::ZERO;
    return infos_[type].category_;
}

unsigned GOT::GetTypeProperties(const StringHash& type)
{
    if (!infos_.Contains(type)) return 0U;
    return infos_[type].properties_;
}

bool GOT::HasReplicatedMode(const StringHash& type)
{
    if (!infos_.Contains(type)) return false;
    return infos_[type].replicatedMode_;
}

int GOT::GetMaxDropQuantityFor(const StringHash& type)
{
    if (!infos_.Contains(type)) return 0;
    return infos_[type].maxdropqty_;
}

int GOT::GetDefaultValue(const StringHash& type)
{
    if (!infos_.Contains(type)) return 0;
    return infos_[type].defaultvalue_;
}

bool GOT::HasObject(const StringHash& type)
{
    HashMap<StringHash, WeakPtr<Node> >::ConstIterator it = objects_.Find(type);
    return it != objects_.End();
}

Node* GOT::GetObject(const StringHash& type)
{
    HashMap<StringHash, WeakPtr<Node> >::ConstIterator it = objects_.Find(type);
    return it != objects_.End() ? it->second_.Get() : 0;
//    return it != objects_.End() ? it->second_ : 0;
}

void GOT::DumpAll()
{
    URHO3D_LOGINFOF("GOT() - DumpAll : infos_ Size =", infos_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, GOTInfo >::ConstIterator it=infos_.Begin();it!=infos_.End();++it, ++index)
    {
        const GOTInfo& info = it->second_;
        URHO3D_LOGINFOF("     => infos_[%u] = (hash:%u - typeName:%s category:%s fileName:%s)", index, it->first_.Value(),
                            info.typename_.CString(), COT::GetName(GetCategory(it->first_)).CString(), info.filename_.CString());
    }
}

/// Category Object Types : COT
HashMap<StringHash, String> COT::categoryNames_;
//HashMap<StringHash, unsigned > COT::categoryAttributes_;
HashMap<StringHash, Vector<StringHash> > COT::typesByCategory_;
HashMap<StringHash, StringHash> COT::categoryBases_;
HashMap<StringHash, Vector<StringHash> > COT::categoriesByBase_;

const StringHash COT::OBJECTS    = StringHash("Objects");
const StringHash COT::ENEMIES    = StringHash("Enemies");
const StringHash COT::BOSSES     = StringHash("Bosses");
const StringHash COT::FRIENDS    = StringHash("Friends");

const StringHash COT::BONUS      = StringHash("Bonus");
const StringHash COT::POWERS     = StringHash("Powers");
const StringHash COT::ITEMS      = StringHash("Items");
const StringHash COT::MOVES      = StringHash("Moves");
const StringHash COT::STARS      = StringHash("Stars");
const StringHash COT::COINS      = StringHash("Coins");
const StringHash COT::SPECIALS   = StringHash("Specials");

const StringHash COT::EXPLOSIONS = StringHash("Explosions");
const StringHash COT::EFFECTS    = StringHash("Effects");
const StringHash COT::TILES      = StringHash("Tiles");
const StringHash COT::GROUND1    = StringHash("Ground1");
const StringHash COT::GROUND2    = StringHash("Ground2");
const StringHash COT::GROUND3    = StringHash("Ground3");
const StringHash COT::WALLS      = StringHash("Walls");
const StringHash COT::ROCKS      = StringHash("Rocks");

const String& COT::GetName(const StringHash& category)
{
    HashMap<StringHash, String>::ConstIterator it = categoryNames_.Find(category);
    return it != categoryNames_.End() ? it->second_ : String::EMPTY;
}

StringHash COT::Register(const String& category, const StringHash& base)
{
    StringHash hashName = StringHash(category);

    if (categoryNames_.Contains(hashName))
        return hashName;

    categoryNames_[hashName] = category;

    typesByCategory_[hashName] = Vector<StringHash>();
    categoryBases_[hashName] = base;

    if (base == StringHash::ZERO)
        categoriesByBase_[hashName] = Vector<StringHash>();

    else if (!categoriesByBase_[base].Contains(hashName))
        categoriesByBase_[base].Push(hashName);


    return hashName;
}

void COT::AddTypeInCategory(const StringHash& category, const StringHash& type)
{
    if (!typesByCategory_[category].Contains(type))
        typesByCategory_[category].Push(type);
}

int COT::IsInCategory(const StringHash& type, const StringHash& category)
{
    const Vector<StringHash>& typesincat = typesByCategory_[category];
    for (int i=0; i < typesincat.Size(); i++)
        if (typesincat[i] == type)
            return i;

    return -1;
}

#define NUMTRIES 4

const StringHash& COT::GetCategoryIn(const StringHash& base, int index)
{
    const Vector<StringHash>& categories = categoriesByBase_[base];
    if (categories.Size() && index && index <= categories.Size())
    {
        return categories[index-1];
    }

    return base;
}

int COT::GetCategoryIndexIn(const StringHash& base, const StringHash& category)
{
    if (base == category)
        return 0;

    const Vector<StringHash>& categories = categoriesByBase_[base];
    Vector<StringHash>::ConstIterator it = categories.Find(category);

    return it != categories.End() ? categories.Find(category) - categories.Begin() + 1 : 0;
}

StringHash COT::GetRandomCategoryFrom(const StringHash& base, int rand)
{
//    URHO3D_LOGINFOF("COT::GetRandomCategoryFrom CategoryBase=%s rand=%d", COT::GetName(base).CString(), rand);

    StringHash category = base;
    int tries = 0;

    if (categoriesByBase_[base].Size())
    {
        while (category == base && tries++ < NUMTRIES)
        {
            int newrand = (rand ? (rand+tries) % (categoriesByBase_[base].Size() + 1) : Random(0, categoriesByBase_[base].Size()+1));
            if (newrand == 0)
            {
                category = base;
                break;
            }

            category = categoriesByBase_[base][(unsigned)(newrand-1)];

            if (tries < NUMTRIES)
            {
                if (categoriesByBase_[category].Size())
                    category = GetRandomCategoryFrom(category, rand ? newrand : 0);
                if (!typesByCategory_[category].Size())
                    category = base;
            }
            else if (!typesByCategory_[category].Size())
                category = base;
        }
    }

    if (typesByCategory_[category].Size())
        return category;
    else
        return base;
}

const StringHash& COT::GetRandomTypeFrom(const StringHash& base, int rand)
{
//    URHO3D_LOGINFOF("COT::GetRandomTypeFrom Category=%s rand=%d", COT::GetName(base).CString(), rand);
    if (!categoryNames_.Contains(base))
        return base;

    StringHash category = GetRandomCategoryFrom(base, rand);

    unsigned size = typesByCategory_[category].Size();

    if (size)
    {
        const StringHash& type = typesByCategory_[category][rand ? rand % size : Random(0, size)];
//        URHO3D_LOGINFOF("COT::GetRandomTypeFrom Category=%s Type=%s", COT::GetName(category).CString(), GOT::GetType(type).CString());

        return type;
    }
    else
        URHO3D_LOGERRORF("COT::GetRandomTypeFrom Category=%s : no type in Category !", COT::GetName(category).CString());

    return StringHash::ZERO;
}

const StringHash& COT::GetTypeFromCategory(const StringHash& base, int index)
{
    if (index < 0)
        return StringHash::ZERO;

    const Vector<StringHash>& categorytypes = typesByCategory_[base];

    if (index < categorytypes.Size())
    {
        return categorytypes[index];
    }
    else
    {
        index -= categorytypes.Size();

        const Vector<StringHash>& categoriesinbase = categoriesByBase_[base];
        for (unsigned i=0; i < categoriesinbase.Size(); i++)
        {
            const StringHash& got = GetTypeFromCategory(categoriesinbase[i], index);
            if (got.Value() != 0)
                return got;

            index -= typesByCategory_[categoriesinbase[i]].Size();

            if (index < 0)
                return StringHash::ZERO;
        }
    }

    return StringHash::ZERO;
}

Node* COT::GetObjectFromCategory(const StringHash& base, int index)
{
    if (index < 0)
        return 0;

    const Vector<StringHash>& categorytypes = typesByCategory_[base];

    if (index < categorytypes.Size())
    {
        const StringHash& type = categorytypes[index];
//        URHO3D_LOGINFOF("COT::GetObjectFromCategory Category=%s Type=%s(%u)", COT::GetName(base).CString(), GOT::GetType(type).CString(), index);

        return GOT::GetObject(type);
    }
    else
    {
        /*
            ex GetObjectFromCategory(cat0, 6)
            cat0 (size=2) index = 6
            -> cat1 (3)   index = 6 - 2 = 4
            -> cat2 (2)   index = 4 - 3
            => getobject in cat2 at index=1
        */

        index -= categorytypes.Size();

        const Vector<StringHash>& categoriesinbase = categoriesByBase_[base];
        for (unsigned i=0; i < categoriesinbase.Size(); i++)
        {
            Node* object = GetObjectFromCategory(categoriesinbase[i], index);
            if (object)
                return object;

            index -= typesByCategory_[categoriesinbase[i]].Size();

            if (index < 0)
                return 0;
        }

//        unsigned icount = typesByCategory_[base].Size();
//        for (unsigned i=0; i < categoriesinbase.Size(); i++)
//        {
//            const Vector<StringHash>& categorytypes = typesByCategory_[categoriesinbase[i]];
//            if (index < icount + categorytypes.Size())
//            {
//                return GOT::GetObject(categorytypes[index - icount]);
//            }
//            icount += categorytypes.Size();
//        }
    }

    return 0;
}

void COT::InitCategoryTables()
{
    categoryNames_.Clear();
    //categoryAttributes_.Clear();
    typesByCategory_.Clear();

    Register("Objects");
    Register("Tiles");

    Register("Enemies", COT::OBJECTS);
    Register("Bosses", COT::OBJECTS);
    Register("Friends", COT::OBJECTS);

    Register("Bonus", COT::OBJECTS);
    Register("Powers", COT::BONUS);

    Register("Items", COT::BONUS);
    Register("Moves", COT::ITEMS);
    Register("Stars", COT::ITEMS);
    Register("Coins", COT::ITEMS);
    Register("Specials", COT::ITEMS);

    Register("Explosions", COT::OBJECTS);
    Register("Effects", COT::OBJECTS);

    Register("Rocks", COT::OBJECTS);
    Register("Ground1", COT::TILES);
    Register("Ground2", COT::TILES);
    Register("Ground3", COT::TILES);
    Register("Walls", COT::TILES);

    categoryNames_.Sort();
    //categoryAttributes_.Sort();
    typesByCategory_.Sort();
    categoryBases_.Sort();
    categoriesByBase_.Sort();
}

void COT::DumpAll()
{
    URHO3D_LOGINFOF("COT() - DumpAll : Size =", categoryNames_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, String >::ConstIterator it=categoryNames_.Begin();it!=categoryNames_.End();++it,++index)
    {
        URHO3D_LOGINFOF("     => categoryNames_[%u] = name:%s (hash:%u(dec:%d))", index, it->second_.CString(), it->first_.Value(), it->first_.Value());
        const Vector<StringHash>& gots = GetObjectsInCategory(it->first_);
        if (gots.Size())
        {
            for (unsigned i=0; i< gots.Size(); i++)
            {
                URHO3D_LOGINFOF("       => gameObject_[%u] = name:%s (hash:%u - file:%s)", i, GOT::GetType(gots[i]).CString(), gots[i].Value(), GOT::GetObjectFile(gots[i]).CString());
            }
        }
    }
}


/// GO States

HashMap<StringHash, String> GOS::states_;

void GOS::InitStateTable()
{
    states_.Clear();
    Register("State_Appear");
    Register("State_Default");
    Register("State_Walk");
    Register("State_Run");
    Register("State_Attack");
    Register("State_Jump");
    Register("State_Fall");
    Register("State_Hurt");
    Register("State_Disappear");
    Register("State_Destroy");
}

StringHash GOS::Register(const String& stateName)
{
    StringHash state = StringHash(stateName);

    if (states_.Contains(state))
    {
//        URHO3D_LOGINFOF("GOS() : Register StateName=%s(%u) (Warning:states_ contains already the key)", stateName.CString(), state.Value());
        return state;
    }

    states_[state] = stateName;
    return state;
}

const String& GOS::GetStateName(StringHash hashstate)
{
    HashMap<StringHash, String>::ConstIterator it = states_.Find(hashstate);
    return it != states_.End() ? it->second_ : String::EMPTY;
}

void GOS::DumpAll()
{
    URHO3D_LOGINFOF("GOS() - DumpAll : states_ Size =", states_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, String>::ConstIterator it=states_.Begin();it!=states_.End();++it)
    {
        URHO3D_LOGINFOF("     => states_[%u] = (hash:%u(dec=%d) - name:%s)", index, it->first_.Value(), it->first_.Value(), it->second_.CString());
        ++index;
    }
}
