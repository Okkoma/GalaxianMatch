#pragma once

namespace Urho3D
{
    class Object;
    class Node;
}

using namespace Urho3D;


const String INTERACTIVES_ROOT("Interactives_Root");

enum InteractiveSelectionMode
{
    SELECTIONONMOVE = 0,
    SELECTIONONCLICK = 1
};

enum InteractiveBehavior
{
    IB_MESSAGEBOX = 0,
    IB_ABILITIES,
    IB_BUTTON,
};

enum InteractiveFrameState
{
    IFrame_Cleaned = 0,
    IFrame_Starting,
    IFrame_Started,
    IFrame_Stopped,
};

struct Slot;

class InteractiveFrame : public Object
{
    URHO3D_OBJECT(InteractiveFrame, Object);

public:
    InteractiveFrame(Context* context, const String& layoutnodefile);
    virtual ~InteractiveFrame();

    static InteractiveFrame* Get(const String& layoutfilename, bool inWaitingState=false);
    static void SetAllowInputs(bool enable) { allowInputs_ = enable; }
    static void FreeAll();
    static void Reset();

    static bool HasRunningInstances() { return numRunningInstances_ > 0; }

    void Init();
    void Start(bool force, bool instant);
    void Stop();
    void Clean();

    void SetBehavior(int behavior);
    void SetScreenPosition(const IntVector2& position, bool instant=false);
    void SetScreenPositionEntrance(const IntVector2& position);
    void SetScreenPositionExit(int index, const IntVector2& position);
    void SetScreenPositionEntranceForBonus(const IntVector2& position);
    void SetReleaseSelectionOnContactEnd(bool state);
    void SetLayer(int layer);
    void SetSelectionMode(int mode);
    void SetAutoStart(float starttime);
    void SetAutoStop(float stoptime);
    void SetBreakInput(bool enabled);
    void SetText(const String& name, const String& text, bool localization);
    void SetBonusEnabled(bool state);
    void SetBonusStart(float time);
    void SetAbilityMoveZone(int dleft, int dright, float dy);

    void AddAbility(int ability, bool autostart=false);
    void RemoveAbility(int ability);
    void MoveAbilities(const Vector2& offset);

    void AddBonus(const Slot& type);
    const Vector<Slot >& GetBonuses() const { return bonuses_; }

    bool IsStarted() const { return state_ == IFrame_Started; }

    const String& GetName() const { return framename_; }
    Node* GetNode() const { return node_; }
    Node* GetBonusNode() const { return nodeBonus_; }

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

private:
    bool IsInteractivesEnabled() const;

    void FindAbilities();
    void UpdateAbilities();

    void ResetAnimations();
    void SetLayering(Node* node, int layer);
    void SetItemState(Node* node,int state);

    void SubscribeToInputEvents();
    void SubscribeToBreakInputEvents();

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void HandleInput(StringHash eventType, VariantMap& eventData);
    void HandleBreakInput(StringHash eventType, VariantMap& eventData);
    void HandleAutoStart(StringHash eventType, VariantMap& eventData);
    void HandleAutoStop(StringHash eventType, VariantMap& eventData);
    void HandleClean(StringHash eventType, VariantMap& eventData);

    void OnAbilityReseted(StringHash eventType, VariantMap& eventData);
    void OnAbilityAdded(StringHash eventType, VariantMap& eventData);
    void OnAbilityRemoved(StringHash eventType, VariantMap& eventData);

    bool breakinput_, releaseSelectionContactEnd_, bonusEnabled_;

    int state_, behavior_, selectionMode_, clicsOnSelection_;
    int layer_, lastSelectedId_, selectedId_;
    int exitindex_;

    float autostarttime_, autostoptime_, bonusstarttime_;

    Vector3 position_, positionentrance_, positionentrancebonus_;
    Vector3 positionexit_[2];

    Rect abilityMoveRect_;

    WeakPtr<Node> node_;
    WeakPtr<Node> nodeBonus_;
    Vector<Node* > interactiveItems_;
    Vector<Slot > bonuses_;
    Vector<Node* > bonusItems_;
    PODVector<Node*> abilities_;
    Node* selectedNode_;
    Node* lastSelectedNode_;

    String layoutname_;
    String framename_;

    static int numRunningInstances_;
    static bool allowInputs_;
    static HashMap<String, Vector<SharedPtr<InteractiveFrame> > > instances_;
};

