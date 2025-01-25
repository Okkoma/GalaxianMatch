#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
//#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Menu.h>

#include "Game.h"
#include "GameOptions.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameUI.h"
#include "GameRand.h"

#include "InteractiveFrame.h"

#include "sPlay.h"
#include "sCinematic.h"
#include "sOptions.h"

#include "DelayAction.h"
#include "DelayInformer.h"
#include "TimerRemover.h"

#include "MAN_Matches.h"

#include "LevelGraph.h"
#include "sLevelMap.h"

extern int UISIZE[NUMUIELEMENTSIZE];
extern bool drawDebug_;
extern const char* levelModeNames[];


const String LABEL_MISSION("M");
const String LABEL_LINK("L");
const String LABEL_CINEMATIC("C");
const String LABEL_ZONE("Zone_");
const String LABEL_ZONE_PREVIOUS("Zone_previous");
const String LABEL_ZONE_NEXT("Zone_next");

const float MISSIONLINK_BLINKDURATION = 5.f;
const float MISSIONPLANET_ZOOM = 8.f;
const float MISSIONLINK_PLANETMODESCALE = 0.25f;

const int MAXPLANETOBJECTS = 12;
const Vector2 PLANETOBJECT_SCALEDEFAULT(0.05f, 0.05f);
const Vector2 PLANETOBJECT_SCALESELECTED(0.15f, 0.15f);
const float SWITCHPLANETMODETIME = 1.5f;

WeakPtr<Node> mapscene_;
WeakPtr<Node> selector_;
WeakPtr<Text3D> leveltxt_;
Vector<Node*> selectablenodes_;
Vector<Node*> selectableplanets_;
WeakPtr<Node> uilevelmapscene_;
WeakPtr<UIElement> uilevelmap_;
WeakPtr<AnimatedSprite2D> prevZoneButton_;
WeakPtr<AnimatedSprite2D> nextZoneButton_;
SharedPtr<UIDialog> dialogbox_;
WeakPtr<LevelGraph> levelGraph_;

DelayInformer* switchPlanetModeInformer_ = 0;

enum
{
    SMOOTHCAMERA_NONE = 0,
    SMOOTHCAMERA_POSITION,
    SMOOTHCAMERA_ZOOM
};

enum
{
    LEVELMAPACTION_STARTLEVEL = 0,
    LEVELMAPACTION_GOTOPREVIOUSZONE,
    LEVELMAPACTION_GOTONEXTZONE,
    LEVELMAPACTION_CHANGESELECTEDLEVEL,
    LEVELMAPACTION_CINEMATICREPLAY
};

const char* LevelMapActionStr[] =
{
    "LEVELMAPACTION_STARTLEVEL",
    "LEVELMAPACTION_GOTOPREVIOUSZONE",
    "LEVELMAPACTION_GOTONEXTZONE",
    "LEVELMAPACTION_CHANGESELECTEDLEVEL",
    "LEVELMAPACTION_CINEMATICREPLAY"
};

const Vector2 ZoneButtonColliderCenter(0.f, 0.2f);
const float ZoneButtonColliderRadius = 0.3f;
const Vector2 CinematicButtonColliderCenter = Vector2::ZERO;
const float CinematicButtonColliderRadius = 0.7f;

LevelMapState::LevelMapState(Context* context) :
    GameState(context, "LevelMap")
{
    drawDebug_ = 0;
//    URHO3D_LOGINFO("LevelMapState()");
}

LevelMapState::~LevelMapState()
{
#ifdef ACTIVE_SPLASHUI
    SendEvent(SPLASHSCREEN_STOP);
#endif

    URHO3D_LOGINFO("~LevelMapState()");
}


bool LevelMapState::Initialize()
{
    return GameState::Initialize();
}

void LevelMapState::Begin()
{
    if (IsBegun())
        return;

    GameStatics::SetStatus(LEVELSTATE);

    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
    URHO3D_LOGINFO("LevelMapState() - Begin ...                              -");
    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");

    // Set Scene
    if (!ResetMap())
    {
        URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
        URHO3D_LOGINFO("LevelMapState() - Begin ...  NOK !                       -");
        URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
        GameState::begun_ = false;
        return;
    }

    GameState::Begin();

    // Entrance Animation
    GameHelpers::SetMoveAnimation(mapscene_, Vector3(10.f * stateManager_->GetStackMove(), 0.f, 0.f), Vector3::ZERO, 0.f, SWITCHSCREENTIME);
    GameHelpers::SetMoveAnimationUI(uilevelmap_, IntVector2(uilevelmap_->GetSize().x_ * stateManager_->GetStackMove(), 0), IntVector2(0, 0), 0.f, SWITCHSCREENTIME);

    GameStatics::AllowInputs(true);

    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
    URHO3D_LOGINFO("LevelMapState() - Begin ...  OK !                        -");
    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
}

bool LevelMapState::HasCinematicAvailable(int levelid) const
{
    const int zone = GameStatics::playerState_->zone;
    const int bosslevel = GameStatics::GetBossLevelId(zone);

    if (levelid != firstMissionID_ && levelid != bosslevel)
        return false;

    const int currentCinematicPartShown = GameStatics::playerState_->cinematicShown_[zone-1];

    if (levelid == bosslevel)
    {
        if (currentCinematicPartShown >= CINEMATIC_BOSSINTRO && GameStatics::playerState_->missionstates[levelid-1].state_ >= GameStatics::MissionState::MISSION_UNLOCKED)
        {
            String filename;
            filename.AppendWithFormat("Data/Cinematic/constellation%d-boss.xml", zone);
            if (context_->GetSubsystem<FileSystem>()->FileExists(GameStatics::gameConfig_.appDir_ + filename))
            {
                URHO3D_LOGINFOF("LevelMapState() - HasCinematicAvailable : bosslvl=%d ok !", levelid);
                return true;
            }
        }
        if (currentCinematicPartShown >= CINEMATIC_BOSSDEFEAT && GameStatics::playerState_->missionstates[levelid-1].state_ == GameStatics::MissionState::MISSION_COMPLETED)
        {
            String filename;
            filename.AppendWithFormat("Data/Cinematic/constellation%d-outro.xml", zone);
            if (context_->GetSubsystem<FileSystem>()->FileExists(GameStatics::gameConfig_.appDir_ + filename))
            {
                URHO3D_LOGINFOF("LevelMapState() - HasCinematicAvailable : bosslvl=%d outro ok !", levelid);
                return true;
            }
        }
    }

    if (levelid == firstMissionID_ && currentCinematicPartShown >= CINEMATIC_INTRO)
    {
        String filename;
        filename.AppendWithFormat("Data/Cinematic/constellation%d-intro.xml", zone);
        if (context_->GetSubsystem<FileSystem>()->FileExists(GameStatics::gameConfig_.appDir_ + filename))
        {
            URHO3D_LOGINFOF("LevelMapState() - HasCinematicAvailable : lvl=%d intro ok !", levelid);
            return true;
        }
    }

    return false;
}

bool LevelMapState::ResetMap(bool purge)
{
    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
    URHO3D_LOGINFOF("LevelMapState() - ResetMap ... purge=%s       -", purge?"true":"false");
    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");

    if (switchPlanetModeInformer_ && !switchPlanetModeInformer_->Expired())
        switchPlanetModeInformer_->Free();

    UnsubscribeFromEvent(GAME_SWITCHPLANETMODE);

    if (purge)
    {
        cameraSmooth_ = SMOOTHCAMERA_NONE;
        planetMode_ = 0;
        lastaction_ = -2;
        lastSelectedLevelID_ = -1;

        levelGraph_.Reset();

        if (mapscene_ && mapscene_->GetName() == "CurrentLevelMap")
            mapscene_->RemoveAllChildren();

        UnsubscribeToEvents();
        ResetUI();

        GameHelpers::ResetCamera();
    }

    // Update Default Components for the Scene
    GameStatics::rootScene_->GetOrCreateComponent<Octree>(LOCAL);
    if (GameStatics::gameConfig_.debugRenderEnabled_)
        GameStatics::rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);

    // Set Scene
    if (!CreateScene(purge))
    {
        URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
        URHO3D_LOGINFO("LevelMapState() - ResetMap ... NOK !                     -");
        URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");

        ReturnMainMenu();
        return false;
    }

    GameStatics::SetMouseVisible(true, false);

    Game::Get()->ShowHeader(uilevelmap_);

    if (purge)
    {
        SubscribeToEvents();

        SendEvent(GAME_STATEREADY);
    }

    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
    URHO3D_LOGINFO("LevelMapState() - ResetMap ... OK !                      -");
    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");

#ifdef ACTIVE_CINEMATICS
    CinematicState::SetCinematic(CINEMATICSELECTIONMODE_INTRO_OUTRO, firstMissionID_, selectedLevelID_);
#endif

    return true;
}

void LevelMapState::End()
{
    if (!IsBegun())
        return;

    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
    URHO3D_LOGINFO("LevelMapState() - End ...                                -");
    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");

    UIElement* menubutton = Game::Get()->GetAccessMenu();
    if (menubutton)
        menubutton->SetVisible(false);

    if (switchPlanetModeInformer_ && !switchPlanetModeInformer_->Expired())
        switchPlanetModeInformer_->Free();

#ifdef ACTIVE_MISSIONDIALOG
    if (dialogbox_)
    {
        dialogbox_->Stop();
        dialogbox_.Reset();
    }
#endif

    if (!GameStatics::gameExit_)
    {
        // Go Out Animation Only if Return to Menu (PopStack)
        if (stateManager_->GetStackMove() == -1)
        {
            if (mapscene_)
                GameHelpers::SetMoveAnimation(mapscene_, Vector3::ZERO, Vector3(10.f, 0.f, 0.f), 0.f, SWITCHSCREENTIME);
            if (uilevelmap_)
                GameHelpers::SetMoveAnimationUI(uilevelmap_, IntVector2::ZERO, IntVector2(GameStatics::graphics_->GetWidth()/2, 0), 0.f, SWITCHSCREENTIME);
        }
        else
        {
        #ifndef ACTIVE_SPLASHUI
            GameHelpers::SetMoveAnimation(mapscene_, Vector3::ZERO, Vector3(-10.f, 0.f, 0.f), 0.f, SWITCHSCREENTIME);
            GameHelpers::SetMoveAnimationUI(uilevelmap_, IntVector2::ZERO, IntVector2(-GameStatics::graphics_->GetWidth()/2, 0), 0.f, SWITCHSCREENTIME);
        #else
            SetVisibleUI(false);
        #endif
            SendEvent(GAME_LEVELTOLOAD);
        }
    }

    if (mapscene_)
        mapscene_->SetName("LevelMap");

    UnsubscribeToEvents();

    RemoveUI();

    GameHelpers::CleanScene(GameStatics::rootScene_, GetStateId(), 0);

    // Call base class implementation
    GameState::End();

    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
    URHO3D_LOGINFO("LevelMapState() - End ... OK !                           -");
    URHO3D_LOGINFO("LevelMapState() - ----------------------------------------");
}

bool LevelMapState::GenerateLevelMapScene(int zone)
{
    URHO3D_LOGINFOF("LevelMapState() - GenerateLevelMapScene ... currentlevel=%d currentzone=%d ...", GameStatics::currentLevel_, zone);

    const int numbranches = 3;

    /// TODO



    return false;
}

