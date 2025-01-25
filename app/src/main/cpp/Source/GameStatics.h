#pragma once

#define NBMAXLVL 159
#define NBMAXZONE 13
#define NBMAXMESSAGES 150

#include "GameOptions.h"

namespace Urho3D
{
    class Context;
    class Graphics;
    class Input;
    class Viewport;
    class Scene;
    class Node;
    class Camera;
    class Light;
    class UI;
    class UIElement;
    class Sprite;
    class Cursor;
    class Plane;
    class Renderer2D;
}

using namespace Urho3D;

const int gameDataVersion_         = 1;
const int gameVersion_             = 1;
const int gameVersionMinor_        = 33;

const int MAXABILITIES             = 6;
const int MAXZONEOBJECTS           = 4;
const int MAXMISSIONBONUSES        = 4;
const int MAXNEWPOWERSBYLVL        = 2;
const int initialLevel             = 1;
#ifdef ACTIVE_TESTMODE
const int initialZone              = TESTMODE_STARTZONE;
#else
const int initialZone              = 1;
#endif

const int initialTries = 4;
const int initialMoves = 10;
const int initialCoins = 2;

const int LayoutDifficulty[10] = { 0, 1, 0, 1, 2, 2, 3, 3, 3, 3 };

static int NBMAXBOSSES;

const float MESSAGEPOSITIONYRATIO = 0.2f;

class GameStateManager;

struct RewardEventItem;

struct Slot
{
    Slot() : cat_(0U), type_(0U), qty_(0), indexcat_(-1) { }
    Slot(const StringHash& cat, int index=-1, int qty=1, bool settype=false);
    Slot(const StringHash& cat, const StringHash& type, int qty=1, bool setindex=false);
    Slot(const Slot& s) : cat_(s.cat_), type_(s.type_), qty_(s.qty_), indexcat_(s.indexcat_) { }

    void Reset();
    void SetIndex();
    String GetString() const;

    unsigned cat_;
    unsigned type_;
    int qty_;
    int indexcat_;

    static Slot ITEMS_2MOVES;
    static Slot ITEMS_4MOVES;
    static Slot ITEMS_2COINS;
};

struct RewardEventItem
{
//    RewardId :
//
//    0       : no reward
//    1 ... 4 : 1 star ... 4 stars
//    5 ... 8 : 1 coin ... 4 coins

    Slot GetSlot()
    {
        rewardid_ = rewardid_ > 8 ? 0 : rewardid_;

        if (rewardid_ == 0)
            return Slot();
        if (rewardid_ < 5)
            return Slot(StringHash("Stars"), StringHash("star1"), rewardid_, true);

        return Slot(StringHash("Coins"), StringHash("coin1"), rewardid_ - 4, true);
    }

    int rewardid_;
    bool interactive_;
};

union PowerState
{
    struct
    {
        unsigned state_;
    };
    struct
    {
        unsigned char enabled_;
        unsigned char shown_;
        unsigned char qty_;
        unsigned char empty_;
    };
};

struct LevelData
{
    int layoutShape_;
    int layoutSize_;
    int levelInfoID_;
    int rockChance_;
    int wallChance_;
    int musicThemeId_;
    Vector<StringHash> authorizedEnemies_;
    Vector<StringHash> authorizedPowers_;
    Vector<StringHash> authorizedMoves_;
    Vector<StringHash> authorizedRocks_;
    Vector<String> objectiveNames_;
    Vector<unsigned> objectiveQties_;
    Vector<StringHash> objectiveTypes_;
};

struct GameConfig
{
    GameConfig();

    int language_;
	int frameLimiter_;
	int entitiesLimiter_;
    bool touchEnabled_;
    bool forceTouch_;
    bool HUDEnabled_;
	String networkMode_;
	String networkServerIP_;
	int networkServerPort_;
    bool ctrlCameraEnabled_;
    bool debugRenderEnabled_;
    bool physics3DEnabled_;
    bool physics2DEnabled_;

