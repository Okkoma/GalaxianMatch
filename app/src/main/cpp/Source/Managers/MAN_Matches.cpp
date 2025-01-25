#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/Localization.h>

#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Graphics.h>

#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/MessageBox.h>

#include "GameOptions.h"
#include "GameAttributes.h"
#include "GameHelpers.h"
#include "GameEvents.h"
#include "TimerRemover.h"
#include "sPlay.h"
#include "Network.h"

#ifdef ACTIVE_GAMELOOPTESTING
#include "GameTest.h"
#endif

#include "InteractiveFrame.h"

#include "MAN_Matches.h"

const Color gridcolors_[2] =
{
    Color(1.f, 0.8f, 0.6f, 1.f),
    Color(0.6f, 0.8f, 1.f, 1.f)
//    Color(0.5f, 0.25f, 0.f, 1.f),
//    Color(0.f, 0.25f, 0.5f, 1.f)
};

const char* matchStateNames[] =
{
    "NoMatchState",
    "StartSelection",
    "SelectionAnimation",
    "CancelMove",
    "SuccessMatch",
};

#ifdef ACTIVE_TESTMODE
bool MatchesManager::testMode_ = false;
#endif
bool MatchesManager::allowSelectionAlongHV_ = false;
bool MatchesManager::allowHints_ = true;
bool MatchesManager::checkallpowersonturn_ = true;
float MatchesManager::sceneScale_ = 1.f;

SharedPtr<MatchesManager> MatchesManager::manager_;

int InputMoveThreshold = 3;

SharedPtr<MessageBox> matchmessageBox;
WeakPtr<InteractiveFrame> shakebuttonFrame;


MatchesManager::MatchesManager(Context* context) :
    Object(context)
{
    manager_ = SharedPtr<MatchesManager>(this);

    gridinfos_.Clear();
    gridinfos_.Push(SharedPtr<MatchGridInfo>(new MatchGridInfo()));

    gridinfos_.Back()->netusage_ = NETLOCAL;
    gridinfos_.Back()->mgrid_.SetId(0);
    gridinfos_.Back()->mgrid_.SetGridColor(gridcolors_[0]);
}

MatchesManager::~MatchesManager()
{
    gridinfos_.Clear();
}


void MatchesManager::RegisterObject(Context* context)
{
    context->RegisterFactory<MatchesManager>();

    Reset(context);
}

#ifdef ACTIVE_TESTMODE
void MatchesManager::SwitchTestMode()
{
    testMode_ = !testMode_;

    URHO3D_LOGINFOF("MatchesManager() - SwitchTestMode : testmode=%s", testMode_ ? "enable":"disable");

    if (manager_ && GameStatics::rootScene_)
    {
        for (int i = 0; i < manager_->gridinfos_.Size(); i++)
            GetGridInfo(i)->testModeNextMove_ = false;

        if (manager_->HasSubscribedToEvent(GameStatics::rootScene_, E_SCENEPOSTUPDATE))
        {
            manager_->UnsubscribeFromEvents();
            manager_->SubscribeToEvents();
        }
    }
}
#endif

void MatchesManager::SetAuthorizedObjects(const StringHash& category, const Vector<StringHash>& types)
{
    MatchGrid::SetAuthorizedTypes(category, types);
}

void MatchesManager::SetNetPlayMod(int netplaymod)
{
    manager_->netplaymod_ = netplaymod;
}

void MatchesManager::AddGrid(int netusage)
{
    if (!manager_)
        return;

    manager_->gridinfos_.Push(SharedPtr<MatchGridInfo>(new MatchGridInfo()));

    const int id = manager_->gridinfos_.Size()-1;
    GetGridInfo(id)->netusage_ = netusage;
    GetGridInfo(id)->Init();
    GetGridInfo(id)->ResetObjectives();
    GetGrid(id).SetId(id);
    GetGrid(id).SetGridColor(gridcolors_[id]);
    GetGrid(id).ClearAll();

    URHO3D_LOGINFOF("MatchesManager() - AddGrid gridid=%d netusage=%d", id, netusage);
}

void MatchesManager::RemoveGrid(int netusage)
{
    if (!manager_)
        return;

    Vector<SharedPtr<MatchGridInfo> >::Iterator it = manager_->gridinfos_.Begin();
    while (it != manager_->gridinfos_.End())
    {
        MatchGridInfo* info = it->Get();
        if (!info || info->netusage_ == netusage)
        {
            if (info)
            {
                URHO3D_LOGINFOF("MatchesManager() - RemoveGrid gridid=%d netusage=%d", it - manager_->gridinfos_.Begin(), netusage);
                info->mgrid_.ClearAll();
            }
            it = manager_->gridinfos_.Erase(it);
        }
        else
        {
            it++;
        }
    }
}

void MatchesManager::SetLayout(int size, GridLayout layout)
{
    if (!manager_)
        return;

    /// for the moment, No Boss level are made over the boss04, so randomize
    if (layout >= L_Custom)
    {
        layout = (GridLayout)GameRand::GetRand(MAPRAND, 0, L_MAXSTANDARDLAYOUT-1);
        size = GameRand::GetRand(MAPRAND, 9, DEFAULT_MAXDIMENSION);
    }

    Match::BONUSMINIMALMATCHES = GameStatics::IsBossLevel() ? BOSS_BONUSMINIMALMATCHES : DEFAULT_BONUSMINIMALMATCHES;

    manager_->gridsize_ = size;
    manager_->gridlayout_ = layout;

    URHO3D_LOGWARNINGF("MatchesManager() - SetLayout : size=%d layout=%d !", size, layout);
}

void MatchesManager::SetPhysicsEnable(bool enable)
{
    if (!manager_)
        return;

    URHO3D_LOGWARNINGF("MatchesManager() - SetPhysicsEnable !");

    for (unsigned i = 0; i < manager_->gridinfos_.Size(); i++)
        GetGrid(i).SetPhysicsEnable(enable);

    // allow when levelwin to allow move matches
    // GameStatics::allowInputs_ = false;
}

void MatchesManager::RegisterObjective(StringHash got, int numitems)
{
    if (!manager_)
        return;

    Node* object = GOT::GetObject(got);
    int colortype = object ? object->GetVar(GOA::MATCHTYPE).GetInt() : NOTYPE;

    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
        GetGridInfo(i)->AddObjective(got, colortype, numitems);
}

void MatchesManager::SetObjectiveRemainingQty(unsigned char index, int remain, int gridid)
{
    if (!manager_)
        return;

    Vector<MatchObjective >& objectives = GetGridInfo(gridid)->objectives_;
    objectives[index].count_ = Min(objectives[index].target_, objectives[index].target_ - remain);
}

void MatchesManager::UpdateGridPositions()
{
    const float tilesize = 2.f * halfTileSize;
    float totalgridsizex = 0.f;
    float totalgridsizey = 0.f;

    Vector<Vector3> gridposition;
    gridposition.Resize(manager_->gridinfos_.Size());

    // calculate the total dimension for all the grids
    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        MatchGrid& grid = GetGrid(i);

        // Grids are aligned horizontally
        totalgridsizex += tilesize * (float)grid.width_;

        if (totalgridsizey < tilesize * (float)grid.height_)
            totalgridsizey = tilesize * (float)grid.height_;
    }

    // Add a tile space between each grid
    totalgridsizex += (manager_->gridinfos_.Size() + 1) * tilesize;
    totalgridsizey += 2.f * tilesize;

    float position = -0.5f * totalgridsizex + tilesize;
    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        // the node position of a grid is on the center of the grid
        gridposition[i].x_ = position + halfTileSize * (float)GetGrid(i).width_;
        gridposition[i].y_ = -halfTileSize;// - halfTileSize * (float)GetGrid(i).height_;

        position = gridposition[i].x_ + halfTileSize * (float)GetGrid(i).width_ + tilesize;
    }

    float zoom = Min((float)GameStatics::graphics_->GetWidth() * PIXEL_SIZE / totalgridsizex,
                     (float)GameStatics::graphics_->GetHeight() * PIXEL_SIZE / totalgridsizey);

    sceneScale_ = zoom / GameStatics::uiScale_;

    Rect totalrect;
    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        MatchGrid& grid = GetGrid(i);

        Rect rect;
        rect.min_ = Vector2(gridposition[i].x_ - float(grid.width_) * halfTileSize, gridposition[i].y_ - float(grid.height_) * halfTileSize);
        rect.max_ = Vector2(gridposition[i].x_ + float(grid.width_) * halfTileSize, gridposition[i].y_ + float(grid.height_) * halfTileSize);

//        if (sceneScale_ < 1.f)
        {
            rect.min_ *= sceneScale_;
            rect.max_ *= sceneScale_;

            Vector2 scale(sceneScale_, sceneScale_);

            if (grid.gridNode_)
                grid.gridNode_->SetScale2D(scale);

            if (grid.objectsNode_)
                grid.objectsNode_->SetScale2D(scale);
        }

        totalrect.Merge(rect);
        grid.SetGridRect(rect);

        grid.gridNode_->SetPosition(rect.Center());
        grid.objectsNode_->SetPosition(grid.gridNode_->GetPosition());
    }

    GameStatics::switchScaleXY_ = totalgridsizex >= totalgridsizey && sceneScale_ < 1.1f && GameStatics::uiScale_ > 1.f;

    GameHelpers::OrthoWorldToScreen(GameStatics::screenSceneRect_, BoundingBox(totalrect));

    URHO3D_LOGINFOF("MatchesManager() - SetSceneScale : gridsize=%F,%F uiscale=%F scenescale=%F switchscaleXY=%s",
                    totalgridsizex, totalgridsizey, GameStatics::uiScale_, sceneScale_, GameStatics::switchScaleXY_ ? "true:gdx>=gdy" : "false:gdx<gdy");
}

void MatchesManager::SetBackGround()
{
    URHO3D_LOGINFO("MatchesManager() - SetBackGround : ... ");

    // add starsky
    {
        Node* node = GameHelpers::AddStaticSprite2D(GameStatics::rootScene_->GetChild("Scene"), LOCAL, "background", "Textures/Background/starsky2.webp", Vector3::ZERO, Vector3::ONE, BACKGROUND);

//        StaticSprite2D* staticSprite = node->GetComponent<StaticSprite2D>();
//        SharedPtr<Material> backgroundMaterial = GameStatics::renderer2d_->CreateMaterial(staticSprite->GetSprite()->GetTexture(), BLEND_REPLACE);
//        node->GetComponent<StaticSprite2D>()->SetCustomMaterial(backgroundMaterial);

        ResizeBackGround();
    }

    URHO3D_LOGINFO("MatchesManager() - SetBackGround : ... OK !");
}

void MatchesManager::ResizeBackGround()
{
    Node* node = GameStatics::rootScene_->GetChild("Scene")->GetChild("background");
    if (!node)
        return;

    node->RemoveObjectAnimation();

    GameHelpers::SetAdjustedToScreen(node, 1.25f, 1.25f, false);

#ifndef RPI
    // add rotation effects (on RPI, bug in texture => disabled)
    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
    SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(node->GetContext()));
    scaleAnimation->SetKeyFrame(0.f, node->GetScale());
    scaleAnimation->SetKeyFrame(500.f, node->GetScale()*1.3f);
    scaleAnimation->SetKeyFrame(1000.f, node->GetScale());
    objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_LOOP);
    SharedPtr<ValueAnimation> rotateAnimation(new ValueAnimation(node->GetContext()));
    rotateAnimation->SetKeyFrame(0.f, Quaternion::IDENTITY);
    rotateAnimation->SetKeyFrame(250.f, Quaternion(90));
    rotateAnimation->SetKeyFrame(500.f, Quaternion(180));
    rotateAnimation->SetKeyFrame(750.f, Quaternion(270));
    rotateAnimation->SetKeyFrame(1000.f, Quaternion(360));
    objectAnimation->AddAttributeAnimation("Rotation", rotateAnimation, WM_LOOP);
    node->SetObjectAnimation(objectAnimation);
