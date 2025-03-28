#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D//Scene/Node.h>
#include <Urho3D/UI/Font.h>

#include "GameHelpers.h"

#include "Text2D.h"

namespace Urho3D
{
    extern const char* horizontalAlignments[];
    extern const char* verticalAlignments[];
    extern const char* textEffects[];
}

static const float TEXT_SCALING = 1.0f / 128.0f;
static const float DEFAULT_EFFECT_DEPTH_BIAS = 0.1f;

Text2D::Text2D(Context* context) :
    Drawable2D(context),
    text_(context),
    textDirty_(true),
    usingSDFShader_(false),
    fontDataLost_(false)
{
    text_.SetEffectDepthBias(DEFAULT_EFFECT_DEPTH_BIAS);
}

Text2D::~Text2D()
{
}

void Text2D::RegisterObject(Context* context)
{
    context->RegisterFactory<Text2D>();
    URHO3D_COPY_BASE_ATTRIBUTES(Drawable2D);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Font", GetFontAttr, SetFontAttr, ResourceRef, ResourceRef(Font::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()),
        AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Font Size", GetFontSize, SetFontSizeAttr, int, DEFAULT_FONT_SIZE, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Text", GetTextAttr, SetTextAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Text Alignment", GetTextAlignment, SetTextAlignment, HorizontalAlignment, Urho3D::horizontalAlignments, HA_LEFT, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Localizable", GetAutoLocalizable, SetAutoLocalizable, bool, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Word Wrap", GetWordwrap, SetWordwrap, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Can Be Occluded", IsOccludee, SetOccludee, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw Distance", GetDrawDistance, SetDrawDistance, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Width", GetWidth, SetWidth, int, 0, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Horiz Alignment", GetHorizontalAlignment, SetHorizontalAlignment, HorizontalAlignment,
        Urho3D::horizontalAlignments, HA_LEFT, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Vert Alignment", GetVerticalAlignment, SetVerticalAlignment, VerticalAlignment, Urho3D::verticalAlignments,
        VA_TOP, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Opacity", GetOpacity, SetOpacity, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColor, Color, Color::WHITE, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Text Effect", GetTextEffect, SetTextEffect, TextEffect, Urho3D::textEffects, TE_NONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Shadow Offset", GetEffectShadowOffset, SetEffectShadowOffset, IntVector2, IntVector2(1, 1), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Stroke Thickness", GetEffectStrokeThickness, SetEffectStrokeThickness, int, 1, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Round Stroke", GetEffectRoundStroke, SetEffectRoundStroke, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Effect Color", GetEffectColor, SetEffectColor, Color, Color::BLACK, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Effect Depth Bias", GetEffectDepthBias, SetEffectDepthBias, float, DEFAULT_EFFECT_DEPTH_BIAS, AM_DEFAULT);
}

void Text2D::ApplyAttributes()
{
    text_.ApplyAttributes();

    MarkTextDirty();

    UpdateTextBatches();
    UpdateTextMaterials();
}

void Text2D::SetMaterial(Material* material)
{
    material_ = material;

    UpdateTextMaterials(true);
}

bool Text2D::SetFont(const String& fontName, int size)
{
    bool success = text_.SetFont(fontName, size);

    // Changing font requires materials to be re-evaluated. Material evaluation can not be done in worker threads,
    // so UI batches must be brought up-to-date immediately
    MarkTextDirty();
    UpdateTextBatches();
    UpdateTextMaterials();

    return success;
}

bool Text2D::SetFont(Font* font, int size)
{
    bool success = text_.SetFont(font, size);

    MarkTextDirty();
    UpdateTextBatches();
    UpdateTextMaterials();

    return success;
}

void Text2D::SetFontSizeAttr(int size)
{
    //text_.SetFontSize2(size);
    text_.SetFontSize(size);
    MarkTextDirty();
    UpdateTextBatches();
    UpdateTextMaterials();
}

bool Text2D::SetFontSize(int size)
{
    //text_.SetFontSize2(size);
    text_.SetFontSize(size);
    MarkTextDirty();
    UpdateTextBatches();
    UpdateTextMaterials();

    return true;
}

