#pragma once

#include <Urho3D/Core/Timer.h>
//#include <Urho3D/Scene/Node.h>

namespace Urho3D
{
    class Node;
    class DebugRenderer;
}

using namespace Urho3D;

#include "TimerSimple.h"
#include "Matches.h"
#include "GameStatics.h"

#define POINTS_ByDestroy 50U
#define POINTS_BySuccess 100U
#define POINTS_ForHighSuccess 200U

enum MatchState
{
    NoMatchState = 0,
    StartSelection,
    SelectionAnimation,
    CancelMove,
    SuccessMatch,
};

enum HintState
{
    HintNoMove = 0,
    HintAnimate,
};

enum NetPlayMod : int
{
    NETPLAY_COLLAB = 0,
    NETPLAY_CONFRONT
};

enum NetUsage : int
{
    NETLOCAL = 0,
    NETREMOTE,
};

#define MIN_CHARECHAP 33

enum NetCommand : int
{
    NETCMD_START = 59,

    NETMATCH_SELECT  = 60,
    NETMATCH_MOVE    = 61,
    NETMATCH_RESET   = 62,
    NETITEM_ACQUIRE  = 63,
    NETABILITY_APPLY = 64,
    NETSTATE_RESET   = 65,
    NETGRID_SET      = 66,

    NETCMD_END = 67,
};

struct NetCommandData
{
    void WriteToBuffer(VectorBuffer& buffer) const;

    NetCommand cmd_;
    VectorBuffer params_;
};

class MatchGridInfo : public RefCounted
{
public:
    MatchGridInfo();
    virtual ~MatchGridInfo();

    void Init();
    void ResetObjectives();
    void Create();
    void CreateObjects();
    void SpawnObjects();
    void StartFirstMoves();

    MatchObjective* GetObjective(StringHash got);
    void AddObjective(StringHash got, unsigned char matchType, int numitems);
    bool CheckObjective(Match* match);

    bool IsGridCreated() const;
    bool IsMoveValidated(const Match& m1, const Match& m2) const;
    bool IsMoveValidated_Boss(const Match& m1, const Match& m2) const;

    void ResetState();
    void ChangeState(int state);
    void ResetSelection();
    void ResetScaleOnSelection();
    void ResetHints();

    void SetHintsAnimations(bool state);
    void SelectStartMatch(const Match& m1);

    void ResetAbility();
    bool CheckCurrentAbilityOn(const Match& match);
    void ApplyCurrentAbilityOn(Match* match);

    void CheckHints(bool state);

    void MoveSelection();
    void ConfirmSelection();
    void ConfirmSelection_Boss();

    bool FindMatches(const Vector<Match*>& tocheck);
    void ApplySuccessMatches();

    void RemoveMatchAndCollapse(const Match& match);
    void AcquireItems(const Match& match);
    void AcquirePower(const Match& match);

    inline void SelectMatch(Match*& match, const IntVector2& screenposition);
    inline void SelectMatch(Match*& match, const Vector2& worldposition);
    inline void GetSecondSelection(Match*& newHit);

    void Net_ReceiveCommands(VectorBuffer& buffer);
    NetCommandData* Net_PrepareCommand(NetCommand cmd);
    void Net_SendCommands();
    void Net_SendGrid();
    void Net_UpdateControl();

    void UpdateControl();
    void UpdateControl_Boss();
#ifdef ACTIVE_TESTMODE
    void UpdateControl_Test();
#endif

    void Update_Initial();
    void Update();
    void Update_Boss();
#ifdef ACTIVE_TESTMODE
    void Update_Test();
#endif

    void UpdateItems();
    void UpdateHints();

    MatchGrid mgrid_;

    int netusage_;
    List<NetCommandData> netReceivedCommands_;
    Vector<NetCommandData> netTosendCommands_;
    VectorBuffer preparedCommands_;

    /// the found matches from Selection
    Vector<Match*> destroymatches_;
    Vector<Match*> successmatches_;
    Vector<Match*> brokenrocks_;
    Vector<WallInfo> hittedwalls_;
    /// the bonuses to activate from success matches
    Vector<Match*> activablebonuses_;
    /// current collapsed matches
    Vector<Match*> collapsematches_;
    /// current matches to check
    Vector<Match*> startMatches_;
    Vector<Match*> matchesToCheck_;
    /// hints
    Vector<Vector<Match*> > hints_;
    /// registered objectives
    Vector<MatchObjective > objectives_;

    /// metrics
    int moveCount_;
    int state_;
    int abilitySelected_;
    int hintstate_;
    int successTurns_, turnProcessed_;
    bool successtoProcess_;
    unsigned turnScore_;
    float turnTime_;
    int ishowhints_;
    int ihintsearchmatch_;
    int ilasthint_;
    TimerSimple hintsearchTimer_;
    Timer animationTimer_, pauseTimer_, hintTimer_, updateitemsTimer_;
    float currentTime_, initialTime_;

