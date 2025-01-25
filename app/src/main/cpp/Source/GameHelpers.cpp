#include <iostream>
#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Engine/Console.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/MessageBox.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/DropDownList.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Audio/SoundSource3D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameStatics.h"

#include "AnimatedSprite.h"
#include "InteractiveFrame.h"
#include "TextMessage.h"
#include "DelayInformer.h"
#include "TimerRemover.h"

//#include "GOC_Animator2D.h"
#include "Matches.h"
#include "ObjectPool.h"


#include "GameHelpers.h"


/// Log Helpers

static unsigned GameLogFilters_;
static int GameLogLock_;

void GameHelpers::SetGameLogFilter(unsigned logfilter)
{
    GameLogFilters_ = logfilter;
    GameLogLock_ = 0;
}

unsigned GameHelpers::GetGameLogFilter()
{
    return GameLogFilters_;
}

void GameHelpers::ResetGameLog(Context* context)
{
    GameLogLock_ = 0;
    context->GetSubsystem<Log>()->SetLevel(GAMELOGLEVEL_DEFAULT);
}

void GameHelpers::SetGameLogEnable(Context* context, unsigned filterbits, bool state)
{
#ifdef ACTIVE_GAMELOGLEVEL
    if ((GameLogFilters_ & filterbits) != 0)
    {
        if (!state)
        {
            context->GetSubsystem<Log>()->SetLevel(GAMELOGLEVEL_MINIMAL);
            GameLogLock_++;
        }
        else
        {
            GameLogLock_--;
            if (GameLogLock_ <= 0)
                context->GetSubsystem<Log>()->SetLevel(GAMELOGLEVEL_DEFAULT);
        }
    }
#endif
}


/// Serialize Data Helpers

bool GameHelpers::LoadData(Context* context, const char* fileName, void* data, bool allocate)
{
    URHO3D_LOGINFOF("GameHelpers() - LoadData : from %s ...", fileName);

    File f(context, fileName, FILE_READ);
    if (!f.IsOpen())
        return false;

    if (allocate)
    {
//        data = (void*) new (f.GetSize());
    }

    if (f.Read(data, f.GetSize()))
    {
        f.Close();
        URHO3D_LOGINFO("GameHelpers() - LoadData OK");
        return true;
    }

    URHO3D_LOGWARNINGF("GameHelpers() - LoadData : from %s ... no file", fileName);
    return false;
}

bool GameHelpers::SaveData(Context* context, void* data, unsigned size, const char* fileName)
{
    URHO3D_LOGINFOF("GameHelpers() - SaveData : to %s ...", fileName);

    File f(context, fileName, FILE_WRITE);
    if (f.Write(data, size))
    {
        URHO3D_LOGINFOF("GameHelpers() - SaveData : to %s ... OK !", fileName);
        f.Close();
        return true;
    }

    URHO3D_LOGWARNINGF("GameHelpers() - SaveData : to %s ... NOK !", fileName);
    return false;
}

char* GameHelpers::GetTextFileToBuffer(const char* fileName, size_t& s)
{
    FILE* pFile;
    char* newbuffer=0;
    char buf[128];
    unsigned lSize;

    memset(buf,0,128*sizeof(char));

    pFile = fopen(fileName, "r");

    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);
    newbuffer = (char*) malloc (sizeof(char)*lSize);
    if (!newbuffer) return 0;
    s = lSize;

    rewind(pFile);
    unsigned index=0;

    while (fgets(buf, 128, pFile))
    {
        unsigned i=0;
        memcpy(newbuffer+index ,buf, strlen(buf));
        index += strlen(buf);
    }

    fclose(pFile);

    return newbuffer;
}


/// Serialize Scene/Node Helpers

bool GameHelpers::LoadSceneXML(Context* context, Node* node, CreateMode mode, const String& scene)
{
    URHO3D_LOGINFOF("GameHelpers() - LoadSceneXML : saveDir_=%s scene=%s", GameStatics::gameConfig_.saveDir_.CString(), scene.CString());
    String file(GameStatics::gameConfig_.saveDir_ +  scene.CString());

	ResourceCache* cache = context->GetSubsystem<ResourceCache>();

    URHO3D_LOGINFOF("GameHelpers() - LoadSceneXML : Loading %s ...", file.CString());

	XMLFile* xmlFile = cache->GetResource<XMLFile>(file);
    if (!xmlFile)
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadSceneXML : Can not find XML file %s !", file.CString());
        return false;
    }

    const XMLElement& xmlNode = xmlFile->GetRoot("node");

    if (!node->LoadXML(xmlNode, mode))
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadSceneXML : Can not Load XML %s file for the node", file.CString());
        return false;
    }

    return true;
}

bool GameHelpers::SaveSceneXML(Context* context, Scene* scene, const char* fileName)
{
    // Save to XML File for test

    URHO3D_LOGINFOF("GameHelpers() - SaveSceneXML : Saving to %s ...", fileName);

    File f(context, fileName, FILE_WRITE);
    if (!scene->SaveXML(f))
    {
        URHO3D_LOGERRORF("GameHelpers() - SaveSceneXML : Can not create XML file %s for the scene !", fileName);
        f.Close();
        return false;
    }

    f.Close();
    return true;
}

bool GameHelpers::SaveNodeXML(Context* context, Node* node, const char* fileName)
{
//    LOGINFOF("GameHelpers() - SaveNodeXML() : file %s ...", fileName);
    if (!node) return false;

    File f(context, fileName, FILE_WRITE);
    if (!node->SaveXML(f))
    {
        URHO3D_LOGERROR("GameHelpers() - SaveNodeXML : Can not create XML file for the node");
        f.Close();
        return false;
    }

    f.Close();
//    URHO3D_LOGINFO("GameHelpers() - SaveNodeXML() : ... OK");

    return true;
}

bool GameHelpers::LoadNodeXML(Context* context, Node* node, const String& fileName, CreateMode mode, bool applyAttr)
{
    URHO3D_LOGINFOF("GameHelpers() - LoadNodeXML : Loading %s on node %u... ", fileName.CString(), node->GetID());

	XMLFile* xmlFile = context->GetSubsystem<ResourceCache>()->GetResource<XMLFile>(fileName);
    if (!xmlFile)
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadNodeXML : Can not find XML file %s !", fileName.CString());
        return false;
    }

    const XMLElement& xmlNode = xmlFile->GetRoot("node");

    if (!node->LoadXML(xmlNode, mode, false, applyAttr))
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadNodeXML : Can not Load XML %s file for the node", fileName.CString());
        return false;
    }

    URHO3D_LOGINFOF("GameHelpers() - LoadNodeXML : Loading %s on node %u... OK !", fileName.CString(), node->GetID());
    return true;
}

bool GameHelpers::PreloadXMLResourcesFrom(Context* context, const String& fileName)
{
    SharedPtr<File> file = context->GetSubsystem<ResourceCache>()->GetFile(fileName);
    if (!file)
    {
        URHO3D_LOGERRORF("GameHelpers() - PreloadXMLResourcesFrom : Can not find XML file %s !", fileName.CString());
        return false;
    }

    if (!GameStatics::rootScene_->LoadAsyncXML(file, LOAD_RESOURCES_ONLY))
        return false;

    URHO3D_LOGINFOF("GameHelpers() - PreloadXMLResourcesFrom : %s ... OK !", fileName.CString());
    return true;
}


/// Node Attributes Helpers

//#define NodeAttributes_ActiveLog_1
//#define NodeAttributes_ActiveLog_2
//#define NodeAttributes_ActiveLog_3

inline void LoadAttributes(Serializable* seria, const VariantMap& attrMap)
{
    const Vector<AttributeInfo>* attributes = seria->GetAttributes();

    if (attributes)
    {
        for (unsigned j = 0; j < attributes->Size(); ++j)
        {
            const AttributeInfo& attr = attributes->At(j);

            #ifdef NodeAttributes_ActiveLog_2
            URHO3D_LOGINFOF("  -> Attribute %s", attr.name_.CString());
            #endif
            VariantMap::ConstIterator it = attrMap.Find(StringHash(attr.name_));
            if (it != attrMap.End())
            {
                #ifdef NodeAttributes_ActiveLog_3
                URHO3D_LOGINFOF("     -> Value %s", it->second_.ToString().CString());
                #endif
                seria->OnSetAttribute(attr, it->second_);
            }
        }
    }
}

inline void SaveAttributes(Serializable* seria, VariantMap& attrMap, Variant& value)
{
    const Vector<AttributeInfo>* attributes = seria->GetAttributes();

    if (attributes)
    {
        for (unsigned j = 0; j < attributes->Size(); ++j)
        {
            const AttributeInfo& attr = attributes->At(j);
            if (!(attr.mode_ & AM_FILE))
                continue;

            #ifdef NodeAttributes_ActiveLog_2
            URHO3D_LOGINFOF("  -> Attribute %s", attr.name_.CString());
            #endif

            seria->OnGetAttribute(attr, value);
            #ifdef NodeAttributes_ActiveLog_3
            URHO3D_LOGINFOF("    -> Value %s", value.ToString().CString());
            #endif

            attrMap[StringHash(attr.name_)] = value;
        }
    }
}

void GameHelpers::LoadNodeAttributes(Node* node, const NodeAttributes& nodeAttr, bool applyAttr)
{
    /// node must have the same composition than nodeAttr
    int index=0;

    #ifdef NodeAttributes_ActiveLog_1
    URHO3D_LOGINFOF("LoadNodeAttributes node %s(%u) :", node->GetName().CString(), node->GetID());
    #endif

    /// Set node attributes in node
    {
        const VariantMap& attrMap = nodeAttr[index];

        LoadAttributes(node, attrMap);
    }

    /// Set components attributes in node
    {
        const Vector<SharedPtr<Component> >& components = node->GetComponents();

        for (Vector<SharedPtr<Component> >::ConstIterator it = components.Begin(); it != components.End(); ++it)
        {
            Component* component = *it;

            if (component->IsTemporary())
                continue;

            #ifdef NodeAttributes_ActiveLog_2
            URHO3D_LOGINFOF(" -> Component %s", component->GetTypeName().CString());
            #endif

            index++;

            const VariantMap& attrMap = nodeAttr[index];

            LoadAttributes(component, attrMap);
        }
    }

    if (applyAttr)
        node->ApplyAttributes();
}

