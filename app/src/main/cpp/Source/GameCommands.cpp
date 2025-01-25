#include <iostream>
#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include "DefsGame.h"

#include "GameRand.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameStateManager.h"

#include "ObjectPool.h"

#include "GameCommands.h"


void GameCommands::Launch(Context* context, const String& input)
{
    String inputLower = input.ToLower().Trimmed();
    if (inputLower.Empty())
        return;

    Vector<String> inputs = inputLower.Split('=');

    if (inputLower == "quit" || inputLower == "exit")
    {
        GameStatics::Exit();
        return;
    }

    else if (inputLower == "break" || inputLower == "menu")
        GameStatics::stateManager_->PopStack();

    else if (inputLower == "statics")
        GameStatics::Dump();

    else if (inputLower == "pool")
    {
        if (ObjectPool::Get())
            ObjectPool::Get()->DumpCategories();
    }
    else if (inputLower == "render")
    {
        GameStatics::rootScene_->GetComponent<Renderer2D>()->Dump();
    }
    else if (inputs[0] == "setlevel")
    {
        int level = GameStatics::SetLevel(inputs.Size() > 1 ? ToInt(inputs[1]) : 1);
        URHO3D_LOGINFOF("SetLevel=%d", level);
    }
    else if (inputs[0] == "scene")
    {
        CreateMode mode = REPLICATED;
        if (inputs.Size() > 1)
            if (inputs[1] == "local")
                mode = LOCAL;

        GameHelpers::DumpNode(GameStatics::rootScene_, true);
    }
    else if (inputs[0] == "node")
    {
        if (inputs.Size() > 1)
            GameHelpers::DumpNode(ToUInt(inputs[1]), false);
    }
    else if (inputs[0] == "activenode")
    {
        if (inputs.Size() > 1)
        {
            Node* node = GameStatics::rootScene_->GetNode(ToUInt(inputs[1]));
            if (node) node->SetEnabled(true);
        }
    }
    else if (inputs[0] == "activecomponent")
    {
        if (inputs.Size() > 1)
        {
            Component* component = GameStatics::rootScene_->GetComponent(ToUInt(inputs[1]));
            if (component) component->SetEnabled(true);
        }
    }
    else if (inputs[0] == "nodeattr")
    {
        if  (inputs.Size() > 1)
            GameHelpers::DumpNode(ToUInt(inputs[1]), true);
    }
    else if (inputs[0] == "vars")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Node* node = GameStatics::rootScene_->GetNode(id);
            if (node)
            {
                URHO3D_LOGINFOF("Node %s(%u) enabled=%s Parent %s(%u)", node->GetName().CString(), id, node->IsEnabled() ? "true" : "false",
                                node->GetParent() ? node->GetParent()->GetName().CString() : "NULL", node->GetParent() ? node->GetParent()->GetID() : 0);
                const VariantMap& vars = node->GetVars();
                for (VariantMap::ConstIterator it=vars.Begin(); it!=vars.End();++it)
                {
                    const String& attrName = context->GetUserAttributeName(it->first_);
                    URHO3D_LOGINFOF(" => Var %s(%u) = %s(%s)", attrName.CString(), it->first_.Value(), it->second_.ToString().CString(), it->second_.GetTypeName().CString());
                }
            }
            else
                URHO3D_LOGINFOF("No Node id=%u", id);
        }
    }
    else if (inputs[0] == "component")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Component* component = GameStatics::rootScene_->GetComponent(id);
            if (component)
                URHO3D_LOGINFOF("Component %s(%u) in node %s(%u)", component->GetTypeName().CString(), id,
                                component->GetNode()->GetName().CString(), component->GetNode()->GetID());
            else
                URHO3D_LOGINFOF("No Component id=%u", id);
        }
    }
    else if (inputs[0] == "componentattr")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Component* component = GameStatics::rootScene_->GetComponent(id);
            if (component)
            {
                URHO3D_LOGINFOF("Component %s(%u) in node %s(%u) enabled=%s", component->GetTypeName().CString(), id,
                                component->GetNode()->GetName().CString(), component->GetNode()->GetID(), component->IsEnabled() ? "true":"false");
                const Vector<AttributeInfo>* attrptr = component->GetAttributes();
                if (attrptr)
                {
                    const Vector<AttributeInfo>& attributes = *attrptr;
                    for (unsigned i=0; i < attributes.Size(); i++)
                    {
                        Variant variant;
                        component->OnGetAttribute(attributes[i], variant);
                        URHO3D_LOGINFOF(" => Attribute : %s value(%s)=%s", attributes[i].name_.CString(), variant.GetTypeName().CString(), variant.ToString().CString());
                    }
                }
                if (component->GetType() == AnimatedSprite2D::GetTypeStatic())
                {
                    AnimatedSprite2D* animatedSprite = (AnimatedSprite2D*)component;
                    URHO3D_LOGINFOF("WBBox=%s BBox=%s", animatedSprite->GetWorldBoundingBox().ToString().CString(),
                                animatedSprite->GetBoundingBox().ToString().CString());
                }
            }
            else
                URHO3D_LOGINFOF("No Component id=%u", id);
        }
    }
    else if (inputs[0] == "random")
    {
        if (inputs.Size() > 1)
        {
            GameRand::SetSeedRand(ALLRAND, ToUInt(inputs[1]));
            URHO3D_LOGINFOF("random() => seed=%u", ToUInt(inputs[1]));
        }

        GameRand::Dump10Value();
    }
    else if (inputs[0] == "checkstars")
    {
        GameStatics::CheckTimeForEarningStars();
    }
}

