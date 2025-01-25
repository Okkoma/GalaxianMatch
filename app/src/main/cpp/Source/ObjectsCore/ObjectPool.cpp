#include <Urho3D/Urho3D.h>

#include <Urho3D/Container/List.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SmoothedTransform.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

//#include "DefsViews.h"

#include "GameAttributes.h"
#include "GameStatics.h"
#include "GameHelpers.h"

#include "ObjectPool.h"

#define DEFAULT_NUMOBJECTS 10


ObjectPoolCategory::ObjectPoolCategory()
{ }

ObjectPoolCategory::ObjectPoolCategory(const ObjectPoolCategory& obj)
{ }

ObjectPoolCategory::~ObjectPoolCategory()
{ }

void ObjectPoolCategory::SetIds(CreateMode mode, unsigned firstNodeID, unsigned firstComponentID)
{
    if (mode == REPLICATED)
    {
        currentReplicatedNodeID_ = firstReplicatedNodeID_ = firstNodeID;
        currentReplicatedComponentID_ = firstReplicatedComponentID_ = firstComponentID;
    }
    else
    {
        firstNodeID_ = firstNodeID;
        firstComponentID_ = firstComponentID;
    }
}

void ObjectPoolCategory::SynchronizeReplicatedNodes(unsigned startNodeID)
{
    if (startNodeID >= firstReplicatedNodeID_ && startNodeID <= lastReplicatedNodeID_)
    {
        currentReplicatedNodeID_ = startNodeID;
        currentReplicatedComponentID_ = firstReplicatedComponentID_ + (startNodeID-firstReplicatedNodeID_) * numComponentIdsByObject_;
    }
}

#define NUMLATENCYOBJECTS 0U

bool ObjectPoolCategory::HasReplicatedMode() const
{
    return replicatedState_;
}

bool ObjectPoolCategory::IsNodeInPool(Node* node) const
{
    return !node->HasTag("InUse");
}

unsigned ObjectPoolCategory::GetNextReplicatedNodeID() const
{
    return currentReplicatedNodeID_+NUMLATENCYOBJECTS+1 <= lastReplicatedNodeID_ ? currentReplicatedNodeID_ + numNodeIdsByObject_*(NUMLATENCYOBJECTS+1) : firstReplicatedNodeID_;
}

void ObjectPoolCategory::SetCurrentReplicatedIDs(unsigned id)
{
    if (currentReplicatedNodeID_ != id)
    {
        currentReplicatedNodeID_ = id;
        currentReplicatedComponentID_ = firstReplicatedComponentID_ + numComponentIdsByObject_ * (id - firstReplicatedNodeID_);
    }
}