void LevelMapState::UpdateStatics()
{
    URHO3D_LOGINFOF("LevelMapState() - UpdateStatics...");

    GameStatics::currentZoneFirstMissionId_ = firstMissionID_;
    GameStatics::currentZoneLastMissionId_ = lastMissionID_;

    const Vector<LevelGraphPoint* >& points = levelGraph_->GetOrderedPoints();

    for (unsigned i=0; i < points.Size(); i++)
    {
        const LevelGraphPoint& point = *points[i];
        const Vector<LevelGraphPoint* >& linkedpoints = point.linkedpoints_;

        // Update linkedmissions
        int maxlinks = Min(4, linkedpoints.Size());
        for (int j=0; j < maxlinks; j++)
            GameStatics::playerState_->missionstates[point.id_-1].linkedmissions_[j] = linkedpoints[j]->id_;

        // Update gains
        if (!point.gains_.Empty())
        {
            Vector<String> gainsStr = point.gains_.Split(' ');
            for (int j=0; j < gainsStr.Size(); j++)
            {
                Vector<String> gainStr = gainsStr[j].Split('-');
                GameStatics::playerState_->missionstates[point.id_-1].bonuses_[j] = Slot(StringHash(gainStr[0]), ToInt(gainStr[1])-1, gainStr.Size() > 2 ? ToInt(gainStr[2]) : 1, true);
            }
        }
    }

//    GameStatics::gameState_.Dump();

    URHO3D_LOGINFOF("LevelMapState() - UpdateStatics... OK !");
}

const unsigned DELAY_PEERUPDATE = 5000U;
Timer peerUpdateTimer_;
bool receivedPeerUpdate_;
bool receivedPeerOffers_[NBMAXLVL];

void LevelMapState::UpdatePeerOffers()
{
    if (receivedPeerUpdate_)
    {
        // Update the peer States animations for each mission in the current constellation
        const Vector<LevelGraphPoint* >& missionpoints = levelGraph_->GetOrderedPoints();
        for (unsigned i = 0; i < missionpoints.Size(); i++)
        {
            AnimatedSprite2D* peerstateAnim = static_cast<AnimatedSprite2D*>(selectablenodes_[i]->GetComponents()[1].Get());
            if (peerstateAnim)
                peerstateAnim->SetEnabled(receivedPeerOffers_[firstMissionID_+i]);
        }

        receivedPeerUpdate_ = false;
    }

    if (peerUpdateTimer_.GetMSec(false) >= DELAY_PEERUPDATE)
    {
        peerUpdateTimer_.Reset();

        ReceivePeerOffers();
    }
}

void LevelMapState::ReceivePeerOffers()
{
    // TODO

    // Test to remove - Simulate a net received
    for (unsigned i=0; i < NBMAXLVL; i++)
    {
        receivedPeerOffers_[i] = random() % 2;
    }

    receivedPeerUpdate_ = true;
}

void LevelMapState::SetMissionNodes(Node* root)
{
    Node* missionDefaultNode = root->GetChild("Mission_Default");
    GameStatics::MissionState* mstates = GameStatics::playerState_->missionstates;

    const Vector<LevelGraphPoint* >& points = levelGraph_->GetOrderedPoints();

    URHO3D_LOGINFOF("LevelMapState() - SetMissionNodes ... numpoints=%u firstMissionID_=%d ...", points.Size(), firstMissionID_);

    selectablenodes_.Clear();

    int nextMissionToUnlock = -1;

    for (unsigned i=0; i < points.Size(); i++)
    {
        // Create Mission Nodes
        Node* node = missionDefaultNode->Clone(LOCAL, true, 0, 0, root);
        node->SetName(points[i]->name_);
        node->SetPosition2D(points[i]->position_);

        unsigned missionid = points[i]->id_;
        String missionstate("unlocked");

        const GameStatics::MissionState& mstate = mstates[missionid-1];
        if (mstate.state_ == GameStatics::MissionState::MISSION_COMPLETED)
            missionstate = "completed";
        else if (mstate.state_ == GameStatics::MissionState::MISSION_LOCKED)
        {
            missionstate = "locked";
        }
        else if (nextMissionToUnlock == -1)
        {
            nextMissionToUnlock = missionid;
        }
        const Vector<SharedPtr<Component> >& nodeComponents = node->GetComponents();
        if (nodeComponents.Size() > 0)
        {
            AnimatedSprite2D* missionstateAnim = static_cast<AnimatedSprite2D*>(node->GetComponents()[0].Get());
            if (missionstateAnim)
            {
                missionstateAnim->SetAnimation(missionstate);
                missionstateAnim->SetLayer(2);
                missionstateAnim->SetEnabled(true);
            }
        }
        else
            URHO3D_LOGERRORF("LevelMapState() - SetMissionNodes ... can't set missionstateAnim !");

        if (nodeComponents.Size() > 1)
        {
            AnimatedSprite2D* peerstateAnim = static_cast<AnimatedSprite2D*>(node->GetComponents()[1].Get());
            if (peerstateAnim)
            {
                peerstateAnim->SetAnimation("on");
                peerstateAnim->SetLayer(3);
                peerstateAnim->SetEnabled(false);
            }
        }
        else
            URHO3D_LOGERRORF("LevelMapState() - SetMissionNodes ... can't set peerstateAnim !");

        node->SetEnabled(true);

        // Set Physics for selection
        node->CreateComponent<RigidBody2D>();
        CollisionCircle2D* collider = node->CreateComponent<CollisionCircle2D>();
        collider->SetRadius(0.5f);

        selectablenodes_.Push(node);
    }

    if (nextMissionToUnlock == -1)
        nextMissionToUnlock = lastMissionID_;

    // Set Selector
    if (!root->GetChild("Selector"))
    {
        selector_ = missionDefaultNode->Clone(LOCAL, true, 0, 0, root);

        AnimatedSprite2D* animatedsprite = selector_->GetComponent<AnimatedSprite2D>();
        animatedsprite->SetLayer(1);
        animatedsprite->SetAnimation("selected");
        animatedsprite->SetEnabled(true);
    }

    if (selectedLevelID_ < firstMissionID_ || selectedLevelID_ > lastMissionID_)
        selectedLevelID_ = nextMissionToUnlock;

    selector_->SetPosition2D(selectablenodes_[selectedLevelID_-firstMissionID_]->GetPosition2D());
    selector_->SetEnabledRecursive(true);

    URHO3D_LOGINFOF("LevelMapState() - SetMissionNodes ... numpoints=%u ... OK !", points.Size());
}

void LevelMapState::SetLinks()
{
    Node* levelscene = mapscene_->GetChild("LevelScene");
    Node* root = levelscene->GetChild("Links") ? levelscene->GetChild("Links") : levelscene->CreateChild("Links", LOCAL);

    const Vector<LevelGraphPoint* >& points = levelGraph_->GetOrderedPoints();
    GameStatics::MissionState* mstates = GameStatics::playerState_->missionstates;

    Sprite2D* sprite = Sprite2D::LoadFromResourceRef(root->GetContext(), ResourceRef(Sprite2D::GetTypeStatic(), "UI/LevelMap/missionLink.png"));
    sprite->SetTextureEdgeOffset(2.f);
    Rect spriterect;
    sprite->GetDrawRectangle(spriterect);
    float spritelength = spriterect.Size().x_;

    for (unsigned i = 0; i < selectablenodes_.Size(); i++)
    {
        Node* node1 = selectablenodes_[i];

        unsigned missionid1 = points[i]->id_;
        Vector<LevelGraphPoint* >& linkedpoints = points[i]->linkedpoints_;

        for (int j = 0; j < linkedpoints.Size(); j++)
        {
            unsigned missionid2 = linkedpoints[j]->id_;
            if (missionid1 > missionid2)
                continue;

            Node* node2 = selectablenodes_[missionid2-firstMissionID_];

            Vector3 position = (node1->GetPosition() + node2->GetPosition()) * 0.5f;
            Vector3 distance = node2->GetPosition() - node1->GetPosition();
            float angle = M_RADTODEG * atan(distance.y_ / distance.x_);
            float scalex = distance.Length() / spritelength;

            Node* linknode = root->CreateChild("link", LOCAL);
            linknode->SetPosition(position);
            linknode->SetScale(Vector3(scalex, 1.f, 1.f));
            linknode->SetRotation2D(angle);

            StaticSprite2D* staticsprite = linknode->CreateComponent<StaticSprite2D>();
            staticsprite->SetSprite(sprite);
            staticsprite->SetLayer(0);

//            URHO3D_LOGINFOF("LevelMapState() - SetMissionNodes ... Add Link Mission%u->Mission%u!", missionid1, missionid2);

            if (mstates[missionid2-1].state_ == GameStatics::MissionState::MISSION_LOCKED)
            {
                staticsprite->SetAlpha(0.1f);
            }
            else
            {
                SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(staticsprite->GetContext()));
                SharedPtr<ValueAnimation> opacityAnimation(new ValueAnimation(staticsprite->GetContext()));
                opacityAnimation->SetKeyFrame(0.f, 0.f);
                opacityAnimation->SetKeyFrame(MISSIONLINK_BLINKDURATION * 0.25f, 0.2f);
                opacityAnimation->SetKeyFrame(MISSIONLINK_BLINKDURATION * 0.75f, 0.8f);
                opacityAnimation->SetKeyFrame(MISSIONLINK_BLINKDURATION, 0.f);
                objectAnimation->AddAttributeAnimation("Alpha", opacityAnimation, WM_LOOP);
                staticsprite->SetObjectAnimation(objectAnimation);
            }
        }
    }
}

void LevelMapState::UpdatLinkScaleY(float scaley)
{
    Node* root = mapscene_->GetChild("LevelScene")->GetChild("Links");
    if (!root)
        return;

    const Vector<SharedPtr<Node> >& links = root->GetChildren();
    for (unsigned i=0; i < links.Size(); i++)
    {
        Node* linknode = links[i];
        linknode->SetScale(Vector3(linknode->GetScale().x_, scaley, 1.f));
    }
}


void LevelMapState::SetPlanetNodes(Node* root)
{
    Node* defaultNode = root->GetChild("Planet_Default");

    const Vector<LevelGraphPoint* >& points = levelGraph_->GetOrderedPoints();
    GameStatics::MissionState* mstates = GameStatics::playerState_->missionstates;

    URHO3D_LOGINFOF("LevelMapState() - SetPlanetNodes ... numpoints=%u firstMissionID_=%d ...", points.Size(), firstMissionID_);

    selectableplanets_.Clear();

    // Create Planets nodes
    if (points.Size())
    {
        GameRand& rand = GameRand::GetRandomizer(OBJRAND);
        rand.SetSeed(points.Front()->id_);
        const Vector2 scale(0.15f, 0.15f);

        for (unsigned i = 0; i < points.Size(); i++)
        {
            const LevelGraphPoint* point = points[i];
            // Create Planets Node
            Node* node = defaultNode->Clone(LOCAL, true, 0, 0, root);
            node->SetName(point->name_);
            node->SetScale2D(scale);
            node->SetPosition2D(point->position_);

            unsigned missionid = point->id_;
            String anim("unlocked");

            const GameStatics::MissionState& mstate = mstates[missionid-1];
            if (mstate.state_ == GameStatics::MissionState::MISSION_COMPLETED)
                anim = "completed";
            else if (mstate.state_ == GameStatics::MissionState::MISSION_LOCKED)
                anim = "locked";

            AnimatedSprite2D* animatedsprite = node->GetComponent<AnimatedSprite2D>();
            if (animatedsprite)
            {
                String entity;
                int planetid = rand.Get(MAXPLANETOBJECTS);
                entity.AppendWithFormat("Planet%d", planetid);
                animatedsprite->SetEntity(entity);
                animatedsprite->SetAnimation(anim);
                animatedsprite->SetLayer(2);
                animatedsprite->SetEnabled(true);
            }

            node->SetEnabled(true);

            if (mstate.state_ > GameStatics::MissionState::MISSION_LOCKED)
            {
                // Set Physics for selection
                node->CreateComponent<RigidBody2D>();
                CollisionCircle2D* collider = node->CreateComponent<CollisionCircle2D>();
                collider->SetRadius(0.5f);
            }

            selectableplanets_.Push(node);
        }
    }

    // Create PlanetInfo node
    Node* planetInfo = root->CreateChild("PlanetInfo", LOCAL);

    // Create LevelText
    Node* node = planetInfo->CreateChild("LevelText", LOCAL);
    leveltxt_ = node->CreateComponent<Text3D>();
    Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/BlueHighway.sdf");
    leveltxt_->SetFont(font, UISIZE[FONTSIZE_NORMAL]);
    leveltxt_->SetAlignment(HA_CENTER, VA_BOTTOM);
    leveltxt_->SetTextAlignment(HA_CENTER);
    leveltxt_->SetWidth(100);
    leveltxt_->SetWordwrap(false);
    for (int i=0; i<4; i++)
        leveltxt_->SetColor(Corner(i), Color::WHITE);

    // Create MissionScore
    AnimationSet2D* animationset = GameStatics::context_->GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>("UI/LevelMap/missionscore.scml");
    if (animationset)
    {
        Node* missionscore = planetInfo->CreateChild("MissionScore", LOCAL);
        AnimatedSprite2D* animatedsprite = missionscore->CreateComponent<AnimatedSprite2D>();
        animatedsprite->SetAnimationSet(animationset);
        animatedsprite->SetLayer(3);
    }

    // Create Cinematic Replay
    AnimatedSprite2D* button = GameHelpers::AddAnimatedSprite2D(planetInfo, "CinematicReplay", "UI/LevelMap/cinematicreplay.scml", String::EMPTY,
                                                                Vector2::ZERO, 0.f, true, CinematicButtonColliderCenter, CinematicButtonColliderRadius);
    if (button)
    {
        button->SetLayer(3);
        button->SetAnimation("idle");
    }
}