void Text2D::SetText(const String& text)
{
    text_.SetText(text);

    // Changing text requires materials to be re-evaluated, in case the font is multi-page
    MarkTextDirty();
    UpdateTextBatches();
    UpdateTextMaterials();
}

void Text2D::SetAlignment(HorizontalAlignment hAlign, VerticalAlignment vAlign)
{
    text_.SetAlignment(hAlign, vAlign);

    MarkTextDirty();
}

void Text2D::SetHorizontalAlignment(HorizontalAlignment align)
{
    text_.SetHorizontalAlignment(align);

    MarkTextDirty();
}

void Text2D::SetVerticalAlignment(VerticalAlignment align)
{
    text_.SetVerticalAlignment(align);

    MarkTextDirty();
}

void Text2D::SetTextAlignment(HorizontalAlignment align)
{
    text_.SetTextAlignment(align);

    MarkTextDirty();
}

void Text2D::SetRowSpacing(float spacing)
{
    text_.SetRowSpacing(spacing);

    MarkTextDirty();
}

void Text2D::SetWordwrap(bool enable)
{
    text_.SetWordwrap(enable);

    MarkTextDirty();
}

void Text2D::SetAutoLocalizable(bool enable)
{
    text_.SetAutoLocalizable(enable);
    text_.UnsubscribeFromEvent(E_CHANGELANGUAGE);
    if (enable)
        SubscribeToEvent(E_CHANGELANGUAGE, URHO3D_HANDLER(Text2D, HandleChangeLanguage));
    else
        UnsubscribeFromEvent(E_CHANGELANGUAGE);

    ApplyAttributes();
}

void Text2D::SetTextEffect(TextEffect textEffect)
{
    text_.SetTextEffect(textEffect);

    MarkTextDirty();
    UpdateTextMaterials(true);
}

void Text2D::SetEffectShadowOffset(const IntVector2& offset)
{
    text_.SetEffectShadowOffset(offset);
}

void Text2D::SetEffectStrokeThickness(int thickness)
{
    text_.SetEffectStrokeThickness(thickness);
}

void Text2D::SetEffectRoundStroke(bool roundStroke)
{
    text_.SetEffectRoundStroke(roundStroke);
}

void Text2D::SetEffectColor(const Color& effectColor)
{
    text_.SetEffectColor(effectColor);

    MarkTextDirty();
    UpdateTextMaterials();
}

void Text2D::SetEffectDepthBias(float bias)
{
    text_.SetEffectDepthBias(bias);

    MarkTextDirty();
}

void Text2D::SetWidth(int width)
{
    // we need to fix width to correctly use word wrapping
    text_.SetFixedWidth(width);

    MarkTextDirty();
}

void Text2D::SetColor(const Color& color)
{
    float oldAlpha = text_.GetColor(C_TOPLEFT).a_;
    text_.SetColor(color);

    MarkTextDirty();

    // If alpha changes from zero to nonzero or vice versa, amount of text batches changes (optimization), so do full update
    if ((oldAlpha == 0.0f && color.a_ != 0.0f) || (oldAlpha != 0.0f && color.a_ == 0.0f))
    {
        UpdateTextBatches();
        UpdateTextMaterials();
    }
}

void Text2D::SetColor(Corner corner, const Color& color)
{
    text_.SetColor(corner, color);

    MarkTextDirty();
}

void Text2D::SetOpacity(float opacity)
{
    float oldOpacity = text_.GetOpacity();
    text_.SetOpacity(opacity);
    float newOpacity = text_.GetOpacity();

    MarkTextDirty();

    // If opacity changes from zero to nonzero or vice versa, amount of text batches changes (optimization), so do full update
    if ((oldOpacity == 0.0f && newOpacity != 0.0f) || (oldOpacity != 0.0f && newOpacity == 0.0f))
    {
        UpdateTextBatches();
        UpdateTextMaterials();
    }
}

Material* Text2D::GetMaterial() const
{
    return material_;
}

Font* Text2D::GetFont() const
{
    return text_.GetFont();
}

int Text2D::GetFontSize() const
{
    return text_.GetFontSize();
}