void ObjectPoolCategory::ChangeToReplicatedID(Node* node, unsigned newid)
{
    if (!newid)
    {
        currentReplicatedNodeID_ += numNodeIdsByObject_;
        currentReplicatedComponentID_ += numComponentIdsByObject_;

        if (currentReplicatedNodeID_ > lastReplicatedNodeID_)
        {
            currentReplicatedNodeID_ = firstReplicatedNodeID_;
            currentReplicatedComponentID_ = firstReplicatedComponentID_;
        }

        usedLocalIds_[currentReplicatedNodeID_] = NodeComponentID(node->GetID(), node->GetNumComponents() ? node->GetComponents().Front()->GetID() : firstComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... before NodeIDChanged ...", GOT::GetType(GOT_).CString(), node->GetID());
        nodeCategory_->GetScene()->NodeIDChanged(node, currentReplicatedNodeID_, currentReplicatedComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... after NodeIDChanged !", GOT::GetType(GOT_).CString(), node->GetID());

        return;
    }

    if (node->GetID() == newid)
    {
        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... with newid=%u same id !", GOT::GetType(GOT_).CString(), node->GetID(), newid);
        return;
    }

    if (newid >= firstReplicatedNodeID_ && newid <= lastReplicatedNodeID_)
    {
        SetCurrentReplicatedIDs(newid);
        usedLocalIds_[currentReplicatedNodeID_] = NodeComponentID(node->GetID(), node->GetNumComponents() ? node->GetComponents().Front()->GetID() : firstComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... before NodeIDChanged with newid=%u ...", GOT::GetType(GOT_).CString(), node->GetID(), newid);
        nodeCategory_->GetScene()->NodeIDChanged(node, currentReplicatedNodeID_, currentReplicatedComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... after NodeIDChanged !", GOT::GetType(GOT_).CString(), node->GetID());
    }
    else
    {
        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... with newid=%u not in the replicatedRange=(%u->%u) ...",
                           GOT::GetType(GOT_).CString(), node->GetID(), newid, firstReplicatedNodeID_, lastReplicatedNodeID_);
    }
}

Node* ObjectPoolCategory::GetPoolNode(unsigned id)
{
    if (!freenodes_.Empty())
    {
        Scene* scene = nodeCategory_->GetScene();
        Node* node = freenodes_.Back();
        unsigned localID = node->GetID();

        node->AddTag("InUse");

        freenodes_.Pop();

        if (localID < firstNodeID_ || localID > lastNodeID_)
        {
            URHO3D_LOGERRORF("ObjectPoolCategory() - GetPoolNode : type=%s id=%u ptr=%u ... out of local category range (%u->%u) !",
                                GOT::GetType(GOT_).CString(), localID, node, firstNodeID_, lastNodeID_);
//            Dump(true);
            return 0;
        }

        // if Replicated State, change to ReplicatedIDs
        if (id != LOCAL && replicatedState_)
        {
            if (!id)
                ChangeToReplicatedID(node);
            else
                ChangeToReplicatedID(node, id);
//            URHO3D_LOGINFOF("ObjectPoolCategory() - GetPoolNode : %s(localID=%u replicatedID=%u id=%u) free=%u/%u",
//                                GOT::GetType(GOT_).CString(), localID, currentReplicatedNodeID_, id, freenodes_.Size(), nodes_.Size());
        }

//        if (id != LOCAL)
//        {
//            if (replicatedState_ && !id)
//                ChangeToReplicatedID(node);
//            else
//                ChangeToReplicatedID(node, id);

//            URHO3D_LOGINFOF("ObjectPoolCategory() - GetPoolNode : %s(localID=%u replicatedID=%u id=%u) free=%u/%u",
//                                GOT::GetType(GOT_).CString(), localID, currentReplicatedNodeID_, id, freenodes_.Size(), nodes_.Size());
//        }
//        else
//            URHO3D_LOGINFOF("ObjectPoolCategory() - GetPoolNode : %s(localID=%u) free=%u/%u", GOT::GetType(GOT_).CString(), localID, freenodes_.Size(), nodes_.Size());

        return node;
    }

    URHO3D_LOGWARNINGF("ObjectPoolCategory() - GetPoolNode : %s(hash=%u) ... No Free Node !!!", GOT::GetType(GOT_).CString(), GOT_.Value());
    return 0;
}

bool ObjectPoolCategory::FreePoolNode(Node* node, bool cleanDependences)
{
    if (!freenodes_.Contains(node))
    {
        unsigned id = node->GetID();
        unsigned localid = id;

        // if ReplicatedID => Restore to LocalID
        if (localid && replicatedState_ && localid < FIRST_LOCAL_ID)
        {
            node->CleanupReplicationStates();

            HashMap<unsigned, NodeComponentID >::Iterator it = usedLocalIds_.Find(localid);

            if (it != usedLocalIds_.End())
            {
                {
//                    URHO3D_LOGWARNINGF("ObjectPoolCategory() - FreePoolNode : %s(%u) ... before NodeIDChanged !", GOT::GetType(GOT_).CString(), id);
                    const NodeComponentID& localids = it->second_;
                    nodeCategory_->GetScene()->NodeIDChanged(node, localids.nodeID_,  localids.componentID_);
                    localid = node->GetID();
//                    URHO3D_LOGWARNINGF("ObjectPoolCategory() - FreePoolNode : %s(%u) ... after NodeIDChanged !", GOT::GetType(GOT_).CString(), id);
                }
                it = usedLocalIds_.Erase(it);
            }
            else
            {
                URHO3D_LOGERRORF("ObjectPoolCategory() - FreePoolNode : %s(%u) ... don't find a localID to restore to !", GOT::GetType(GOT_).CString(), id);
                return false;
            }
        }

//        URHO3D_LOGINFOF("ObjectPoolCategory() - FreePoolNode : %s(%u-local=%u) ...", GOT::GetType(GOT_).CString(), id, localID);

        if (localid < firstNodeID_ || localid > lastNodeID_)
        {
            URHO3D_LOGERRORF("ObjectPoolCategory() - FreePoolNode : type=%s localid=%u(%u) ptr=%u ... out of local category range (%u->%u) : Remove it !",
                                GOT::GetType(GOT_).CString(), localid, id, node, firstNodeID_, lastNodeID_);
//            Dump(true);
            node->Remove();
            return false;
        }

        // clean dependences in the components (ex : clear triggers in animatedSprite2D)
        if (cleanDependences)
        {
            const Vector<SharedPtr<Component> >& components = node->GetComponents();
            for (Vector<SharedPtr<Component> >::ConstIterator it=components.Begin(); it!=components.End(); ++it)
                (*it)->CleanDependences();
        }

        GameHelpers::CopyAttributes(template_, node, false);
        node->RemoveAllTags();

        /// Restore poolnode to Category node
        // Prevent SceneCleaner to Remove ObjectPool Nodes
        // For Example : GOC_BodyExploder2D reparent ExplodedParts to the mapnode
        // For Example : GOC_Animator2D::SpawnAnimation reparent animation to the localmapnode
        if (node->GetParent() != nodeCategory_)
            nodeCategory_->AddChild(node);

        node->SetEnabled(false);

        node->ApplyAttributes();

        freenodes_.Push(node);

//        URHO3D_LOGINFOF("ObjectPoolCategory() - FreePoolNode : type=%s localid=%u(%u) ptr=%u restored to the pool (free=%u/%u) !",
//                GOT::GetType(GOT_).CString(), localid, id, node, freenodes_.Size(), nodes_.Size());

        return true;
    }

    return false;
}

bool ObjectPoolCategory::Create(bool replicate, const StringHash& got, Node* nodePool, Node* templateNode, unsigned* ids)
{
    template_.Reset();
    nodes_.Clear();
    freenodes_.Clear();

    if (!templateNode)
    {
        return false;
    }

    replicatedState_ = replicate;
    template_ = templateNode;
    GOT_ = got;
    gotinfo_ = &GOT::GetConstInfo(GOT_);
    requestedSize_ = 0;

    if (nodeCategory_)
    {
        nodeCategory_->RemoveAllChildren();
        nodeCategory_.Reset();
    }
    nodeCategory_ = nodePool->CreateChild(GOT::GetType(GOT_), LOCAL, ids[0]);
    nodeCategory_->SetVar(GOA::GOT, GOT_);
    nodeCategory_->SetTemporary(true);

    // Set Template Node
    template_->SetVar(GOA::GOT, GOT_.Value());
    template_->SetEnabled(false);

    // Mark Template before Cloning
    template_->isPoolNode_ = true;
    for (unsigned j=0; j < template_->GetChildren().Size(); j++)
        template_->GetChildren()[j]->isPoolNode_ = true;

    // Create Smooth Transform Component in LOCAL and disable ChangeMode for this component
    if (replicatedState_)
    {
        SmoothedTransform* smooth = template_->GetOrCreateComponent<SmoothedTransform>(LOCAL);
        smooth->SetChangeModeEnable(false);
    }

    // Set Ids
    SetIds(LOCAL, ids[0] + 1, ids[1]);
    if (replicatedState_)
        SetIds(REPLICATED, ids[2], ids[3]);
    else
        SetIds(REPLICATED, 0, 0);

    numNodeIdsByObject_ = 1 + template_->GetChildren().Size();
    numComponentIdsByObject_ = template_->GetNumComponents();

    URHO3D_LOGINFOF("ObjectPoolCategory() - Create : type=%s(%u) ... CreateMode=%s templateID=%u NodeCategoryId=%u LOCAL n=%u c=%u REPLI n=%u c=%u(%u) ... OK !",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), replicatedState_ ? "REPLICATED":"LOCAL", template_->GetID(), nodeCategory_->GetID(),
                    firstNodeID_, firstComponentID_, firstReplicatedNodeID_, firstReplicatedComponentID_, ids[3]);

    GameHelpers::DumpNode(template_, 0, true);

    return true;
}

void ObjectPoolCategory::Resize(unsigned size)
{
    requestedSize_ = size;
    updateState_ = 0;

    nodes_.Reserve(size);
    freenodes_.Reserve(size);

    // Calculate the last Ids
    lastNodeID_ = firstNodeID_ + numNodeIdsByObject_ * size - 1;
    lastComponentID_ = firstComponentID_ + numComponentIdsByObject_ * size - 1;
    if (replicatedState_)
    {
        lastReplicatedNodeID_ = firstReplicatedNodeID_ + numNodeIdsByObject_ * size - 1 ;
        lastReplicatedComponentID_ = firstReplicatedComponentID_ + numComponentIdsByObject_ * size - 1;
    }
    else
    {
        lastReplicatedNodeID_ = 0;
        lastReplicatedComponentID_ = 0;
    }

    URHO3D_LOGINFOF("ObjectPoolCategory() - Resize type=%s(%u) CreateMode=%s templateID=%u size=%u LOCAL n=%u->%u c=%u->%u REPLI n=%u->%u c=%u->%u ... OK !",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), replicatedState_ ? "REPLICATED":"LOCAL", template_->GetID(), size,
                    firstNodeID_, lastNodeID_, firstComponentID_, lastComponentID_,
                    firstReplicatedNodeID_, lastReplicatedNodeID_, firstReplicatedComponentID_, lastReplicatedComponentID_);
}

bool ObjectPoolCategory::Update(HiresTimer* timer, const long long& delay)
{
    if (updateState_ == 0)
    {
        unsigned nodeid = firstNodeID_ + numNodeIdsByObject_ * nodes_.Size();
        unsigned componentid = firstComponentID_ + numComponentIdsByObject_ * nodes_.Size();

        URHO3D_LOGINFOF("ObjectPoolCategory() - Update : Resize type=%s(%u) ... Clone Template => PoolSize=%u/%u ... (nodeid=%u, componentid=%u)",
                        GOT::GetType(GOT_).CString(), GOT_.Value(), nodes_.Size(), requestedSize_, nodeid, componentid);

        while (nodes_.Size() < requestedSize_)
        {
            SharedPtr<Node> node(template_->Clone(LOCAL, false, nodeid, componentid));
            //Node* Clone(CreateMode mode = REPLICATED, bool applyAttr = true, unsigned nodeid=0, unsigned componentid=0, Node* parent=0);

            if (!node)
            {
                URHO3D_LOGERRORF("ObjectPoolCategory() - Update : Resize Error => Can't Clone Template !");
                return true;
            }

            nodes_.Push(node);
            freenodes_.Push(node.Get());

            if (nodes_.Size() < requestedSize_ && timer && timer->GetUSec(false) > delay)
                return false;

            nodeid += numNodeIdsByObject_;
            componentid += numComponentIdsByObject_;
        }

        updateState_++;
    }

    if (updateState_ > 0)
    {
        unsigned inode = updateState_-1;
        Node* node;

        URHO3D_LOGINFOF("ObjectPoolCategory() - Update : Resize type=%s(%u) ... Apply Attributes => PoolSize=%u/%u ...",
                        GOT::GetType(GOT_).CString(), GOT_.Value(), inode, requestedSize_);

        while (inode < requestedSize_)
        {
            node = nodes_[inode];

            node->ApplyAttributes();

            inode++;

            if (inode < requestedSize_ && timer && timer->GetUSec(false) > delay)
            {
                updateState_ = inode + 1;
                return false;
            }
        }

        URHO3D_LOGINFOF("ObjectPoolCategory() - Update : Resize type=%s(%u) ... OK !", GOT::GetType(GOT_).CString(), GOT_.Value());

//        Dump();
        return true;
    }

    return false;
}


/// restore all the categories to the pool
void ObjectPoolCategory::RestoreObjects(bool allObjects)
{
    if (GetFreeSize() == GetSize())
        return;

    URHO3D_LOGINFOF("ObjectPoolCategory() - RestoreObjects : Category = %s(%u)  ... templateID=%u IDs=%u->%u",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), template_->GetID(), firstNodeID_, lastNodeID_);

    bool r;
    unsigned count=0;

    if (!allObjects)
    {
        Node* node;
        for (unsigned i=0; i < GetSize(); i++)
        {
            node = nodes_[i];

//            if (replicatedState_ && ObjectPool::createChildMode_ == REPLICATED)
//                node->GetScene()->NodeIDChanged(node, linkedIds_[node->GetID()]);

//            URHO3D_LOGINFOF(" -> Object(%u) : Ptr=%u ID=%u...", i, node, node->GetID());

            if (node->HasTag("UsedAsPart"))
                continue;

            count++;
            r = FreePoolNode(node, true);
        }
    }
    else
    {
        for (unsigned i=0; i < GetSize(); i++)
        {
            count++;
            r = FreePoolNode(nodes_[i], true);
        }
    }

    // Reset IDs Generator
    currentReplicatedNodeID_ = firstReplicatedNodeID_;
    currentReplicatedComponentID_ = firstReplicatedComponentID_;

    URHO3D_LOGINFOF("ObjectPoolCategory() - RestoreObjects : Category = %s(%u) - RestoreMode=%s ... %u/%u restored OK !",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), allObjects ? "allObjects" : "NotUsedAsPart", count, GetSize());