#endif
}

void MatchesManager::Init()
{
    checkallpowersonturn_ = GameStatics::GetLevelInfo().mode_ == CLASSICLEVELMODE;
    allowHints_ = GameStatics::GetLevelInfo().hints_;
    Match::MINIMALMATCHES = GameStatics::GetLevelInfo().minmatches_;

    GameStatics::allowInputs_ = true;
}

void MatchesManager::Reset(Context* context)
{
    manager_.Reset();

    if (context)
        manager_ = new MatchesManager(context);
}

void MatchesManager::Start()
{
    if (!manager_)
        return;

    URHO3D_LOGWARNINGF("MatchesManager() - Start !");

    for (unsigned i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        if (!GetGridInfo(i)->IsGridCreated())
        {
            GetGridInfo(i)->Create();
            GetGridInfo(i)->StartFirstMoves();
        }
    }

    SetBackGround();
    UpdateGridPositions();

    manager_->SubscribeToEvent(GameStatics::rootScene_, E_SCENEPOSTUPDATE, new Urho3D::EventHandlerImpl<MatchesManager>(manager_, &MatchesManager::HandleUpdateInitial));
}

void MatchesManager::Stop()
{
    if (shakebuttonFrame)
        shakebuttonFrame->Stop();

    if (!manager_)
        return;

    URHO3D_LOGWARNINGF("MatchesManager() - Stop !");

    for (unsigned i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        GetGridInfo(i)->ResetAbility();
        GetGridInfo(i)->CheckHints(false);
    }

    allowHints_ = false;

    // allow when levelwin to allow move matches
//    manager_->UnsubscribeFromEvents();
}

void MatchesManager::Clear()
{
    if (shakebuttonFrame)
        shakebuttonFrame->Clean();

    if (!manager_)
        return;

    URHO3D_LOGWARNING("MatchesManager() - Clear !");

    manager_->UnsubscribeFromEvents();

    manager_->Init();

    for (unsigned i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        GetGridInfo(i)->Init();
        GetGridInfo(i)->ResetObjectives();
        GetGrid(i).ClearAll();
    }

    sceneScale_ = 1.f;
}

// Redistribute New Objects (no matches and no tiles)
void MatchesManager::ResetObjects(int gridid)
{
    if (!manager_)
        return;

    URHO3D_LOGWARNINGF("MatchesManager() - ResetObjects !");

    manager_->UnsubscribeFromEvents();

    GetGridInfo(gridid)->Init();
    GetGridInfo(gridid)->SpawnObjects();

    UpdateGridPositions();

    manager_->SubscribeToEvents();
}

void MatchesManager::CheckAllowShake()
{
    if (!manager_)
        return;

    bool shakeAllowed = false;

    if (GameStatics::GetLevelInfo().nomorematchaction_ == SHAKE)
    {
        shakeAllowed = true;
    }
    else if (GameStatics::GetLevelInfo().nomorematchaction_ == LOOSELIFE)
    {
        URHO3D_LOGINFOF("MatchesManager() - CheckAllowShake : Loose star !");

        static_cast<PlayState*>(GameStatics::stateManager_->GetActiveState())->CheckStars(true);
        if (GameStatics::tries_)
            shakeAllowed = true;
    }
    else if (GameStatics::GetLevelInfo().nomorematchaction_ == LOOSEGAME)
    {
        URHO3D_LOGINFOF("MatchesManager() - CheckAllowShake : Loose instantly the game !");
        manager_->SendEvent(GAME_OVER);
    }

    if (shakeAllowed)
    {
        URHO3D_LOGINFOF("MatchesManager() - CheckAllowShake : open frame !");

        if (!shakebuttonFrame)
        {
            shakebuttonFrame = GameHelpers::AddInteractiveFrame("UI/InteractiveFrame/GameShakeMatches.xml", manager_,
                                                                new Urho3D::EventHandlerImpl<MatchesManager>(manager_, &MatchesManager::HandleShakeButtonPressedAck), false);
            shakebuttonFrame->SetBehavior(IB_BUTTON);
        }

        shakebuttonFrame->Start(false, false);
    }
}

void MatchesManager::ShakeMatches(int gridid)
{
    URHO3D_LOGINFOF("MatchesManager() - ShakeMatches : Respawn Matches !");

    ResetObjects(gridid);
}

void MatchesManager::SelectAbility(int ability, int gridid)
{
    if (!manager_)
        return;

    GetGridInfo(gridid)->abilitySelected_ = ability;
}

void MatchesManager::DestroyMatch(const Vector2& position)
{
    if (!manager_)
        return;

    int gridid;
    Match* match = manager_->GetMatchAtPosition(position, &gridid);
    if (match && match->ctype_)
    {
        GetGrid(gridid).Remove(*match);
        GetGridInfo(gridid)->ResetHints();
    }
}

void MatchesManager::SubscribeToEvents()
{
    if (!GameStatics::rootScene_ || !manager_)
        return;

#ifdef ACTIVE_TESTMODE
    if (testMode_)
    {
        manager_->SubscribeToEvent(GameStatics::rootScene_, E_SCENEPOSTUPDATE, new Urho3D::EventHandlerImpl<MatchesManager>(manager_, &MatchesManager::HandleUpdateTestMode));
    }
    else
#endif
    if (GameStatics::GetLevelInfo().mode_ == CLASSICLEVELMODE)
    {
        manager_->SubscribeToEvent(GameStatics::rootScene_, E_SCENEPOSTUPDATE, new Urho3D::EventHandlerImpl<MatchesManager>(manager_, &MatchesManager::HandleUpdateClassicMode));
    }
    else
    {
        manager_->SubscribeToEvent(GameStatics::rootScene_, E_SCENEPOSTUPDATE, new Urho3D::EventHandlerImpl<MatchesManager>(manager_, &MatchesManager::HandleUpdateBossMode));
    }
}

void MatchesManager::UnsubscribeFromEvents()
{
    if (manager_ && GameStatics::rootScene_)
        manager_->UnsubscribeFromEvent(GameStatics::rootScene_, E_SCENEPOSTUPDATE);
}


// Getters

const StringHash& MatchesManager::GetAuthorizedObjectByIndex(const StringHash& category, unsigned index)
{
    assert(index < MatchGrid::authorizedTypes_[category].Size());

    return MatchGrid::authorizedTypes_[category][index];
}

float MatchesManager::GetElapsedTime(int gridid)
{
    if (!manager_)
        return 0.f;

    return GetGridInfo(gridid)->currentTime_ - GetGridInfo(gridid)->initialTime_;
}


const Vector<MatchObjective >& MatchesManager::GetObjectives(int gridid)
{
    return GetGridInfo(gridid)->objectives_;
}

int MatchesManager::GetObjectiveIndex(StringHash got, int gridid)
{
    const Vector<MatchObjective >& objectives = GetGridInfo(gridid)->objectives_;
    for (Vector<MatchObjective >::ConstIterator it = objectives.Begin(); it != objectives.End(); ++it)
    {
        if (it->type_ == got)
            return it - objectives.Begin();
    }
    return -1;
}

int MatchesManager::GetObjectiveIndex(unsigned char ctype, int gridid)
{
    const Vector<MatchObjective >& objectives = GetGridInfo(gridid)->objectives_;
    for (Vector<MatchObjective >::ConstIterator it = objectives.Begin(); it != objectives.End(); ++it)
    {
        if (it->ctype_ == ctype)
            return it - objectives.Begin();
    }
    return -1;
}

int MatchesManager::GetObjectiveCounter(unsigned char index, int gridid)
{
    return GetGridInfo(gridid)->objectives_[index].count_;
}

int MatchesManager::GetObjectiveRemainingQty(unsigned char index, int gridid)
{
    if (gridid == -1)
    {
        if (manager_->netplaymod_ == NETPLAY_COLLAB)
        {
            int remain = 0;
            for (gridid = 0; gridid < manager_->gridinfos_.Size(); gridid++)
            {
                const Vector<MatchObjective >& objectives = GetObjectives(gridid);
                remain += (objectives[index].target_ - objectives[index].count_);
            }
            return Max(remain, 0);
        }
        else
            gridid = 0;
    }

    return Max(GetGridInfo(gridid)->objectives_[index].target_ - GetGridInfo(gridid)->objectives_[index].count_, 0);
}


int MatchesManager::GetNumMovesUsed(int gridid)
{
    return GetGridInfo(gridid)->moveCount_;
}

Node* MatchesManager::GetRandomObject(int gridid)
{
    GameRand& random = GameRand::GetRandomizer(OBJRAND);
    MatchGrid& mgrid = GetGrid(gridid);

    for (int tries = 0; tries < 100; tries++)
    {
        Node* node = mgrid.GetObject(random.Get(mgrid.width_), random.Get(mgrid.height_));
        if (node)
            return node;
    }

    return 0;
}

unsigned MatchesManager::GetAllItemsOnGrid(int gridid)
{
    if (!manager_)
        return 0U;

    Vector<const Match*> items = GetGrid(gridid).GetAllItems();
    for (unsigned i=0; i < items.Size(); i++)
        GetGridInfo(gridid)->AcquireItems(*items[i]);

    return items.Size();
}

void MatchesManager::GetPowerBonusesOnGrid(float ratio, int gridid)
{
    if (!manager_)
        return;

    Vector<Match*> powers = GetGrid(gridid).GetAllPowers();
    unsigned numpowers = Round(powers.Size() * ratio);
    for (unsigned i=0; i < numpowers; i++)
        GetGridInfo(gridid)->AcquirePower(*powers[i]);
}

unsigned MatchesManager::GetNumPossibleMatches(int gridid)
{
    if (!manager_)
        return 0U;

    return !GetGrid(gridid).ContainsItems() ? 0U : GetGridInfo(gridid)->hints_.Size();
}

Node* MatchesManager::GetWorldGridNode(int x, int y, int gridid)
{
    if (!manager_)
        return 0;

    return GetGrid(gridid).tiles_(x, y);
}

Match* MatchesManager::GetMatchAtPosition(const Vector2& position, int* gridid)
{
    if (!manager_)
        return 0;

    RigidBody2D* body = GameStatics::rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBody(position, 1);
    if (body && body->GetNode() && body->GetNode()->GetVar(GOA::GRIDCOORD) != Variant::EMPTY)
    {
        const IntVector3& coord = body->GetNode()->GetVar(GOA::GRIDCOORD).GetIntVector3();
        if (gridid)
            *gridid = coord.z_;
        return &GetGrid(coord.z_).matches_(coord.x_, coord.y_);
    }

    return 0;
}

const Match& MatchesManager::GetMatchHitted(const Vector2& position, int* gridid) const
{
    if (!manager_)
        return Match::EMPTY;

    RigidBody2D* body = GameStatics::rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBody(position, 1);
    if (body && body->GetNode() && body->GetNode()->GetVar(GOA::GRIDCOORD) != Variant::EMPTY)
    {
        const IntVector3& coord = body->GetNode()->GetVar(GOA::GRIDCOORD).GetIntVector3();
        if (gridid)
            *gridid = coord.z_;
        return GetGrid(coord.z_).matches_(coord.x_, coord.y_);;
    }

    return Match::EMPTY;
}


// Handlers

void MatchesManager::HandleUpdateInitial(StringHash eventType, VariantMap& eventData)
{
    int numfinishedinitial = 0;

    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        GetGridInfo(i)->Update_Initial();

        if (GetGridInfo(i)->state_ == NoMatchState)
            numfinishedinitial++;
    }

    if (numfinishedinitial == manager_->gridinfos_.Size())
    {
        for (int i = 0; i < manager_->gridinfos_.Size(); i++)
        {
            GetGridInfo(i)->Init();
            GetGridInfo(i)->ResetState();
        }

        SubscribeToEvents();
    }
}

