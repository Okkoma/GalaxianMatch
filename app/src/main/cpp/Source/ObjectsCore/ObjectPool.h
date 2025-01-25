#pragma once

#include <Urho3D/Container/List.h>

#include "DefsGame.h"


using namespace Urho3D;


class ObjectPool;


static const unsigned OBJECTPOOL_FIRSTREPLICATEDNODEID = 0x00000020;
static const unsigned OBJECTPOOL_FIRSTREPLICATEDCOMPONENTID = 0x00000020;
static const unsigned OBJECTPOOL_FIRSTLOCALNODEID = 0x01000100;
static const unsigned OBJECTPOOL_FIRSTLOCALCOMPONENTID = 0x01001000;

struct NodeComponentID
{
    NodeComponentID() : nodeID_(0), componentID_(0) { }
    NodeComponentID(unsigned nodeid, unsigned componentid) : nodeID_(nodeid), componentID_(componentid) { }
    NodeComponentID(const NodeComponentID& n) : nodeID_(n.nodeID_), componentID_(n.componentID_) { }
    unsigned nodeID_;
    unsigned componentID_;
};

class ObjectPoolCategory : public RefCounted
{
friend class ObjectPool;

public :
    ObjectPoolCategory();
    ObjectPoolCategory(const ObjectPoolCategory& obj);
    ~ObjectPoolCategory();

    ObjectPoolCategory& operator = (const ObjectPoolCategory& obj) { return *this; }

    bool Create(bool replicate, const StringHash& GOT, Node* nodePool, Node* templateNode, unsigned* ids);
    void Resize(unsigned size);
    void SetIds(CreateMode mode, unsigned firstNodeID=0, unsigned firstComponentID=0);
    void SynchronizeReplicatedNodes(unsigned startNodeID);
    void SetCurrentReplicatedIDs(unsigned id);

    void RestoreObjects(bool allObjects=false);

    bool HasReplicatedMode() const;
    bool IsSet() const { return GOT_ && nodes_.Size() != 0; }
    bool IsNodeInPool(Node* node) const;

    unsigned GetNextReplicatedNodeID() const;
    unsigned GetFirstNodeID(CreateMode mode) const { return mode == REPLICATED && replicatedState_ ? firstReplicatedNodeID_ : firstNodeID_; }
    unsigned GetLastNodeID(CreateMode mode) const { return mode == REPLICATED && replicatedState_ ? lastReplicatedNodeID_ :  lastNodeID_; }
    unsigned GetFirstComponentID(CreateMode mode) const { return mode == REPLICATED && replicatedState_ ? firstReplicatedComponentID_ : firstComponentID_; }
    unsigned GetLastComponentID(CreateMode mode) const { return mode == REPLICATED && replicatedState_ ? lastReplicatedComponentID_ :  lastComponentID_; }
    unsigned GetSize() const { return nodes_.Size(); }
    unsigned GetFreeSize() const { return freenodes_.Size(); }

    Node* GetPoolNode(unsigned id=0);
    Node* GetPoolNode(CreateMode mode=LOCAL);
    bool FreePoolNode(Node* node, bool cleanDependences=false);
    void ChangeToReplicatedID(Node* node, unsigned newid=0);

    void Dump(bool logonlyerrors=false) const;

    unsigned firstNodeID_, lastNodeID_;
    unsigned firstComponentID_, lastComponentID_;
    unsigned firstReplicatedNodeID_, lastReplicatedNodeID_;
    unsigned firstReplicatedComponentID_, lastReplicatedComponentID_;

private :
    bool Update(HiresTimer* timer, const long long& delay);

    StringHash GOT_;
    const GOTInfo* gotinfo_;
    unsigned requestedSize_;
    int updateState_;

    WeakPtr<Node> nodeCategory_;
    WeakPtr<Node> template_;
    Vector<SharedPtr<Node> > nodes_;
    PODVector<Node* > freenodes_;
    bool replicatedState_;

    HashMap<unsigned, NodeComponentID > usedLocalIds_;
    unsigned numNodeIdsByObject_;
    unsigned numComponentIdsByObject_;
    unsigned currentReplicatedNodeID_;
    unsigned currentReplicatedComponentID_;
};

class ObjectPool : public Object
{
    URHO3D_OBJECT(ObjectPool, Object);

public :
    ObjectPool(Context* context);
    ~ObjectPool();

    ObjectPoolCategory* CreateCategory(const GOTInfo& info, Node* templateNode, unsigned* ids);
    void ResizeCategory(ObjectPoolCategory& category, unsigned size);
    bool CreateCategories(Scene* scene, const HashMap<StringHash, GOTInfo >& infos, const HashMap<StringHash, WeakPtr<Node> >& templates, HiresTimer* timer, const long long& delay);

    ObjectPoolCategory* GetCategory(const StringHash& GOT);
    bool AreCategoriesUpdated() const { return categoriesToUpdate_.Size() == 0; }

    void RestoreCategories(bool allObjects=false);
    void Stop();

    const HashMap<StringHash, ObjectPoolCategory >& GetCategories() const { return categories_; }
    void DumpCategories() const;

    static void SetCreateChildMode(CreateMode mode);
    static ObjectPool* Get() { return pool_; }
    static const String& GetDebugData() { UpdateDebugData(); return debugTxt_; }
    static void Reset(Node* node=0);

    static Node* CreateChildIn(const StringHash& GOT, Node* parent=0, unsigned id=0, int viewZ=-1, const NodeAttributes* nodeAttr=0, bool applyAttr=false, ObjectPoolCategory** category=0);
    static void ChangeToReplicatedID(Node* node, unsigned newid);
    static bool Free(Node* node);

    static CreateMode createChildMode_;

private :
    static void UpdateDebugData();

    WeakPtr<Node> nodePool_;
    HashMap<StringHash, ObjectPoolCategory > categories_;

    List<ObjectPoolCategory* > categoriesToUpdate_;

    unsigned firstLocalNodeID_;
    unsigned lastLocalNodeID_;
    unsigned firstReplicatedNodeID_;
    unsigned lastReplicatedNodeID_;
    unsigned firstLocalComponentID_;
    unsigned lastLocalComponentID_;
    unsigned firstReplicatedComponentID_;
    unsigned lastReplicatedComponentID_;

    int createstate_;

    static ObjectPool* pool_;
    static String debugTxt_;
};