//    Dump(true);
}

void ObjectPoolCategory::Dump(bool logonlyerrors) const
{
    URHO3D_LOGINFOF("ObjectPoolCategory() - Dump() : category=%s(%u) - free=%u/%u localids=(%u->%u) replicatedids=(%u->%u)",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), GetFreeSize(), GetSize(),
                    firstNodeID_, lastNodeID_, firstReplicatedNodeID_, lastReplicatedNodeID_);

    if (!logonlyerrors)
        for (unsigned i=0; i <freenodes_.Size(); i++)
        {
            Node* node = freenodes_[i];
            if (!node || node->GetID() < firstNodeID_ || node->GetID() > lastNodeID_)
                URHO3D_LOGERRORF("-> freenode[%d] : ptr=%u id=%u", i, node, node ? node->GetID():0);
            else
                URHO3D_LOGINFOF("-> freenode[%d] : ptr=%u id=%u", i, node, node ? node->GetID():0);
        }
    else
        for (unsigned i=0; i <freenodes_.Size(); i++)
        {
            Node* node = freenodes_[i];
            if (!node || node->GetID() < firstNodeID_ || node->GetID() > lastNodeID_)
                URHO3D_LOGERRORF("-> freenode[%d] : ptr=%u id=%u", i, node, node ? node->GetID():0);
        }
}