void MatchesManager::HandleUpdateClassicMode(StringHash eventType, VariantMap& eventData)
{
    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        MatchGridInfo* gridinfo = manager_->gridinfos_[i];

        if (gridinfo->netusage_ == NETLOCAL && GameStatics::allowInputs_)
            gridinfo->UpdateControl();
        else if (gridinfo->netusage_ == NETREMOTE)
            gridinfo->Net_UpdateControl();

        gridinfo->Update();
    }
}

void MatchesManager::HandleUpdateBossMode(StringHash eventType, VariantMap& eventData)
{
    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        GetGridInfo(i)->UpdateItems();

        if (GameStatics::allowInputs_)
            GetGridInfo(i)->UpdateControl_Boss();

        GetGridInfo(i)->Update_Boss();
    }
}

#ifdef ACTIVE_TESTMODE
void MatchesManager::HandleUpdateTestMode(StringHash eventType, VariantMap& eventData)
{
    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        if (GameStatics::allowInputs_)
            GetGridInfo(i)->UpdateControl_Test();

        GetGridInfo(i)->Update_Test();
    }
}
#endif


void MatchesManager::HandleShakeButtonPressedAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

    UnsubscribeFromEvent(E_MESSAGEACK);
    matchmessageBox.Reset();

    // shakebuttonFrame is self stopped in InteractiveFrame::HandleInput()
	if (eventData[P_OK].GetBool())
    {
        URHO3D_LOGINFO("MatchesManager() - HandleShakeButtonPressedAck : Shake Matches !");
        ShakeMatches();
    }

    GameHelpers::SetMusicVolume(1.f);
    GameHelpers::SetSoundVolume(1.f);
}


// Debug

void MatchesManager::DrawDebugGeometry(DebugRenderer* debug, bool depth) const
{
    for (int i = 0; i < manager_->gridinfos_.Size(); i++)
    {
        const MatchGrid& mgrid = GetGrid(i);
        // Add grid rect
        debug->AddLine(Vector3(mgrid.gridRect_.min_), Vector3(mgrid.gridRect_.min_.x_, mgrid.gridRect_.max_.y_), Color::CYAN, depth);
        debug->AddLine(Vector3(mgrid.gridRect_.min_.x_, mgrid.gridRect_.max_.y_), Vector3(mgrid.gridRect_.max_), Color::CYAN, depth);
        debug->AddLine(Vector3(mgrid.gridRect_.max_), Vector3(mgrid.gridRect_.max_.x_, mgrid.gridRect_.min_.y_), Color::CYAN, depth);
        debug->AddLine(Vector3(mgrid.gridRect_.max_.x_, mgrid.gridRect_.min_.y_), Vector3(mgrid.gridRect_.min_), Color::CYAN, depth);

        // Add grid node
        debug->AddNode(mgrid.gridNode_.Get());

        if (!mgrid.matches_.Size() || !mgrid.objects_.Size())
            continue;

        // Add Circle if object exists
        // Add Red Line if wall exists
        for (int y=0; y < mgrid.height_; y++)
        {
            for (int x=0; x < mgrid.width_; x++)
            {
                Vector3 pos = mgrid.CalculateMatchPosition(x, y);

                if (sceneScale_ < 1.f)
                    pos *= sceneScale_;

                if (mgrid.objects_(x,y))
                    debug->AddCircle(pos, Vector3::FORWARD, 0.05f, Color::GREEN, 8, depth);

                const Match& match = mgrid.matches_(x,y);

                if (mgrid.grid_(x,y).wallorientation_ != 0)
                {
                    if (mgrid.HasWalls(match, WO_WALLRIGHT))
                        debug->AddLine(Vector3(pos.x_ + 0.1f, pos.y_ - 0.1f, 0.f), Vector3(pos.x_ + 0.1f, pos.y_ + 0.1f, 0.f), Color::RED, depth);
                    if (mgrid.HasWalls(match, WO_WALLLEFT))
                        debug->AddLine(Vector3(pos.x_ - 0.1f, pos.y_ - 0.1f, 0.f), Vector3(pos.x_ - 0.1f, pos.y_ + 0.1f, 0.f), Color::RED, depth);
                    if (mgrid.HasWalls(match, WO_WALLNORTH))
                        debug->AddLine(Vector3(pos.x_ - 0.1f, pos.y_ + 0.1f, 0.f), Vector3(pos.x_ + 0.1f, pos.y_ + 0.1f, 0.f), Color::RED, depth);
                    if (mgrid.HasWalls(match, WO_WALLSOUTH))
                        debug->AddLine(Vector3(pos.x_ - 0.1f, pos.y_ - 0.1f, 0.f), Vector3(pos.x_ + 0.1f, pos.y_ - 0.1f, 0.f), Color::RED, depth);
                }
            }
        }
    }
}



/// MatchGridInfo

MatchGridInfo::MatchGridInfo() :
    RefCounted(),
    abilitySelected_(StringHash::ZERO),
    hintsearchTimer_(0U)
{
    netTosendCommands_.Reserve(100);
}

MatchGridInfo::~MatchGridInfo()
{ }

void MatchGridInfo::Init()
{
    objectives_.Reserve(MAXOBJECTIVES);
    objectiveDirty_ = false;
    moveCount_ = 0;

    animationTimer_.Reset();
    updateitemsTimer_.Reset();
    hintTimer_.Reset();

    initialTime_ = currentTime_ = GameStatics::context_->GetSubsystem<Time>()->GetElapsedTime();

    abilitySelected_ = -1;
    ishowhints_ = ilasthint_ = -1;

    unblockinput_ = true;
    hintsenabled_ = false;
    canUnselect_ = false;

    for (int i=0; i < 2; i++)
        selected_[i] = Match::EMPTY;

//    netReceivedCommands_.Clear();
    netTosendCommands_.Clear();

    GameStatics::allowInputs_ = true;
}

void MatchGridInfo::StartFirstMoves()
{
    URHO3D_LOGINFOF("MatchGridInfo() - StartFirstMoves !");

    // first search
    allowCheckObjectives_ = false;
    hintsenabled_ = false;
    // save Tutorial state
    tutorialStateSaved_ = GameStatics::playerState_->tutorialEnabled_;
    state_ = NoMatchState;

    if (FindMatches(startMatches_))
    {
        ChangeState(SuccessMatch);
        successTurns_++;
    }
}

void MatchGridInfo::Create()
{
    Init();

    mgrid_.ClearObjects();
    mgrid_.SetLayout(MatchesManager::Get()->gridsize_, MatchesManager::Get()->gridlayout_, HA_CENTER, VA_CENTER, true);

    InputMoveThreshold = Max(3, GameStatics::graphics_->GetWidth() / 10 / Max(MatchesManager::Get()->gridsize_, 5));

    URHO3D_LOGINFOF("MatchGridInfo() - Create : level=%d InputMoveThreshold=%d !", GameStatics::currentLevel_, InputMoveThreshold);

    CreateObjects();
}

void MatchGridInfo::CreateObjects()
{
    startMatches_.Clear();

    mgrid_.Create(startMatches_);

    ResetState();
    ResetHints();
}

void MatchGridInfo::SpawnObjects()
{
    startMatches_.Clear();

    mgrid_.ClearObjects(false);
    mgrid_.InitializeObjects(startMatches_);

    ResetState();
    ResetHints();

    // first search
    allowCheckObjectives_ = false;

    if (GameStatics::GetLevelInfo().mode_ == CLASSICLEVELMODE)
    {
        if (FindMatches(startMatches_))
        {
            ChangeState(SuccessMatch);

            successTurns_++;
//            animationTimer_ = turnTime_ = MOVETIME;
            turnTime_ = MOVETIME*1000;
            animationTimer_.Reset();
        }
    }
}

// Handle Objectives

MatchObjective* MatchGridInfo::GetObjective(StringHash got)
{
    for (Vector<MatchObjective >::Iterator it = objectives_.Begin(); it != objectives_.End(); ++it)
    {
        if (it->type_ == got)
            return &(*it);
    }
    return 0;
}

void MatchGridInfo::AddObjective(StringHash got, unsigned char matchType, int numitems)
{
    MatchObjective* objective = GetObjective(got);
    if (objective)
    {
        URHO3D_LOGWARNINGF("MatchGridInfo() - AddObjective : type=%u ctype=%u already registered update numitems=%d!", got.Value(), objective->ctype_, numitems);
    }
    else
    {
        URHO3D_LOGWARNINGF("MatchGridInfo() - AddObjective : registers objective type=%u numitems=%d!", got.Value(), numitems);
        objectives_.Resize(objectives_.Size()+1);
        objective = &objectives_.Back();
        objective->type_  = got;
        objective->ctype_ = matchType;
    }

    objective->target_ = numitems;
    objective->count_  = 0;
}

bool MatchGridInfo::CheckObjective(Match* match)
{
    if (!allowCheckObjectives_)
        return false;

    if (match->ctype_ && match->ctype_ < ITEMS)
    {
        // Classic Mode
        if (GameStatics::GetLevelInfo().mode_ == CLASSICLEVELMODE)
        {
            int objindex = match->effect_ == NOEFFECT ? match->otype_ : MatchesManager::GetObjectiveIndex(match->ctype_, mgrid_.gridid_);
            if (objindex >= 0 && objindex < objectives_.Size())
            {
                objectives_[objindex].count_++;
                URHO3D_LOGINFOF("MatchGridInfo() - CheckObjective : match=%s index=%d counter=%d/%d",
                                match->ToString().CString(), objindex, objectives_[objindex].count_, objectives_[objindex].target_);

//                if (objectives_[objindex].count_ <= objectives_[objindex].target_)
                {
                    objectiveDirty_ = true;
                    currentTime_ = GameStatics::context_->GetSubsystem<Time>()->GetElapsedTime();
                    return true;
                }
            }
        }
        // Boss Mode
        else
        {
            objectives_[0].count_++;
//            if (objectives_[0].count_ <= objectives_[0].target_)
            {
                objectiveDirty_ = true;
                currentTime_ = GameStatics::context_->GetSubsystem<Time>()->GetElapsedTime();
                return true;
            }
        }
    }

    return false;
}


bool MatchGridInfo::IsGridCreated() const
{
    return mgrid_.GetGridNode() != nullptr;
}

bool MatchGridInfo::IsMoveValidated(const Match& m1, const Match& m2) const
{
    if (MatchesManager::allowSelectionAlongHV_)
        return (m1.x_ == m2.x_ && m1.y_ != m2.y_) || (m1.x_ != m2.x_ && m1.y_ == m2.y_);
    else
        return (m1.x_ == m2.x_ && (m1.y_ == m2.y_+1 || m1.y_ == m2.y_-1)) || ((m1.x_ == m2.x_+1 || m1.x_ == m2.x_-1) && m1.y_ == m2.y_);
}

bool MatchGridInfo::IsMoveValidated_Boss(const Match& m1, const Match& m2) const
{
    if (!mgrid_.IsDirectionalTile(m2))
    {
//        URHO3D_LOGINFOF("MatchesManager() - IsMoveValidated_Boss : m2 != DirectionTile m1(%u,%u) m2(%u,%u)", m1.x_, m1.y_, m2.x_, m2.y_);

        if ((m1.x_ == m2.x_ && (m1.y_ == m2.y_+1 || m1.y_ == m2.y_-1)) || ((m1.x_ == m2.x_+1 || m1.x_ == m2.x_-1) && m1.y_ == m2.y_))
            return true;
    }

    return false;
}

void MatchGridInfo::ResetSelection()
{
    // Reset Selection & Reset position

    for (int i=0; i < 2; i++)

        if (selected_[i].ctype_)
        {
            mgrid_.ResetPosition(selected_[i]);

            selected_[i] = Match::EMPTY;
        }
}

void MatchGridInfo::ResetScaleOnSelection()
{
    if (selected_[0].x_ != 255)
        mgrid_.ResetScale(selected_[0]);

    if (selected_[1].x_ != 255)
        mgrid_.ResetScale(selected_[1]);

    selected_[0] = selected_[1] = Match::EMPTY;
}