const String& Text2D::GetText() const
{
    return text_.GetText();
}

HorizontalAlignment Text2D::GetHorizontalAlignment() const
{
    return text_.GetHorizontalAlignment();
}

VerticalAlignment Text2D::GetVerticalAlignment() const
{
    return text_.GetVerticalAlignment();
}

HorizontalAlignment Text2D::GetTextAlignment() const
{
    return text_.GetTextAlignment();
}

float Text2D::GetRowSpacing() const
{
    return text_.GetRowSpacing();
}

bool Text2D::GetWordwrap() const
{
    return text_.GetWordwrap();
}

TextEffect Text2D::GetTextEffect() const
{
    return text_.GetTextEffect();
}

const IntVector2& Text2D::GetEffectShadowOffset() const
{
    return text_.GetEffectShadowOffset();
}

int Text2D::GetEffectStrokeThickness() const
{
    return text_.GetEffectStrokeThickness();
}

bool Text2D::GetEffectRoundStroke() const
{
    return text_.GetEffectRoundStroke();
}

const Color& Text2D::GetEffectColor() const
{
    return text_.GetEffectColor();
}

float Text2D::GetEffectDepthBias() const
{
    return text_.GetEffectDepthBias();
}

int Text2D::GetWidth() const
{
    return text_.GetWidth();
}

int Text2D::GetHeight() const
{
    return text_.GetHeight();
}

int Text2D::GetRowHeight() const
{
    return text_.GetRowHeight();
}

unsigned Text2D::GetNumRows() const
{
    return text_.GetNumRows();
}

unsigned Text2D::GetNumChars() const
{
    return text_.GetNumChars();
}

int Text2D::GetRowWidth(unsigned index) const
{
    return text_.GetRowWidth(index);
}
/*
IntVector2 Text2D::GetCharPosition(unsigned index)
{
    return text_.GetCharPosition(index);
}

IntVector2 Text2D::GetCharSize(unsigned index)
{
    return text_.GetCharSize(index);
}
*/
const Color& Text2D::GetColor(Corner corner) const
{
    return text_.GetColor(corner);
}

float Text2D::GetOpacity() const
{
    return text_.GetOpacity();
}

void Text2D::MarkTextDirty()
{
    textDirty_ = true;

    OnMarkedDirty(node_);
    MarkNetworkUpdate();
}

void Text2D::SetMaterialAttr(const ResourceRef& value)
{
    SetMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>(value.name_));
}

void Text2D::SetFontAttr(const ResourceRef& value)
{
    // text_.font_ = GetSubsystem<ResourceCache>()->GetResource<Font>(value.name_);
    text_.SetFontAttr(value);
}

void Text2D::SetTextAttr(const String& value)
{
    text_.SetTextAttr(value);
}

String Text2D::GetTextAttr() const
{
    return text_.GetTextAttr();
}

ResourceRef Text2D::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

ResourceRef Text2D::GetFontAttr() const
{
    return text_.GetFontAttr();
}

void Text2D::UpdateTextBatches()
{
    if (!enabled_)
        return;

    uiBatches_.Clear();
    uiVertexData_.Clear();

    Vector3 offset(Vector3::ZERO);

    if (text_.GetText().Length())
    {
        text_.GetBatches(uiBatches_, uiVertexData_, IntRect::ZERO);

        switch (text_.GetHorizontalAlignment())
        {
        case HA_LEFT:
            break;

        case HA_CENTER:
            offset.x_ -= (float)text_.GetWidth() * 0.5f;
            break;

        case HA_RIGHT:
            offset.x_ -= (float)text_.GetWidth();
            break;

        case HA_CUSTOM:
            break;
        }

        switch (text_.GetVerticalAlignment())
        {
        case VA_TOP:
            break;

        case VA_CENTER:
            offset.y_ -= (float)text_.GetHeight() * 0.5f;
            break;

        case VA_BOTTOM:
            offset.y_ -= (float)text_.GetHeight();
            break;

        case VA_CUSTOM:
            break;
        }
    }

    if (uiVertexData_.Size())
    {
        boundingBox_.Clear();

        for (unsigned i = 0; i < uiVertexData_.Size(); i += UI_VERTEX_SIZE)
        {
            Vector3& position = *(reinterpret_cast<Vector3*>(&uiVertexData_[i]));
            position += offset;
            position *= TEXT_SCALING;
            position.y_ = -position.y_;
            boundingBox_.Merge(position);
        }
    }
    else
        boundingBox_.Define(Vector3::ZERO, Vector3::ZERO);

    textDirty_ = false;
    sourceBatchesDirty_ = true;
}