ObjectPool* ObjectPool::pool_ = 0;
CreateMode ObjectPool::createChildMode_ = LOCAL;
String ObjectPool::debugTxt_;

void ObjectPool::Reset(Node* node)
{
	if (pool_)
	{
        delete pool_;
		pool_ = 0;

		if (!node)
        {
            URHO3D_LOGINFOF("ObjectPool() - Reset ... OK !");
            return;
        }
	}

    if (node)
    {
		pool_ = new ObjectPool(node->GetContext());
		pool_->categories_.Clear();
        pool_->nodePool_ = node->CreateChild("ObjectPool", LOCAL);

        URHO3D_LOGINFOF("ObjectPool() - Reset ... Root=%u NodePool=%u ... OK !", node->GetID(), pool_->nodePool_->GetID());
    }
}

ObjectPool::ObjectPool(Context* context) :
    Object(context),
    firstLocalNodeID_(0),
    createstate_(0)
{
    SetCreateChildMode(LOCAL);
}

ObjectPool::~ObjectPool()
{
    Stop();
}

/// restore all the categories to the pool
void ObjectPool::RestoreCategories(bool allObjects)
{
    URHO3D_LOGINFO("------------------------------------------------------------");
    URHO3D_LOGINFO("- ObjectPool() - RestoreCategories ...                     -");
    URHO3D_LOGINFO("------------------------------------------------------------");

    for (HashMap<StringHash, ObjectPoolCategory >::Iterator it=categories_.Begin();it!=categories_.End();++it)
        it->second_.RestoreObjects(allObjects);

    SetCreateChildMode(LOCAL);

    URHO3D_LOGINFO("------------------------------------------------------------");
    URHO3D_LOGINFO("- ObjectPool() - RestoreCategories ...                 OK! -");
    URHO3D_LOGINFO("------------------------------------------------------------");
}