    bool enlightScene_;

    bool debugPhysics_;
    bool debugLights_;
    bool debugUI_;
    bool debugAnimatedSprite2D;
    bool debugGOC_BodyExploder2D;
    bool debugMatches_;

    String initState_;
    String logString;
    String appDir_;
    String saveDir_;
    int screenJoystickID_;
    int screenJoysticksettingsID_;
    bool autoHideCursorEnable_;

    bool splashviewed_;
};

const unsigned STATE_MAINMENU  = StringHash("MainMenu").Value();
const unsigned STATE_OPTIONS   = StringHash("Options").Value();
const unsigned STATE_LEVELMAP  = StringHash("LevelMap").Value();
const unsigned STATE_PLAY      = StringHash("Play").Value();
const unsigned STATE_CINEMATIC = StringHash("Cinematic").Value();

enum GameStatus
{
    MENUSTATE = 0,
    LEVELSTATE = 1,
    LEVELSTATE_CHANGEZONE,
    LEVELSTATE_CONSTELLATION,
    CINEMATICSTATE,
    PLAYSTATE_INITIALIZING,
    PLAYSTATE_LOADING,
    PLAYSTATE_FINISHLOADING,
    PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS,
    PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES,
    PLAYSTATE_SYNCHRONIZING,
    PLAYSTATE_READY,
    PLAYSTATE_STARTGAME,
    PLAYSTATE_RUNNING,
    PLAYSTATE_ENDGAME,
    MAX_GAMESTATUS
};

enum LevelMode
{
    CLASSICLEVELMODE = 0,
    BOSSMODE
};

enum NoMoreMatchAction
{
    SHAKE = 0,
    LOOSELIFE = 1,
    LOOSEGAME = 2,
};

struct LevelInfo
{
    LevelInfo() { }
    LevelInfo(int mode, int minmatches, int nomorematchaction, bool hints) : mode_(mode), minmatches_(minmatches), nomorematchaction_(nomorematchaction), hints_(hints)
    { newpowers_[0] = -1; newpowers_[1] = -1; }
    LevelInfo(const LevelInfo& i) : mode_(i.mode_), minmatches_(i.minmatches_), nomorematchaction_(i.nomorematchaction_), hints_(i.hints_)
    { newpowers_[0] = i.newpowers_[0]; newpowers_[1] = i.newpowers_[1]; }

    int mode_;
    int minmatches_;
    int nomorematchaction_;
    bool hints_;
    int newpowers_[MAXNEWPOWERSBYLVL];
};

struct MatchObjective
{
    void Reset() { type_ = StringHash::ZERO; target_ = 0; count_ = 0; ctype_ = 0; }

    StringHash type_;
    int target_;
    int count_;
    unsigned char ctype_;
};

enum UIElementSize
{
    TRIESUISIZE = 0,
    OBJECTIVEUISIZE,
    BUTTONUISIZE,
    MBUTTONUISIZE,
    BIGOBJECTIVEUISIZE,
    FONTSIZE_SMALL,
    FONTSIZE_NORMAL,
    FONTSIZE_BIG,
    FONTSIZE_HUGE,
    NUMUIELEMENTSIZE,
};

const int UIDEFAULTSIZE[NUMUIELEMENTSIZE] = { 32, 64, 64, 72, 96, 14, 20, 50, 70 };

enum SoundChannel
{
    MAINMUSIC = 0,
    SECONDARYMUSIC,
    NUMSOUNDCHANNELS,
};

enum MusicList
{
    MAINTHEME = 0,
    WINTHEME,
    WINTHEME2,
    LOOSETHEME,

    CINEMATICTHEME1,
    CINEMATICTHEME2,

    LEVELMAPTHEME1,
    LEVELMAPTHEME2,
    LEVELMAPTHEME3,

    PLAYTHEME1,
    PLAYTHEME2,
    PLAYTHEME3,