void LevelMapState::SetPlanetLinkButtons()
{
    URHO3D_LOGINFOF("LevelMapState() - SetPlanetLinkButtons : selectedLevelID_=%d", selectedLevelID_);

    Node* planets = mapscene_->GetChild("LevelScene")->GetChild("Planets");
    Node* linkbuttons = planets->GetChild("LinkButtons") ? planets->GetChild("LinkButtons") : planets->CreateChild("LinkButtons", LOCAL);
    linkbuttons->RemoveAllChildren();

    if (selectedLevelID_ > 0)
    {
        GameStatics::MissionState* mstates = GameStatics::playerState_->missionstates;
        if (mstates[selectedLevelID_-1].state_ > GameStatics::MissionState::MISSION_LOCKED)
        {
            unsigned index = selectedLevelID_-firstMissionID_;
            Node* node = selectablenodes_[index];
            const LevelGraphPoint* point = levelGraph_->GetOrderedPoints()[index];

            // Add Link Buttons
            const Vector<LevelGraphPoint* >& linkedpoints = point->linkedpoints_;
            for (int j = 0; j < linkedpoints.Size(); j++)
            {
                const LevelGraphPoint* linkedpoint = linkedpoints[j];
                unsigned linkmissionid = linkedpoint->id_;
                if (mstates[linkmissionid-1].state_ > GameStatics::MissionState::MISSION_LOCKED)
                {
                    Vector2 length = (linkedpoint->position_ - point->position_).Normalized();
                    float angle = Atan(length.y_ / length.x_);
                    float scale = 0.25f * node->GetWorldScale2D().x_;
                    Vector2 position = node->GetWorldPosition2D() + length * scale * GameStatics::uiScale_;

                    String name;
                    name.AppendWithFormat("%s%u", LABEL_LINK.CString(), linkmissionid);
                    AnimatedSprite2D* button = GameHelpers::AddAnimatedSprite2D(linkbuttons, name, "UI/LevelMap/zonebutton.scml", "zonebutton",
                                                                            Vector2::ZERO, 0.f, true, ZoneButtonColliderCenter, ZoneButtonColliderRadius);
                    button->GetNode()->SetWorldPosition2D(position);
                    button->GetNode()->SetRotation2D(angle + (length.x_ > 0.f ? -90.f : 90.f));
                    button->GetNode()->SetWorldScale2D(scale, scale);
                    button->SetLayer(4);
                    button->SetAnimation("selected");
                }
            }
        }
    }
}

void LevelMapState::UpdatePlanetSelection()
{
    for (unsigned i = 0; i < selectableplanets_.Size(); i++)
        selectableplanets_[i]->SetScale2D(PLANETOBJECT_SCALEDEFAULT);

    int iplanet = selectedLevelID_ - firstMissionID_;
    if (iplanet < 0 || iplanet > selectableplanets_.Size())
        return;

    selectableplanets_[iplanet]->SetScale2D(PLANETOBJECT_SCALESELECTED);

    // Set Planet Links
    if (planetMode_ == 1)
        SetPlanetLinkButtons();

    // Update Planet Infos : mission score, cinematic replay
    // selectableplanets_[iplanet]->GetParent()  is equivalent to   mapscene_->GetChild("LevelScene")->GetChild("Planets");
    Node* planetInfo = selectableplanets_[iplanet]->GetParent()->GetChild("PlanetInfo");
    if (planetInfo)
    {
        if (planetMode_ == 1)
        {
            Node* node = selectablenodes_[iplanet];

            URHO3D_LOGINFOF("LevelMapState() - UpdatePlanetSelection ... selectedlevel=%d ...", selectedLevelID_);

            // Update Level Text
            if (leveltxt_)
            {
                leveltxt_->GetNode()->SetWorldPosition2D(node->GetWorldPosition2D());// - Vector2(0.f, 0.12f) * node->GetWorldScale2D());
                leveltxt_->GetNode()->SetWorldScale2D(node->GetWorldScale2D() * Vector2(0.6f, 0.6f));
                leveltxt_->SetText(String(selectedLevelID_));
                GameHelpers::AddAnimation(leveltxt_->GetNode(), "Scale", leveltxt_->GetNode()->GetWorldScale() * Vector3(0.9f, 0.9f),
                                          leveltxt_->GetNode()->GetWorldScale() * Vector3(1.2f, 1.2f), 2.f, WM_LOOP);
                leveltxt_->GetNode()->SetEnabled(true);
            }
            // Update Mission Score
            Node* missionscore = planetInfo->GetChild("MissionScore");
            if (missionscore)
            {
                GameStatics::MissionState* mstates = GameStatics::playerState_->missionstates;
                if (mstates[selectedLevelID_-1].score_ > 0)
                {
                    URHO3D_LOGINFOF("LevelMapState() - UpdatePlanetSelection ... selectedlevel=%d score=%d ...", selectedLevelID_, mstates[selectedLevelID_-1].score_);
                    missionscore->SetWorldPosition2D(node->GetWorldPosition2D() + Vector2(0.f, 0.2f) * node->GetWorldScale2D());
                    missionscore->SetWorldScale2D(node->GetWorldScale2D() * Vector2(0.1f, 0.1f));
                    AnimatedSprite2D* animatedsprite = missionscore->GetComponent<AnimatedSprite2D>();
                    animatedsprite->SetEntity(String(mstates[selectedLevelID_-1].score_));
                    animatedsprite->SetAnimation("idle");
                    missionscore->SetEnabled(true);
                }
                else
                {
                    missionscore->SetEnabled(false);
                }
            }
            // Update Cinematic Replay
            Node* cinematicreplay = planetInfo->GetChild("CinematicReplay");
            if (cinematicreplay)
            {
                bool hascinematic = false;

            #ifdef ACTIVE_CINEMATICS
                if (CinematicState::GetCinematicParts(CINEMATICSELECTIONMODE_REPLAY, firstMissionID_, selectedLevelID_))
                {
                #ifdef ACTIVE_CINEMATICS_BUTTONONSCENE
                    cinematicreplay->SetWorldPosition2D(node->GetWorldPosition2D() - Vector2(0.25f, 0.25f) * node->GetWorldScale2D());
                    cinematicreplay->SetWorldScale2D(node->GetWorldScale2D() * Vector2(0.1f, 0.1f));
                    cinematicreplay->SetEnabled(true);

                    // Patch Collider Scaling : Recreate CollisionCircle2D
                    cinematicreplay->GetComponent<CollisionCircle2D>()->Remove();
                    CollisionCircle2D* collider = cinematicreplay->CreateComponent<CollisionCircle2D>();
                    collider->SetRadius(CinematicButtonColliderRadius);
                    collider->SetCenter(CinematicButtonColliderCenter);
                    cinematicreplay->SetEnabled(true);
                #else
                    cinematicreplay->SetEnabled(false);
                #endif
                    hascinematic = true;
                }
                else
                    cinematicreplay->SetEnabled(false);
            #else
                cinematicreplay->SetEnabled(false);
            #endif

                Game::Get()->SetAccessMenuButtonVisible(3, hascinematic);
            }
        }
        else
        {
            if (leveltxt_)
                leveltxt_->GetNode()->SetEnabled(false);
            Game::Get()->SetAccessMenuButtonVisible(3, false);
        }
        planetInfo->SetEnabled(planetMode_ == 1);
    }
}