void ObjectPool::SetCreateChildMode(CreateMode mode)
{
    createChildMode_ = mode;
}

void ObjectPool::Stop()
{
    URHO3D_LOGINFO("ObjectPool() - Stop ...");

    createstate_ = 0;
    categories_.Clear();

    GameHelpers::RemoveNodeSafe(nodePool_, false);
    nodePool_.Reset();

    SetCreateChildMode(LOCAL);

    URHO3D_LOGINFO("ObjectPool() - Stop ... OK !");
}

ObjectPoolCategory* ObjectPool::CreateCategory(const GOTInfo& info, Node* templateNode, unsigned* categoryids)
{
    const StringHash& got = info.typename_;

    if (GOT::GetType(got) == String::EMPTY && !templateNode)
        return 0;

    HashMap<StringHash, ObjectPoolCategory >::Iterator it = categories_.Find(got);
    ObjectPoolCategory& category = categories_[got];

    if (it == categories_.End())
    {
        if (!category.Create(info.replicatedMode_, got, nodePool_, templateNode, categoryids))
        {
            categories_.Erase(got);
            URHO3D_LOGERRORF("ObjectPool() - CreateCategory : can't create category(GOT=%u type=%s) !", got.Value(), GOT::GetType(got).CString());
            return 0;
        }
    }

    category.Resize(info.poolqty_);

	return &category;
}