    BOSSTHEME1,
    BOSSTHEME2,
    BOSSTHEME3,

    CONSTELLATION1,
    CONSTELLATION2,
    CONSTELLATION3,

    NUMMUSICS,

    MAXCINEMATICTHEMES = 2,
    MAXLEVELMAPTHEMES = 3,
    MAXPLAYTHEMES = 3,
    MAXBOSSTHEMES = 3,
    MAXCONSTELLATIONTHEMES = 3
};

enum SoundList
{
    SND_OBJECTIVESUCCESS,
    SND_NOOBJECTIVESUCCESS,
    SND_EXPLOSION1,
    SND_EXPLOSION2,
    SND_DESTROY1,
    SND_DESTROY2,
    SND_BOSSWARNING,
    SND_ADDTRY,
    SND_REMOVETRY,
    SND_ADDCOIN,
    NUMSOUNDS,
};

enum CimematicPart
{
    CINEMATIC_NONE = -1,
    CINEMATIC_INTRO = 0,
    CINEMATIC_BRANCH,
    CINEMATIC_BOSSINTRO,
    CINEMATIC_BOSSDEFEAT,
    CINEMATIC_TOBECONTINUED
};

class GameStatics
{
public:
    /// Functions
    static void Initialize(Context* context);
    static void InitTouchInput(Context* context);
    static void SetTouchEmulation(bool enable);
	static void InitMouse(Context* context);
    static void InitSoundChannels();
    static void ResetCamera();
    static void ChangeCameraTo(Node* node, bool ortho=true);
    static void AllowInputs(bool state);
    static void UpdateViews();

    static void CreateUICursors();
    static void CreatePreloaderIcon();
    static bool PreloadResources();
    static bool UnloadResources();
    static bool IsPreloading() { return preloading_; }
    static void CheckTimeForEarningStars();
    static void AddEarnStars(int qty);

    static void Start();
    static void Stop();
    static void Exit();

    static void SetStatus(GameStatus status);
    static GameStatus GetStatus() { return gameStatus_; }
    static GameStatus GetPreviousStatus() { return prevGameStatus_; }

    static void SetGameScale();
    static void SetConsoleVisible(bool state);
    static void SetMouseVisible(bool uistate, bool osstate);
    static void ResetGameStates();
    static void SetLevelParameters(unsigned seed, int lvl, int objmax, int& layoutshape, int& layoutsize, int& numobjectives, int& totalitems);
    static int SetLevel(int level=1);
    static void SetLevelInfo(int level, int levelInfoId);
    static void SetLevelInfos();
    static void UpdateLevel();

    static void AddBonus(const Slot& slot, Object* sender, float time = 0.1f);
    static void AddBonuses(const Vector<Slot >& slots, Object* sender);

    static int GetNextUnlockedMission(bool checkunblockzone=true);
    static const LevelInfo& GetLevelInfo() { return levelInfos_[currentLevel_-1]; }
    static int GetCurrentBoss();
    static bool CanUnblockConstellation(int zone);
    static int GetMinLevelId(int zone);
    static int GetMaxLevelId(int zone);
    static unsigned GetBossLevelId(int zone);
    static bool IsBossLevel();
    static void GetMissionBonuses(int missionid, Vector<Slot >& bonuses);

    static void Dump();

    /// Vars
    // Game Config
    static const int targetwidth_;
    static const int targetheight_;
    static GameConfig gameConfig_;
    static bool gameExit_;
    static bool preloading_;
    static Sprite* preloaderIcon_;
    static int preloaderState_;
    static const long long preloadDelayUsec_;
    static long long preloadtime_;

    // Game Scene
    static Context* context_;
    static Graphics* graphics_;
    static Input* input_;
    static UI* ui_;
    static Viewport* viewport_;
    static SharedPtr<Viewport> fixedViewport_;
    static Camera* camera_;
    static Camera* fixedCamera_;
	static SharedPtr<Scene> rootScene_;
	static Renderer2D* renderer2d_;
	static WeakPtr<Node> soundNodes_[NUMSOUNDCHANNELS];
	static int currentMusic_;
    static Scene* mainMenuScene_;
    static SharedPtr<Node> cameraNode_;
    static SharedPtr<Node> fixedCameraNode_;
    static WeakPtr<Node> controllablesNode_;