void MatchGridInfo::ResetState()
{
//    URHO3D_LOGINFOF("MatchesManager() - ResetState !");

    ChangeState(NoMatchState);
    successTurns_ = turnProcessed_ = 0;

    CheckHints(true);

    if (!objectiveDirty_)
        MatchesManager::Get()->SendEvent(GAME_NOMATCHSTATE);
}

void MatchGridInfo::ChangeState(int state)
{
    if (state != state_)
    {
        state_ = state;
        MatchesManager::Get()->SendEvent(GAME_MATCHSTATECHANGE);
    }
}

void MatchGridInfo::ResetObjectives()
{
    objectiveDirty_ = false;

    objectives_.Clear();
}

void MatchGridInfo::SelectStartMatch(const Match& match)
{
    if (!mgrid_.SetSelectAnimation(match, 1))
        return;

    selected_[0] = match;

    ChangeState(StartSelection);
}

void MatchGridInfo::ResetAbility()
{
    abilitySelected_ = -1;
    MatchesManager::Get()->SendEvent(GAME_ABILITYRESETED);
}

bool MatchGridInfo::CheckCurrentAbilityOn(const Match& match)
{
    if (abilitySelected_ == -1)
        return false;

    if (match.effect_)
        return false;

//    // BONUS "WHITE"
//    if (abilitySelected_ == 1)
//    {
//        return true;
//    }
//    // BONUS "PURPLE"
//    else if (abilitySelected_ == 2)
//    {
//        return match.ctype_ == PURPLE;
//    }
//    // BONUS "BLUE"
//    else if (abilitySelected_ == 3)
//    {
//        return match.ctype_ == BLUE;
//    }
//    // BONUS "GREEN"
//    else if (abilitySelected_ == 4)
//    {
//        return match.ctype_ == GREEN;
//    }

    return true;
}

void MatchGridInfo::ApplyCurrentAbilityOn(Match* match)
{
    Node* node = mgrid_.AddPowerBonus(match, abilitySelected_-1);
    if (!node)
        return;

	URHO3D_LOGINFOF("MatchGridInfo() - ApplyCurrentAbilityOn : match=%s ...", match->ToString().CString());

	matchesToCheck_.Clear();
	if (MatchesManager::checkallpowersonturn_)
        Match::AddDistinctEntries(mgrid_.GetAllPowers(), matchesToCheck_);

    matchesToCheck_.Push(match);

    allowCheckObjectives_ = true;

    if (FindMatches(matchesToCheck_))
    {
//        SendEvent(GAME_MOVEREMOVED);

        ChangeState(SuccessMatch);

        successTurns_++;
//        animationTimer_ = turnTime_ = MOVETIME;
        animationTimer_.Reset();
        turnTime_ = MOVETIME*1000;

        URHO3D_LOGINFOF("MatchGridInfo() - ApplyCurrentAbilityOn : entry=%s matches=%u => APPLY SUCCESS",
						match->ToString().CString(), successmatches_.Size());
    }

    // Update Ability Qty in GameStatic
    GameStatics::playerState_->powers_[abilitySelected_-1].qty_ = Max(GameStatics::playerState_->powers_[abilitySelected_-1].qty_-1, 0);

    VariantMap& eventdata = GameStatics::context_->GetEventDataMap();
    eventdata[Game_AbilityRemoved::ABILITY_ID] = abilitySelected_;
    MatchesManager::Get()->SendEvent(GAME_ABILITYREMOVED, eventdata);

    ResetAbility();
    ResetHints();
    CheckHints(true);

	URHO3D_LOGINFOF("MatchGridInfo() - ApplyCurrentAbilityOn : ... match=%s", match->ToString().CString());
}

void MatchGridInfo::ResetHints()
{
    if (!MatchesManager::allowHints_)
        return;

    if (ishowhints_ != -1)
        SetHintsAnimations(false);

    hints_.Clear();
    hintstate_ = 0;
    ishowhints_ = ilasthint_ = -1;
    ihintsearchmatch_ = 0;
    hintTimer_.Reset();
}

void MatchGridInfo::SetHintsAnimations(bool state)
{
    if (ishowhints_ == -1 || ishowhints_ >= hints_.Size())
    {
        URHO3D_LOGWARNINGF("MatchGridInfo() - SetHintsAnimations : %s ; ERROR index=%d !", state ? "SHOW":"MASK", ishowhints_);
        return;
    }

    const Vector<Match*>& hints = hints_[ishowhints_];

//    URHO3D_LOGINFOF("MatchGridInfo() - SetHintsAnimations : %s ; index=%d hints=%u !", state ? "SHOW":"MASK", ishowhints_, hints.Size());

    for (Vector<Match*>::ConstIterator it=hints.Begin();it!=hints.End();++it)
        mgrid_.SetHintAnimation(**it, state);
}

void MatchGridInfo::CheckHints(bool state)
{
    if (!MatchesManager::allowHints_)
        return;

    if (state != hintsenabled_)
    {
        hintsenabled_ = state;

        if (hintsenabled_)
        {
//            URHO3D_LOGINFOF("MatchGridInfo() - CheckHints : ENABLED !");
        }
        else
        {
//            URHO3D_LOGINFOF("MatchGridInfo() - CheckHints : DISABLED !");
            ResetHints();
        }
    }
}

void MatchGridInfo::MoveSelection()
{
    const Match& m1 = selected_[0];
    const Match& m2 = selected_[1];

    WeakPtr<Node>& object1 = mgrid_.objects_(m1.x_, m1.y_);
    WeakPtr<Node>& object2 = mgrid_.objects_(m2.x_, m2.y_);

    if (!object1 && !object2)
        return;

    mgrid_.SetSelectAnimation(m2, 0);

    // unblock checking objectives
    allowCheckObjectives_ = true;

    // Permute Object Selection
	{
        if (object1)
        {
            mgrid_.SetMoveAnimation(m1, object1, object2 ? object2->GetPosition() : mgrid_.CalculateMatchPosition(m2.x_, m2.y_), MOVETIME);
//            URHO3D_LOGINFOF("MatchGridInfo() - MoveSelection : move m1(%u,%u) -> m2(%u,%u) !", m1.x_, m1.y_, m2.x_, m2.y_);
        }

        if (object2)
        {
            mgrid_.SetMoveAnimation(m2, object2, object1 ? object1->GetPosition() : mgrid_.CalculateMatchPosition(m1.x_, m1.y_), MOVETIME);
//            URHO3D_LOGINFOF("MatchGridInfo() - MoveSelection : move m2(%u,%u) -> m1(%u,%u) !", m2.x_, m2.y_, m1.x_, m1.y_);
        }

        mgrid_.PermuteMatchType(m1, m2);
    }

	matchesToCheck_.Clear();
	if (MatchesManager::checkallpowersonturn_)
        Match::AddDistinctEntries(mgrid_.GetAllPowers(), matchesToCheck_);

    Match::AddDistinctEntries(mgrid_.GetPermuttedMatches(), matchesToCheck_);

    // assign a first turnTime
//    animationTimer_ = turnTime_ = MOVETIME;
    turnTime_ = MOVETIME*1000;
    animationTimer_.Reset();

    ChangeState(SelectionAnimation);
}

void MatchGridInfo::ConfirmSelection()
{
    if (FindMatches(matchesToCheck_))
    {
        mgrid_.ConfirmPermute();

        for (int i=0; i < 2; i++)
            selected_[i] = Match::EMPTY;

        moveCount_++;
        // TODO : for local multiplayer ... send the gridid
        MatchesManager::Get()->SendEvent(GAME_MOVEREMOVED);

        ChangeState(SuccessMatch);
        successTurns_++;

        URHO3D_LOGINFOF("MatchGridInfo() - ConfirmSelection : matches=%u => APPLY SUCCESS", successmatches_.Size());
    }
    // Undo Permutation if no matches
    else
    {
        mgrid_.UndoPermute();

        ChangeState(CancelMove);

        URHO3D_LOGINFOF("MatchGridInfo() - ConfirmSelection : matches=%u => UNDO PERMUTATION", successmatches_.Size());
    }

    turnTime_ = MOVETIME*1000;
    animationTimer_.Reset();
}

void MatchGridInfo::ConfirmSelection_Boss()
{
    if (FindMatches(matchesToCheck_))
    {
        ChangeState(SuccessMatch);

        successTurns_++;
        turnTime_ = MOVETIME*1000;
        animationTimer_.Reset();
//        URHO3D_LOGINFOF("MatchGridInfo() - ConfirmSelection : matches=%u => APPLY SUCCESS", successmatches_.Size());
    }
    else
        ResetState();

    bool confirm = false;

    // always confirm permute on tileramps
    if (state_ == SuccessMatch || mgrid_.IsDirectionalTile(selected_[0]) || mgrid_.IsDirectionalTile(selected_[1]))
    {
        confirm = true;
        mgrid_.ResetScale(selected_[0]);
        mgrid_.ResetScale(selected_[1]);
        mgrid_.ConfirmPermute();

        moveCount_++;
        // TODO : for local multiplayer ... send the gridid
        MatchesManager::Get()->SendEvent(GAME_MOVEREMOVED);
    }
    else
    {
        mgrid_.UndoPermute();
        ChangeState(CancelMove);
        turnTime_ = MOVETIME*1000;
        animationTimer_.Reset();
    }

    if (state_ != SuccessMatch)
    {
        // Find columns
        Vector<unsigned char> columns;

        if (!mgrid_.IsDirectionalTile(selected_[0]))
            columns.Push(selected_[0].x_);

        if (!mgrid_.IsDirectionalTile(selected_[1]) && !columns.Contains(selected_[1].x_))
            columns.Push(selected_[1].x_);

        // Collapse columns
        Vector<Match*> collapsedMatches;
        int maxdistance = 1;
        for (Vector<unsigned char>::ConstIterator xt=columns.Begin();xt!=columns.End();++xt)
        {
            int distance = mgrid_.CollapseColumn(*xt, collapsedMatches);
            if (distance > maxdistance)
                maxdistance = distance;
        }
        turnTime_ = Min(Max((float)maxdistance, 1.f) * MOVETIME, MAXMOVETIME)*1000;
        animationTimer_.Reset();
    }

    if (confirm)
        selected_[0] = selected_[1] = Match::EMPTY;

//    URHO3D_LOGINFOF("MatchGridInfo() - ConfirmSelection_Boss !");
}

void MatchGridInfo::RemoveMatchAndCollapse(const Match& m)
{
    Match& match = mgrid_.matches_(m.x_, m.y_);
    destroymatches_.Push(&match);

    ChangeState(SuccessMatch);
    successtoProcess_ = true;
    successTurns_ = 1;
    turnTime_ = MOVETIME*1000;
    animationTimer_.Reset();
}

void MatchGridInfo::AcquireItems(const Match& m)
{
    if (m.ctype_ >= ITEMS && m.ctype_ < ROCKS)
    {
        URHO3D_LOGINFOF("MatchGridInfo() - AcquireItems : Selection x=%d y=%d type=%d", m.x_, m.y_, m.ctype_);
        GameStatics::AddBonus(Slot(COT::GetCategoryIn(COT::ITEMS, m.ctype_-ITEMS), (int)m.otype_, int(m.qty_)), MatchesManager::Get());

        RemoveMatchAndCollapse(mgrid_.matches_(m.x_, m.y_));
    }
}

void MatchGridInfo::AcquirePower(const Match& m)
{
    if (m.ctype_ < ITEMS && m.otype_ != -1 && m.effect_ != NOEFFECT)
    {
        StringHash bonustype = COT::GetTypeFromCategory(COT::POWERS, m.otype_);
        URHO3D_LOGINFOF("MatchGridInfo() - AcquirePower : Selection x=%d y=%d type=%s(%d)", m.x_, m.y_, GOT::GetType(bonustype).CString(), bonustype.Value());
        GameStatics::AddBonus(Slot(COT::POWERS, m.otype_, 1), MatchesManager::Get());

        RemoveMatchAndCollapse(mgrid_.matches_(m.x_, m.y_));
    }
}