bool ObjectPool::CreateCategories(Scene* scene, const HashMap<StringHash, GOTInfo >& infos, const HashMap<StringHash, WeakPtr<Node> >& templates, HiresTimer* timer, const long long& delay)
{
    // Reserve Scene Ids
    if (createstate_ == 0)
    {
        // Get Scene Next Free Ids
        firstReplicatedNodeID_ = lastReplicatedNodeID_ = scene->GetNextFreeReplicatedNodeID();
        firstReplicatedComponentID_ = lastReplicatedComponentID_ = scene->GetNextFreeReplicatedComponentID();
        firstLocalNodeID_ = lastLocalNodeID_ = scene->GetNextFreeLocalNodeID();
        firstLocalComponentID_ = lastLocalComponentID_ = scene->GetNextFreeLocalComponentID() + 0x01000000;

//        GameHelpers::SetGameLogEnable(GameStatics::rootScene_->GetContext(), GAMELOG_PRELOAD, true);

        for (HashMap<StringHash, GOTInfo >::ConstIterator it = infos.Begin(); it != infos.End(); ++it)
        {
            const StringHash& got = it->first_;
            const GOTInfo& info = it->second_;

            if (!info.poolqty_)
            {
                URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) no poolqty => Skip !", info.typename_.CString(), got.Value());
                continue;
            }

            const HashMap<StringHash, WeakPtr<Node> >::ConstIterator itt = templates.Find(got);
            if (itt == templates.End())
            {
                URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) no template => Skip !", info.typename_.CString(), got.Value());
                continue;
            }

            Node* node = itt->second_;
            unsigned numObjects = info.poolqty_;
            unsigned numNodes = 1 + node->GetChildren().Size();
            unsigned numComponents = node->GetNumComponents();
            if (info.replicatedMode_)
                numComponents++;

            URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) Reserve LOCAL nodeCategory=%u nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                            info.typename_.CString(), got.Value(), lastLocalNodeID_, lastLocalNodeID_ + 1, lastLocalNodeID_ + numObjects * numNodes,
                            lastLocalComponentID_, lastLocalComponentID_ + numObjects * numComponents - 1);

            if (info.replicatedMode_)
                URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) Reserve REPLI nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                                info.typename_.CString(), got.Value(), lastReplicatedNodeID_, lastReplicatedNodeID_ + numObjects * numNodes - 1,
                                lastReplicatedComponentID_, lastReplicatedComponentID_ + numObjects * numComponents - 1);

            // Calculate the next Local Ids
            {
                lastLocalNodeID_ += numObjects * numNodes + 1; // Add Category Object Nodes + 1 Category Local Node
                lastLocalComponentID_ += numObjects * numComponents;

                // Calculate the next Replicate Ids
                if (info.replicatedMode_)
                {
                    lastReplicatedNodeID_ += numObjects * numNodes; // Add Category Object Nodes
                    lastReplicatedComponentID_ += numObjects * numComponents;
                }
            }
        }

        // Set Scene Reserved Ids
        scene->SetFirstReservedLocalNodeID(firstLocalNodeID_);
        scene->SetLastReservedLocalNodeID(lastLocalNodeID_);
        scene->SetFirstReservedReplicateNodeID(firstReplicatedNodeID_);
        scene->SetLastReservedReplicateNodeID(lastReplicatedNodeID_);
        scene->SetFirstReservedLocalComponentID(firstLocalComponentID_);
        scene->SetLastReservedLocalComponentID(lastLocalComponentID_);
        scene->SetFirstReservedReplicateComponentID(firstReplicatedComponentID_);
        scene->SetLastReservedReplicateComponentID(lastReplicatedComponentID_);

        createstate_++;

        URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Scene Reserve LOCAL nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                        firstLocalNodeID_, lastLocalNodeID_, firstLocalComponentID_, lastLocalComponentID_);
        URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Scene Reserve REPLI nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                        firstReplicatedNodeID_, lastReplicatedNodeID_, firstReplicatedComponentID_, lastReplicatedComponentID_);