void GameHelpers::SaveNodeAttributes(Node* node, NodeAttributes& nodeAttr)
{
    nodeAttr.Clear();

    Variant value;

    /// Get node attributes
    {
        /// create default attributes nodeAttr[0]
        nodeAttr.Push(VariantMap());
        VariantMap& attrMap = nodeAttr.Back();

        #ifdef NodeAttributes_ActiveLog_1
        URHO3D_LOGINFOF("SaveNodeAttributes node %s(%u) :", node->GetName().CString(), node->GetID());
        #endif

        SaveAttributes(node, attrMap, value);
    }

    /// Get components attributes
    {
        const Vector<SharedPtr<Component> >& components = node->GetComponents();

        for (Vector<SharedPtr<Component> >::ConstIterator it = components.Begin(); it != components.End(); ++it)
        {
            Component* component = *it;

            if (component->IsTemporary())
                continue;

            #ifdef NodeAttributes_ActiveLog_2
            URHO3D_LOGINFOF(" -> Component %s", component->GetTypeName().CString());
            #endif

            nodeAttr.Push(VariantMap());
            VariantMap& attrMap = nodeAttr.Back();

            SaveAttributes(component, attrMap, value);
        }
    }
}

const Variant& GameHelpers::GetNodeAttributeValue(const StringHash& attr, const NodeAttributes& nodeAttr, unsigned index)
{
    /// NodeAttributes = Vector<VariantMap >
    /// index=0 => get in node attributes
    /// index>0 => get in component attributes

    if (index >= nodeAttr.Size())
        return Variant::EMPTY;

    VariantMap::ConstIterator it = nodeAttr[index].Find(attr);

    return it != nodeAttr[index].End() ? it->second_ : Variant::EMPTY;
}

void GameHelpers::CopyAttributes(Node* source, Node* dest, bool apply, bool removeUnusedComponents)
{
    #ifdef NodeAttributes_ActiveLog_1
    URHO3D_LOGINFOF("CopyAttributes source=%s(%u) to dest==%s(%u) ...", source->GetName().CString(), source->GetID(), dest->GetName().CString(), dest->GetID());
    #endif

    /// Copy node attributes
    const Vector<AttributeInfo>* attributes = source->GetAttributes();
    for (unsigned j = 0; j < attributes->Size(); ++j)
    {
        const AttributeInfo& attr = attributes->At(j);
        // Do not copy network-only attributes, as they may have unintended side effects
        if (attr.mode_ & AM_FILE)
        {
            Variant value;
            source->OnGetAttribute(attr, value);
//            #ifdef NodeAttributes_ActiveLog_1
//            URHO3D_LOGINFOF("-> attr[%u] %s=%s ...", j, attr.name_.CString(), value.ToString().CString());
//            #endif
            dest->OnSetAttribute(attr, value);
        }
    }

    /// copy components attributes
    const Vector<SharedPtr<Component> >& tcomponents = source->GetComponents();
    const Vector<SharedPtr<Component> >& components = dest->GetComponents();

    PODVector<Component*> componentToremove;

    #ifdef NodeAttributes_ActiveLog_1
    URHO3D_LOGINFOF("-> Num Template Components = %u, Num Dest Components = %u", tcomponents.Size(), components.Size());
    #endif

    for (Vector<SharedPtr<Component> >::ConstIterator it = tcomponents.Begin(), jt = components.Begin(); it != tcomponents.End() && jt != components.End(); ++it, ++jt)
    {
        if (it == tcomponents.End())
        {
            while (jt != components.End())
            {
                componentToremove.Push(*jt);
                jt++;
            }
            break;
        }

        Component* component = *jt;
        Component* tcomponent = *it;

        if (!tcomponent || !component)
            continue;

        if (tcomponent->IsTemporary())
            continue;

        const Vector<AttributeInfo>* tAttributes = tcomponent->GetAttributes();
        const Vector<AttributeInfo>* attributes = component->GetAttributes();

        if (tAttributes && attributes)
        {
            #ifdef NodeAttributes_ActiveLog_2
            URHO3D_LOGINFOF(" -> Copy %s attributes size=%u(%u)", tcomponent->GetTypeName().CString(), tAttributes->Size(), attributes->Size());
            #endif
            if (tAttributes->Size() == 0 || attributes->Size() == 0)
                continue;

//            if (tAttributes->Size() == attributes->Size())
            {
                for (unsigned i = 0; i < attributes->Size(); ++i)
                {
                    const AttributeInfo& tattr = tAttributes->At(i);
                    const AttributeInfo& attr = attributes->At(i);
                    if (tattr.mode_ & AM_FILE)
                    {
                        Variant value;
                        tcomponent->OnGetAttribute(tattr, value);
                        component->OnSetAttribute(attr, value);
                        #ifdef NodeAttributes_ActiveLog_3
                        URHO3D_LOGINFOF("   -> Attribute=%s(%s) Value=%s", attr.name_.CString(), attr.name_.CString(), value.ToString().CString());
                        #endif
                    }
                }
            }
//            else if (tAttributes->Size() > attributes->Size())
//            {
//                for (unsigned i = 0; i < attributes->Size(); ++i)
//                {
//                    const AttributeInfo& attr = attributes->At(i);
//                    if (attr.mode_ & AM_FILE)
//                    {
//                        for (unsigned j=0; j < tAttributes->Size(); ++j)
//                        {
//                            if (tAttributes->At(j).name_ == attr.name_)
//                            {
//                                const AttributeInfo& tattr = tAttributes->At(j);
//
//                                Variant value;
//                                tcomponent->OnGetAttribute(tattr, value);
//                                component->OnSetAttribute(attr, value);
//
//                                #ifdef NodeAttributes_ActiveLog_3
//                                URHO3D_LOGINFOF("   -> Attribute=%s(%s) Value=%s", attr.name_.CString(), attr.name_.CString(), value.ToString().CString());
//                                #endif
//                                break;
//                            }
//                        }
//                    }
//                }
//            }
//            else
//            {
//                for (unsigned i = 0; i < tAttributes->Size(); ++i)
//                {
//                    const AttributeInfo& tattr = tAttributes->At(i);
//                    if (tattr.mode_ & AM_FILE)
//                    {
//                        for (unsigned j = 0; j < attributes->Size(); ++j)
//                        {
//                            if (attributes->At(j).name_ == tattr.name_)
//                            {
//                                const AttributeInfo& attr = attributes->At(j);
//
//                                Variant value;
//                                tcomponent->OnGetAttribute(tattr, value);
//                                component->OnSetAttribute(attr, value);
//
//                                #ifdef NodeAttributes_ActiveLog_3
//                                URHO3D_LOGINFOF("   -> Attribute=%s(%s) Value=%s", attr.name_.CString(), attr.name_.CString(), value.ToString().CString());
//                                #endif
//                                break;
//                            }
//                        }
//                    }
//                }
//            }
        }
    }

    if (removeUnusedComponents && componentToremove.Size())
        for (PODVector<Component* >::ConstIterator it = componentToremove.Begin() ; it != componentToremove.End(); ++it)
        {
            #ifdef NodeAttributes_ActiveLog_2
            URHO3D_LOGINFOF(" -> Dest Remove Component %s", (*it)->GetTypeName().CString());
            #endif
            dest->RemoveComponent(*it);
        }

    if (apply)
        dest->ApplyAttributes();

    #ifdef NodeAttributes_ActiveLog_1
    URHO3D_LOGINFOF("CopyAttributes ... OK ! ");
    #endif
}



/// Scene/Node Remover Helpers

/// clean scene and keep the nodes preloader and pools
bool GameHelpers::CleanScene(Scene* rootscene, const String& stateToClean, int step)
{
    URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... state=%s step=%d", stateToClean.CString(), step);

    if (!rootscene)
    {
        URHO3D_LOGERROR("GameHelpers() - CleanScene : no RootScene !");
        return false;
    }

    bool initialPhysicIsUpdateEnable = false;

    // Stop the physics
    PhysicsWorld2D* physicworld = rootscene->GetComponent<PhysicsWorld2D>();
    if (physicworld)
    {
        initialPhysicIsUpdateEnable = physicworld->IsUpdateEnabled();
        physicworld->SetUpdateEnabled(false);
    }

    // clean-up step 0 : remove common nodes & physics
    if (!step)
    {
        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... CleanupNetwork ...");

        rootscene->CleanupNetwork();

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... ObjectPool Restore ...");

        if (ObjectPool::Get())
            ObjectPool::Get()->RestoreCategories();
        else
            URHO3D_LOGWARNINGF(" ... no ObjectPool to restore !");

        PODVector<Node*> nodesToRemove;

        if (stateToClean == "Play")
        {
            nodesToRemove.Push(rootscene->GetChild("Scene"));
            nodesToRemove.Push(rootscene->GetChild("Controllables"));
        }

        nodesToRemove.Push(rootscene->GetChild("LevelMap"));

        nodesToRemove.Push(rootscene->GetChild("Pool"));
        nodesToRemove.Push(rootscene->GetChild("LocalScene"));
        nodesToRemove.Push(rootscene->GetChild("StaticScene"));
        //nodesToRemove.Push(rootscene->GetChild("Interactive"));

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... Removing Temp Nodes ...");

        for (unsigned i=0; i < nodesToRemove.Size(); ++i)
            if (nodesToRemove[i])
                GameHelpers::RemoveNodeSafe(nodesToRemove[i], false);

        /// TODO : Test this with network
        // Clear Replicated Nodes
        rootscene->Clear(true, false);
//        rootscene_->Clear(!GameStatics::ClientMode_, false);

        /// For Debug
//        GameHelpers::DumpNode(rootscene, 2, true);

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... stateToClean=%s step=0 OK !", stateToClean.CString());
    }
    // clean-up step 1 : remove special node from state
    else
    {
//        /// For Debug
//        GameHelpers::DumpNode(rootscene, 2, true);

        PODVector<Node*> nodesToRemove;

        if (stateToClean != "Play")
            nodesToRemove.Push(rootscene->GetChild(stateToClean));

        for (unsigned i=0; i < nodesToRemove.Size(); ++i)
            if (nodesToRemove[i])
                GameHelpers::RemoveNodeSafe(nodesToRemove[i], false);

//        /// For Debug
//        GameHelpers::DumpNode(rootscene, 2, true);

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... stateToClean=%s step=1 OK !", stateToClean.CString());
    }

    // Start the physics
    if (initialPhysicIsUpdateEnable)
        rootscene->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(true);

    return true;
}