void Text2D::UpdateTextMaterials(bool forceUpdate)
{
    if (!uiBatches_.Size())
        return;

    Font* font = GetFont();
    bool isSDFFont = font ? font->IsSDFFont() : false;

    sourceBatches_.Resize(uiBatches_.Size());

    for (unsigned i = 0; i < sourceBatches_.Size(); ++i)
    {
        SourceBatch2D& batch = sourceBatches_[i];

        if (!batch.material_ || forceUpdate || isSDFFont != usingSDFShader_)
        {
            batch.owner_ = this;
            batch.drawOrder_ = GetDrawOrder();

            // If material not defined, create a reasonable default from scratch
            if (!material_)
            {
                Material* material = new Material(context_);
                Technique* tech = new Technique(context_);
                Pass* pass = tech->CreatePass("alpha");
                pass->SetVertexShader("Text");
                pass->SetPixelShader("Text");
                pass->SetBlendMode(BLEND_ALPHA);
                pass->SetDepthWrite(false);
                material->SetTechnique(0, tech);
                material->SetCullMode(CULL_NONE);
                batch.material_ = material;
            }
            else
                batch.material_ = material_->Clone();

            usingSDFShader_ = isSDFFont;
        }

        Material* material = batch.material_;
        Texture* texture = uiBatches_[i].texture_;
        material->SetTexture(TU_DIFFUSE, texture);

        if (isSDFFont)
        {
            // Note: custom defined material is assumed to have right shader defines; they aren't modified here
            if (!material_)
            {
                Technique* tech = material->GetTechnique(0);
                Pass* pass = tech ? tech->GetPass("alpha") : nullptr;
                if (pass)
                {
                    switch (GetTextEffect())
                    {
                    case TE_NONE:
                        pass->SetPixelShaderDefines("SIGNED_DISTANCE_FIELD");
                        break;

                    case TE_SHADOW:
                        pass->SetPixelShaderDefines("SIGNED_DISTANCE_FIELD TEXT_EFFECT_SHADOW");
                        break;

                    case TE_STROKE:
                        pass->SetPixelShaderDefines("SIGNED_DISTANCE_FIELD TEXT_EFFECT_STROKE");
                        break;
                    }
                }
            }

            switch (GetTextEffect())
            {
            case TE_SHADOW:
                if (texture)
                {
                    Vector2 shadowOffset(0.5f / texture->GetWidth(), 0.5f / texture->GetHeight());
                    material->SetShaderParameter("ShadowOffset", shadowOffset);
                }
                material->SetShaderParameter("ShadowColor", GetEffectColor());
                break;

            case TE_STROKE:
                material->SetShaderParameter("StrokeColor", GetEffectColor());
                break;

            default:
                break;
            }
        }
        else
        {
            // If not SDF, set shader defines based on whether font texture is full RGB or just alpha
            if (!material_)
            {
                Technique* tech = material->GetTechnique(0);
                Pass* pass = tech ? tech->GetPass("alpha") : nullptr;
                if (pass)
                {
                    if (texture && texture->GetFormat() == Graphics::GetAlphaFormat())
                        pass->SetPixelShaderDefines("ALPHAMAP");
                    else
                        pass->SetPixelShaderDefines("");
                }
            }
        }
    }
}