bool MatchGridInfo::FindMatches(const Vector<Match*>& tocheck)
{
    destroymatches_.Clear();
    successmatches_.Clear();
    activablebonuses_.Clear();
    brokenrocks_.Clear();
    hittedwalls_.Clear();

    Vector<Match*> newfoundmatches;

    for (int i=0; i < tocheck.Size(); i++)
    {
        newfoundmatches.Clear();

        if (mgrid_.GetMatches(tocheck[i], newfoundmatches, successmatches_, activablebonuses_, brokenrocks_, hittedwalls_))
            Match::AddDistinctEntries(newfoundmatches, destroymatches_);
    }

	if (brokenrocks_.Size() && GameStatics::IsBossLevel())
	{
		Match::AddDistinctEntries(brokenrocks_, destroymatches_);
		brokenrocks_.Clear();
	}

	if (hittedwalls_.Size())
	{
		mgrid_.SetHittedWalls(hittedwalls_);
	}

    successtoProcess_ = (destroymatches_.Size() > 0);

    return successtoProcess_;
}

void MatchGridInfo::ApplySuccessMatches()
{
//    URHO3D_LOGINFOF("MatchGridInfo() - ApplySuccessMatches : ...");

    // Reset hints searching
    if (MatchesManager::allowHints_)
    {
        if (ishowhints_ != -1)
            SetHintsAnimations(false);

        hintstate_ = HintNoMove;
        hintTimer_.Reset();
        ihintsearchmatch_ = 0;
    }

    successtoProcess_ = false;

    matchesToCheck_.Clear();

    Vector<unsigned char> columns;

    Match turnbonus;

    if (allowCheckObjectives_)
    {
//        URHO3D_LOGINFOF("MatchesManager() - ApplySuccessMatches : ... matches=%u => SUCCESS=%d", successmatches_.Size(), successTurns_);
        // Activate effects for activablebonuses
        for (Vector<Match*>::ConstIterator mt=activablebonuses_.Begin();mt!=activablebonuses_.End();++mt)
            mgrid_.AddPowerEffects(**mt);

        for (Vector<Match*>::ConstIterator mt=successmatches_.Begin();mt!=successmatches_.End();++mt)
            mgrid_.AddSuccessEffect(**mt);

        // search for a bonus if enough success
        if (successmatches_.Size() > 0  && (successTurns_ >= Match::BONUSMINIMALSUCCESS || successmatches_.Size() >= Match::BONUSMINIMALMATCHES))
        {
            turnbonus = *successmatches_.Front();
            turnbonus.effect_ = XEXPLOSION;
//            URHO3D_LOGINFOF("MatchGrid() - ApplySuccessMatches : Add Turn Bonus ! ...", turnbonus.ToString().CString());
        }

        if (successTurns_ >= Match::BONUSGAINMOVE && successTurns_%Match::BONUSGAINMOVE == 0)
            // TODO : for local multiplayer ... send the gridid
            MatchesManager::Get()->SendEvent(GAME_MOVEADDED);
    }

    // find columns to collapse and remove objects
    {
//        URHO3D_LOGINFOF("MatchesManager() - ApplySuccessMatches : find columns to collapse and remove objects ...");
        for (Vector<Match*>::ConstIterator mt=destroymatches_.Begin();mt!=destroymatches_.End();++mt)
        {
            Match* match = *mt;

            if (!columns.Contains(match->x_))
                columns.Push(match->x_);

            if (CheckObjective(match))
                mgrid_.AddObjectiveEffect(*match);
            else
                GameHelpers::SpawnSound(GameStatics::rootScene_, SND_NOOBJECTIVESUCCESS, 0.5f);

            mgrid_.Remove(*match);
        }

        for (Vector<WallInfo>::ConstIterator mt=hittedwalls_.Begin();mt!=hittedwalls_.End();++mt)
        {
            const WallInfo& winfo = *mt;

            if (!columns.Contains(winfo.x_))
                columns.Push(winfo.x_);
        }
    }

    // add the new turn bonus
    Node* bonusNode = 0;
    if (turnbonus.effect_)
        bonusNode = mgrid_.AddPowerBonus(&turnbonus);

    int givenBonuses = 0;
    if (GameStatics::gameState_.storyitems_[0])
    {
        // add broken rocks bonuses
        givenBonuses = brokenrocks_.Size() / 2;
        const Vector<StringHash>& items = COT::GetObjectsInCategory(COT::MOVES);
        if (givenBonuses > 0 && items.Size())
        {
            // replace rocks by item
            for (unsigned i=0; i < givenBonuses; i++)
            {
//                Node* node = mgrid_.AddItemBonus(*brokenrocks_[i], COT::MOVES, i % items.Size());
                Node* node = mgrid_.AddItemBonus(*brokenrocks_[i], COT::MOVES, 0);
                GameHelpers::SpawnSound(GameStatics::rootScene_, SND_NOOBJECTIVESUCCESS, 0.5f);
            }
        }
    }

    // remove rocks
    if (givenBonuses < brokenrocks_.Size())
    {
        for (unsigned i=givenBonuses; i < brokenrocks_.Size(); i++)
        {
            const Match& match = *brokenrocks_[i];
            mgrid_.Remove(match);
            if (!columns.Contains(match.x_))
                columns.Push(match.x_);
        }
    }

    if (allowCheckObjectives_)
    {
        // calculate turnScore
        unsigned turnMultiplier   = (successTurns_/5 + 1);
        unsigned turnDestroyScore = destroymatches_.Size() * POINTS_ByDestroy;
        unsigned turnSuccessScore = successmatches_.Size() * POINTS_BySuccess;
        unsigned turnBonusSuccess = Max(0, successmatches_.Size()+1-Match::BONUSMINIMALMATCHES) * POINTS_ForHighSuccess;
        turnScore_ = (turnDestroyScore+turnSuccessScore) * turnMultiplier + turnBonusSuccess;

        // TODO : for local multiplayer ... send the gridid
        MatchesManager::Get()->SendEvent(GAME_SCORECHANGE);

//        URHO3D_LOGINFOF("MatchGridInfo() - ApplySuccessMatches : SCORE=%u (TurnSuccess=%d Mul=%u Destroy=%u Success=%u Bonus=%u)!",
//                        turnScore_, successTurns_, turnMultiplier, turnDestroyScore, turnSuccessScore, turnBonusSuccess);
    }

    // for calculating the turn time
    int maxdistance = 0;

    // collapse columns
//    URHO3D_LOGINFOF("MatchGridInfo() - ApplySuccessMatches : collapse columns ...");
    Vector<Match*> collapsedMatches;
    for (Vector<unsigned char>::ConstIterator xt=columns.Begin();xt!=columns.End();++xt)
    {
        int distance = mgrid_.CollapseColumn(*xt, collapsedMatches);
        if (distance > maxdistance)
            maxdistance = distance;
    }

    // add new objects in these columns
    if (GameStatics::GetLevelInfo().mode_ == CLASSICLEVELMODE)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - ApplySuccessMatches : add new objects ...");
        for (Vector<unsigned char>::ConstIterator xt=columns.Begin();xt!=columns.End();++xt)
        {
            int distance = mgrid_.AddColumn(*xt, matchesToCheck_);
            if (distance > maxdistance)
                maxdistance = distance;
        }
    }

//    animationTimer_ = turnTime_ = Min(Max((float)maxdistance, 1.f) * MOVETIME, MAXMOVETIME);
    turnTime_ = Min(Max((float)maxdistance, 1.f) * MOVETIME, MAXMOVETIME)*1000;
    animationTimer_.Reset();

    // add matchbonus to find new matches
    if (bonusNode)
        matchesToCheck_.Push(&mgrid_.matches_(turnbonus.x_, turnbonus.y_));

    // update matches to check
	if (MatchesManager::checkallpowersonturn_)
        Match::AddDistinctEntries(mgrid_.GetAllPowers(), matchesToCheck_);
    Match::AddDistinctEntries(collapsedMatches, matchesToCheck_);

    URHO3D_LOGINFOF("MatchGridInfo() - ApplySuccessMatches : ... Finished ! (objectivedirty=%u allowcheck=%u)", objectiveDirty_, allowCheckObjectives_);

    // update objectives
    if (allowCheckObjectives_ && objectiveDirty_)
    {
        // TODO : for local multiplayer ... send the gridid
        MatchesManager::Get()->SendEvent(GAME_OBJECTIVECHANGE);
        objectiveDirty_ = false;
    }
}


inline void MatchGridInfo::SelectMatch(Match*& match, const IntVector2& screenposition)
{
#ifdef ACTIVE_GAMELOOPTESTING
//    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : screenpos=%s", screenposition.ToString().CString());
#endif
    SelectMatch(match, GameHelpers::ScreenToWorld2D(screenposition));
}

inline void MatchGridInfo::SelectMatch(Match*& match, const Vector2& worldposition)
{
#ifdef ACTIVE_GAMELOOPTESTING
//    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : worldpos=%s", worldposition.ToString().CString());
#endif
    match = 0;
    Vector2 tilesize(mgrid_.gridRect_.Size().x_ / (float)mgrid_.width_, mgrid_.gridRect_.Size().y_ / (float)mgrid_.height_);
    int x = (int)floor((worldposition.x_ - mgrid_.gridRect_.min_.x_) / tilesize.x_);
    int y = (int)floor((mgrid_.gridRect_.max_.y_ - worldposition.y_) / tilesize.y_);

    // inside grid
    if (x < 0 || x >= mgrid_.width_ || y < 0 || y >= mgrid_.height_)
        return;

    // Tile exists ?
    Node* node = mgrid_.tiles_(x, y);
    if (node)
    {
        hitposition_ = node->GetWorldPosition2D();
        match = &mgrid_.matches_(x, y);

//        URHO3D_LOGINFOF("MatchGridInfo() - SelectMatch : hit x=%d y=%d match=%u", x, y, match);
    }
}

inline void MatchGridInfo::GetSecondSelection(Match*& newHit)
{
    newHit = 0;

    int x = 0, y = 0;
    int absx, absy;
    bool ok = false;

    // Relative Touch Move check
    if (GameStatics::input_->GetNumTouches() > 0)
    {
        x = GameStatics::input_->GetTouch(0)->position_.x_ - inputposition_.x_;
        y = GameStatics::input_->GetTouch(0)->position_.y_ - inputposition_.y_;
        absx = Abs(x);
        absy = Abs(y);
        ok = Max(absx, absy) > InputMoveThreshold;
    }
    // Relative Mouse Move check
    if (!ok && GameStatics::input_->GetMouseButtonDown(MOUSEB_LEFT))
    {
        x = GameStatics::input_->GetMousePosition().x_ - inputposition_.x_;
        y = GameStatics::input_->GetMousePosition().y_ - inputposition_.y_;
        absx = Abs(x);
        absy = Abs(y);
        ok = Max(absx, absy) > InputMoveThreshold;
    }

    if (!ok)
        return;

    // Inside the grid rect ?
    if (absx > absy)
    {
        x = selected_[0].x_ + Sign(x);
        y = selected_[0].y_;
        ok = (x >= 0 && x < mgrid_.width_);
    }
    else
    {
        x = selected_[0].x_;
        y = selected_[0].y_ + Sign(y);
        ok = (y >= 0 && y < mgrid_.height_);
    }

    // Tile exists ?
    Node* node = 0;
    if (ok)
        node = mgrid_.tiles_(x, y);

    if (ok && node)
    {
        hitposition_ = node->GetWorldPosition2D();
        newHit = &mgrid_.matches_(x, y);
    }
    else
    {
        ResetSelection();
        ResetState();
    }
}