void ScanNodeRecursive(Node* node, Vector<WeakPtr<Node> >& nodes, Vector<WeakPtr<Component> >& components)
{
    if (!node)
        return;

    nodes.Push(WeakPtr<Node>(node));

    // Scan Components
    for (Vector<SharedPtr<Component> >::ConstIterator it=node->GetComponents().Begin(); it!=node->GetComponents().End(); ++it)
        if (*it)
            components.Push(WeakPtr<Component>(*it));

    // Scan Childrens
    for (Vector<SharedPtr<Node> >::ConstIterator it=node->GetChildren().Begin();it!=node->GetChildren().End();++it)
        if (*it)
            ScanNodeRecursive(*it, nodes, components);
}

void GameHelpers::RemoveNodeSafe(Node* node, bool dumpnodes)
{
    if (!node)
        return;

    Vector<WeakPtr<Node> > nodes;
    Vector<WeakPtr<Component> > components;

    ScanNodeRecursive(node, nodes, components);

    URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : name=%s id=%u numref=%d(w=%d) numNodes=%u numComponents=%u ...",
                    node->GetName().CString(), node->GetID(), node->Refs(), node->WeakRefs(), nodes.Size(), components.Size());

    if (dumpnodes)
        GameHelpers::DumpNode(node, 50, true);

    // Remove All Components
    for (unsigned i=components.Size()-1; i < components.Size(); --i)
        if (components[i])
            components[i]->Remove();
        else
            URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : c%u Already Empty ... OK !", i);

    // Remove All Nodes
    for (unsigned i=nodes.Size()-1; i < nodes.Size(); --i)
        if (nodes[i])
            nodes[i]->Remove();
        else
            URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : n%u Already Empty ... OK !", i);

    URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : nodesRemoved=%u componentsRemoved=%u ... OK !", nodes.Size(), components.Size());
}



/// Spawn Node Helpers

Node* GameHelpers::SpawnGOtoNode(Context* context, StringHash type, Node* scenenode)
{
    Node* templatenode = GOT::GetObject(type);

    if (templatenode)
    {
        Node* node = templatenode->Clone(LOCAL, true, 0, 0, scenenode);
        node->SetVar(GOA::ENTITYCLONE, true);
        return node;
    }

    URHO3D_LOGWARNINGF("GameHelpers() - SpawnGOtoNode : type=%s(%u) no templatenode !", GOT::GetType(type).CString(), type.Value());

    const String& fileName = GOT::GetObjectFile(type);
    if (fileName.Empty())
    {
        URHO3D_LOGERRORF("GameHelpers() - SpawnGOtoNode : No XML file for the type %s(%u)",  GOT::GetType(type).CString(), type.Value());
        return 0;
    }

    Node* node = scenenode->CreateChild("", LOCAL);
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    XMLFile* xmlFile = cache->GetResource<XMLFile>(fileName);
    const XMLElement& xmlNode = xmlFile->GetRoot("node");
    if (!node->LoadXML(xmlNode))
    {
        URHO3D_LOGERROR("GameHelpers() - SpawnGOtoNode : Can not Load XML file for the node");
        node->Remove();
        return 0;
    }

    return node;
}

void GameHelpers::SpawnParticleEffect(Context* context, Node* node, const Vector2& direction, int layer, const String& effectName, bool removeTimer, float duration, CreateMode mode)
{
    const Vector2& position = node->GetWorldPosition2D();
    const Vector2& scale = node->GetWorldScale2D();

    Node* newNode = node->GetScene()->CreateChild("ParticuleEffect", mode);

    if (direction.x_ !=0.0f)
        newNode->SetWorldTransform2D(position, (direction.x_ > 0.0f ? 180.0f : 0.0f), scale);
    else
        newNode->SetWorldTransform2D(position, (direction.y_ > 0.0f ? 270.0f : 90.0f), scale);

    // Create the particle emitter
    ParticleEffect2D* particleEffect = context->GetSubsystem<ResourceCache>()->GetResource<ParticleEffect2D>(effectName);
    if (!particleEffect)
        return;

    ParticleEmitter2D* particleEmitter = newNode->CreateComponent<ParticleEmitter2D>(mode);
    particleEmitter->SetEffect(particleEffect);
    particleEmitter->SetLayer(layer+1);

    if (removeTimer)
    {
        if (duration == -1.0f)
            duration = particleEffect->GetDuration()-0.2f;

        TimerRemover::Get()->Start(newNode, duration);
//        TimerRemover* timer = new TimerRemover(context);
//        timer->Start(newNode, duration);
    }

    URHO3D_LOGINFOF("GameHelpers() - SpawnParticleEffect %s to node=%u", effectName.CString(), newNode->GetID());
}

void GameHelpers::SpawnParticleEffect(Context* context, const String& effectName, int layer, const Vector2& position, float worldAngle, float worldScale, bool removeTimer, float duration, const Color& color, CreateMode mode)
{
    Node* newNode = context->GetSubsystem<Renderer>()->GetViewport(0)->GetScene()->CreateChild("ParticuleEffect", mode);
    newNode->SetTemporary(true);

    newNode->SetWorldTransform2D(position, worldAngle, Vector2::ONE * worldScale);

    // Create the particle emitter
    SharedPtr<ParticleEffect2D> particleEffect(context->GetSubsystem<ResourceCache>()->GetResource<ParticleEffect2D>(effectName));
    if (!particleEffect)
        return;

    if (color != Color::WHITE)
    {
        particleEffect = particleEffect->Clone();
        particleEffect->SetStartColor(color);
        particleEffect->SetFinishColor(Color::TRANSPARENT);
    }

    ParticleEmitter2D* particleEmitter = newNode->CreateComponent<ParticleEmitter2D>();
    particleEmitter->SetLooped(false);
    particleEmitter->SetEffect(particleEffect);
    particleEmitter->SetLayer(layer+1);

    if (removeTimer)
    {
        if (duration == -1.0f)
            duration = particleEffect->GetDuration();

        // add alpha fade out
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context));
        alphaAnimation->SetKeyFrame(0.f, Color::GRAY);
        alphaAnimation->SetKeyFrame(0.75f * duration, Color::YELLOW);
        alphaAnimation->SetKeyFrame(duration, Color::TRANSPARENT);
        objectAnimation->AddAttributeAnimation("Color", alphaAnimation, WM_ONCE);
        particleEmitter->SetObjectAnimation(objectAnimation);

        TimerRemover::Get()->Start(newNode, duration + particleEffect->GetParticleLifeSpan());
//        (new TimerRemover(context))->Start(newNode, duration + particleEffect->GetParticleLifeSpan());
    }

    URHO3D_LOGINFOF("GameHelpers() - SpawnParticleEffect %s to node=%u", effectName.CString(), newNode->GetID());
}

Node* GameHelpers::SpawnParticleEffectInNode(Context* context, const String& effectName, int layer, Node* node, const Vector2& direction, float fscale, bool removeTimer, float duration, const Color& color, CreateMode mode)
{
    if (effectName == String::EMPTY) return 0;
    // Create the particle emitter
    ParticleEffect2D* particleEffect = context->GetSubsystem<ResourceCache>()->GetResource<ParticleEffect2D>(effectName);
    if (!particleEffect)
    {
        URHO3D_LOGERRORF("GameHelpers() - SpawnParticleEffectInNode : no particle Effect %s", effectName.CString());
        return 0;
    }

    const Vector2& position = node->GetWorldPosition2D();
    const Vector2& scale = node->GetWorldScale2D();

    Node* newNode = node->CreateChild("Effect", mode);

    if (direction.x_ !=0.0f)
        newNode->SetWorldTransform2D(position, (direction.x_ > 0.0f ? 180.0f : 0.0f), scale * fscale);
    else
        newNode->SetWorldTransform2D(position, (direction.y_ > 0.0f ? 270.0f : 90.0f), scale * fscale);

    ParticleEmitter2D* particleEmitter = newNode->CreateComponent<ParticleEmitter2D>();
    particleEmitter->SetEffect(particleEffect);
    particleEmitter->SetLayer(layer+1);

    if (removeTimer)
    {
        if (duration == -1.0f)
            duration = particleEffect->GetDuration()-0.2f;
        // add alpha fade out
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context));
        alphaAnimation->SetKeyFrame(0.f, color);
        alphaAnimation->SetKeyFrame(duration, Color::TRANSPARENT);
        objectAnimation->AddAttributeAnimation("Color", alphaAnimation, WM_ONCE);
        particleEmitter->SetObjectAnimation(objectAnimation);

        TimerRemover::Get()->Start(newNode, duration + particleEffect->GetParticleLifeSpan());
//        (new TimerRemover(context))->Start(newNode, duration + particleEffect->GetParticleLifeSpan());
    }
    return newNode;
}

void GameHelpers::AddEffects(Node* root, int index, int num, float x, float y, float delaybetweenspawns, float removedelay, int layer)
{
    Node* effects = COT::GetObjectFromCategory(COT::EFFECTS, index);

    for (int i=0; i < num; i++)
    {
        Node* effect = effects->Clone(LOCAL, true, 0, 0, root);
        effect->SetWorldPosition(Vector3(x+Random(-0.3f,0.3f), y+i*0.3f, 0.f));
        effect->GetDerivedComponent<StaticSprite2D>()->SetLayer(layer);
        effect->SetEnabled(false);

        TimerRemover::Get()->Start(effect, removedelay, FREEMEMORY, i*delaybetweenspawns);
    }
}

/// Component Helpers

void GameHelpers::AddPlane(const char *nodename, Context* context, Node* node, const char *materialname,const Vector3& pos, const Vector2& scale, CreateMode mode)
{
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();

    Node* planeNode = node->CreateChild(nodename, mode);

    planeNode->SetPosition(pos);
    planeNode->Pitch(-90);
    planeNode->SetScale(Vector3(scale.x_, 1.0f, scale.y_));
    StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>(materialname));
}

Node* GameHelpers::AddStaticSprite2D(Node* rootnode, CreateMode mode, const char *nodename, const char *texturename, const Vector3& position, const Vector3& scale, int layer, const char *materialname)
{
    ResourceCache* cache = GameStatics::context_->GetSubsystem<ResourceCache>();

    Node* node = rootnode->CreateChild(nodename, mode);
    node->SetPosition(position);
    node->SetScale(scale);

//    Sprite2D* sprite = node->CreateComponent<Sprite2D>();
//    sprite->SetTexture (cache->GetResource<Texture2D>(texturename));
//    sprite->SetRectangle (const IntRect &rectangle);
//    sprite->SetHotSpot (const Vector2 &hotSpot);
//    sprite->SetOffset (const IntVector2 &offset);
    Sprite2D* sprite = cache->GetResource<Sprite2D>(texturename);

    StaticSprite2D* staticSprite = node->CreateComponent<StaticSprite2D>();
    staticSprite->SetBlendMode(BLEND_ALPHA);
    staticSprite->SetSprite(sprite);
    staticSprite->SetLayer(layer);

    if (materialname)
        staticSprite->SetCustomMaterial(cache->GetResource<Material>(materialname));
//    staticSprite->SetUseHotSpot(true);

//    staticSprite->SetHotSpot(hotSpot_);

    return node;
}