    Match selected_[2];

    bool hintsenabled_;
    bool unblockinput_;
    bool canUnselect_;
    bool objectiveDirty_;
    bool allowCheckObjectives_;
    bool tutorialStateSaved_;
#ifdef ACTIVE_TESTMODE
    bool testModeNextMove_;
#endif
    IntVector2 inputposition_;
    Vector2 hitposition_;
};

class MatchesManager : public Object
{
    URHO3D_OBJECT(MatchesManager, Object);

    friend class MatchGridInfo;

public :
    MatchesManager(Context* context);

    virtual ~MatchesManager();

    static void RegisterObject(Context* context);
#ifdef ACTIVE_TESTMODE
    static void SwitchTestMode();
#endif
    static void SetAuthorizedObjects(const StringHash& category, const Vector<StringHash>& types);
    static void SetNetPlayMod(int netplaymod);
    static void AddGrid(int netusage);
    static void RemoveGrid(int netusage);
    static void SetLayout(int size, GridLayout layout);
    static void SetPhysicsEnable(bool enable);
    static void RegisterObjective(StringHash got, int numitems);
    static void SetObjectiveRemainingQty(unsigned char index, int newvalue, int gridid=0);
    static void UpdateGridPositions();
    static void SetBackGround();
    static void ResizeBackGround();

    static void Reset(Context* context=0);
    static void Start();
    static void Stop();
    static void Clear();

    static void ResetObjects(int gridid=0);
    static void CheckAllowShake();
    static void ShakeMatches(int gridid=0);
    static void SelectAbility(int ability, int gridid=0);
    static void DestroyMatch(const Vector2& position);

    static void SubscribeToEvents();
    static void UnsubscribeFromEvents();

    // General Getters
    static MatchesManager* Get()
    {
        return manager_;
    }
    static const Vector<StringHash>& GetAuthorizedObjects(const StringHash& category)
    {
        return MatchGrid::authorizedTypes_[category];
    }
    static int GetNetPlayMod() { return manager_ ? manager_->netplaymod_ : NETPLAY_COLLAB; }

    static const StringHash& GetAuthorizedObjectByIndex(const StringHash& category, unsigned index);

    // MatchGrid Getters
    static MatchGridInfo* GetGridInfo(int gridid=0)
    {
        return gridid < manager_->gridinfos_.Size() ? manager_->gridinfos_[gridid].Get() : nullptr;
    }
    static unsigned GetNumGrids()
    {
        return manager_->gridinfos_.Size();
    }
    static MatchGrid& GetGrid(int gridid=0)
    {
        return GetGridInfo(gridid)->mgrid_;
    }
    static int GetState(int gridid=0)
    {
        return GetGridInfo(gridid)->state_;
    }
    static float GetElapsedTime(int gridid=0);
    static unsigned GetTurnScore(int gridid=0)
    {
        return GetGridInfo(gridid)->turnScore_;
    }

    static const Vector<MatchObjective >& GetObjectives(int gridid=0);
    static int GetObjectiveIndex(StringHash got, int gridid=0);
    static int GetObjectiveIndex(unsigned char ctype, int gridid=0);
    static int GetObjectiveCounter(unsigned char index, int gridid=0);
    static int GetObjectiveRemainingQty(unsigned char index, int gridid=-1);

    static int GetNumMovesUsed(int gridid=0);
    static Node* GetRandomObject(int gridid=0);
    static unsigned GetAllItemsOnGrid(int gridid=0);
    static void GetPowerBonusesOnGrid(float ratio, int gridid=0);
    static unsigned GetNumPossibleMatches(int gridid=0);

    static Node* GetWorldGridNode(int x, int y, int gridid=0);

    Match* GetMatchAtPosition(const Vector2& position, int* gridid=0);
    const Match& GetMatchHitted(const Vector2& position, int* gridid=0) const;

    void DrawDebugGeometry(DebugRenderer* debug, bool depth=false) const;

private :
    void Init();

    void HandleUpdateInitial(StringHash eventType, VariantMap& eventData);
    void HandleUpdateClassicMode(StringHash eventType, VariantMap& eventData);
    void HandleUpdateBossMode(StringHash eventType, VariantMap& eventData);
#ifdef ACTIVE_TESTMODE
    void HandleUpdateTestMode(StringHash eventType, VariantMap& eventData);
#endif
    void HandleShakeButtonPressedAck(StringHash eventType, VariantMap& eventData);

    /// all the Grids local & network
    Vector<SharedPtr<MatchGridInfo > > gridinfos_;

    int netplaymod_;
    int gridsize_;
    GridLayout gridlayout_;

    static float sceneScale_;
#ifdef ACTIVE_TESTMODE
    static bool testMode_;
#endif
    static bool allowSelectionAlongHV_;
    static bool allowHints_, checkallpowersonturn_;
    static SharedPtr<MatchesManager> manager_;
};