void MatchGridInfo::Net_ReceiveCommands(VectorBuffer& buffer)
{
    buffer.Seek(0);
    while (!buffer.IsEof())
    {
        unsigned char d = buffer.ReadUByte();
        if (d >= NETCMD_START && d <= NETCMD_END)
        {
            const NetCommand cmd = (NetCommand)d;
            if (cmd == NETCMD_START || cmd == NETCMD_END)
                continue;

            netReceivedCommands_.Resize(netReceivedCommands_.Size()+1);
            NetCommandData& cmddata = netReceivedCommands_.Back();
            cmddata.cmd_ = cmd;

            unsigned paramsize = buffer.ReadVLE();
            if (paramsize)
                cmddata.params_.SetData(buffer, paramsize);
        }
    }
}

void NetCommandData::WriteToBuffer(VectorBuffer& buffer) const
{
    buffer.WriteUByte(cmd_);
    buffer.WriteVLE(params_.GetSize());
    buffer.Write(params_.GetData(), params_.GetSize());
}

NetCommandData* MatchGridInfo::Net_PrepareCommand(NetCommand cmd)
{
    if (!Network::Get(false))
        return nullptr;
    if (!GameStatics::peerConnected_)
        return nullptr;

    netTosendCommands_.Resize(netTosendCommands_.Size()+1);
    NetCommandData& cmddata = netTosendCommands_.Back();
    cmddata.cmd_ = cmd;
    return &cmddata;
}

void MatchGridInfo::Net_SendCommands()
{
    if (!Network::Get(false))
        return;
    if (!GameStatics::peerConnected_ || !netTosendCommands_.Size())
        return;

    preparedCommands_.Clear();
    for (Vector<NetCommandData>::ConstIterator it = netTosendCommands_.Begin(); it != netTosendCommands_.End(); ++it)
        it->WriteToBuffer(preparedCommands_);

    Network::Get()->SendBuffer(preparedCommands_, "griddata");

    netTosendCommands_.Clear();
}

void MatchGridInfo::Net_SendGrid()
{
    NetCommandData* cmd = Net_PrepareCommand(NETGRID_SET);
    if (!cmd)
        return;

    mgrid_.Save(cmd->params_);
    Net_SendCommands();
}

void MatchGridInfo::Net_UpdateControl()
{
    // For Test Match
//    if (state_ == NoMatchState)
//    {
//        if (GameStatics::input_->GetMouseButtonPress(MOUSEB_RIGHT))
//        {
//            inputposition_ = GameStatics::input_->GetMousePosition();
//            hitposition_ = GameHelpers::ScreenToWorld2D(inputposition_);
//            Match* newHit;
//            SelectMatch(newHit, hitposition_);
//            if (newHit)
//                URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : MOUSEB_RIGHT : remotegrid position=%s match=%s !", inputposition_.ToString().CString(), newHit->ToString().CString());
//        }
//    }

    if (!netReceivedCommands_.Size())
        return;

    NetCommandData& cmd = netReceivedCommands_.Front();

    switch (cmd.cmd_)
    {
    case NETMATCH_SELECT:
        {
            ResetSelection();
            if (mgrid_.matches_.Size() && mgrid_.objects_.Size())
                SelectStartMatch(mgrid_.matches_(cmd.params_.GetData()[0], cmd.params_.GetData()[1]));
        }
        break;
    case NETMATCH_MOVE:
        {
            ResetSelection();
            if (mgrid_.matches_.Size() && mgrid_.objects_.Size())
            {
                SelectStartMatch(mgrid_.matches_(cmd.params_.GetData()[0], cmd.params_.GetData()[1]));
                selected_[1] = mgrid_.matches_(cmd.params_.GetData()[2], cmd.params_.GetData()[3]);
                CheckHints(false);
                MoveSelection();
            }
        }
        break;
    case NETMATCH_RESET:
        {
            ResetSelection();
        }
        break;
    case NETITEM_ACQUIRE:
        {
            if (mgrid_.matches_.Size() && mgrid_.objects_.Size())
            {
                Match& hittedmatch = mgrid_.matches_(cmd.params_.GetData()[0], cmd.params_.GetData()[1]);
                if (hittedmatch.ctype_ >= ITEMS)
                {
                    AcquireItems(hittedmatch);
                    ResetSelection();
                }
            }
        }
        break;
    case NETABILITY_APPLY:
        {
            abilitySelected_ = cmd.params_.GetData()[0];
            if (mgrid_.matches_.Size() && mgrid_.objects_.Size())
            {
                Match& hittedmatch = mgrid_.matches_(cmd.params_.GetData()[1], cmd.params_.GetData()[2]);
                if (CheckCurrentAbilityOn(hittedmatch))
                {
                    ApplyCurrentAbilityOn(&hittedmatch);
                    URHO3D_LOGWARNINGF("MatchGridInfo() - Net_UpdateControl : Use Ability on x=%d y=%d type=%d", hittedmatch.x_, hittedmatch.y_, hittedmatch.ctype_);
                }
            }
        }
        break;
    case NETSTATE_RESET:
        {
            ResetState();
        }
        break;
    case NETGRID_SET:
        {
            URHO3D_LOGWARNINGF("MatchGridInfo() - Net_UpdateControl : Load Grid ...");
            mgrid_.Load(cmd.params_);
            URHO3D_LOGWARNINGF("MatchGridInfo() - Net_UpdateControl : Load Grid !");
            ResetState();
            ResetSelection();
        }
        break;
    }

    netReceivedCommands_.PopFront();
}