//        GameHelpers::SetGameLogEnable(GameStatics::rootScene_->GetContext(), GAMELOG_PRELOAD, false);

        return false;
    }

    // Create Categories
    if (createstate_ == 1)
    {
        unsigned categoryids[4];
        categoryids[0] = firstLocalNodeID_;
        categoryids[1] = firstLocalComponentID_;
        categoryids[2] = firstReplicatedNodeID_;
        categoryids[3] = firstReplicatedComponentID_;

        for (HashMap<StringHash, GOTInfo >::ConstIterator it = infos.Begin(); it != infos.End(); ++it)
        {
            const StringHash& got = it->first_;
            const GOTInfo& info = it->second_;

            if (!info.poolqty_)
                continue;

            const HashMap<StringHash, WeakPtr<Node> >::ConstIterator itt = templates.Find(got);
            if (itt == templates.End())
                continue;

            Node* node = itt->second_;
            unsigned numObjects = info.poolqty_;
            unsigned numNodes = 1 + node->GetChildren().Size();
            unsigned numComponents = node->GetNumComponents();
            if (info.replicatedMode_)
                numComponents++;

            ObjectPoolCategory* category = CreateCategory(info, node, categoryids);

            if (category)
                categoriesToUpdate_.Push(category);

            // Calculate the next Ids
            categoryids[0] += numObjects * numNodes + 1; // Add Category Object Nodes + 1 Category Local Node
            categoryids[1] += numObjects * numComponents;
            if (info.replicatedMode_)
            {
                categoryids[2] += numObjects * numNodes; // Add Category Object Nodes
                categoryids[3] += numObjects * numComponents;
            }
        }

        createstate_++;
        return false;
    }

    // Update Categories Size
    if (createstate_ == 2)
    {
        ObjectPoolCategory* category;

        while (categoriesToUpdate_.Size())
        {
            category = categoriesToUpdate_.Front();

            assert(category);

            if (!category->Update(timer, delay))
                return false;

            categoriesToUpdate_.PopFront();
        }

        return true;
    }

    return false;
}

ObjectPoolCategory* ObjectPool::GetCategory(const StringHash& got)
{
    HashMap<StringHash, ObjectPoolCategory >::Iterator it = categories_.Find(got);

    return it != categories_.End() ? &(it->second_) : 0;
}

Node* ObjectPool::CreateChildIn(const StringHash& got, Node* parent, unsigned id, int viewZ, const NodeAttributes* nodeAttr, bool applyAttr, ObjectPoolCategory** retcategory)
{
    if (got == StringHash::ZERO)
        return 0;

    ObjectPoolCategory* category = pool_->GetCategory(got);

    if (retcategory)
        *retcategory = category;

    if (!category)
        return 0;

    // Force Local Mode if not server : Only Server can assign replicatedID
//    if (createChildMode_ == LOCAL && !GameStatics::ServerMode_ && !id)
//        id = LOCAL;
    if (createChildMode_ == LOCAL || !id)
        id = LOCAL;

    Node* node = category->GetPoolNode(id);
    if (!node)
        return 0;

    if (node->GetID() == 0)
    {
        URHO3D_LOGWARNINGF("ObjectPool() - CreateChild node id=0 !");
        return 0;
    }

    // don't use setparent (hang program), prefer addchild
    if (parent)
        parent->AddChild(node);

    if (nodeAttr)
        GameHelpers::LoadNodeAttributes(node, *nodeAttr, false);

    if (parent)
        node->SetEnabled(parent->IsEnabled());

    if (applyAttr)
        node->ApplyAttributes();

//    URHO3D_LOGINFOF("ObjectPool() - CreateChild got=%s node=%s(%u) in parent=%s ... OK !", GOT::GetType(got).CString(),
//             node->GetName().CString(), node->GetID(), parent ? parent->GetName().CString() : "");

    return node;
}