    static Cursor* cursor_;

    static IntRect screenSceneRect_;
    static Vector2 fScreenSize_;
    static Vector2 gameScale_;
    static float uiScale_;

    static int playTest_;
    static bool playingTest_;
    static bool switchScaleXY_;

    // Game States
    static GameStatus gameStatus_, prevGameStatus_;
    static int numRemainObjectives_;
    static bool allowInputs_;
    static const String GAMENAME;
    static const char* saveDir_;
    static const char* savedGameFile_;
    static const char* savedPlayerFile_;
    static const char* savedPlayerMissionFile_;
    struct GameState;
    static GameState gameState_;

    static GameStateManager* stateManager_;

    // Player's States
    enum ZoneState
    {
        ZONE_BLOCKED = 0,
        ZONE_UNBLOCKED = 1,
        CONSTELLATION_UNBLOCKED = 2,
    };
    struct MissionState
    {
        enum
        {
            MISSION_LOCKED,
            MISSION_UNLOCKED,
            MISSION_COMPLETED
        };

        MissionState& operator =(const MissionState& rhs)
        {
            state_ = rhs.state_;
            score_ = rhs.score_;
            for (unsigned i=0; i < MAXOBJECTIVES; i++)
                objectives_[i] = rhs.objectives_[i];
            numMovesUsed_ = rhs.numMovesUsed_;
            elapsedTime_ = rhs.elapsedTime_;
            return *this;
        }

        int state_;
        int score_;

        // Metrics when MISSION_COMPLETED
        unsigned numMovesUsed_;
        float elapsedTime_;
        MatchObjective objectives_[MAXOBJECTIVES];

        Slot bonuses_[MAXMISSIONBONUSES];
        int linkedmissions_[4];
    };
    struct PlayerState
    {
        unsigned lastplaytime;
        unsigned score;
        int cinematicShown_[NBMAXZONE];
        int soundEnabled_;
        int musicEnabled_;
        int tutorialEnabled_;

        int level;
        int zone;
        int coins;
        int tries;
        int moves;
        int language_;

        PowerState powers_[MAXABILITIES];
        MissionState missionstates[NBMAXLVL];
        ZoneState zonestates[NBMAXZONE];
        unsigned archivedmessages_[NBMAXMESSAGES+1];
    };
    struct GameState
    {
        void Load();
        void Save();
        void Reset();

        void SetMissionState(int missionid, const MissionState& mission);
        void GetMissionState(int missionid, MissionState& mission);
        unsigned GetNextLevel(int currentlevel=0) const;
        bool UpdateMission(int missionid, int state=-1);
        int UpdateMissionScores(int missionid);
        void UnlockPowers(int missionid, PODVector<int>& unlockedpowers);
        void UpdateStoryItems();
        void ResetTutorialState();

        void Dump() const;

        PlayerState pstate_;
        bool storyitems_[MAXZONEOBJECTS];
    };

    static PlayerState* playerState_;

    static PODVector<RewardEventItem> rewardEventStack_;

    // Level's States
    static int gameMode_;
    static bool peerConnected_;
    static String netidentity_;
    static LevelData* currentLevelDatas_;
    static LevelInfo levelInfos_[NBMAXLVL];
    static const unsigned MAXZONES;
    static const Plane GroundPlane_;
    static float CameraZ_;
    static Rect MapBounds_;
    static int& currentLevel_;
    static int& currentZone_;
    static int currentZoneFirstMissionId_;
    static int currentZoneLastMissionId_;
    static int& tries_;
    static int& coins_;
    static int& moves_;

    static const char* Musics[];
    static const char* Sounds[];
};