bool LevelMapState::CreateScene(bool reset)
{
    // Set current selected mission
    if (GameStatics::currentLevel_ < 1)
        GameStatics::currentLevel_ = 1;

    const int zone = GameStatics::currentZone_;

    URHO3D_LOGINFOF("LevelMapState() - CreateScene ... currentzone=%d ...", zone);

    // Set level Graph
    if (!levelGraph_)
    {
        String levelgraphfile;
        levelgraphfile = levelgraphfile.AppendWithFormat("UI/LevelMap/levelmappoints%d.svg", zone);
        levelGraph_ = WeakPtr<LevelGraph>(GameStatics::context_->GetSubsystem<ResourceCache>()->GetResource<LevelGraph>(levelgraphfile));

        URHO3D_LOGINFOF("LevelMapState() - CreateScene ... load %s...", levelgraphfile.CString());

        if (!levelGraph_)
        {
            URHO3D_LOGERRORF("LevelMapState() - CreateScene ... can't load file for zone=%d !", zone);
            return false;
        }

        if (levelGraph_->GetOrderedPoints().Size())
        {
            firstMissionID_ = levelGraph_->GetOrderedPoints().Front()->id_;
            lastMissionID_ = levelGraph_->GetOrderedPoints().Back()->id_;

            UpdateStatics();

            URHO3D_LOGINFOF("LevelMapState() - CreateScene ... firstMissionID_=%d lastMissionID_=%d ...", firstMissionID_, lastMissionID_);
        }
        else
        {
            firstMissionID_ = lastMissionID_ = -1;
            URHO3D_LOGERROR("LevelMapState() - CreateScene ... No Points in LevelGraph !");
            return false;
        }
    }

    // Create the map scene node
    if (!mapscene_)
        mapscene_ = GameStatics::rootScene_->GetChild("CurrentLevelMap");
    if (!mapscene_)
        mapscene_ = GameStatics::rootScene_->CreateChild("CurrentLevelMap", LOCAL);

    if (GameStatics::currentLevel_ != selectedLevelID_)
        selectedLevelID_ = GameStatics::currentLevel_;

    if (GameStatics::currentLevel_ < firstMissionID_ || GameStatics::currentLevel_ > lastMissionID_ ||
        GameStatics::playerState_->missionstates[GameStatics::currentLevel_-1].state_ == GameStatics::MissionState::MISSION_LOCKED)
        selectedLevelID_ = GameStatics::currentLevel_ = GameStatics::GetNextUnlockedMission();

    // Create the ui level map scene node
    if (!uilevelmapscene_)
        uilevelmapscene_ = mapscene_->GetChild("UILevelMapScene");
    if (!uilevelmapscene_)
        uilevelmapscene_ = mapscene_->CreateChild("UILevelMapScene", LOCAL);

    // Create the level map scene node
    Node* levelscene = mapscene_->GetChild("LevelScene");
    if (!levelscene)
    {
        levelscene = mapscene_->CreateChild("LevelScene", LOCAL);

        // Load LevelMap Scene Animation
        String levelscenefile;
        levelscenefile = levelscenefile.AppendWithFormat("UI/LevelMap/levelmap%d.xml", zone);
        URHO3D_LOGINFOF("LevelMapState() - CreateScene ... load %s...", levelscenefile.CString());
        if (!GameHelpers::LoadNodeXML(context_, levelscene, levelscenefile, LOCAL))
        {
            URHO3D_LOGWARNINGF("LevelMapState() - CreateScene ... no scene file for zone=%d !", zone);

            if (!GenerateLevelMapScene(zone))
                return false;
        }

        // Find Constellation Mode
        constellationMode_ = false;
        Node* constellationNode = levelscene->GetChild("Constellation");
        if (constellationNode)
        {
            if (GameStatics::CanUnblockConstellation(GameStatics::playerState_->zone))
            {
                constellationMode_ = true;
                /// inactive this line for TESTing animation all time
                GameStatics::playerState_->zonestates[GameStatics::playerState_->zone-1] = GameStatics::CONSTELLATION_UNBLOCKED;
            }
        }

        // Unblock Next Zone
        if (GameStatics::playerState_->missionstates[GameStatics::GetBossLevelId(GameStatics::playerState_->zone)-1].state_ == GameStatics::MissionState::MISSION_COMPLETED &&
            GameStatics::playerState_->zonestates[GameStatics::playerState_->zone] == GameStatics::ZONE_BLOCKED)
        {
            GameStatics::playerState_->zonestates[GameStatics::playerState_->zone] = GameStatics::ZONE_UNBLOCKED;
        }

        // Unlock First Mission if need
        GameStatics::MissionState& firstmissionstate = GameStatics::playerState_->missionstates[firstMissionID_-1];
        if (firstmissionstate.state_ == GameStatics::MissionState::MISSION_LOCKED)
            firstmissionstate.state_ = GameStatics::MissionState::MISSION_UNLOCKED;

        // Set Missions nodes
        Node* missionpoints = levelscene->GetChild("MissionPoints");
        if (missionpoints && missionpoints->GetChild("Mission_Default") && levelGraph_->GetOrderedPoints().Size())
        {
            if (GameStatics::playerState_->zonestates[GameStatics::playerState_->zone] >= GameStatics::ZONE_UNBLOCKED)
                missionpoints->SetEnabledRecursive(true);

            GameStatics::gameState_.UpdateMission(selectedLevelID_);

            SetMissionNodes(missionpoints);
        }

        // Set Links
        SetLinks();

        // Set Planets
        Node* planets = levelscene->GetChild("Planets");
        if (planets && planets->GetChild("Planet_Default") && levelGraph_->GetOrderedPoints().Size())
        {
            SetPlanetNodes(planets);
            planets->SetEnabledRecursive(false);
        }

        constellationMode_ &= (constellationNode != 0);
        // Enter in Constellation Mode
        if (constellationMode_)
        {
           // Subscribe To Reactive Standard Mode
            DelayInformer::Get(this, 3.f, GAME_UIFRAME_ADD);
            SubscribeToEvent(this, GAME_UIFRAME_ADD, URHO3D_HANDLER(LevelMapState, OnGainFrame));

            GameStatics::currentMusic_ = CONSTELLATION1 + GameStatics::playerState_->zone % MAXCONSTELLATIONTHEMES;
            GameHelpers::SetMusic(MAINMUSIC, 0.7f, GameStatics::currentMusic_, false);

            // Align constellation on missions
            constellationNode->SetPosition2D(missionpoints->GetPosition2D());
            constellationNode->SetEnabled(true);

            URHO3D_LOGINFOF("LevelMapState() - CreateScene ... Constellation Mode position=%s(scale=%s) aligned on Missions node=%s(scale=%s) !",
                            constellationNode->GetWorldPosition2D().ToString().CString(), constellationNode->GetWorldScale2D().ToString().CString(),
                            missionpoints->GetWorldPosition2D().ToString().CString(), missionpoints->GetWorldScale2D().ToString().CString());
        }
        else // Standard Mode
        {
            GameStatics::currentMusic_ = LEVELMAPTHEME1 + GameStatics::playerState_->zone % MAXLEVELMAPTHEMES;
            GameHelpers::SetMusic(MAINMUSIC, 0.4f, GameStatics::currentMusic_, true);
            SetEnableConstellation(true);
        }

        GameStatics::cameraNode_->SetPosition(Vector3::ZERO);
        cameraZoom_ = GameStatics::camera_->GetZoom();
        GameStatics::rootScene_->SetUpdateEnabled(true);
        lastSelectedLevelID_ = -1;
        planetMode_ = 0;
    }

    // Adjust LevelMaps Scene
    float scale = 0.85f * GameHelpers::GetMaximizedScaleWithImageRatioKeeped(Vector2((float)GameStatics::targetwidth_, (float)GameStatics::targetheight_), levelGraph_->GetFrameSize());
    levelscene->SetWorldScale(Vector3(scale, scale, 1.f));

    // Adjust Backgrounds
    Node* backgrd1 = levelscene->GetChild("BackGroundFrame1");
    if (backgrd1)
        GameHelpers::SetAdjustedToScreen(backgrd1, 1.25f, 1.25f, false);
    Node* backgrd2 = levelscene->GetChild("BackGroundFrame2");
    if (backgrd2)
        GameHelpers::SetAdjustedToScreen(backgrd2, 1.25f, 1.25f, false);

    UpdateSceneRect();

    CreateUI();
    SetVisibleUI(true);

    // Go in PlanetMode if zone is unblocked only
    if (planetMode_ == 0 && (GameStatics::playerState_->zone == NBMAXZONE || GameStatics::playerState_->zonestates[GameStatics::playerState_->zone] < GameStatics::ZONE_UNBLOCKED))
    {
        selector_->SetPosition2D(selectablenodes_[selectedLevelID_-firstMissionID_]->GetPosition2D());
        GoToSelectedLevel();

        if (GameStatics::GetPreviousStatus() >= PLAYSTATE_INITIALIZING)
        {
            SwitchPlanetMode();
        }
        else
        {
            if (switchPlanetModeInformer_ && !switchPlanetModeInformer_->Expired())
                switchPlanetModeInformer_->Free();
            switchPlanetModeInformer_ = DelayInformer::Get(this, SWITCHPLANETMODETIME, GAME_SWITCHPLANETMODE);
            SubscribeToEvent(this, GAME_SWITCHPLANETMODE, URHO3D_HANDLER(LevelMapState, HandleChangePlanetMode));
        }
    }

    return true;
}

void LevelMapState::UpdateSceneRect()
{
    BoundingBox graphBox;
    for (unsigned i=0; i < selectablenodes_.Size(); i++)
        graphBox.Merge(selectablenodes_[i]->GetWorldPosition2D());
    GameHelpers::OrthoWorldToScreen(GameStatics::screenSceneRect_, graphBox);
    GameStatics::screenSceneRect_.left_  = 0;
    GameStatics::screenSceneRect_.right_ = GameStatics::graphics_->GetWidth();
}