void CopyVerticesUIBatchToSourceBatch2D(const UIBatch& uibatch, SourceBatch2D& batch2d)
{
    // uiquad (=2 uitriangles =6 uivertices)
    // uivertex (=6 float : position(3float) + color(1float) + uv(2float)
    const unsigned VERTEXUI_SIZE = UI_VERTEX_SIZE;
    const unsigned UI_QUAD_STRIDE = 2 * 3 * VERTEXUI_SIZE;
    const unsigned numquads = (uibatch.vertexEnd_ - uibatch.vertexStart_) / UI_QUAD_STRIDE;
    // 2dquad (=4 2dvertex_gl)
    // 2dvertex_gl (=10 float : position(3float) + color(1float) + uv(2float) + texmode(4float))
    const unsigned VERTEX2D_SIZE = 10;
    batch2d.vertices_.Resize(numquads * 4);

    unsigned ivertices = 0U;
    for (unsigned q = 0; q < numquads; q++)
    {
        /*
        V1---------V2
        |         / |
        |       /   |
        |     /     |
        |   /       |
        | /         |
        V0---------V3
        */

        float* datas = &batch2d.vertices_[q*4].position_.x_;

        memcpy(datas, &uibatch.vertexData_->At(ivertices+2*VERTEXUI_SIZE), VERTEXUI_SIZE * sizeof(float));      // V0
        memcpy(&datas[VERTEX2D_SIZE], &uibatch.vertexData_->At(ivertices), VERTEXUI_SIZE * sizeof(float));   // V1
        memcpy(&datas[2*VERTEX2D_SIZE], &uibatch.vertexData_->At(ivertices+VERTEXUI_SIZE), VERTEXUI_SIZE * sizeof(float));   // V2
        memcpy(&datas[3*VERTEX2D_SIZE], &uibatch.vertexData_->At(ivertices+4*VERTEXUI_SIZE), VERTEXUI_SIZE * sizeof(float)); // V3
        ivertices += UI_QUAD_STRIDE;
    }
}

void Text2D::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    for (unsigned i = 0; i < uiBatches_.Size(); ++i)
    {
        if (uiBatches_[i].texture_ && uiBatches_[i].texture_->IsDataLost())
        {
            fontDataLost_ = true;
            break;
        }
    }

    if (fontDataLost_ || textDirty_)
    {
        // Re-evaluation of the text triggers the font face to reload itself
        UpdateTextBatches();
        UpdateTextMaterials();
        fontDataLost_ = false;
    }

    // Copy Text vertices to SourceBatch vertices
    for (unsigned i = 0; i < uiBatches_.Size(); ++i)
        CopyVerticesUIBatchToSourceBatch2D(uiBatches_[i], sourceBatches_[i]);

    Matrix2x3 worldTransform(node_->GetWorldPosition2D(), node_->GetWorldRotation2D(), node_->GetWorldScale2D());

    // Apply world transform
    int draworder = GetDrawOrder();

    for (unsigned i = 0; i < sourceBatches_.Size(); ++i)
    {
        sourceBatches_[i].drawOrder_ = draworder;
        Vector<Vertex2D>& vertices = sourceBatches_[i].vertices_;
        for (unsigned j = 0; j < vertices.Size(); ++j)
            vertices[j].position_ = worldTransform * vertices[j].position_;
    }

    sourceBatchesDirty_ = false;
}

const BoundingBox& Text2D::GetWorldBoundingBox2D()
{
    if (textDirty_)
    {
        UpdateTextBatches();
        UpdateTextMaterials();
    }

    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
    worldBoundingBoxDirty_ = false;

    return worldBoundingBox_;
}

void Text2D::OnWorldBoundingBoxUpdate()
{
    if (worldBoundingBoxDirty_ || textDirty_)
    {
        GetWorldBoundingBox2D();
        sourceBatchesDirty_ = true;
    }
}

void Text2D::OnSetEnabled()
{
    Drawable2D::OnSetEnabled();

    bool enabled = IsEnabledEffective();

    if (GetScene() && enabled && renderer_)
        OnWorldBoundingBoxUpdate();
}

void Text2D::OnDrawOrderChanged()
{
    int draworder = GetDrawOrder();
    for (unsigned i = 0; i < sourceBatches_.Size(); ++i)
        sourceBatches_[i].drawOrder_ = draworder;
}

void Text2D::HandleChangeLanguage(StringHash eventType, VariantMap& eventData)
{
    ApplyAttributes();
}