AnimatedSprite2D* GameHelpers::AddAnimatedSprite2D(Node* root, const String& label, const String& spritename, const String& entityname,
                                            const Vector2& position, float rotation, bool reactive, const Vector2& reactivecenter, float reactiveradius)
{
    AnimationSet2D* animationset = GameStatics::context_->GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>(spritename);
    if (!animationset)
        return 0;

    Node* node = root->CreateChild(label, LOCAL);
    if (position.x_ != 0.f || position.y_ != 0.f)
        node->SetWorldPosition2D(position);
    if (rotation != 0.f)
        node->SetRotation2D(rotation);

    AnimatedSprite2D* animated = node->CreateComponent<AnimatedSprite2D>(LOCAL);
    animated->SetAnimationSet(animationset);
    if (!entityname.Empty())
    {
        animated->SetEntity(entityname);
        animated->SetSpriterAnimation(0);
        animated->SetLayer(1);
    }

    if (reactive)
    {
        node->CreateComponent<RigidBody2D>();
        CollisionCircle2D* collider = node->CreateComponent<CollisionCircle2D>(LOCAL);
        collider->SetRadius(reactiveradius);
        collider->SetCenter(reactivecenter);
    }

    return animated;
}

void GameHelpers::AddLight(Context* context, Node *node, LightType type, const Vector3& position, const Vector3& direction, const float& fov, const float& range, float brightness, bool pervertex, bool colorAnimated)
{
    if (!node)
    {
        URHO3D_LOGERROR("GameHelpers() - AddLight : node = 0 !");
        return;
    }

//    URHO3D_LOGINFOF("GameHelpers() - AddLight : on Node %s(%u) ...", node->GetName().CString(), node->GetID());

    Node* nodeLight = node->CreateChild("Light");
    nodeLight->SetPosition(position);
    nodeLight->SetDirection(direction);

    Light* light = nodeLight->CreateComponent<Light>();
    light->SetLightType(type);
    light->SetPerVertex(pervertex);
    light->SetBrightness(brightness);

    //if (type==LIGHT_POINT)
    {
        light->SetRange(range);
        light->SetFov(fov);
    }
    if (colorAnimated)
    {
        // Create light animation
        SharedPtr<ObjectAnimation> lightAnimation(new ObjectAnimation(context));
        // Create light color animation
        SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation(context));
        colorAnimation->SetKeyFrame(0.0f, Color::WHITE);
        colorAnimation->SetKeyFrame(2.0f, Color::YELLOW);
        colorAnimation->SetKeyFrame(4.0f, Color::WHITE);
        // Set Light component's color animation
        lightAnimation->AddAttributeAnimation("@Light/Color", colorAnimation);
        // Apply light animation to light node
        nodeLight->SetObjectAnimation(lightAnimation);
    }
//    light->SetEnabled(true);
}

void GameHelpers::AddAnimation(Node* node, const String& attribute, const Variant& value1, const Variant& value2, float time, WrapMode mode)
{
    if (!node)
        return;

    Animatable* animatable = node->GetDerivedComponent<StaticSprite2D>();
    if (!animatable)
        animatable = node;

    bool addnewobject = !animatable->GetObjectAnimation();
    SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(animatable->GetContext()) : animatable->GetObjectAnimation());

    SharedPtr<ValueAnimation> animation(new ValueAnimation(animatable->GetContext()));
    animation->SetKeyFrame(0.f, value1);
    if (mode == WM_LOOP)
    {
        animation->SetKeyFrame(time/2, value2);
        animation->SetKeyFrame(time, value1);
    }
    else
    {
        animation->SetKeyFrame(time, value2);
    }

    objectAnimation->AddAttributeAnimation(attribute, animation, mode);

    if (addnewobject)
        animatable->SetObjectAnimation(objectAnimation);
}

void GameHelpers::SetMoveAnimation(Node* node, const Vector3& from, const Vector3& to, float start, float delay)
{
    node->SetPosition(from);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GameStatics::context_));
    SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation(GameStatics::context_));
    positionAnimation->SetKeyFrame(start, from);
    positionAnimation->SetKeyFrame(start+delay, to);
    objectAnimation->AddAttributeAnimation("Position", positionAnimation, WM_ONCE);

    node->SetObjectAnimation(objectAnimation);

    node->SetEnabled(true);
}

void GameHelpers::AddText3DFadeAnim(Node* rootNode, const String& text, Text* originaltext, const Vector3& deltamove, float fadetime, float scalefactor)
{
    Node* node = rootNode->CreateChild("textanim", LOCAL);

    Text3D* text3d = node->CreateComponent<Text3D>();
    text3d->SetFont(originaltext->GetFont(), originaltext->GetFontSize());
    text3d->SetAlignment(originaltext->GetHorizontalAlignment(), originaltext->GetVerticalAlignment());
    if (originaltext->GetPivot().x_ == 0.5f)
        text3d->SetHorizontalAlignment(HA_CENTER);
    text3d->SetTextAlignment(originaltext->GetTextAlignment());
    for (int i=0; i<4; i++)
        text3d->SetColor(Corner(i), originaltext->GetColor(Corner(i)));

    text3d->SetText(text);

    // Start with null opacity in case of the disabled scene update
    float opacity = text3d->GetOpacity();
    text3d->SetOpacity(0.f);
    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> opacityAnimation(new ValueAnimation(node->GetContext()));
        opacityAnimation->SetKeyFrame(0.1f, opacity);
        opacityAnimation->SetKeyFrame(fadetime, 0.f);
        objectAnimation->AddAttributeAnimation("Opacity", opacityAnimation, WM_ONCE);
        text3d->SetObjectAnimation(objectAnimation);
    }

    IntVector2 correcttextposition;

    if (text3d->GetHorizontalAlignment() == HA_CENTER)
        correcttextposition.x_ = originaltext->GetWidth()/2;

    if (text3d->GetVerticalAlignment() == VA_BOTTOM)
        correcttextposition.y_ = originaltext->GetHeight();

    // add offset in y
    correcttextposition.y_ += originaltext->GetFontSize();

    Vector3 position = GameHelpers::ScreenToWorld2D(originaltext->GetScreenPosition().x_ * GameStatics::uiScale_ + correcttextposition.x_ * GameStatics::uiScale_,
                                                    originaltext->GetScreenPosition().y_ * GameStatics::uiScale_ + correcttextposition.y_ * GameStatics::uiScale_);

    node->SetPosition(position);

    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(node->GetContext()));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(node->GetContext()));
        scaleAnimation->SetKeyFrame(0.f, node->GetScale());
        scaleAnimation->SetKeyFrame(fadetime, node->GetScale()*scalefactor);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(node->GetContext()));
        moveAnimation->SetKeyFrame(0.f, position);
        moveAnimation->SetKeyFrame(fadetime, position+deltamove);
        objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
        node->SetObjectAnimation(objectAnimation);
    }

    TimerRemover::Get()->Start(node, fadetime, FREEMEMORY);
}


/// UI Helpers

void GameHelpers::ResetCamera()
{
    URHO3D_LOGINFOF("GameHelpers() - ResetCamera !");

    GameStatics::camera_->SetZoom(GameStatics::uiScale_);
    GameStatics::cameraNode_->SetPosition(Vector3::ZERO);
    GameStatics::ResetCamera();
}

void GameHelpers::ApplySizeRatio(int w, int h, IntVector2& size)
{
    if (size.x_ < size.y_)
        size.x_ = size.y_ * w / (float)h;
    else
        size.y_ = size.x_ * h / (float)w;
}

void GameHelpers::ApplySizeRatioMaximized(int w, int h, IntVector2& size)
{
    float scale = Max(size.x_/w, size.y_/h);

    size *= scale;
}

void GameHelpers::AddTextFadeAnim(UIElement* uiroot, const String& msg, Text* originaltext, const IntVector2& deltamove, float fadetime, float scalefactor)
{
    Text* text = uiroot->CreateChild<Text>();

    IntVector2 position = originaltext->GetScreenPosition();
    text->SetPosition(position);
    text->SetFont(originaltext->GetFont(), originaltext->GetFontSize());
    for (int i=0; i<4; i++)
        text->SetColor(Corner(i), originaltext->GetColor(Corner(i)));
    text->SetText(msg);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(uiroot->GetContext()));
    SharedPtr<ValueAnimation> opacityAnimation(new ValueAnimation(uiroot->GetContext()));
    opacityAnimation->SetKeyFrame(0.f, text->GetOpacity());
    opacityAnimation->SetKeyFrame(fadetime, 0.f);
    objectAnimation->AddAttributeAnimation("Opacity", opacityAnimation, WM_ONCE);
//    SharedPtr<ValueAnimation> fontAnimation(new ValueAnimation(uiroot->GetContext()));
//    fontAnimation->SetKeyFrame(0.f, text->GetFontSize());
//    fontAnimation->SetKeyFrame(fadetime, text->GetFontSize()*3);
//    objectAnimation->AddAttributeAnimation("FontSize", fontAnimation, WM_ONCE);
    SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(uiroot->GetContext()));
    moveAnimation->SetKeyFrame(0.f, position);
    moveAnimation->SetKeyFrame(fadetime, position+deltamove);
    objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
    text->SetObjectAnimation(objectAnimation);

    TimerRemover::Get()->Start(text, fadetime, FREEMEMORY);
}

TextMessage* GameHelpers::AddUIMessage(const String& text, bool localize, const String& font, int fontsize, const Color& color, const IntVector2& position, float duration, float delaystart)
{
    TextMessage* message = TextMessage::Get();
    message->Set(localize ? GameStatics::context_->GetSubsystem<Localization>()->Get(text)+" !" : text, font.CString(), fontsize, duration, position, true, delaystart);
    if (&color != &Color::WHITE)
        message->SetColor(color, color, color, color);
    return message;
}