void LevelMapState::UpdateScene()
{
    Node* levelscene = mapscene_->GetChild("LevelScene");
    Node* planets = levelscene->GetChild("Planets");

    if (planetMode_ == 0)
    {
        cameraSmooth_ = SMOOTHCAMERA_POSITION | SMOOTHCAMERA_ZOOM;
        cameraTargetPosition_ = Vector3::ZERO;
        cameraTargetZoom_ = cameraZoom_;

        levelscene->GetChild("MissionPoints")->SetEnabledRecursive(true);
        if (planets)
            planets->SetEnabledRecursive(false);

        UpdatLinkScaleY(1.f);
        SetEnableConstellation(true);

        if (prevZoneButton_)
            prevZoneButton_->SetEnabled(true);
        if (nextZoneButton_)
            nextZoneButton_->SetEnabled(true);

        // active BackGround1
        Node* backgrd1 = levelscene->GetChild("BackGroundFrame1");
        if (backgrd1)
        {
            GameHelpers::SetAdjustedToScreen(backgrd1, 1.25f, 1.25f, false);
            GameHelpers::AddAnimation(backgrd1, "Alpha", backgrd1->GetDerivedComponent<StaticSprite2D>()->GetAlpha(), 1.f, SWITCHSCREENTIME);
        }
        Node* backgrd2 = levelscene->GetChild("BackGroundFrame2");
        if (backgrd2)
        {
            GameHelpers::SetAdjustedToScreen(backgrd2, 1.25f, 1.25f, false);
            GameHelpers::AddAnimation(backgrd2, "Alpha", backgrd2->GetDerivedComponent<StaticSprite2D>()->GetAlpha(), 0.f, SWITCHSCREENTIME);
        }
    }
    else if (planetMode_ == 1 && planets && selectedLevelID_ > 0)
    {
        cameraSmooth_ = SMOOTHCAMERA_POSITION | SMOOTHCAMERA_ZOOM;
        cameraTargetPosition_ = selector_->GetWorldPosition();
        cameraTargetZoom_ = cameraZoom_ * MISSIONPLANET_ZOOM;

        levelscene->GetChild("MissionPoints")->SetEnabledRecursive(false);
        planets->SetEnabledRecursive(true);
        UpdatLinkScaleY(MISSIONLINK_PLANETMODESCALE);
        SetEnableConstellation(false);

        UpdatePlanetSelection();

        if (prevZoneButton_)
            prevZoneButton_->SetEnabled(false);
        if (nextZoneButton_)
            nextZoneButton_->SetEnabled(false);

        // active BackGround2
        Node* backgrd1 = levelscene->GetChild("BackGroundFrame1");
        if (backgrd1)
        {
            float alpha = Max(backgrd1->GetDerivedComponent<StaticSprite2D>()->GetAlpha(), 0.5f);
            GameHelpers::AddAnimation(backgrd1, "Alpha", alpha, 0.f, SWITCHSCREENTIME);
        }
        Node* backgrd2 = levelscene->GetChild("BackGroundFrame2");
        if (backgrd2)
            GameHelpers::AddAnimation(backgrd2, "Alpha", backgrd2->GetDerivedComponent<StaticSprite2D>()->GetAlpha(), 1.f, SWITCHSCREENTIME);

        // add story messages
        if (Game::Get()->GetCompanion())
        {
            if (selectedLevelID_ == 9 && GameStatics::playerState_->missionstates[9-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
                Game::Get()->GetCompanion()->AddMessage(STATE_LEVELMAP, false, "story_warnboss01", "capi", 0.f);
            else if (selectedLevelID_ == 24 && GameStatics::playerState_->missionstates[24-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
                Game::Get()->GetCompanion()->AddMessage(STATE_LEVELMAP, false, "story_warnboss02", "rosi", 0.f);
            else if (selectedLevelID_ == 30 && GameStatics::playerState_->missionstates[30-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
                Game::Get()->GetCompanion()->AddMessage(STATE_LEVELMAP, false, "story_warnboss03_1", "jona", 0.f);
            else if (selectedLevelID_ == 42 && GameStatics::playerState_->missionstates[42-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
                Game::Get()->GetCompanion()->AddMessage(STATE_LEVELMAP, false, "story_warnboss03_2", "jona", 0.f);
        }
    }

    selector_->GetComponent<AnimatedSprite2D>()->SetAnimation("selected");
    selector_->SetEnabled(selectedLevelID_ > 0);

    UpdateSceneRect();
}

void LevelMapState::SetEnableConstellation(bool enable)
{
    Node* constellationNode = mapscene_->GetChild("LevelScene")->GetChild("Constellation");
    if (constellationNode)
    {
        AnimatedSprite2D* animation = constellationNode->GetComponent<AnimatedSprite2D>();
        if (enable && animation && animation->HasAnimation("keep") && GameStatics::playerState_->zonestates[GameStatics::playerState_->zone-1] == GameStatics::CONSTELLATION_UNBLOCKED)
        {
            Node* missionpoints = mapscene_->GetChild("LevelScene")->GetChild("MissionPoints");
            constellationNode->SetScale2D(missionpoints->GetScale2D());
            constellationNode->SetPosition2D(missionpoints->GetPosition2D());
            constellationNode->SetEnabled(true);
            animation->SetAnimation("keep");
            animation->SetLayer(500);
        }
        else
        {
            constellationNode->SetEnabled(false);
        }
    }
}

void LevelMapState::SwitchPlanetMode()
{
    if (switchPlanetModeInformer_ && !switchPlanetModeInformer_->Expired())
        switchPlanetModeInformer_->Free();

    if (planetMode_ == 0 && InteractiveFrame::HasRunningInstances())
        return;

    if (selectedLevelID_ > 0)
    {
        planetMode_ = (planetMode_+1) % 2;
        lastSelectedLevelID_ = -1;

        URHO3D_LOGINFOF("LevelMapState() - SwitchPlanetMode : ... planetmode=%d !", planetMode_);

        UpdateScene();
    }
}


WeakPtr<InteractiveFrame> constellationgainframe_;
WeakPtr<InteractiveFrame> nomoretriesframe_;
WeakPtr<InteractiveFrame> messageframe_;

void LevelMapState::CloseInteractiveFrames()
{
    if (nomoretriesframe_ && HasSubscribedToEvent(nomoretriesframe_, E_MESSAGEACK))
    {
        nomoretriesframe_->Stop();
        UnsubscribeFromEvent(nomoretriesframe_, E_MESSAGEACK);
    }
    if (constellationgainframe_ && HasSubscribedToEvent(constellationgainframe_, E_MESSAGEACK))
    {
        UnsubscribeFromEvent(constellationgainframe_, E_MESSAGEACK);
        constellationgainframe_->Stop();
    }
}

void LevelMapState::OnGainFrame(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, GAME_UIFRAME_ADD);

    constellationgainframe_ = InteractiveFrame::Get("UI/InteractiveFrame/ConstellationGainFrame.xml");

    Graphics* graphics = GetSubsystem<Graphics>();
    constellationgainframe_->SetScreenPositionEntrance(IntVector2(-300, graphics->GetHeight()/2));
    constellationgainframe_->SetScreenPosition(IntVector2(graphics->GetWidth()/2, graphics->GetHeight()/2));
    constellationgainframe_->SetScreenPositionExit(0, IntVector2(-300, graphics->GetHeight()/2));
    constellationgainframe_->SetScreenPositionExit(1, IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
    constellationgainframe_->SetScreenPositionEntranceForBonus(IntVector2(graphics->GetWidth()/2, -200));

    constellationgainframe_->AddBonus(Slot::ITEMS_2COINS);
    const Vector<StringHash>& powertypes = MatchesManager::GetAuthorizedObjects(COT::POWERS);
    if (powertypes.Size())
        constellationgainframe_->AddBonus(Slot(COT::POWERS, powertypes[GameRand::GetRand(ALLRAND, powertypes.Size())], 1, true));

    GameStatics::AddBonuses(constellationgainframe_->GetBonuses(), this);

    constellationgainframe_->Start(false, false);

    SubscribeToEvent(constellationgainframe_, E_MESSAGEACK, URHO3D_HANDLER(LevelMapState, OnGainFrameMessageAck));
}

void LevelMapState::OnGainFrameMessageAck(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(constellationgainframe_, E_MESSAGEACK);

    URHO3D_LOGINFO("LevelMapState() - OnGainFrameMessageAck  !");

    DelayInformer::Get(this, 0.1f, GAME_SWITCHSCENEMODE);
    SubscribeToEvent(this, GAME_SWITCHSCENEMODE, URHO3D_HANDLER(LevelMapState, HandleChangeSceneMode));

    if (constellationgainframe_)
        constellationgainframe_->Stop();

    GameHelpers::SetMusic(MAINMUSIC, 1.f);

    constellationMode_ = false;
}

void LevelMapState::OnGameNoMoreTries(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, GAME_UIFRAME_ADD);

    if (planetMode_ == 1)
        SwitchPlanetMode();

    GameStatics::AllowInputs(false);

#if defined(ACTIVE_ADS)
    nomoretriesframe_ = InteractiveFrame::Get("UI/InteractiveFrame/GameNeedMoreStars_Ads.xml");
#else
    nomoretriesframe_ = InteractiveFrame::Get("UI/InteractiveFrame/GameNeedMoreStars.xml");
#endif

    Graphics* graphics = GetSubsystem<Graphics>();
    nomoretriesframe_->SetScreenPositionEntrance(IntVector2(-300, graphics->GetHeight()/2));
    nomoretriesframe_->SetScreenPosition(IntVector2(graphics->GetWidth()/2, graphics->GetHeight()/2));
    nomoretriesframe_->SetScreenPositionExit(0, IntVector2(-300, graphics->GetHeight()/2));
    nomoretriesframe_->SetScreenPositionExit(1, IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
    nomoretriesframe_->SetScreenPositionEntranceForBonus(IntVector2(graphics->GetWidth()/2, -200));

    nomoretriesframe_->Start(false, false);

    SubscribeToEvent(nomoretriesframe_, E_MESSAGEACK, URHO3D_HANDLER(LevelMapState, OnGameNoMoreTriesAck));
}

void LevelMapState::OnGameNoMoreTriesAck(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(nomoretriesframe_, E_MESSAGEACK);

	if (eventData[MessageACK::P_OK].GetBool())
    {
        String action = eventData[TextEntry::P_TEXT].GetString();
        if (action == "ShowAd")
        {
            bool ok = false;
#if defined(ACTIVE_ADS)
        #if defined(__ANDROID__)
            URHO3D_LOGINFO("LevelMapState() - OnGameNoMoreTriesAck : Show Ad !");
            ok = Android_ShowAds();
        #endif
#endif
            if (!ok)
                action = "GoShop";
        }
        if (action == "GoShop")
        {
            OptionState* optionState = (OptionState*)GameStatics::stateManager_->GetState("Options");
            if (!optionState->IsShopOpened())
            {
                URHO3D_LOGINFO("LevelMapState() - OnGameNoMoreTriesAck : Show Shop ...");
                optionState->SwitchShopFrame();
            }
        }
    }
	else
    {
        URHO3D_LOGINFO("LevelMapState() - OnGameNoMoreTriesAck : cancel !");
    }

    if (nomoretriesframe_)
    {
    #if defined(__ANDROID__)
        nomoretriesframe_->SendEvent(GAME_UIFRAME_REMOVE);
    #else
        nomoretriesframe_->Stop();
    #endif
    }

    GameStatics::AllowInputs(true);
}


void LevelMapState::ResetUI()
{
    SetVisibleUI(false);

    if (GameStatics::ui_)
    {
        GameStatics::ui_->SetFocusElement(0);

        dialogbox_.Reset();

        if (uilevelmap_)
        {
            UIElement* root = GameStatics::ui_->GetRoot();
            if (root)
                root->RemoveChild(uilevelmap_);
            else
                uilevelmap_->Remove();
        }

        uilevelmap_.Reset();
    }
}

void LevelMapState::SetVisibleUI(bool state)
{
    if (uilevelmap_)
        uilevelmap_->SetVisible(state);

    if (uilevelmapscene_)
    {
        uilevelmapscene_->SetEnabledRecursive(state);

        if (state)
        {
            // Add previous zone button
            if (!prevZoneButton_ && firstMissionID_ > 1)// && GameStatics::playerState_->missionstates[firstMissionID_-2].state_ == GameStatics::MissionState::MISSION_COMPLETED)
            {
                prevZoneButton_ = GameHelpers::AddAnimatedSprite2D(uilevelmapscene_, LABEL_ZONE_PREVIOUS, "UI/LevelMap/zonebutton.scml", "zonebutton",
                                                                    Vector2::ZERO, 180, true, ZoneButtonColliderCenter, ZoneButtonColliderRadius);
                URHO3D_LOGINFO("LevelMapState() - SetVisibleUI ... create button previous zone !");
            }
            // Add next zone button
            if (!nextZoneButton_ && lastMissionID_ > 0 && lastMissionID_ < NBMAXLVL && GameStatics::playerState_->zonestates[GameStatics::currentZone_] >= GameStatics::ZONE_UNBLOCKED)// lastMissionID_ < NBMAXLVL && GameStatics::playerState_->missionstates[lastMissionID_-1].state_ == GameStatics::MissionState::MISSION_COMPLETED)
            {
                nextZoneButton_ = GameHelpers::AddAnimatedSprite2D(uilevelmapscene_, LABEL_ZONE_NEXT, "UI/LevelMap/zonebutton.scml", "zonebutton",
                                                                    Vector2::ZERO, 0, true, ZoneButtonColliderCenter, ZoneButtonColliderRadius);
                URHO3D_LOGINFO("LevelMapState() - SetVisibleUI ... create button next zone !");
            }
            // Adjust ZoneButtons to LevelMap
            if (prevZoneButton_)
                prevZoneButton_->GetNode()->SetWorldPosition2D(selectablenodes_.Front()->GetWorldPosition2D() - Vector2(0.f, 0.8f) * selectablenodes_.Front()->GetWorldScale2D());
            if (nextZoneButton_)
            {
                int i = Clamp(0, lastMissionID_-firstMissionID_, (int)selectablenodes_.Size()-1);
                nextZoneButton_->GetNode()->SetWorldPosition2D(selectablenodes_[i]->GetWorldPosition2D() + Vector2(0.f, 0.8f) * selectablenodes_[i]->GetWorldScale2D());
                // Highlight ZoneNext Button
                UpdateActionAnimations(LEVELMAPACTION_GOTONEXTZONE);
            }
        }

        if (prevZoneButton_)
            prevZoneButton_->GetNode()->SetEnabled(!constellationMode_);
        if (nextZoneButton_)
            nextZoneButton_->GetNode()->SetEnabled(!constellationMode_);
    }
}

void LevelMapState::CreateUI()
{
    if (!uilevelmap_)
    {
        URHO3D_LOGINFO("LevelMapState() - CreateUI ...");

        // Create uilevelmap
        uilevelmap_ = GameStatics::ui_->GetRoot()->CreateChild<UIElement>("levelmaprootui");
        uilevelmap_->SetSize(GameStatics::graphics_->GetWidth(), GameStatics::graphics_->GetHeight());

        URHO3D_LOGINFO("LevelMapState() - CreateUI ... OK !");
    }

    // Create mission message box
#ifdef ACTIVE_MISSIONDIALOG
    if (!dialogbox_)
    {
        dialogbox_ = new UIDialog(context_);
        dialogbox_->Set(uilevelmap_, "UI/missiondialogbox.xml");
    }

    UpdateDialogBox(selectedLevelID_);
#endif

    Game::Get()->CreateAccessMenu(uilevelmap_);

    // Reset Focus, Keep Update restore the focus
    GameStatics::ui_->SetFocusElement(0);

    SetVisibleUI(false);
}

void LevelMapState::RemoveUI()
{
	if (GameStatics::ui_)
        GameStatics::ui_->SetFocusElement(0);

    if (constellationgainframe_)
    {
        constellationgainframe_->Clean();
        constellationgainframe_.Reset();
    }
    if (messageframe_)
    {
        messageframe_->Clean();
        messageframe_.Reset();
    }

    uilevelmap_->Remove();
    uilevelmap_.Reset();

    // Remove Delayed for Go Out Animation
//    TimerRemover::Get()->Start(uilevelmap_, SWITCHSCREENTIME + 0.1f);
}


void LevelMapState::UnselectLevel()
{
//    URHO3D_LOGINFO("LevelMapState() - UnselectLevel !");

    selectedLevelID_ = -1;

    if (selector_)
        selector_->SetEnabled(false);

    UpdateDialogBox(-1);
}

void LevelMapState::GoToSelectedLevel()
{
    URHO3D_LOGINFOF("LevelMapState() - GoToSelectedLevel : %d", selectedLevelID_);

    selector_->SetPosition2D(selectablenodes_[selectedLevelID_-firstMissionID_]->GetPosition2D());

    UpdateScene();
}

void LevelMapState::StartLevel()
{
    URHO3D_LOGINFO("LevelMapState() - ---------------------------------------");
    URHO3D_LOGINFOF("LevelMapState() - StartLevel %d                       -", selectedLevelID_);
    URHO3D_LOGINFO("LevelMapState() - ---------------------------------------");

    GameStatics::ResetGameStates();

    // Set the current LevelData pointer
    GameStatics::currentLevelDatas_ = 0;
    const Vector<LevelGraphPoint* >& points = levelGraph_->GetOrderedPoints();
    for (unsigned i=0; i < points.Size(); i++)
    {
        const LevelGraphPoint& point = *points[i];
        if (point.levelData_ && point.id_ == selectedLevelID_)
        {
            URHO3D_LOGINFOF("LevelMapState() - StartLevel %d : set GameStatics::currentLevelDatas_ !", selectedLevelID_);
            GameStatics::currentLevelDatas_ = point.levelData_;
            break;
        }
    }

    GameStatics::SetLevel(selectedLevelID_);

    GameStatics::SetConsoleVisible(false);
    GameStatics::SetMouseVisible(false, false);

    UnsubscribeToEvents();

#ifdef ACTIVE_CINEMATICS
    if (!CinematicState::SetCinematic(CINEMATICSELECTIONMODE_LEVELSELECTED, firstMissionID_, selectedLevelID_, "Play"))
        stateManager_->PushToStack("Play");
#else
    stateManager_->PushToStack("Play");
#endif
}

void LevelMapState::StartState(int action)
{
    URHO3D_LOGINFO("LevelMapState() - ---------------------------------------");
    URHO3D_LOGINFOF("LevelMapState() - StartState %s(%d) -", LevelMapActionStr[action], action);
    URHO3D_LOGINFO("LevelMapState() - ---------------------------------------");

    if (action == LEVELMAPACTION_STARTLEVEL && selectedLevelID_ != -1)
    {
        if (!GameStatics::tries_)
        {
            // No more Try (Stars)
            SubscribeToEvent(this, GAME_UIFRAME_ADD, URHO3D_HANDLER(LevelMapState, OnGameNoMoreTries));
            DelayInformer::Get(this, 0.5f, GAME_UIFRAME_ADD);
        }
        else
            StartLevel();
    }
    else if (action == LEVELMAPACTION_GOTOPREVIOUSZONE)
    {
        GameStatics::SetLevel(firstMissionID_-1);
        GameStatics::SetStatus(LEVELSTATE);
        bool reset = ResetMap();
    }
    else if (action == LEVELMAPACTION_GOTONEXTZONE)
    {
    #ifdef DEMOMODE_MAXZONE
        if (GameStatics::currentZone_ == DEMOMODE_MAXZONE)
        {
            if (!CinematicState::SetCinematic(CINEMATICSELECTIONMODE_TOBECONTINUED))
                return;
        }
        else
    #endif
        {
            GameStatics::SetLevel(lastMissionID_+1);
        #ifdef ACTIVE_CINEMATICS
            if (!CinematicState::SetCinematic(CINEMATICSELECTIONMODE_INTRO_OUTRO))
        #endif
            {
                GameStatics::SetStatus(LEVELSTATE);
                bool reset = ResetMap();
            }
        }
    }
    else if (action == LEVELMAPACTION_CHANGESELECTEDLEVEL)
    {
        GameStatics::SetLevel(selectedLevelID_);
        GoToSelectedLevel();
    }
    else if (action == LEVELMAPACTION_CINEMATICREPLAY)
    {
    #ifdef ACTIVE_CINEMATICS
        if (CinematicState::GetCinematicParts(CINEMATICSELECTIONMODE_REPLAY, firstMissionID_, selectedLevelID_))
        {
            // Show InteractiveFrame Confirmation Message
        #ifdef ACTIVE_CINEMATICS_BUTTONONSCENE
            UnsubscribeFromEvent(this, GAME_UIFRAME_ADD);

            if (!messageframe_ || !messageframe_->IsStarted())
            {
                messageframe_ = InteractiveFrame::Get("UI/InteractiveFrame/MessageFrame.xml");
                messageframe_->SetText("Message", "watchcinematic", true);
                messageframe_->SetScreenPositionEntrance(IntVector2(-300, GameStatics::graphics_->GetHeight()/2));
                messageframe_->SetScreenPosition(IntVector2(GameStatics::graphics_->GetWidth()/2, GameStatics::graphics_->GetHeight()/2));
                messageframe_->SetScreenPositionExit(0, IntVector2(-300, GameStatics::graphics_->GetHeight()/2));
                messageframe_->SetScreenPositionExit(1, IntVector2(GameStatics::graphics_->GetWidth()+300, GameStatics::graphics_->GetHeight()/2));
                messageframe_->SetScreenPositionEntranceForBonus(IntVector2(GameStatics::graphics_->GetWidth()/2, -200));
                messageframe_->Start(false, false);
            }

            SubscribeToEvent(messageframe_, E_MESSAGEACK, URHO3D_HANDLER(LevelMapState, HandleConfirmCinematicLaunch));
        #endif

            ;
        }
    #endif
        ;
    }
}

void LevelMapState::ReturnMainMenu()
{
    if (!GameStatics::ui_)
    {
        GameStatics::Exit();
        return;
    }

    URHO3D_LOGINFO("LevelMapState() - ReturnMainMenu !");
    stateManager_->PopStack();
}

void LevelMapState::SubscribeToEvents()
{
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(LevelMapState, HandleKeyDown));

    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(LevelMapState, HandleSelection));
    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(LevelMapState, HandleSelection));
#ifndef __ANDROID__
    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(LevelMapState, HandleSelection));
#endif
    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(LevelMapState, HandleSelection));

    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(LevelMapState, HandleSelection));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(LevelMapState, HandleSelection));
//    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(LevelMapState, HandleSelection));
    SubscribeToEvent(E_MULTIGESTURE, URHO3D_HANDLER(LevelMapState, HandleSelection));

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(LevelMapState, HandleSelection));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(LevelMapState, HandleSelection));
    SubscribeToEvent(E_MOUSEWHEEL, URHO3D_HANDLER(LevelMapState, HandleSelection));

    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(LevelMapState, HandleSceneUpdate));

    SubscribeToEvent(GAME_UIFRAME_START, URHO3D_HANDLER(LevelMapState, HandleInteractiveFrameStart));

    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(LevelMapState, HandleScreenResized));

    Game::Get()->SubscribeToAccessMenuEvents();

    UIElement* accessmenu = Game::Get()->GetAccessMenu();
    if (accessmenu)
    {
        SubscribeToEvent(accessmenu, UIMENU_SHOWCONTENT, URHO3D_HANDLER(LevelMapState, HandleAccessMenu));
        SubscribeToEvent(accessmenu, UIMENU_HIDECONTENT, URHO3D_HANDLER(LevelMapState, HandleAccessMenu));
    }
}

