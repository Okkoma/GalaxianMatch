#pragma once

#include "InteractiveFrame.h"

using namespace Urho3D;

enum TutorialCmd
{
    TUT_NONE,
    TUT_ARROW,
    TUT_SELECT,
    TUT_FOCUS,
    TUT_MOVE,
    TUT_ANIMATE,
    TUT_FINDPOWER,
    TUT_FINDMATCHES,
};

struct TutorialData
{

};

struct TutorialInfo
{
    TutorialInfo() { }
    TutorialInfo(Node* node, int powerid, int colorid) : node_(node), powerid_(powerid), colorid_(colorid) { }
    TutorialInfo(const TutorialInfo& i) : node_(i.node_), powerid_(i.powerid_), colorid_(i.colorid_) { }

    WeakPtr<Node> node_;
    int powerid_;
    int colorid_;
};

class Tutorial : public Object
{
    URHO3D_OBJECT(Tutorial, Object);

public :
    Tutorial(Context* context);

    virtual ~Tutorial();

    static void RegisterObject(Context* context);

    static void Reset(Context* context=0);
    static void SetDatas(const Vector<TutorialData*>& datas);

    static void Start();
    static void Stop();

    void Clear();
    void SetEnabled(bool enable);

    static Tutorial* Get() { return manager_.Get(); }

private :
    void Init();
    void Launch();
    void Next();

    void SubscribeToEvents();
    void UnsubscribeFromEvents();

    void HandleEvents(StringHash eventType, VariantMap& eventData);
    void HandleTutorialStart(StringHash eventType, VariantMap& eventData);
    void HandleTutorialFrameStopped(StringHash eventType, VariantMap& eventData);
    void HandleTutorialLaunch(StringHash eventType, VariantMap& eventData);
    void HandleTutorialNext(StringHash eventType, VariantMap& eventData);

    Vector<TutorialData*> datas_;
    List<TutorialInfo> tutorialInfos_;
    WeakPtr<InteractiveFrame> frame_;

    bool enabled_;

    static SharedPtr<Tutorial> manager_;
};