MessageBox* GameHelpers::AddMessageBox(const String& title, const String& question, bool questionl10, const String& answer1, const String& answer2, Object* subscriber, EventHandler* handler, XMLFile* layoutFile)
{
    Localization* l10n = subscriber->GetContext()->GetSubsystem<Localization>();

    String q = questionl10 ? l10n->Get(question) : question;
    if (q != " ")
        q += " ?";

    MessageBox* message = new Urho3D::MessageBox(subscriber->GetContext(), q, l10n->Get(title), layoutFile);
    UIElement* box = message->GetWindow();

    if (box)
    {
        box->SetPivot(0.5f, 0.5f);
        box->SetPosition(GameStatics::ui_->GetRoot()->GetSize()/2);

        Button* yesButton = static_cast<Button*>(box->GetChild("OkButton", true));
        static_cast<Text*>(yesButton->GetChild("Text", false))->SetText(l10n->Get(answer1));
        yesButton->SetVisible(true);
        yesButton->SetFocus(true);

        Button* noButton = static_cast<Button*>(box->GetChild("CancelButton", true));
        static_cast<Text*>(noButton->GetChild("Text", false))->SetText(l10n->Get(answer2));

        subscriber->SubscribeToEvent(message, E_MESSAGEACK, handler);

        if (GameStatics::cursor_)
            if (!GameStatics::cursor_->IsVisible())
                GameStatics::cursor_->SetVisible(true);

		return message;
    }


	return 0;
}

InteractiveFrame* GameHelpers::AddInteractiveFrame(const String& framefile, Object* subscriber, EventHandler* handler, bool autostart)
{
    InteractiveFrame* frame = InteractiveFrame::Get(framefile, true);
    if (frame)
    {
        if (!frame->GetNode())
            frame->Init();

        if (subscriber && handler)
        {
            subscriber->SubscribeToEvent(frame, E_MESSAGEACK, handler);
            if (autostart)
                frame->Start(false, false);
        }

        if (!frame->GetNode())
            frame->Init();
    }

    return frame;
}

AnimatedSprite* GameHelpers::AddAnimatedSpriteUI(UIElement* root, const String& label, const String& spritename, const String& entityname, const Vector2& position, float rotation)
{
    AnimationSet2D* animationset = GameStatics::context_->GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>(spritename);
    if (!animationset)
        return 0;

    AnimatedSprite* animatedSprite = root ? root->CreateChild<AnimatedSprite>() : new AnimatedSprite(GameStatics::context_);
    animatedSprite->SetName(label);
    if (position.x_ != 0.f || position.y_ != 0.f)
        animatedSprite->SetPosition(position);
    if (rotation != 0.f)
        animatedSprite->SetRotation(rotation);

    animatedSprite->SetAnimationSet(animationset);
    if (!entityname.Empty())
        animatedSprite->SetEntity(entityname);

    animatedSprite->SetAnimation(String::EMPTY);

    return animatedSprite;
}

void GameHelpers::SetMoveAnimationUI(UIElement* elt, const IntVector2& from, const IntVector2& to, float start, float delay)
{
    if (!elt)
        return;

    elt->SetPosition(from);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GameStatics::context_));
    SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation(GameStatics::context_));
    positionAnimation->SetKeyFrame(start, from);
    positionAnimation->SetKeyFrame(start+delay, to);
    objectAnimation->AddAttributeAnimation("Position", positionAnimation, WM_ONCE);

    elt->SetObjectAnimation(objectAnimation);

    elt->SetVisible(true);
}

void GameHelpers::SetScaleAnimationUI(UIElement* elt, float from, float to, float mid, float start, float delay)
{
    if (!elt)
        return;

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GameStatics::context_));
    SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(GameStatics::context_));

    if (elt->IsInstanceOf<Sprite>())
    {
        scaleAnimation->SetKeyFrame(start, Vector2(from, from));
        scaleAnimation->SetKeyFrame(start+delay*0.5f, Vector2(mid, mid));
        scaleAnimation->SetKeyFrame(start+delay, Vector2(to, to));
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
    }
    else
    {
        scaleAnimation->SetKeyFrame(start, IntVector2(elt->GetSize().x_ * from, elt->GetSize().y_ * from));
        scaleAnimation->SetKeyFrame(start+delay*0.5f, IntVector2(elt->GetSize().x_ * mid, elt->GetSize().y_ * mid));
        scaleAnimation->SetKeyFrame(start+delay, IntVector2(elt->GetSize().x_ * to, elt->GetSize().y_ * to));
        objectAnimation->AddAttributeAnimation("Size", scaleAnimation, WM_ONCE);
    }

    elt->SetObjectAnimation(objectAnimation);

    elt->SetEnabled(true);
}

void GameHelpers::FindParentWithAttribute(const StringHash& attribute, UIElement* element, UIElement*& parent, Variant& var)
{
    bool ok = false;
    parent = element;
//    URHO3D_LOGINFOF("element name=%s ok=%u", element->GetName().CString(), ok);
    while (!ok)
    {
        if (parent)
        {
            var = parent->GetVar(attribute);
            ok = (var == Variant::EMPTY ? false : true);
//            URHO3D_LOGINFOF("parent name=%s ok=%u", parent->GetName().CString(), ok);
            if (!ok)
                parent = parent->GetParent();
        }
        else
            ok = true;
    }
//    URHO3D_LOGINFOF("FindParentWithAttribute ok=%u", ok);
    //if (parent == GetSubsystem<UI>()->GetRoot() || !parent) return 0;
}

void GameHelpers::FindParentWhoContainsName(const String& name, UIElement* element, UIElement*& parent)
{
    bool ok = false;
    parent = element;
//    URHO3D_LOGINFOF("element name=%s ok=%u", element->GetName().CString(), ok);
    while (!ok)
    {
        if (parent)
        {
            ok = (parent->GetName().Contains(name) ? true : false);
//            URHO3D_LOGINFOF("parent name=%s ok=%u", parent->GetName().CString(), ok);
            if (!ok)
                parent = parent->GetParent();
        }
        else
            ok = true;
    }
//    URHO3D_LOGINFOF("FindParentWhoContainsName ok=%u", ok);
    //if (parent == GetSubsystem<UI>()->GetRoot() || !parent) return 0;
}

void GameHelpers::SetUIScale(UIElement* root, const Vector2& scale)
{
//    if (scale == Vector2::ONE)
//        return;

//    root->DisableLayoutUpdate();
    SetScaleChildRecursive(root, scale);
//    root->EnableLayoutUpdate();

//    root->UpdateLayout();

	URHO3D_LOGINFOF("GameHelpers() - SetUIScale : root=%s(ptr=%u) scale=%u", root->GetName().CString(), root, scale.ToString().CString());
}