void LevelMapState::UnsubscribeToEvents()
{
    UnsubscribeFromEvent(E_KEYDOWN);

    UnsubscribeFromEvent(E_KEYUP);
    UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
#ifndef __ANDROID__
    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
#endif
    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);

    UnsubscribeFromEvent(E_TOUCHBEGIN);
    UnsubscribeFromEvent(E_TOUCHMOVE);
    UnsubscribeFromEvent(E_TOUCHEND);
    UnsubscribeFromEvent(E_MULTIGESTURE);

    UnsubscribeFromEvent(E_MOUSEMOVE);
    UnsubscribeFromEvent(E_MOUSEBUTTONDOWN);
    UnsubscribeFromEvent(E_MOUSEWHEEL);

    UnsubscribeFromEvent(E_SCENEUPDATE);

    UnsubscribeFromEvent(GAME_UIFRAME_START);

    UnsubscribeFromEvent(GAME_SCREENRESIZED);

    Game::Get()->UnsubscribeFromAccessMenuEvents();

    UIElement* accessmenu = Game::Get()->GetAccessMenu();
    if (accessmenu)
    {
        UnsubscribeFromEvent(accessmenu, UIMENU_SHOWCONTENT);
        UnsubscribeFromEvent(accessmenu, UIMENU_HIDECONTENT);
    }
}


void LevelMapState::HandleInteractiveFrameStart(StringHash eventType, VariantMap& eventData)
{
    if (planetMode_ > 0)
        HandleChangePlanetMode(eventType, eventData);
}

void LevelMapState::HandleConfirmCinematicLaunch(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(messageframe_, E_MESSAGEACK);
    if (messageframe_)
        messageframe_->Stop();

	if (eventData[MessageACK::P_OK].GetBool())
        CinematicState::LaunchCinematicParts(CINEMATICSELECTIONMODE_REPLAY);
}

void LevelMapState::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    if (cameraSmooth_)
    {
        const float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

        if (cameraSmooth_ & SMOOTHCAMERA_POSITION)
        {
            const float speed = 10.f;
            const float squaredSnapThreshold = 25.f;
            float constant = 1.0f - Clamp(powf(2.0f, -timeStep * speed), 0.0f, 1.0f);

            Vector3 position = GameStatics::cameraNode_->GetPosition();
            // If position snaps, snap everything to the end
            float delta = (position - cameraTargetPosition_).LengthSquared();
            if (delta > squaredSnapThreshold)
                constant = 1.0f;

            if (delta < M_EPSILON || constant >= 1.0f)
            {
                position = cameraTargetPosition_;
                cameraSmooth_ &= ~ SMOOTHCAMERA_POSITION;
            }
            else
            {
                position = position.Lerp(cameraTargetPosition_, constant);
            }

            GameStatics::cameraNode_->SetPosition(position);
        }

        if (cameraSmooth_ & SMOOTHCAMERA_ZOOM)
        {
            float zoom = GameStatics::camera_->GetZoom();
            float delta = zoom - cameraTargetZoom_;

            const float speed = delta < 0.f ? 2.f : 15.f;
            float constant = 1.0f - Clamp(powf(2.0f, -timeStep * speed), 0.0f, 1.0f);

            delta *= delta;
            if (delta < M_EPSILON || constant >= 1.0f)
            {
                zoom = cameraTargetZoom_;
                cameraSmooth_ &= ~ SMOOTHCAMERA_ZOOM;
            }
            else
            {
                zoom = Lerp(zoom, cameraTargetZoom_, constant);
            }

            GameStatics::camera_->SetZoom(zoom);
        }
    }

    // Test
    UpdatePeerOffers();
}

void LevelMapState::HandleChangePlanetMode(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("LevelMapState() - HandleChangePlanetMode : ... !");

    UnsubscribeFromEvent(GAME_SWITCHPLANETMODE);

    SwitchPlanetMode();
}

void LevelMapState::HandleChangeSceneMode(StringHash eventType, VariantMap& eventData)
{
    if (!mapscene_ || !mapscene_->GetChild("LevelScene"))
        return;

    URHO3D_LOGINFOF("LevelMapState() - HandleChangeSceneMode : ... !");

    UnsubscribeFromEvent(GAME_SWITCHSCENEMODE);

    // Reactive Standard Mode
    Node* constellationNode = mapscene_->GetChild("LevelScene")->GetChild("Constellation");
    if (constellationNode)
    {
        AnimatedSprite2D* animation = constellationNode->GetComponent<AnimatedSprite2D>();
        if (animation && animation->HasAnimation("keep") && GameStatics::playerState_->zonestates[GameStatics::playerState_->zone-1] == GameStatics::CONSTELLATION_UNBLOCKED)
        {
            Node* missionpoints = mapscene_->GetChild("LevelScene")->GetChild("MissionPoints");
            constellationNode->SetScale2D(missionpoints->GetScale2D());
            constellationNode->SetPosition2D(missionpoints->GetPosition2D());
            constellationNode->SetEnabled(true);
            animation->SetAnimation("keep");
            animation->SetLayer(500);
        }
        else
        {
            constellationNode->SetEnabled(false);
        }
    }

    SetVisibleUI(true);

    SubscribeToEvents();
}