void ObjectPool::ChangeToReplicatedID(Node* node, unsigned newid)
{
    if (!node)
    {
        URHO3D_LOGERRORF("ObjectPool() - ChangeToReplicatedID : newid=%u no node !", newid);
        return;
    }

    StringHash got = node->GetVar(GOA::GOT).GetStringHash();
//    StringHash got = node->GetParent()->GetVar(GOA::GOT).GetStringHash();
    ObjectPoolCategory* category = pool_->GetCategory(got);
    if (category)
    {
        if (category->IsNodeInPool(node))
        {
            URHO3D_LOGWARNINGF("ObjectPool() - ChangeToReplicatedID : node in the pool => don't change it !");
            return;
        }
        category->ChangeToReplicatedID(node, newid);
    }
    else
        URHO3D_LOGERRORF("ObjectPool() - ChangeToReplicatedID : newid=%u node=%s(%u) got=%s  no category ... OK !", newid, node->GetName().CString(), node->GetID(), GOT::GetType(got).CString());
}

bool ObjectPool::Free(Node* node)
{
    if (!node)
        return false;

//    unsigned got = node->GetVar(GOA::GOT).GetUInt();
    StringHash got = node->GetVar(GOA::GOT).GetStringHash();
//    StringHash got = node->GetParent()->GetVar(GOA::GOT).GetStringHash();
    if (!got)
    {
        URHO3D_LOGWARNINGF("ObjectPool() - Free : Node=%s(%u) no GOT : Remove Node !",
                          node->GetName().CString(), node->GetID());
        node->Remove();
        return false;
    }

    ObjectPoolCategory* category = pool_->GetCategory(got);
    if (!category)
    {
        URHO3D_LOGERRORF("ObjectPool() - Free : Node=%s(%u) GOT=%u... No category : Remove Node !",
                          node->GetName().CString(), node->GetID(), got.Value());
        node->Remove();
        return false;
    }

    if (!(category->FreePoolNode(node)))
    {
        URHO3D_LOGERRORF("ObjectPool() - Free : Node=%s(%u) ... already inside Pool !!!",
                            node->GetName().CString(), node->GetID());
        return false;
    }

    return true;
}

void ObjectPool::DumpCategories() const
{
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : ----------------------------------------------");
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : size=%u", categories_.Size());
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : LOCAL n=%u->%u c=%u->%u REPLI n=%u->%u c=%u->%u",
                    firstLocalNodeID_, lastLocalNodeID_, firstLocalComponentID_, lastLocalComponentID_,
                    firstReplicatedNodeID_, lastReplicatedNodeID_, firstReplicatedComponentID_, lastReplicatedComponentID_);

    for (HashMap<StringHash, ObjectPoolCategory >::ConstIterator it=categories_.Begin();it!=categories_.End();++it)
    {
        const ObjectPoolCategory& category = it->second_;
//        URHO3D_LOGINFOF("  PoolCategory %s(%u) : free=%u/%u LOCAL n=%u->%u c=%u->%u REPLI n=%u->%u c=%u->%u",
//                    GOT::GetType(it->first_).CString(), it->first_.Value(), category.GetFreeSize(), category.GetSize(),
//                    category.firstNodeID_, category.lastNodeID_, category.firstComponentID_, category.lastComponentID_,
//                    category.firstReplicatedNodeID_, category.lastReplicatedNodeID_, category.firstReplicatedComponentID_, category.lastReplicatedComponentID_);
        category.Dump(true);
    }

    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : ... OK !                                     -");
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : ----------------------------------------------");
}

void ObjectPool::UpdateDebugData()
{
    String line;
    debugTxt_.Clear();
    const int tabs = 1;

    unsigned i=0;
    for (HashMap<StringHash, ObjectPoolCategory >::ConstIterator it=pool_->categories_.Begin();it!=pool_->categories_.End();++it)
    {
        const ObjectPoolCategory& category = it->second_;
        line.Clear();
        line.AppendWithFormat(" %s(%u/%u) ", GOT::GetType(it->first_).CString(), category.GetFreeSize(), category.GetSize());
        debugTxt_ += line;
        i++;
        if (i == tabs)
        {
            debugTxt_ += "\n";
            i = 0;
        }
    }
}