String GameHelpers::GetRandomString(unsigned size)
{
    const String lexic("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    String randomstr;
    for (unsigned i = 0; i < size; i++)
        randomstr += lexic[Random((int)lexic.Length())];
    return randomstr;
}

void GameHelpers::SetScaleChildRecursive(UIElement* elt, const Vector2& scale)
{
    elt->SetPosition(floor((float)elt->GetPosition().x_ * scale.x_), floor((float)elt->GetPosition().y_ * scale.y_));

    const Vector<SharedPtr<UIElement> >& children = elt->GetChildren();
    for (Vector<SharedPtr<UIElement> >::ConstIterator it=children.Begin(); it != children.End(); ++it)
        SetScaleChildRecursive(it->Get(), scale);

    if (elt->IsInstanceOf<Text>())
    {
        float scalefactor = Min(scale.x_, scale.y_);

        Text* text = static_cast<Text*>(elt);
//        text->SetFontSize(Max((int)floor(text->GetFontSize()*scalefactor), 6));
        text->SetFontSize(Clamp((int)floor(text->GetFontSize()*scalefactor), 6, 40));
        //URHO3D_LOGINFOF("GameHelpers() - SetScaleChildRecursive : ptr=%u parent=%u name=%s type=%s scale=%f fontsize=%d size=%s",
        //                text, text->GetParent(), text->GetName().CString(), text->GetTypeName().CString(), scalefactor,
        //                text->GetFontSize(), text->GetSize().ToString().CString());
    }
    else
    {
        if (elt->IsInstanceOf<DropDownList>())
            SetScaleChildRecursive(static_cast<DropDownList*>(elt)->GetPopup(), scale);

        if (elt->GetMinSize() != IntVector2::ZERO)
            elt->SetMinSize(elt->GetMinSize().x_ * scale.x_, elt->GetMinSize().y_ * scale.y_);
        if (elt->GetMaxSize() != IntVector2(M_MAX_INT, M_MAX_INT))
            elt->SetMaxSize(elt->GetMaxSize().x_ * scale.x_, elt->GetMaxSize().y_ * scale.y_);

        if (elt->GetSize().x_ == elt->GetSize().y_)
        {
            // keep square aspect
            int size = floor(Min(elt->GetSize().x_, elt->GetSize().y_) * Min(scale.x_, scale.y_));
            elt->SetSize(size, size);
        }
        else
            elt->SetSize(floor((float)elt->GetSize().x_ * scale.x_), floor((float)elt->GetSize().y_ * scale.y_));

        //URHO3D_LOGINFOF("GameHelpers() - SetScaleChildRecursive : ptr=%u parent=%u name=%s type=%s size=%s(min=%s,max=%s)",
        //                 elt, elt->GetParent(), elt->GetName().CString(), elt->GetTypeName().CString(),
        //                 elt->GetSize().ToString().CString(), elt->GetMinSize().ToString().CString(), elt->GetMaxSize().ToString().CString());
    }
}


/// Input Helpers

void GameHelpers::GetInputPosition(int& x, int& y, UIElement** uielt)
{
    if (GameStatics::gameConfig_.touchEnabled_ && GameStatics::input_->GetTouch(0))
    {
        x = GameStatics::input_->GetTouch(0)->position_.x_;
        y = GameStatics::input_->GetTouch(0)->position_.y_;
    }
    else if (GameStatics::cursor_)
    {
        x = GameStatics::cursor_->GetPosition().x_ * GameStatics::ui_->GetScale();
        y = GameStatics::cursor_->GetPosition().y_ * GameStatics::ui_->GetScale();
    }
    else
    {
        x = GameStatics::input_->GetMousePosition().x_;
        y = GameStatics::input_->GetMousePosition().y_;
    }

    if (uielt)
        *uielt = GameStatics::ui_->GetElementAt(x, y);
}


/// Math / Converter Helpers


float GameHelpers::GetMaximizedScaleWithImageRatioKeeped(Vector2 framesize, Vector2 imagesize)
{
    float aratio = imagesize.x_ / imagesize.y_;

    if (framesize.x_ < framesize.y_)
        framesize.x_ = framesize.y_ * aratio;
    else
        framesize.y_ = framesize.x_ / aratio;

    return Min(framesize.x_ / imagesize.x_, framesize.y_ / imagesize.y_);
}

//// Adjust image size inside frame
float GameHelpers::GetMinAdjustedScale(Vector2 framesize, Vector2 imagesize)
{
    return Min(framesize.x_/imagesize.x_, framesize.y_/imagesize.y_);
}

Vector2 GameHelpers::GetMinAdjustedSize(Vector2 framesize, Vector2 imagesize)
{
    return imagesize * GetMinAdjustedScale(framesize, imagesize);
}

// Adjust image size outside frame
float GameHelpers::GetMaxAdjustedScale(Vector2 framesize, Vector2 imagesize)
{
    return Max(framesize.x_/imagesize.x_, framesize.y_/imagesize.y_);
}

Vector2 GameHelpers::GetMaxAdjustedSize(Vector2 framesize, Vector2 imagesize)
{
    return imagesize * GetMaxAdjustedScale(framesize, imagesize);
}

void GameHelpers::SetAdjustedToScreen(Node* node, float escale, float minscale, bool maximize)
{
    if (!node)
        return;

    float scale = 1.f;

    Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
    if (drawable)
        scale = maximize ? GetMaxAdjustedScale(GameStatics::fScreenSize_, drawable->GetDrawRectangle().Size()) :
                           GetMinAdjustedScale(GameStatics::fScreenSize_, drawable->GetDrawRectangle().Size());
    // don't reduce
    if (!maximize && scale < minscale)
        scale = minscale;

    node->SetWorldScale(scale * escale * Vector3::ONE);

    URHO3D_LOGINFOF("GameHelpers() - SetAdjustedToScreen : node=%s(%u) entryscale=%F adjustscale=%F outputScale=%F",
                    node->GetName().CString(), node->GetID(), escale, scale, scale*escale);
}

void GameHelpers::ClampSizeTo(IntVector2& size, int dimension)
{
    float ratioSize = float(size.y_) / float(size.x_);
    if (ratioSize > 1.f)
    {
        size.y_ = Clamp(size.y_, 1, dimension);
        size.x_ = int(float(size.y_) / ratioSize);
    }
    else
    {
        size.x_ = Clamp(size.x_, 1, dimension);
        size.y_ = int(float(size.x_) * ratioSize);
    }
}


Vector2 GameHelpers::ScreenToWorld2D(int screenx, int screeny)
{
    return GameStatics::viewport_->ScreenToWorldPoint(screenx, screeny, 0.f).ToVector2();
}

Vector2 GameHelpers::ScreenToWorld2D(const IntVector2& screenpoint)
{
    return GameStatics::viewport_->ScreenToWorldPoint(screenpoint.x_, screenpoint.y_, 0.f).ToVector2();
}

Vector2 GameHelpers::ScreenToWorld2D_FixedZoom(const IntVector2& screenpoint)
{
    return GameStatics::fixedViewport_->ScreenToWorldPoint(screenpoint.x_, screenpoint.y_, 0.f).ToVector2();
}

//Vector3 GameHelpers::ScreenToWorld2D(int x, int y)
//{
//    const BoundingBox& frustrumBox = GameStatics::rootScene_->GetComponent<Renderer2D>()->GetFrustumBoundingBox();
//    Vector3 position;
//    float screenX = (float)x / (float)GameStatics::graphics_->GetWidth();
//    float screenY = 1.f - ((float)y / (float)GameStatics::graphics_->GetHeight());
//    position.x_ = frustrumBox.min_.x_ + screenX * (frustrumBox.max_.x_ - frustrumBox.min_.x_);
//    position.y_ = frustrumBox.min_.y_ + screenY * (frustrumBox.max_.y_ - frustrumBox.min_.y_);
//    return position;
//}

//Vector3 GameHelpers::ScreenToWorld2D(int x, int y)
//{
//    float screenX = (float)x / (float)GameStatics::graphics_->GetWidth();
//    float screenY = (float)y / (float)GameStatics::graphics_->GetHeight();
//    return GameStatics::camera_->ScreenToWorldPoint(Vector3(screenX, screenY, 0.f));
//}

//Vector3 GameHelpers::ScreenToWorld2D(const IntVector2& screenpoint)
//{
//    return ScreenToWorld2D(screenpoint.x_, screenpoint.y_);
//}

//void GameHelpers::OrthoWorldToScreen(IntVector2& point, const Vector3& position, Camera* camera)
//{
//    if (!camera)
//        camera = GameStatics::camera_;
//
//    Graphics* graphics = camera->GetSubsystem<Graphics>();
//    point.x_ = (int)(0.5f * graphics->GetWidth() - (position.x_ * camera->GetZoom() / PIXEL_SIZE));
//    point.y_ = (int)(0.5f * graphics->GetHeight() - (position.y_ * camera->GetZoom() / PIXEL_SIZE));
//}

void GameHelpers::OrthoWorldToScreen(IntVector2& point, const Vector3& position)
{
    // we can do it with renderer2d
//    const BoundingBox& frustrumBox = GameStatics::renderer2d_->GetFrustumBoundingBox();
//    point.x_ = (int)((position.x_ - frustrumBox.min_.x_) / (frustrumBox.max_.x_ - frustrumBox.min_.x_) * (float)GameStatics::graphics_->GetWidth());
//    point.y_ = (int)((1.f - (position.y_ - frustrumBox.min_.y_) / (frustrumBox.max_.y_ - frustrumBox.min_.y_)) * (float)GameStatics::graphics_->GetHeight());

    // or use viewport, we just need to have z > 0.f so use PIXEL_SIZE as value.
    point = GameStatics::viewport_->WorldToScreenPoint(Vector3(position.x_, position.y_, PIXEL_SIZE));
}

void GameHelpers::OrthoWorldToScreen(IntVector2& point, Node* node)
{
    if (!node)
        return;

    OrthoWorldToScreen(point, node->GetWorldPosition());
}

void GameHelpers::OrthoWorldToScreen(IntRect& rect, const BoundingBox& b)
{
//    IntVector2 imin, imax;
//    OrthoWorldToScreen(imin, Vector3(b.min_.x_, b.max_.y_, PIXEL_SIZE));
//    OrthoWorldToScreen(imax, Vector3(b.max_.x_, b.min_.y_, PIXEL_SIZE));
//    rect.Define(imin, imax);

    // Works well, we just need to have z > 0.f so use PIXEL_SIZE as value.
    IntVector2 imin = GameStatics::viewport_->WorldToScreenPoint(Vector3(b.min_.x_, b.max_.y_, PIXEL_SIZE));
    IntVector2 imax = GameStatics::viewport_->WorldToScreenPoint(Vector3(b.max_.x_, b.min_.y_, PIXEL_SIZE));
    rect.Define(imin, imax);
}

void GameHelpers::EqualizeValues(PODVector<float>& values, int coeff, bool incbleft, bool incbright)
{
    unsigned numvalues = values.Size();

    if (numvalues < 2)
        return;

    unsigned starti = (incbleft || numvalues < 3) ? 0 : 1;
    unsigned endi = (incbright || numvalues < 3) ? numvalues : numvalues-1;

    // get the average

    float average = 0.f;
    for (unsigned i=0; i < numvalues; i++)
    {
        average += values[i];
    }
    average /= numvalues;

    // smooth each value to the average
    int sumcoeff = coeff+1;
    for (unsigned i=starti; i < endi; i++)
        values[i] = (values[i] + coeff*average) / sumcoeff;
}

/// FloodFill : remplit les zones continues definies par la valeur patternToFill avec le motif fillPattern
template< typename T >
bool GameHelpers::FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFill, const T& fillPattern, const int& xo, const int& yo)
{
//    URHO3D_LOGINFOF("FloodFill ... x=%d y=%d patternToFill=%d fillPattern=%d", xo, yo, (int)patternToFill, (int)fillPattern);

    int width = buffer.Width();
    int height = buffer.Height();
    int y1, num = 0;
    bool spanLeft, spanRight;

//    URHO3D_LOGINFOF("FloodFill ... Init OK ...");

    stack.Clear();

    int x = xo, y = yo;
    unsigned addr = width*y + x;

    if (!stack.Push(addr))
    {
        URHO3D_LOGERROR("FloodFill ... Memory Stack Error");
        return false;
    }

//    URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d", x, y);

    while (stack.Pop(addr))
    {
        x = addr % width;
        y = addr / width;
        y1 = y;
        while(y1 >= 0 && buffer(x,y1) == patternToFill) y1--;
        y1++;
        spanLeft = spanRight = 0;
        while (y1 < height && buffer(x,y1) == patternToFill)
        {
            buffer(x,y1) = fillPattern;
//            URHO3D_LOGINFOF("FloodFill ... Fill x=%d y=%d with fillPattern=%d result=%d", x, y1, (int)fillPattern, (int)buffer(x,y1));
            num++;
            if (!spanLeft && x > 0 && buffer(x-1,y1) == patternToFill)
            {
                if (!stack.Push(y1*width+x-1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Left", x-1, y1, num);
                spanLeft = 1;
            }
            else if (spanLeft && x > 0 && buffer(x-1,y1) != patternToFill)
            {
                spanLeft = 0;
            }
            if (!spanRight && x < width - 1 && buffer(x+1,y1) == patternToFill)
            {
                if (!stack.Push(y1*width+x+1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Right", x+1, y1, num);
                spanRight = 1;
            }
            else if (spanRight && x < width - 1 && buffer(x+1,y1) != patternToFill)
            {
                spanRight = 0;
            }
            y1++;
        }
    }

//    URHO3D_LOGINFOF("FloodFill ... OK ! num = %d", num);

    return num!=0;
}

/// FloodFill : remplit les zones continues definit par les valeurs comprises entre patternToFillFrom et patternToFillTo avec le motif fillPattern
template< typename T >
bool GameHelpers::FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFillFrom, const T& patternToFillTo, const T& fillPattern, const int& xo, const int& yo)
{
//    URHO3D_LOGINFOF("FloodFill ... x=%d y=%d patternToFill=%d fillPattern=%d", xo, yo, (int)patternToFill, (int)fillPattern);

    int width = buffer.Width();
    int height = buffer.Height();
    int y1, num = 0;
    bool spanLeft, spanRight;

//    URHO3D_LOGINFOF("FloodFill ... Init OK ...");

    stack.Clear();

    int x = xo, y = yo;
    unsigned addr = width*y + x;

    if (!stack.Push(addr))
    {
        URHO3D_LOGERROR("FloodFill ... Memory Stack Error");
        return false;
    }

//    URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d", x, y);

    while (stack.Pop(addr))
    {
        x = addr % width;
        y = addr / width;
        y1 = y;
        while(y1 >= 0 && (buffer(x,y1) >= patternToFillFrom && buffer(x,y1) <= patternToFillTo)) y1--;
        y1++;
        spanLeft = spanRight = 0;
        while (y1 < height && (buffer(x,y1) >= patternToFillFrom && buffer(x,y1) <= patternToFillTo))
        {
            buffer(x,y1) = fillPattern;
//            URHO3D_LOGINFOF("FloodFill ... Fill x=%d y=%d with fillPattern=%d result=%d", x, y1, (int)fillPattern, (int)buffer(x,y1));
            num++;
            if (!spanLeft && x > 0 && (buffer(x-1,y1) >= patternToFillFrom && buffer(x-1,y1) <= patternToFillTo))
            {
                if (!stack.Push(y1*width+x-1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Left", x-1, y1, num);
                spanLeft = 1;
            }
            else if (spanLeft && x > 0 && !(buffer(x-1,y1) >= patternToFillFrom && buffer(x-1,y1) <= patternToFillTo))
            {
                spanLeft = 0;
            }
            if (!spanRight && x < width - 1 && (buffer(x+1,y1) >= patternToFillFrom && buffer(x+1,y1) <= patternToFillTo))
            {
                if (!stack.Push(y1*width+x+1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Right", x+1, y1, num);
                spanRight = 1;
            }
            else if (spanRight && x < width - 1 && !(buffer(x+1,y1) >= patternToFillFrom && buffer(x+1,y1) <= patternToFillTo))
            {
                spanRight = 0;
            }
            y1++;
        }
    }

//    URHO3D_LOGINFOF("FloodFill ... OK ! num = %d", num);

    return num!=0;
}

// FloodFill Type use
template bool GameHelpers::FloodFill(Matrix2D<unsigned>& buffer, Stack<unsigned>& stack, const unsigned& patternToFill, const unsigned& fillPattern, const int& xo, const int& yo);




/// Audio Helpers

void GameHelpers::SpawnSound(Node* node, const char* fileName, float gain)
{
    if (!node || !GameStatics::playerState_->soundEnabled_)
        return;

    ResourceCache* cache = node->GetContext()->GetSubsystem<ResourceCache>();
    Sound* sound = cache->GetResource<Sound>(String(fileName));
    SoundSource* soundSource = node->CreateComponent<SoundSource>();
    soundSource->SetSoundType(SOUND_EFFECT);
    soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
    soundSource->SetGain(gain);
    soundSource->Play(sound);

//    URHO3D_LOGINFOF("GameHelpers() - SpawnSound : %s ", fileName);
}

void GameHelpers::SpawnSound(Node* node, int soundid, float gain)
{
    if (!node || !GameStatics::playerState_->soundEnabled_ || soundid == -1)
        return;

    const char* fileName = GameStatics::Sounds[soundid];
    ResourceCache* cache = node->GetContext()->GetSubsystem<ResourceCache>();
    Sound* sound = cache->GetResource<Sound>(fileName);
    SoundSource* soundSource = node->CreateComponent<SoundSource>();
    soundSource->SetSoundType(SOUND_EFFECT);
    soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
    soundSource->SetGain(gain);
    soundSource->Play(sound);

//    URHO3D_LOGINFOF("GameHelpers() - SpawnSound : %s ", fileName);
}

void GameHelpers::SpawnSound3D(Node* node, const char* fileName, float gain)
{
    if (!node || !GameStatics::playerState_->soundEnabled_)
        return;

    ResourceCache* cache = node->GetContext()->GetSubsystem<ResourceCache>();
    Sound* sound = cache->GetResource<Sound>(fileName);
    SoundSource3D* soundSource = node->CreateComponent<SoundSource3D>();
    soundSource->SetSoundType(SOUND_EFFECT);
    soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
    soundSource->SetGain(gain);
    soundSource->SetAttenuation(0.7f);
    soundSource->SetFarDistance(12.f);
    soundSource->Play(sound);
}

void GameHelpers::SetMusic(int channel, float gain, int musicid, bool loop)
{
    if (!GameStatics::playerState_->musicEnabled_ && (channel == MAINMUSIC || !GameStatics::playerState_->soundEnabled_))
        return;

    Node* channelNode = GameStatics::soundNodes_[channel];
    if (!channelNode)
        return;

    SoundSource* soundcomponent = channelNode->GetOrCreateComponent<SoundSource>(LOCAL);
    if (!soundcomponent)
        return;

    // Set gain
    soundcomponent->SetGain(gain);

    URHO3D_LOGINFOF("GameStatics() - SetMusic : musicid=%d ...", musicid);

    // Set sound
    if (musicid != -1)
    {
        const char* musicfilename = GameStatics::Musics[musicid];
        Sound* soundrsc = channelNode->GetContext()->GetSubsystem<ResourceCache>()->GetResource<Sound>(musicfilename);
        if (!soundrsc)
        {
            URHO3D_LOGERRORF("GameStatics() - SetMusic : no fileName=%s !", musicfilename);
            return;
        }

        if (soundcomponent->IsPlaying() && soundrsc && soundcomponent->GetSound() != soundrsc)
        {
            soundcomponent->Stop();
            URHO3D_LOGINFOF("GameStatics() - SetMusic : Stop ...");
        }

        if (!soundcomponent->IsPlaying() && soundrsc)
        {
            soundrsc->SetLooped(loop);
            // Reset the frequence to be sure to get the frequence of the current sample
            soundcomponent->SetFrequency(0.f);
            soundcomponent->SetSoundType(SOUND_MUSIC);
            soundcomponent->Play(soundrsc);
            URHO3D_LOGINFOF("GameStatics() - SetMusic : %s !", musicfilename);
        }
    }
}

void GameHelpers::StopMusic(int channel)
{
    Node* channelNode = GameStatics::soundNodes_[channel];
    if (!channelNode)
        return;

    SoundSource* soundcomponent = channelNode->GetComponent<SoundSource>();
    if (soundcomponent)
        soundcomponent->Stop();

    URHO3D_LOGINFOF("GameHelpers() - StopMusic : channel=%d", channel);
}

void GameHelpers::StopMusics()
{
    for (int i=0; i < NUMSOUNDCHANNELS; i++)
        StopMusic(i);
}

void GameHelpers::StopSound(Node* node)
{
    SoundSource* source = node->GetComponent<SoundSource>();
    if (source)
        source->Stop();
}

void GameHelpers::SetSoundVolume(float volume)
{
    GameStatics::context_->GetSubsystem<Audio>()->SetMasterGain(SOUND_EFFECT, volume);
}

void GameHelpers::SetMusicVolume(float volume)
{
    GameStatics::context_->GetSubsystem<Audio>()->SetMasterGain(SOUND_MUSIC, volume);
}



/// Dump Helpers

static String dumpDeepth;
static unsigned dumpComponentIndex;
static unsigned dumpNodeIndex;

void GameHelpers::DumpText(const char* buffer)
{
    if (buffer) std::cout << buffer << std::endl;
}

void GameHelpers::DumpMap(int width, int height, const int* data)
{
    unsigned Size = width * height;
    char blockChar;
	String str;
	str.Reserve(Size + 25);
	str += "GameHelpers() - DumpMap\n";
	for (unsigned i = 0; i < Size; i++)
	{
		if (i > 0 && i % width == 0) str += "\n";

        if (data[i] == 0)
            blockChar = '.';
        else
            blockChar = '*';

		str += blockChar;
		//str += " ";
	}
    URHO3D_LOGINFOF("%s",str.CString());
}

void GameHelpers::DumpObject(Object* ref)
{
    if (ref)
    {
        URHO3D_LOGINFOF("GameHelpers() - DumpObject : ptr=%u numRefs=%d(w=%d) objectType=%s", ref, ref->Refs(), ref->WeakRefs(),ref->GetTypeName().CString());
    }
    else
        URHO3D_LOGINFO("GameHelpers() - DumpObject : ptr=0");
}

void GameHelpers::DumpNodeRecursive(Node* node, unsigned currentDeepth, bool withcomponent, int deepth)
{
    if (!node)
        return;

    dumpDeepth = "";
    {
        int i = 0;
        while (i++ < currentDeepth) dumpDeepth += "=";
    }

    if (withcomponent)
    {
        for (Vector<SharedPtr<Component> >::ConstIterator it=node->GetComponents().Begin(); it!=node->GetComponents().End(); ++it, ++dumpComponentIndex)
            URHO3D_LOGINFOF(" %s=> [c%u] = %s(%u)", dumpDeepth.CString(), dumpComponentIndex, (*it)->GetTypeName().CString(), (*it)->GetID());
    }

    for (Vector<SharedPtr<Node> >::ConstIterator it=node->GetChildren().Begin();it!=node->GetChildren().End(); ++it)
    {
        ++dumpNodeIndex;

        URHO3D_LOGINFOF(" %s=> [n%u] = %s(%u)", dumpDeepth.CString(), dumpNodeIndex, (*it)->GetName().CString(), (*it)->GetID());
        if (deepth)
            GameHelpers::DumpNodeRecursive(*it, currentDeepth+1, withcomponent, deepth-1);
    }
}

void GameHelpers::DumpNode(Node* node, int maxrecursdeepth, bool withcomponent)
{
    if (!node)
        return;

    URHO3D_LOGINFOF("GameHelpers() - DumpNode : name=%s id=%u numref=%d(w=%d) numchild=%u numcomponents=%u",
             node->GetName().CString(), node->GetID(), node->Refs(), node->WeakRefs(), node->GetNumChildren(), node->GetNumComponents());

    unsigned currentDeepth = 0;
    dumpComponentIndex = 0;
    dumpNodeIndex = 0;

    if (withcomponent)
    {
        for (Vector<SharedPtr<Component> >::ConstIterator it=node->GetComponents().Begin();it!=node->GetComponents().End(); ++it, ++dumpComponentIndex)
            URHO3D_LOGINFOF(" => [c%u] = %s(%u) enabled=%s", dumpComponentIndex, (*it)->GetTypeName().CString(), (*it)->GetID(), (*it)->IsEnabled() ? "true" : "false");
    }

    for (Vector<SharedPtr<Node> >::ConstIterator it=node->GetChildren().Begin();it!=node->GetChildren().End(); ++it)
    {
        if (*it == 0)
        {
            URHO3D_LOGWARNINGF("GameHelpers() - DumpNode : NodeIndex=n%u Empty !", dumpNodeIndex);
            continue;
        }

        ++dumpNodeIndex;

        String name = (*it)->GetName();
        URHO3D_LOGINFOF(" => [n%u] = %s(%u)", dumpNodeIndex, name.CString(), (*it)->GetID());

        // skip preloader and pool
        if (name == "PreLoad" || name == "PreLoadGOT" || name == "PrepareNodes" || name == "StaticScene")
            continue;

        if (maxrecursdeepth)
            GameHelpers::DumpNodeRecursive(*it, currentDeepth+1, withcomponent, maxrecursdeepth-1);
    }

    if (dumpNodeIndex+dumpComponentIndex > 0)
        URHO3D_LOGINFO(" ------------ ");
}

void GameHelpers::DumpNode(unsigned id, bool withcomponentattr)
{
    Node* node = GameStatics::rootScene_->GetNode(id);
    if (!node)
    {
        URHO3D_LOGINFOF("GameHelpers() - DumpNode : No Node id=%u", id);
        return;
    }
    // Dump Variables
    {
        const VariantMap& variables = node->GetVars();
        Context* context = node->GetContext();
        for (VariantMap::ConstIterator it=variables.Begin();it!=variables.End();++it)
        {
            const String& attrname = context->GetUserAttributeName(it->first_);
            URHO3D_LOGINFOF(" => Var %s(%u) type=%s value=%s", attrname.CString(), it->first_.Value(), it->second_.GetTypeName().CString(), it->second_.ToString().CString());
        }
    }
    if (!withcomponentattr)
    {

        URHO3D_LOGINFOF("GameHelpers() - DumpNode : Node %s(%u) enabled=%s position=%s Parent %s(%u)", node->GetName().CString(), id, node->IsEnabled() ? "true" : "false",
            node->GetWorldPosition().ToString().CString(), node->GetParent() ? node->GetParent()->GetName().CString() : "NULL", node->GetParent() ? node->GetParent()->GetID() : 0);

        const Vector<SharedPtr<Component> >& components = node->GetComponents();
        for (unsigned i=0; i < components.Size(); i++)
        {
            URHO3D_LOGINFOF(" => Component %s(%u) enabled=%s", components[i]->GetTypeName().CString(), components[i]->GetID(), components[i]->IsEnabled() ? "true" : "false");
            if (components[i]->IsInstanceOf<Drawable2D>())
            {
                Drawable2D* drawable = (Drawable2D*) components[i].Get();
                URHO3D_LOGINFOF("  => Drawable2D BoundingBox World=%s Local=%s", drawable->GetWorldBoundingBox().ToString().CString(), drawable->GetBoundingBox().ToString().CString());
            }
        }
        const Vector<SharedPtr<Node> >& children = node ->GetChildren();
        for (unsigned i=0; i < children.Size(); i++)
            URHO3D_LOGINFOF(" => Child %s(%u)", children[i]->GetName().CString(), children[i]->GetID());
    }
    else
    {
        URHO3D_LOGINFOF("GameHelpers() - DumpNode : Node %s(%u) enabled=%s Parent %s(%u)", node->GetName().CString(), id, node->IsEnabled() ? "true" : "false",
            node->GetParent() ? node->GetParent()->GetName().CString() : "NULL", node->GetParent() ? node->GetParent()->GetID() : 0);

        const Vector<SharedPtr<Component> >& components = node->GetComponents();

        for (unsigned i=0; i < components.Size(); i++)
        {
            URHO3D_LOGINFOF(" => Component %s(%u)", components[i]->GetTypeName().CString(), components[i]->GetID());
            Component* component = components[i];
            if (component)
            {
                const Vector<AttributeInfo>* attrptr = component->GetAttributes();
                if (attrptr)
                {
                    const Vector<AttributeInfo>& attributes = *attrptr;
                    for (unsigned i=0; i < attributes.Size(); i++)
                    {
                        Variant variant;
                        component->OnGetAttribute(attributes[i], variant);
                        URHO3D_LOGINFOF("   => Attribute : %s value(%s)=%s", attributes[i].name_.CString(), variant.GetTypeName().CString(), variant.ToString().CString());
                    }
                }
                if (component->IsInstanceOf<Drawable2D>())
                {
                    Drawable2D* drawable = (Drawable2D*) component;
                    URHO3D_LOGINFOF("  => Drawable2D BoundingBox World=%s Local=%s", drawable->GetWorldBoundingBox().ToString().CString(), drawable->GetBoundingBox().ToString().CString());
                }
            }
        }
        const Vector<SharedPtr<Node> >& children = node ->GetChildren();
        for (unsigned i=0; i < children.Size(); i++)
            URHO3D_LOGINFOF(" => Child %s(%u)", children[i]->GetName().CString(), children[i]->GetID());
    }
    URHO3D_LOGINFO("GameHelpers() - DumpNode : ---------------");
}

void GameHelpers::DumpVariantMap(const VariantMap& varmap)
{
//    if (!varmap.Size())
//        return;

    URHO3D_LOGINFOF("GameHelpers() - DumpVariantMap : VariantMap Size = %u :", varmap.Size());
    for (VariantMap::ConstIterator it=varmap.Begin(); it!= varmap.End();++it)
    {
        StringHash hash = it->first_;
        const Variant& variant = it->second_;
        URHO3D_LOGINFOF(" hash=%u variant(type=%s;value=%s)",hash.Value(), variant.GetTypeName().CString(), variant.ToString().CString());
    }
}

void GameHelpers::AppendBufferToString(String& string, const char* format, ...)
{
    static char buffer[256];

    va_list args;
    va_start (args, format);
    unsigned length = vsprintf(buffer, format, args);
    va_end (args);

    string.Append(buffer, length);
}

/// DumpData
template< typename T >
void GameHelpers::DumpData(const T* buffer, int markpattern, int numargs,  ...)
{
//    T mk = (T) markpattern;

    const unsigned char* data = (const unsigned char*) buffer;
    unsigned sizeT = sizeof(T);

    if (numargs<2) return ;

    va_list args;
    va_start(args, numargs);

    unsigned width = va_arg(args, unsigned);
    unsigned height = va_arg(args, unsigned);
    unsigned Size = width * height;

//    URHO3D_LOGINFOF("DumpData : ");

	String str;
	str.Reserve(Size);

    // Pattern Mode : Get args (for MapColliderGenerator Marking)
	if (markpattern != -1)
    {
        unsigned neighborPoint = 0;
        unsigned contourStartPoint = 0;
        unsigned backtrackStartPoint = 0;
        unsigned boundaryPoint = 0;
        unsigned backtrackPoint = 0;
        if (numargs > 2)
        {
            str += "\n";
            unsigned p = va_arg(args, unsigned);
            char c = 'A' + (p - markpattern);
            str.AppendWithFormat("- BlockID = %c(c=%u) :\n", c, c);
        }
        if (numargs > 3)
        {
            neighborPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(neighborPoint % width)-1, (int)(neighborPoint / width)-1);
            str.AppendWithFormat("currentPoint(*) at %s[%u] data= %u\n", coord.ToString().CString(), neighborPoint, data[neighborPoint]);
        }
        if (numargs > 4)
        {
            contourStartPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(contourStartPoint % width)-1, (int)(contourStartPoint / width)-1);
            str.AppendWithFormat("startPoint(!) at %s\n", coord.ToString().CString());
        }
        if (numargs > 5)
        {
            backtrackStartPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(backtrackStartPoint % width)-1, (int)(backtrackStartPoint / width)-1);
            str.AppendWithFormat("startBackTrackPoint(#) at %s\n", coord.ToString().CString());
        }
        if (numargs > 6)
        {
            boundaryPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(boundaryPoint % width)-1, (int)(boundaryPoint / width)-1);
            str.AppendWithFormat("boundaryPoint(+) at %s[%u]\n", coord.ToString().CString(), boundaryPoint);
        }
        if (numargs > 7)
        {
            backtrackPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(backtrackPoint % width)-1, (int)(backtrackPoint / width)-1);
            str.AppendWithFormat("boundaryBackTrackPoint(&) to %s[%u]\n",coord.ToString().CString(), backtrackPoint);
        }
    }

    va_end(args);

    str += "\n";

    // Pattern Mode
    if (markpattern != -1)
	{
	    for (unsigned i = 0; i < Size; i++)
        {
            if (i > 0 && i % width == 0) str += "\n";

            char currChar = (char)data[i*sizeT];

            if (currChar == 0)
                currChar = '.';
            else if (currChar == 11) // NoDecal
                currChar = ';';
            else if (currChar == 12) // NoRender
                currChar = ':';
            else if (currChar - markpattern < 0)
                currChar = '\?';
            else
                currChar = 'A' + (currChar - markpattern);

            // Write escape sequence too
            if (currChar == 0x22)
                str += '\'';
            else if (currChar == 0x27)
                str += '\"';
            else if (currChar == 0x5C)
                str += '\\';
            else
                str += currChar;

            str += " ";
        }
	}
	else
    // No Pattern - Show Number
    {
	    for (unsigned i = 0; i < Size; i++)
        {
            if (i > 0 && i % width == 0) str += "\n";

            if (data[i*sizeT] == 0)
                str += ".. ";
            else
                AppendBufferToString(str, "%02d ", data[i*sizeT]);
        }
    }

    URHO3D_LOGINFOF("%s", str.CString());
}


template void GameHelpers::DumpData(const unsigned* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(const int* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(const unsigned char* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(const Match* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(const GridTile* data, int markpattern, int n,  ...);

void GameHelpers::DrawDebugRect(const Rect& rect, DebugRenderer* debugRenderer, bool depthTest, const Color& color)
{
    debugRenderer->AddLine(Vector3(rect.min_.x_, rect.min_.y_, 0.f), Vector3(rect.min_.x_, rect.max_.y_, 0.f), color, depthTest);
    debugRenderer->AddLine(Vector3(rect.min_.x_, rect.max_.y_, 0.f), Vector3(rect.max_.x_, rect.max_.y_, 0.f), color, depthTest);
    debugRenderer->AddLine(Vector3(rect.max_.x_, rect.max_.y_, 0.f), Vector3(rect.max_.x_, rect.min_.y_, 0.f), color, depthTest);
    debugRenderer->AddLine(Vector3(rect.max_.x_, rect.min_.y_, 0.f), Vector3(rect.min_.x_, rect.min_.y_, 0.f), color, depthTest);
}