void LevelMapState::UpdateActionAnimations(int newaction)
{
    if (newaction != lastaction_)
    {
        lastaction_ = newaction;

        if (prevZoneButton_ && newaction == LEVELMAPACTION_GOTOPREVIOUSZONE)
        {
            if (nextZoneButton_)
                nextZoneButton_->SetAnimation("unselected");

//            if (selector_)
//                selector_->SetEnabled(false);

            UpdateDialogBox(-1);

            prevZoneButton_->SetAnimation("selected");
        }
        else if (nextZoneButton_ && newaction == LEVELMAPACTION_GOTONEXTZONE)
        {
            if (prevZoneButton_)
                prevZoneButton_->SetAnimation("unselected");

//            if (selector_)
//                selector_->SetEnabled(false);

            UpdateDialogBox(-1);

            nextZoneButton_->SetAnimation("selected");
        }
        else if (newaction == LEVELMAPACTION_STARTLEVEL || newaction == LEVELMAPACTION_CHANGESELECTEDLEVEL || newaction == LEVELMAPACTION_CINEMATICREPLAY)
        {
            if (prevZoneButton_)
                prevZoneButton_->SetAnimation("unselected");

            if (nextZoneButton_)
                nextZoneButton_->SetAnimation("unselected");

            if (selectedLevelID_ > 0)
            {
                selector_->SetPosition2D(selectablenodes_[selectedLevelID_-firstMissionID_]->GetPosition2D());
                selector_->GetComponent<AnimatedSprite2D>()->SetAnimation("selected");
            }

            UpdateDialogBox(selectedLevelID_);

            selector_->SetEnabled(selectedLevelID_ > 0);
            UpdatePlanetSelection();
        }
    }
    else if ((newaction == LEVELMAPACTION_STARTLEVEL || newaction == LEVELMAPACTION_CHANGESELECTEDLEVEL) && selectedLevelID_ >= firstMissionID_ && selectedLevelID_ <= lastMissionID_)
    {
        selector_->SetPosition2D(selectablenodes_[selectedLevelID_-firstMissionID_]->GetPosition2D());
        selector_->GetComponent<AnimatedSprite2D>()->SetAnimation("selected");
        UpdatePlanetSelection();

        URHO3D_LOGINFOF("LevelMapState() - UpdateActionAnimations : newaction=%d selectorpos=%s !", newaction, selector_->GetPosition2D().ToString().CString());

        UpdateDialogBox(selectedLevelID_);
    }

//    URHO3D_LOGINFOF("LevelMapState() - UpdateActionAnimations : newaction=%d !", newaction);
}

void LevelMapState::UpdateDialogBox(int level)
{
#ifdef ACTIVE_MISSIONDIALOG
    if (!dialogbox_)
        return;

    if (level < 1)
    {
        dialogbox_->Stop();
    }
    else
    {
        GameStatics::MissionState m = GameStatics::playerState_->missionstates[level-1];

        if (m.state_ == GameStatics::MissionState::MISSION_COMPLETED)
        {
            String metrics;
            metrics.AppendWithFormat("Sc=%d Moves=%d T=%F", m.score_, m.numMovesUsed_, m.elapsedTime_);
            for (unsigned i=0; i < MAXOBJECTIVES; i++)
            {
                if (m.objectives_[i].target_)
                    metrics.AppendWithFormat(" O[%d]=%d/%d", i, m.objectives_[i].count_, m.objectives_[i].target_);
            }

            dialogbox_->SetContent(metrics);
        }
        else
            dialogbox_->SetMessage("m"+String(selectedLevelID_));

        dialogbox_->Start();
    }
#endif
}

void LevelMapState::HandleSelection(StringHash eventType, VariantMap& eventData)
{
    // touchEnabled KEY_SELECT Settings
    if (GetSubsystem<Input>()->GetKeyPress(KEY_SELECT) && GameStatics::gameConfig_.touchEnabled_)
    {
        if (GameStatics::gameConfig_.screenJoysticksettingsID_ == -1)
        {
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            GameStatics::gameConfig_.screenJoysticksettingsID_ = GetSubsystem<Input>()->AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings2.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
        }

        if (GameStatics::gameConfig_.screenJoysticksettingsID_ != -1)
            GetSubsystem<Input>()->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoysticksettingsID_, !GetSubsystem<Input>()->IsScreenJoystickVisible(GameStatics::gameConfig_.screenJoysticksettingsID_));

        return;
    }

    if (!GameStatics::allowInputs_)
        return;

    if (GameStatics::ui_->GetFocusElement())
    {
        return;
    }

    static bool inside = false;
    static IntVector2 beginposition;
    static Node* touchbeginnode = 0;
    static Node* touchendnode = 0;
    static bool touchmultigesture = false;
    bool launch = false;
    bool move = false;

    /// Set Mouse Mouvements in Menu Items
    if (eventType == E_MOUSEBUTTONDOWN || eventType == E_MOUSEMOVE || eventType == E_TOUCHBEGIN || eventType == E_TOUCHMOVE || eventType == E_TOUCHEND)
    {
        static IntVector2 position;

        if (eventType == E_MOUSEMOVE)
        {
            position.x_ = eventData[MouseMove::P_X].GetInt();
            position.y_ = eventData[MouseMove::P_Y].GetInt();
            touchmultigesture = false;
//            URHO3D_LOGINFOF("LevelMapState() - HandleSelection : mousemove=%s !", position.ToString().CString());
        }
        else if (eventType == E_MOUSEBUTTONDOWN)
        {
            touchmultigesture = false;
//            URHO3D_LOGINFOF("LevelMapState() - HandleSelection : mousebutton=%s !", position.ToString().CString());
        }
        else if (eventType == E_TOUCHBEGIN)
        {
            SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(LevelMapState, HandleSelection));
            position.x_ = eventData[TouchBegin::P_X].GetInt();
            position.y_ = eventData[TouchBegin::P_Y].GetInt();
            beginposition = position;
            touchbeginnode = touchendnode = 0;
            touchmultigesture = false;
//            URHO3D_LOGINFOF("LevelMapState() - HandleSelection : E_TOUCHBEGIN=%s !", position.ToString().CString());
        }
        else if (eventType == E_TOUCHMOVE)
        {
            position.x_ = eventData[TouchMove::P_X].GetInt();
            position.y_ = eventData[TouchMove::P_Y].GetInt();
//            URHO3D_LOGINFOF("LevelMapState() - HandleSelection : E_TOUCHMOVE=%s !", position.ToString().CString());
        }
        else if (eventType == E_TOUCHEND)
        {
            UnsubscribeFromEvent(E_TOUCHEND);
            position.x_ = eventData[TouchEnd::P_X].GetInt();
            position.y_ = eventData[TouchEnd::P_Y].GetInt();
            touchendnode = 0;
//            URHO3D_LOGINFOF("LevelMapState() - HandleSelection : E_TOUCHEND=%s !", position.ToString().CString());
        }

        inside = false;

        Vector2 wposition = GameHelpers::ScreenToWorld2D(position);
        RigidBody2D* body = GameStatics::rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBody(wposition);
        if (body && body->GetNode())
        {
            if (eventType == E_TOUCHBEGIN)
                touchbeginnode = body->GetNode();
            else if (eventType == E_TOUCHEND)
                touchendnode = body->GetNode();

            if (!touchmultigesture && body->GetNode()->GetName().StartsWith(LABEL_MISSION))
            {
                int missionid = ToInt(body->GetNode()->GetName().Substring(LABEL_MISSION.Length()));
                if (GameStatics::playerState_->missionstates[missionid-1].state_ != GameStatics::MissionState::MISSION_LOCKED)
                {
                    if (missionid != selectedLevelID_ || lastSelectedLevelID_ == -1)
                    {
                        lastSelectedLevelID_ = selectedLevelID_;
                        selectedLevelID_ = missionid;
                        move = true;

                        if (eventType == E_TOUCHMOVE && planetMode_ == 0)
                        {
                            URHO3D_LOGINFOF("LevelMapState() - HandleSelection : Break gesture !");
                            touchbeginnode = (Node*)1;
                        }

                        UpdateActionAnimations(LEVELMAPACTION_STARTLEVEL);
                        URHO3D_LOGINFOF("LevelMapState() - HandleSelection : selectedLevelID_=%d !", selectedLevelID_);
                    }
                }
                inside = true;

                if (planetMode_ == 1 && leveltxt_ && (eventType == E_TOUCHMOVE || eventType == E_MOUSEMOVE))
                {
                    leveltxt_->GetNode()->SetWorldPosition2D(selectableplanets_[selectedLevelID_ - firstMissionID_]->GetWorldPosition2D());
                    leveltxt_->SetEnabled(true);
                }
            }
            else if (planetMode_ == 1 && (eventType == E_MOUSEBUTTONDOWN || (eventType == E_TOUCHEND && touchbeginnode == touchendnode)))
            {
                if (body->GetNode()->GetName().StartsWith(LABEL_LINK))
                {
                    int missionid = ToInt(body->GetNode()->GetName().Substring(LABEL_LINK.Length()));
                    if (missionid != selectedLevelID_ || lastSelectedLevelID_ == -1)
                    {
                        lastSelectedLevelID_ = -1;
                        selectedLevelID_ = missionid;
                        move = true;
                        UpdateActionAnimations(LEVELMAPACTION_CHANGESELECTEDLEVEL);
                        URHO3D_LOGINFOF("LevelMapState() - HandleSelection : changeto selectedLevelID_=%d !", selectedLevelID_);
                    }
                    inside = true;
                }
            #if defined(ACTIVE_CINEMATICS_BUTTONONSCENE) && defined(ACTIVE_CINEMATICS)
                else if (body->GetNode()->GetName().StartsWith(LABEL_CINEMATIC))
                {
                    URHO3D_LOGINFOF("LevelMapState() - HandleSelection : selectedLevelID_=%d - Clic on Cinematic Replay !", selectedLevelID_);
                    UpdateActionAnimations(LEVELMAPACTION_CINEMATICREPLAY);
                    inside = true;
                }
            #endif
            }
            else if (planetMode_ == 0 && body->GetNode()->GetName().StartsWith(LABEL_ZONE))
            {
                inside = true;
                selectedLevelID_ = -1;
                UpdateActionAnimations(body->GetNode()->GetName() == LABEL_ZONE_PREVIOUS ? LEVELMAPACTION_GOTOPREVIOUSZONE : LEVELMAPACTION_GOTONEXTZONE);
            }
        }
        else
        {
            if (eventType == E_TOUCHEND || eventType == E_MOUSEBUTTONDOWN)
            {
                // allow the constellation swapping
                if (planetMode_ == 0 && eventType == E_TOUCHEND)
                {
                    int deltay = position.y_ - beginposition.y_;
                    int deltax = position.x_ - beginposition.x_;
                    int adeltax = Abs(deltax);
                    int adeltay = Abs(deltay);

                    const int TouchMoveThreshold = 200;
                    if (adeltay > TouchMoveThreshold && adeltay > 2 * adeltax)
                    {
                        if (deltay < 0 && prevZoneButton_)
                        {
                            touchbeginnode = touchendnode = 0;
                            inside = true;
                            selectedLevelID_ = -1;
                            UpdateActionAnimations(LEVELMAPACTION_GOTOPREVIOUSZONE);
        //                    GameStatics::input_->ResetStates();
                        }
                        else if (deltay > 0 && nextZoneButton_)
                        {
                            touchbeginnode = touchendnode = 0;
                            inside = true;
                            selectedLevelID_ = -1;
                            UpdateActionAnimations(LEVELMAPACTION_GOTONEXTZONE);
        //                    GameStatics::input_->ResetStates();
                        }
                    }
                }
                // allow to change to a nearest planet/mission
                else if (planetMode_ == 1 && !touchmultigesture && selectedLevelID_ != -1)
                {
                    const LevelGraphPoint* point = levelGraph_->GetOrderedPoints()[selectedLevelID_-firstMissionID_];
                    const Vector<LevelGraphPoint* >& linkedpoints = point->linkedpoints_;
                    GameStatics::MissionState* mstates = GameStatics::playerState_->missionstates;

                    int missionid = selectedLevelID_;
                    float minangle = 360.f;
                    Vector2 length = wposition - selectablenodes_[selectedLevelID_-firstMissionID_]->GetWorldPosition2D();
                    float pointangle = Atan(length.y_ / length.x_);
                    if (length.x_ < 0.f)
                        pointangle = 180.f + pointangle;

                    // for each mission calculate the angle and compare with ref angle
                    // find the mission (which have the smallest delta in the angle)
                    for (int j=0; j < linkedpoints.Size(); j++)
                    {
                        const LevelGraphPoint* linkedpoint = linkedpoints[j];
                        if (mstates[linkedpoint->id_-1].state_ > GameStatics::MissionState::MISSION_LOCKED)
                        {
                            length = linkedpoint->position_ - point->position_;
                            float angle = Atan(length.y_ / length.x_);
                            if (length.x_ < 0.f)
                                angle = 180.f + angle;

                            float diffangle = Abs(angle - pointangle);
    //                        URHO3D_LOGINFOF("LevelMapState() - HandleSelection : calculate missionid=%d pointangle=%F diffangle=%F", linkedpoint->id_, pointangle, diffangle);
                            if (diffangle < minangle)
                            {
                                minangle = diffangle;
                                missionid = linkedpoint->id_;
                            }
                        }
                    }

                    if (minangle < 15.f && missionid != selectedLevelID_)
                    {
                        touchbeginnode = touchendnode;
                        lastSelectedLevelID_ = -1;
                        selectedLevelID_ = missionid;
                        inside = true;
                        move = true;

                        URHO3D_LOGINFOF("LevelMapState() - HandleSelection : E_TOUCHEND=%s select a nearest planet minangle=%F => missionid=%d", position.ToString().CString(), minangle, missionid);

                        UpdateActionAnimations(LEVELMAPACTION_CHANGESELECTEDLEVEL);
                    }
                }
            }

//            if (eventType == E_MOUSEMOVE && leveltxt_)
//                leveltxt_->SetEnabled(false);
        }
    }

    else if (eventType == E_MULTIGESTURE || eventType == E_MOUSEWHEEL)
    {
        inside = launch = false;

        if (eventType == E_MULTIGESTURE)
        {
            touchmultigesture = true;
            touchbeginnode = 0;
            int numfingers = eventData[MultiGesture::P_NUMFINGERS].GetInt();

            if (numfingers > 1)
            {
                float angle = eventData[MultiGesture::P_DTHETA].GetFloat();
                float dist = eventData[MultiGesture::P_DDIST].GetFloat() / PIXEL_SIZE;

//                URHO3D_LOGINFOF("LevelMapState() - HandleSelection : E_MULTIGESTURE numfingers=%d dist=%F angle=%F!", numfingers, dist, angle);

                if ((dist > 1.f && planetMode_ == 0) ||
                    (dist < -1.f && planetMode_ == 1))
                {
                    if (selectedLevelID_ < firstMissionID_)
                        selectedLevelID_ = firstMissionID_;
                    else if (selectedLevelID_ > lastMissionID_)
                        selectedLevelID_ = lastMissionID_;

                    UpdateActionAnimations(LEVELMAPACTION_CHANGESELECTEDLEVEL);
                    SwitchPlanetMode();
                }
            }
        }
        else if (eventType == E_MOUSEWHEEL)
        {
            if ((eventData[MouseWheel::P_WHEEL].GetInt() > 0 && planetMode_ == 0) ||
                (eventData[MouseWheel::P_WHEEL].GetInt() < 0 && planetMode_ == 1))
            {
                if (selectedLevelID_ < firstMissionID_)
                    selectedLevelID_ = firstMissionID_;
                else if (selectedLevelID_ > lastMissionID_)
                    selectedLevelID_ = lastMissionID_;
                UpdateActionAnimations(LEVELMAPACTION_CHANGESELECTEDLEVEL);
                SwitchPlanetMode();
            }
        }

        return;
    }

    /// Set KeyBoard/JoyPad Mouvements in Menu Items
    else if (eventType == E_KEYUP || eventType == E_JOYSTICKAXISMOVE || eventType == E_JOYSTICKHATMOVE || eventType == E_JOYSTICKBUTTONDOWN)
    {
        if (eventType == E_KEYUP)
        {
            int key = eventData[KeyUp::P_KEY].GetInt();

            if (key == KEY_SPACE || key == KEY_RETURN)
                launch = true;
            else if (key == KEY_TAB)
                SwitchPlanetMode();
            else if ((lastaction_ == LEVELMAPACTION_GOTOPREVIOUSZONE && key == KEY_DOWN) ||
                     (lastaction_ == LEVELMAPACTION_GOTONEXTZONE && key == KEY_UP))
                launch = true;
        }

        if (eventType == E_JOYSTICKBUTTONDOWN && eventData[JoystickButtonDown::P_BUTTON] == 0) // PS4controller = X
            launch = true;

        if (!launch)
        {
            int inc = 1;

            if (selectedLevelID_ == -1)
            {
                selectedLevelID_ = GameStatics::playerState_->level;
                inc = 0;
                move = true;
            }
            else
            {
                if (eventType == E_KEYUP)
                {
                    URHO3D_LOGINFOF("LevelMapState() - HandleSelection : test hasfocus=%s !", GetSubsystem<Input>()->HasFocus() ? "true" : "false");

                    int key = eventData[KeyUp::P_KEY].GetInt();
                    if (key == KEY_DOWN)
                        inc = -1;

                    move = key == KEY_UP || key == KEY_DOWN;
                }

                if (eventType == E_JOYSTICKAXISMOVE)
                {
                    move = abs(eventData[JoystickAxisMove::P_POSITION].GetFloat()) >= 0.9f;
                    if (move && eventData[JoystickAxisMove::P_POSITION].GetFloat() > 0.f)
                        inc = -1;
                }

                if (eventType == E_JOYSTICKHATMOVE)
                {
                    int state = eventData[JoystickHatMove::P_POSITION].GetInt();
                    move = state & HAT_UP || state & HAT_DOWN;
                    if (move && state & HAT_DOWN)
                        inc = -1;
                }
            }

            if (move)
            {
                URHO3D_LOGINFOF("LevelMapState() - HandleSelection : inc=%d selectedLevelID_=%d firstMissionID_=%d lastMissionID_=%d!",
                                inc, selectedLevelID_, firstMissionID_, lastMissionID_);

                if (selectedLevelID_+inc < 0 || selectedLevelID_+inc > NBMAXLVL)
                    move = false;
                else
                {
                    if (GameStatics::playerState_->missionstates[selectedLevelID_+inc-1].state_ != GameStatics::MissionState::MISSION_LOCKED)
                    {
                        int newaction = 0;

                        if ((inc <= 0 && selectedLevelID_ > firstMissionID_) || (inc >= 0 && selectedLevelID_ < lastMissionID_))
                        {
                            lastSelectedLevelID_ = selectedLevelID_;
                            selectedLevelID_ += inc;

                            newaction = LEVELMAPACTION_STARTLEVEL;
                        }
                        else
                        {
                            selectedLevelID_ = -1;
                            newaction = inc < 0 ? LEVELMAPACTION_GOTOPREVIOUSZONE : LEVELMAPACTION_GOTONEXTZONE;
                        }

                        UpdateActionAnimations(newaction);
                    }
                    else move = false;
                }

                URHO3D_LOGINFOF("LevelMapState() - HandleSelection : move=%d", move);
            }
        }
    }

    if (inside)
    {
        if (eventType == E_MOUSEBUTTONDOWN)
        {
            launch = (eventData[MouseButtonDown::P_BUTTON].GetInt() == MOUSEB_LEFT);
        }
        else if (eventType == E_TOUCHEND)
        {
            launch = !touchmultigesture && touchbeginnode == touchendnode;
        }
    }

    /// Launch Action
    if (launch)
        StartState(lastaction_);
}