void MatchGridInfo::UpdateControl()
{
    if (state_ == NoMatchState)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : NoMatchState NumTouches=%u MouseLeft=%u", GameStatics::input_->GetNumTouches(), GameStatics::input_->GetMouseButtonPress(MOUSEB_LEFT));

        if (GameStatics::input_->GetNumTouches() > 0 || GameStatics::input_->GetMouseButtonPress(MOUSEB_LEFT))
        {
            if (unblockinput_)
            {
                #ifdef ACTIVE_GAMELOOPTESTING
                if (InputPlayer::Get() && InputPlayer::Get()->IsPlaying())
                {
                    inputposition_ = InputPlayer::Get()->GetPosition();
//                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : looptestinf screenpos=%s", inputposition_.ToString().CString());
                }
                else
                    inputposition_ = GameStatics::input_->GetNumTouches() > 0 ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition();
                #else
                inputposition_ = GameStatics::input_->GetNumTouches() > 0 ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition();
                #endif
//                URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : position=%s", inputposition_.ToString().CString());

//                if (GameStatics::ui_->GetElementAt(inputposition_.x_, inputposition_.y_))
//                {
//                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : UIElement find ! break");
//                    return;
//                }

                hitposition_ = GameHelpers::ScreenToWorld2D(inputposition_);
                Match* newHit;
                SelectMatch(newHit, hitposition_);
                if (newHit)
                {
                    mgrid_.AddTouchEffect(hitposition_);
                    const Match& hit = *newHit;

                    if (hit.ctype_ != NOTYPE)
                    {
                        if (hit.ctype_ < ROCKS)
                        {
                            if (hit.ctype_ >= ITEMS)
                            {
                                AcquireItems(hit);

                                NetCommandData* cmddata = Net_PrepareCommand(NETITEM_ACQUIRE);
                                if (cmddata)
                                {
                                    cmddata->params_.WriteUByte(hit.x_);
                                    cmddata->params_.WriteUByte(hit.y_);
                                }
                            }
                            else
                            {
                                if (abilitySelected_ != -1)
                                {
                                    if (CheckCurrentAbilityOn(hit))
                                    {
                                        ApplyCurrentAbilityOn(MatchesManager::Get()->GetMatchAtPosition(hitposition_));
                                        URHO3D_LOGWARNINGF("MatchGridInfo() - UpdateInput : Use Ability on Selection x=%d y=%d type=%d", hit.x_, hit.y_, hit.ctype_);

                                        NetCommandData* cmddata = Net_PrepareCommand(NETABILITY_APPLY);
                                        if (cmddata)
                                        {
                                            cmddata->params_.WriteUByte(abilitySelected_);
                                            cmddata->params_.WriteUByte(hit.x_);
                                            cmddata->params_.WriteUByte(hit.y_);
                                        }
                                    }
                                }
                                else
                                {
                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : Start Selection x=%d y=%d type=%d", hit.x_, hit.y_, hit.ctype_);
                                    // debug trace
                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : %s %s", mgrid_.HasNoWalls(hit, WO_WALLRIGHT) ? "nowallright":"wallright", mgrid_.HasNoWalls(hit, WO_WALLLEFT)? "nowallleft":"wallleft");

                                    SelectStartMatch(hit);
                                    canUnselect_ = false;

                                    NetCommandData* cmddata = Net_PrepareCommand(NETMATCH_SELECT);
                                    if (cmddata)
                                    {
                                        cmddata->params_.WriteUByte(hit.x_);
                                        cmddata->params_.WriteUByte(hit.y_);
                                        URHO3D_LOGWARNINGF("MatchGridInfo() - UpdateInput : NETMATCH_SELECT x=%u y=%u (size=%u)!", hit.x_, hit.y_, cmddata->params_.GetSize());
                                    }
                                }
                            }

                            unblockinput_ = false;
                        }
                    }
//                    else
//                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : Empty Match !");
                }
//                else
//                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : OutSide Grid !");
            }
        }
        // For Test Match
//        else if (GameStatics::input_->GetMouseButtonPress(MOUSEB_RIGHT))
//        {
//            inputposition_ = GameStatics::input_->GetMousePosition();
//            hitposition_ = GameHelpers::ScreenToWorld2D(inputposition_);
//            Match* newHit;
//            SelectMatch(newHit, hitposition_);
//            if (newHit)
//                URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : MOUSEB_RIGHT : position=%s match=%s !", inputposition_.ToString().CString(), newHit->ToString().CString());
//        }
        else if (!unblockinput_)
        {
//            URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : unblockinput OK !");
            unblockinput_ = true;
        }
    }

    else if (state_ == StartSelection)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : StartSelection NumTouches=%u", GameStatics::input_->GetNumTouches());

        if (GameStatics::input_->GetNumTouches() > 0 || GameStatics::input_->GetMouseButtonDown(MOUSEB_LEFT))
        {
            Match* newHit;
            SelectMatch(newHit, GameStatics::input_->GetNumTouches() ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition());
            if (newHit)
            {
                int dx = Abs((int)newHit->x_ - (int)selected_[0].x_);
                int dy = Abs((int)newHit->y_ - (int)selected_[0].y_);
                if ((dx > 1 || dy > 1) || (dx == 1 && dy == 1))
                {
                    if (newHit->ctype_ && newHit->ctype_ < ROCKS)
                    {
                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : Break Selection with newhit x=%d y=%d type=%d", newHit->x_, newHit->y_, newHit->ctype_);

                        mgrid_.AddTouchEffect(hitposition_);

                        ResetSelection();
                        SelectStartMatch(*newHit);
                        inputposition_ = GameStatics::input_->GetNumTouches() ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition();

                        NetCommandData* cmddata = Net_PrepareCommand(NETMATCH_SELECT);
                        if (cmddata)
                        {
                            cmddata->params_.WriteUByte(newHit->x_);
                            cmddata->params_.WriteUByte(newHit->y_);
                            URHO3D_LOGWARNINGF("MatchGridInfo() - UpdateInput : NETMATCH_SELECT x=%u y=%u (size=%u)!", newHit->x_, newHit->y_, cmddata->params_.GetSize());
                        }
                    }
                    else
                    {
                        ResetSelection();
                        ResetState();

                        Net_PrepareCommand(NETMATCH_RESET);
                        Net_PrepareCommand(NETSTATE_RESET);
                    }
                }
                else
                {
                    GetSecondSelection(newHit);
                    if (newHit)
                    {
                        const Match& newhit = *newHit;
                        if (newhit.ctype_ < ROCKS && mgrid_.HaveNoAdjacentWalls(selected_[0], newhit))
                        {
                            if (newhit != selected_[0])
                            {
                                if (newhit.ctype_ >= ITEMS)
                                {
                                    AcquireItems(newhit);
                                    ResetSelection();
                                    canUnselect_ = false;

                                    NetCommandData* cmddata = Net_PrepareCommand(NETITEM_ACQUIRE);
                                    if (cmddata)
                                    {
                                        cmddata->params_.WriteUByte(newhit.x_);
                                        cmddata->params_.WriteUByte(newhit.y_);
                                    }
                                }
                                else if (IsMoveValidated(selected_[0], newhit))
                                {
                                    CheckHints(false);

            //                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : End Selection x=%d y=%d type=%d",
            //                                        newhit.x_, newhit.y_, newhit.ctype_);

                                    selected_[1] = newhit;
                                    MoveSelection();

                                    NetCommandData* cmddata = Net_PrepareCommand(NETMATCH_MOVE);
                                    if (cmddata)
                                    {
                                        cmddata->params_.WriteUByte(selected_[0].x_);
                                        cmddata->params_.WriteUByte(selected_[0].y_);
                                        cmddata->params_.WriteUByte(selected_[1].x_);
                                        cmddata->params_.WriteUByte(selected_[1].y_);
                                    }
                                }
                                else
                                {
                                    canUnselect_ = false;

                                    if (newhit.ctype_)
                                    {
                                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput : Start Selection with newhit x=%d y=%d type=%d", newhit.x_, newhit.y_, newhit.ctype_);
                                        ResetSelection();
                                        SelectStartMatch(newhit);

                                        NetCommandData* cmddata = Net_PrepareCommand(NETMATCH_SELECT);
                                        if (cmddata)
                                        {
                                            cmddata->params_.WriteUByte(newhit.x_);
                                            cmddata->params_.WriteUByte(newhit.y_);
                                            URHO3D_LOGWARNINGF("MatchGridInfo() - UpdateInput : NETMATCH_SELECT x=%u y=%u (size=%u)!", newhit.x_, newhit.y_, cmddata->params_.GetSize());
                                        }
                                    }
                                    else
                                    {
            //                            URHO3D_LOGINFOF("MatchGridInfo() - Update : UnSelect no valid move!");
                                        ResetSelection();
                                        ResetState();

                                        Net_PrepareCommand(NETMATCH_RESET);
                                        Net_PrepareCommand(NETSTATE_RESET);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Net_SendCommands();
}

void MatchGridInfo::UpdateControl_Boss()
{
    if (state_ == NoMatchState && animationTimer_.GetMSec(false) >= turnTime_)
    {
        if (GameStatics::input_->GetNumTouches() > 0 || GameStatics::input_->GetMouseButtonPress(MOUSEB_LEFT))
        {
            if (unblockinput_)
            {
                inputposition_ = GameStatics::input_->GetNumTouches() > 0 ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition();

//                if (GameStatics::ui_->GetElementAt(inputposition_.x_, inputposition_.y_))
//                    return;

                hitposition_ = GameHelpers::ScreenToWorld2D(inputposition_);
                Match* newHit;
                SelectMatch(newHit, hitposition_);
                if (newHit)
                {
                    mgrid_.AddTouchEffect(hitposition_);
                    const Match& hit = *newHit;

                    if (hit.ctype_ != NOTYPE)
                    {
                        if (hit.ctype_ < ROCKS)
                        {
                            if (hit.ctype_ >= ITEMS)
                            {
                                AcquireItems(hit);
                            }
                            else
                            {
                                if (abilitySelected_ != -1)
                                {
                                    if (CheckCurrentAbilityOn(hit))
                                    {
                                        ApplyCurrentAbilityOn(MatchesManager::Get()->GetMatchAtPosition(hitposition_));
                                        URHO3D_LOGWARNINGF("MatchGridInfo() - UpdateInput_Boss : Use Ability on Selection x=%d y=%d type=%d", hit.x_, hit.y_, hit.ctype_);
                                    }
                                }
                                else
                                {
                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : Start Selection x=%d y=%d type=%d", hit.x_, hit.y_, hit.ctype_);
                                    // debug trace
                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : %s %s", mgrid_.HasNoWalls(hit, WO_WALLRIGHT) ? "nowallright":"wallright", mgrid_.HasNoWalls(hit, WO_WALLLEFT)? "nowallleft":"wallleft");

                                    SelectStartMatch(hit);
                                    canUnselect_ = false;
                                }
                            }
                            unblockinput_ = false;
                        }
                    }
//                    else
//                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : Empty Match !");
                }
//                else
//                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : OutSide Grid !");
            }
        }
        else if (!unblockinput_)
        {
//            URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : unblockinput OK !");
            unblockinput_ = true;
        }
    }

    else if (state_ == StartSelection)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : StartSelection NumTouches=%u", GameStatics::input_->GetNumTouches());

        if (GameStatics::input_->GetNumTouches() > 0 || GameStatics::input_->GetMouseButtonDown(MOUSEB_LEFT))
        {
            Match* newHit;
            SelectMatch(newHit, GameStatics::input_->GetNumTouches() ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition());
            if (newHit)
            {
                int dx = Abs((int)newHit->x_ - (int)selected_[0].x_);
                int dy = Abs((int)newHit->y_ - (int)selected_[0].y_);
                if ((dx > 1 || dy > 1) || (dx == 1 && dy == 1))
                {
                    if (newHit->ctype_ && newHit->ctype_ < ROCKS)
                    {
                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : Break Selection with newhit x=%d y=%d type=%d", newHit->x_, newHit->y_, newHit->ctype_);

                        mgrid_.AddTouchEffect(hitposition_);

                        ResetSelection();
                        SelectStartMatch(*newHit);
                        inputposition_ = GameStatics::input_->GetNumTouches() ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition();
                    }
                    else
                    {
                        ResetSelection();
                        ResetState();
                    }
                }
                else
                {
                    GetSecondSelection(newHit);
                    if (!newHit)
                        return;

                    mgrid_.AddTouchEffect(hitposition_);

                    const Match& newhit = *newHit;

                    // allow to move on an empty tile
                    if (newhit.ctype_ < ROCKS && mgrid_.HaveNoAdjacentWalls(selected_[0], newhit))
                    {
                        if (newhit.ctype_ >= ITEMS)
                        {
                            AcquireItems(newhit);
                            ResetSelection();
                            canUnselect_ = false;
                        }
                        else if (newhit != selected_[0])
                        {
                            if (IsMoveValidated(selected_[0], newhit))
                            {
                                CheckHints(false);

//                                URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : End Selection x=%d y=%d type=%d",
//                                                 newhit.x_, newhit.y_, newhit.ctype_);

                                selected_[1] = newhit;
                                MoveSelection();
                            }
                            else
                            {
                                canUnselect_ = false;

                                if (newhit.ctype_)
                                {
//                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : Start Selection with newhit x=%d y=%d type=%d", newhit.x_, newhit.y_, newhit.ctype_);
                                    ResetSelection();
                                    SelectStartMatch(newhit);
                                }
                                else
                                {
//                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : UnSelect no valid move!");
                                    ResetSelection();
                                    ResetState();
                                }
                            }
                        }
                    }
                    else if (canUnselect_)
                    {
//                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Boss : UnSelect !");
                        ResetSelection();
                        ResetState();
                    }
                }
            }
        }
        else
            canUnselect_ = true;
    }
}

#ifdef ACTIVE_TESTMODE
void MatchGridInfo::UpdateControl_Test()
{
    if (GameStatics::input_->GetKeyPress(KEY_SPACE))
    {
        URHO3D_LOGINFOF("--------------------------------------------------------------------------------------------------------------------------");
        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : <Space> Pressed ...");
        testModeNextMove_ = true;
    }

    if (state_ == NoMatchState)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : NoMatchState NumTouches=%u MouseLeft=%u", GameStatics::input_->GetNumTouches(), GameStatics::input_->GetMouseButtonPress(MOUSEB_LEFT));

        if (GameStatics::input_->GetNumTouches() > 0 || GameStatics::input_->GetMouseButtonPress(MOUSEB_LEFT))
        {
            if (unblockinput_)
            {
                inputposition_ = GameStatics::input_->GetNumTouches() > 0 ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition();

//                URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : position=%s", inputposition_.ToString().CString());

//                if (GameStatics::ui_->GetElementAt(inputposition_.x_, inputposition_.y_))
//                {
//                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : UIElement find ! break");
//                    return;
//                }

                hitposition_ = GameHelpers::ScreenToWorld2D(inputposition_);
                Match* newHit;
                SelectMatch(newHit, hitposition_);
                if (newHit)
                {
                    mgrid_.AddTouchEffect(hitposition_);
                    const Match& hit = *newHit;

                    if (hit.ctype_ != NOTYPE)
                    {
                        if (hit.ctype_ < ROCKS)
                        {
                            if (hit.ctype_ >= ITEMS)
                            {
                                AcquireItems(hit);
                            }
                            else
                            {
                                if (abilitySelected_ != -1)
                                {
                                    if (CheckCurrentAbilityOn(hit))
                                    {
                                        ApplyCurrentAbilityOn(MatchesManager::Get()->GetMatchAtPosition(hitposition_));
                                        URHO3D_LOGWARNINGF("MatchGridInfo() - UpdateInput_Test : Use Ability on Selection x=%d y=%d type=%d", hit.x_, hit.y_, hit.ctype_);
                                    }
                                }
                                else
                                {
                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : Start Selection x=%d y=%d type=%d", hit.x_, hit.y_, hit.ctype_);
                                    // debug trace
                                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : %s %s", mgrid_.HasNoWalls(hit, WO_WALLRIGHT) ? "nowallright":"wallright", mgrid_.HasNoWalls(hit, WO_WALLLEFT)? "nowallleft":"wallleft");

                                    SelectStartMatch(hit);
                                    canUnselect_ = false;
                                }
                            }
                            unblockinput_ = false;
                        }
                    }
//                    else
//                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : Empty Match !");
                }
//                else
//                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : OutSide Grid !");
            }
        }
        else if (!unblockinput_)
        {
//            URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : unblockinput OK !");
            unblockinput_ = true;
        }
    }

    else if (state_ == StartSelection)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : StartSelection NumTouches=%u", GameStatics::input_->GetNumTouches());

        if (GameStatics::input_->GetNumTouches() > 0 || GameStatics::input_->GetMouseButtonDown(MOUSEB_LEFT))
        {
            Match* newHit;
            SelectMatch(newHit, GameStatics::input_->GetNumTouches() ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition());
            if (newHit)
            {
                int dx = Abs((int)newHit->x_ - (int)selected_[0].x_);
                int dy = Abs((int)newHit->y_ - (int)selected_[0].y_);
                if ((dx > 1 || dy > 1) || (dx == 1 && dy == 1))
                {
                    if (newHit->ctype_ && newHit->ctype_ < ROCKS)
                    {
                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : Break Selection with newhit x=%d y=%d type=%d", newHit->x_, newHit->y_, newHit->ctype_);

                        mgrid_.AddTouchEffect(hitposition_);

                        ResetSelection();
                        SelectStartMatch(*newHit);
                        inputposition_ = GameStatics::input_->GetNumTouches() ? GameStatics::input_->GetTouch(0)->position_ : GameStatics::input_->GetMousePosition();
                    }
                    else
                    {
                        ResetSelection();
                        ResetState();
                    }
                }
                else
                {
                    GetSecondSelection(newHit);
                    if (newHit)
                    {
                        const Match& newhit = *newHit;
                        if (newhit.ctype_ < ROCKS && mgrid_.HaveNoAdjacentWalls(selected_[0], newhit))
                        {
                            if (newhit != selected_[0])
                            {
                                if (newhit.ctype_ >= ITEMS)
                                {
                                    AcquireItems(newhit);
                                    ResetSelection();
                                    canUnselect_ = false;
                                }
                                else if (IsMoveValidated(selected_[0], newhit))
                                {
                                    CheckHints(false);

            //                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : End Selection x=%d y=%d type=%d",
            //                                        newhit.x_, newhit.y_, newhit.ctype_);

                                    selected_[1] = newhit;
                                    MoveSelection();
                                }
                                else
                                {
                                    canUnselect_ = false;

                                    if (newhit.ctype_)
                                    {
                                        URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : Start Selection with newhit x=%d y=%d type=%d", newhit.x_, newhit.y_, newhit.ctype_);
                                        ResetSelection();
                                        SelectStartMatch(newhit);

                                    }
                                    else
                                    {
            //                            URHO3D_LOGINFOF("MatchGridInfo() - UpdateInput_Test : UnSelect no valid move!");
                                        ResetSelection();
                                        ResetState();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif


void MatchGridInfo::Update_Initial()
{
    if (state_ == SuccessMatch)
    {
        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Initial : state_=SuccessMatch turn=%d/%d ...",  turnProcessed_, successTurns_);

        if (successtoProcess_)
        {
            ApplySuccessMatches();
            turnProcessed_++;
        }
        else
        {
            if (FindMatches(matchesToCheck_))
            {
                successTurns_++;
            }
            else if (turnProcessed_ == successTurns_)
            {
                state_ = NoMatchState;
            }
        }
    }

    if (state_ == NoMatchState)
    {
        allowCheckObjectives_ = true;
        GameStatics::playerState_->tutorialEnabled_ = tutorialStateSaved_;

        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Initial : finish !");
    }
}

void MatchGridInfo::Update()
{
    if (state_ == CancelMove)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic : state_=CancelMove timer=%u ...", animationTimer_.GetMSec(false) );

//        if (animationTimer_ <= 0.f)
        if (animationTimer_.GetMSec(false) >= turnTime_)
        {
            ResetSelection();
            ResetState();
        }
    }
    else if (state_ == SelectionAnimation)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic : state_=SelectionAnimation timer=%u/%f ...", animationTimer_.GetMSec(false), turnTime_);

//        if (animationTimer_ <= 0.f)
        if (animationTimer_.GetMSec(false) >= turnTime_)
        {
//            URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic : state_=SelectionAnimation timer=%u/%f ... Confirm ...", animationTimer_.GetMSec(false) , turnTime_);
            ConfirmSelection();
        }
    }
    else if (state_ == SuccessMatch)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic : state_=SuccessMatch timer=%u turn=%d/%d ...", animationTimer_.GetMSec(false) , turnProcessed_, successTurns_);

        if (successtoProcess_)
        {
            // wait pausetimer between turns
            if (pauseTimer_.GetMSec(false) >= PAUSETIME*1000)
            {
                ApplySuccessMatches();
                pauseTimer_.Reset();
                turnProcessed_++;
            }
        }
//        else if (animationTimer_ <= 0.f)
        else if (animationTimer_.GetMSec(false) >= turnTime_)
        {
            if (FindMatches(matchesToCheck_))
            {
                successTurns_++;

                animationTimer_.Reset();
                pauseTimer_.Reset();
                turnTime_ = MOVETIME*1000;
            }
            else if (turnProcessed_ == successTurns_)
            {
                ResetState();
                allowCheckObjectives_ = true;
            }
        }
    }

    if (state_ != SuccessMatch && GameStatics::allowInputs_)
        UpdateHints();
}

void MatchGridInfo::Update_Boss()
{
    if (state_ == CancelMove)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Boss : state_=CancelMove timer=%f ...", animationTimer_);

        if (animationTimer_.GetMSec(false) >= turnTime_)
        {
            ResetSelection();
            ResetState();
        }
    }
    else if (state_ == SelectionAnimation)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Boss : state_=SelectionAnimation timer=%f ...", animationTimer_);

        if (animationTimer_.GetMSec(false) >= turnTime_)
            ConfirmSelection_Boss();
    }
    else if (state_ == SuccessMatch)
    {
//        URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Boss : state_=SuccessMatch timer=%f turn=%d/%d ...", animationTimer_, turnProcessed_, successTurns_);

//        if (successtoProcess_)
//        {
//            ApplySuccessMatches();
//
//            turnProcessed_++;
//        }
        if (successtoProcess_)
        {
            // wait pausetimer between turns
            if (pauseTimer_.GetMSec(false) >= PAUSETIME*1000)
            {
                ApplySuccessMatches();
                pauseTimer_.Reset();
                turnProcessed_++;
            }
        }
        else if (animationTimer_.GetMSec(false) >= turnTime_)
        {
            if (FindMatches(matchesToCheck_))
            {
                successTurns_++;

                animationTimer_.Reset();
                pauseTimer_.Reset();
                turnTime_ = MOVETIME*1000;
            }
            else if (turnProcessed_ == successTurns_)
            {
                ResetState();
                allowCheckObjectives_ = true;
            }
        }
    }

    if (state_ != SuccessMatch && GameStatics::allowInputs_)
        UpdateHints();
}

#ifdef ACTIVE_TESTMODE
void MatchGridInfo::Update_Test()
{
    if (state_ == CancelMove)
    {
        if (animationTimer_.GetMSec(false) >= turnTime_)
        {
            URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Test : state_=CancelMove timer=%F ...", animationTimer_.GetMSec(false));
            ResetSelection();
            ResetState();
        }
    }
    else if (state_ == SelectionAnimation)
    {
        if (animationTimer_.GetMSec(false) >= turnTime_)
        {
            URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Test : state_=SelectionAnimation timer=%F ...", animationTimer_.GetMSec(false));
            ConfirmSelection();

            if (successTurns_ > turnProcessed_)
            {
                URHO3D_LOGINFOF("==========================================================================================================================");
                URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Test : state_=SelectionAnimation NumSuccess=%d turn=%d/%d ... Press <Space> To Apply Success Turn", successmatches_.Size(), turnProcessed_, successTurns_);
            }
        }
    }
    else if (state_ == SuccessMatch)
    {
        if (successtoProcess_)
        {
            if (testModeNextMove_)
            {
                // wait pausetimer between turns
                if (pauseTimer_.GetMSec(false) >= PAUSETIME*1000)
                {
                    testModeNextMove_ = false;
                    turnProcessed_++;

                    URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Test : state_=SuccessMatch ApplySuccess NumSuccess=%d turn=%d/%d ...", successmatches_.Size(), turnProcessed_, successTurns_);
                    ApplySuccessMatches();
                    pauseTimer_.Reset();
                }
            }
        }
        else if (animationTimer_.GetMSec(false) >= turnTime_)
        {
            if (FindMatches(matchesToCheck_))
            {
                successTurns_++;
                URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Test : state_=SuccessMatch FindMatches NumSuccess=%d turn=%d/%d ... Press <Space> To Next Success Turn", successmatches_.Size(), turnProcessed_, successTurns_);

                animationTimer_.Reset();
                pauseTimer_.Reset();
                turnTime_ = MOVETIME*1000;
            }
            else if (turnProcessed_ == successTurns_)
            {
                URHO3D_LOGINFOF("MatchGridInfo() - UpdateLogic_Test : state_=SuccessMatch ResetState turn=%d/%d ... Success Turns Finished", turnProcessed_, successTurns_);
                URHO3D_LOGINFOF("==========================================================================================================================");
                ResetState();
                allowCheckObjectives_ = true;
            }
        }
    }

    if (state_ != SuccessMatch && GameStatics::allowInputs_)
        UpdateHints();
}
#endif

void MatchGridInfo::UpdateItems()
{
    if (updateitemsTimer_.GetMSec(false) < UPDATERAMPSDELAY || animationTimer_.GetMSec(false) < turnTime_ || state_ == SelectionAnimation || state_ == SuccessMatch || state_ == CancelMove)
        return;

    ResetScaleOnSelection();
    ChangeState(NoMatchState);

    // Update Ramp Tiles
    mgrid_.UpdateDirectionalTiles();

    // Collapse fixed columns
    {
        Vector<Match*> collapsedMatches;
        int maxdistance = 1;
        for (Vector<unsigned char>::ConstIterator xt=mgrid_.fixedcolumns_.Begin();xt!=mgrid_.fixedcolumns_.End();++xt)
        {
            int distance = mgrid_.CollapseColumn(*xt, collapsedMatches);
            if (distance > maxdistance)
                maxdistance = distance;
        }

        turnTime_ = Min(Max((float)maxdistance, 1.f) * MOVETIME, MAXMOVETIME)*1000;
    }

    updateitemsTimer_.Reset();
    animationTimer_.Reset();
//    URHO3D_LOGINFOF("MatchGridInfo() - UpdateItems : running !");
}


#define HINTSEARCHDELAY 6U   // milliseconds for search pass

void MatchGridInfo::UpdateHints()
{
//    if (!MatchesManager::allowHints_ || !hintsenabled_)
//        return;

    if (!hintsenabled_)
        return;

    // Update Search
    if (ihintsearchmatch_ < mgrid_.size_)
    {
        if (!ihintsearchmatch_)
        {
            hints_.Clear();
        }

        hintsearchTimer_.SetExpirationTime(HINTSEARCHDELAY);

        int i=0;
        for (i = ihintsearchmatch_; i < mgrid_.size_; i++)
        {
            mgrid_.GetHints(i, hints_);

            if (hintsearchTimer_.Expired())
            {
                ihintsearchmatch_ = i+1;
                break;
            }
        }

        if (i == mgrid_.size_)
            ihintsearchmatch_ = mgrid_.size_;

        URHO3D_LOGINFOF("MatchGridInfo() - UpdateHints : gridid=%d - find %u hints (%d/%u-%s) !", mgrid_.gridid_,
                        hints_.Size(), i, mgrid_.size_, ihintsearchmatch_ == mgrid_.size_ ? "finished" : "running");
    }

    // No Results, Shake the world !
    if (!hints_.Size() && ihintsearchmatch_ >= mgrid_.size_)
    {
        // If has tiles entrances (ex : boss01) we don't have to shake if no hints
        if (mgrid_.HasTileEntrances())
            return;

        URHO3D_LOGINFOF("MatchGridInfo() - UpdateHints : gridid=%d - No More Match !", mgrid_.gridid_);

        hintsenabled_ = false;

        if (!allowCheckObjectives_)
        {
            // no first move : redistribute silently
            MatchesManager::ShakeMatches(mgrid_.gridid_);
        }
        else if (!mgrid_.ContainsItems())
        {
            Localization* l10n = GameStatics::context_->GetSubsystem<Localization>();
            GameHelpers::AddUIMessage(l10n->Get("nomatches"), false, "Fonts/BlueHighway.ttf", 30, Color::WHITE,
                                      IntVector2(0, MESSAGEPOSITIONYRATIO * GameStatics::ui_->GetRoot()->GetSize().y_), 1.2f);

            MatchesManager::CheckAllowShake();
        }

        return;
    }

    if (hints_.Size() && shakebuttonFrame && shakebuttonFrame->IsStarted())
    {
        shakebuttonFrame->Stop();
        shakebuttonFrame.Reset();
    }

    // Switch Animation
    if (MatchesManager::allowHints_)
    {
        if (hintstate_ == HintNoMove)
        {
            if (hintTimer_.GetMSec(false) >= HINTTIME*1000)
            {
                if (ishowhints_ != -1)
                    SetHintsAnimations(false);

                ilasthint_ = ishowhints_;
                ishowhints_ = -1;

                hintstate_ = HintAnimate;
                hintTimer_.Reset();
            }
        }
        else if (hintstate_ == HintAnimate)
        {
            if (hintTimer_.GetMSec(false) >= HINTSWITCH*1000)
            {
                if (hints_.Size())
                {
                    ishowhints_ = (ilasthint_+1) % hints_.Size();
                    SetHintsAnimations(true);
                }

                hintstate_ = HintNoMove;
                hintTimer_.Reset();
            }
        }
    }
}