void LevelMapState::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;

    switch (eventData[P_KEY].GetInt())
    {
    case KEY_G:
        drawDebug_ = !drawDebug_;
        URHO3D_LOGINFOF("LevelMapState() - HandleKeyDown : KEY_G : Debug=%s", drawDebug_ ? "ON":"OFF");
        if (drawDebug_) SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(LevelMapState, OnPostRenderUpdate));
        else UnsubscribeFromEvent(E_POSTRENDERUPDATE);
        break;
    case KEY_PAGEUP :
        if (GameStatics::camera_)
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 1.01f);
        break;
    case KEY_PAGEDOWN :
        if (GameStatics::camera_)
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 0.99f);
        break;
    }

    // do escape here only (not with eventdata => prevent residual events from last state)
    if (GetSubsystem<Input>()->GetKeyPress(KEY_ESCAPE))
    {
        if (((OptionState*)stateManager_->GetState("Options"))->IsFrameOpened())
            ((OptionState*)stateManager_->GetState("Options"))->CloseFrame();
        else
            ReturnMainMenu();
    }
}

void LevelMapState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    if (mapscene_)
        ResetMap(true);

#ifdef ACTIVE_MISSIONDIALOG
    if (dialogbox_)
        dialogbox_->Resize();
#endif
}

void LevelMapState::HandleAccessMenu(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("LevelMapState() - HandleAccessMenu !");
    if (eventType == UIMENU_SHOWCONTENT)
    {
        CloseInteractiveFrames();

//        UnselectLevel();
        GameStatics::AllowInputs(false);

        if (uilevelmap_)
            Game::Get()->HideHeader(uilevelmap_);
    }
    else if (eventType == UIMENU_HIDECONTENT)
    {
        if (!static_cast<OptionState*>(stateManager_->GetState("Options"))->IsFrameOpened())
            GameStatics::AllowInputs(true);

        if (uilevelmap_)
            Game::Get()->ShowHeader(uilevelmap_);
    }
}


void LevelMapState::OnAccessMenuOpenFrame(bool state)
{
    GameStatics::AllowInputs(!state);
    GameStatics::rootScene_->SetUpdateEnabled(!state);
}

void LevelMapState::OnPostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (GameStatics::rootScene_)
    {
        Octree* tree = GameStatics::rootScene_->GetComponent<Octree>();
        tree->DrawDebugGeometry(true);
//        if (GameStatics::gameConfig_.debugLights_)
//        {
//            PODVector<Light*> lights;
//            GameStatics::rootScene_->GetComponents<Light>(lights,true);
////            URHO3D_LOGINFOF("nb Light=%d",lights.Size());
//            for (unsigned i = 0; i < lights.Size(); ++i)
//                lights[i]->DrawDebugGeometry(GameStatics::rootScene_->GetComponent<DebugRenderer>(),true);
//        }
//        if (GameStatics::gameConfig_.debugAnimatedSprite2D)
        {
            PODVector<AnimatedSprite2D*> drawables;
            GameStatics::rootScene_->GetComponents<AnimatedSprite2D>(drawables, true);
//            URHO3D_LOGINFOF("nb AnimatedSprite2D=%d", drawables.Size());
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(GameStatics::rootScene_->GetComponent<DebugRenderer>(), true);
        }
//        {
//            PODVector<StaticSprite2D*> drawables;
//            GameStatics::rootScene_->GetComponents<StaticSprite2D>(drawables, true);
//            for (unsigned i = 0; i < drawables.Size(); ++i)
//                drawables[i]->DrawDebugGeometry(GameStatics::rootScene_->GetComponent<DebugRenderer>(), true);
//        }
        if (GameStatics::gameConfig_.physics2DEnabled_ && GameStatics::gameConfig_.debugPhysics_)
        {
            PhysicsWorld2D* physicsWorld2D = GameStatics::rootScene_->GetComponent<PhysicsWorld2D>();
            if (physicsWorld2D) physicsWorld2D->DrawDebugGeometry();
        }

        //levelGraph_->DrawDebugGeometry(GameStatics::rootScene_->GetComponent<DebugRenderer>(), true);
    }

    if (GameStatics::gameConfig_.debugUI_)
    {
        GameStatics::ui_->DebugDraw(GameStatics::ui_->GetRoot());

        PODVector<UIElement*> children;
        GameStatics::ui_->GetRoot()->GetChildren(children, true);
        for (PODVector<UIElement*>::ConstIterator it=children.Begin(); it!=children.End(); ++it)
        {
            UIElement* element = *it;
            if (element->IsInternal())
                continue;
            if (element->HasTag("Console"))
                continue;

            GameStatics::ui_->DebugDraw(element);
        }

        const Vector<WeakPtr<UIElement> >& elements = GameStatics::ui_->GetElementsAlwaysOnTop();
        for (Vector<WeakPtr<UIElement> >::ConstIterator it=elements.Begin(); it!=elements.End(); ++it)
        {
            if (*it)
                GameStatics::ui_->DebugDraw(*it);
        }
    }
}
