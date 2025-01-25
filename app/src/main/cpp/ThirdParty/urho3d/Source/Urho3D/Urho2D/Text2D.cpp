//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Material.h"
#include "../Graphics/Technique.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"
#include "../UI/Font.h"
#include "../UI/Text.h"

#include "../Urho2D/Text2D.h"

namespace Urho3D
{

extern const char* horizontalAlignments[];
extern const char* verticalAlignments[];
extern const char* textEffects[];
extern const char* URHO2D_CATEGORY;

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
    context->RegisterFactory<Text2D>(URHO2D_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Font", GetFontAttr, SetFontAttr, ResourceRef, ResourceRef(Font::GetTypeStatic()), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()),
        AM_DEFAULT);
    URHO3D_ATTRIBUTE("Font Size", int, text_.fontSize_, DEFAULT_FONT_SIZE, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Text", GetTextAttr, SetTextAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Text Alignment", text_.textAlignment_, horizontalAlignments, HA_LEFT, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Row Spacing", float, text_.rowSpacing_, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Localizable", GetAutoLocalizable, SetAutoLocalizable, bool, false, AM_FILE);
    URHO3D_ATTRIBUTE("Word Wrap", bool, text_.wordWrap_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Can Be Occluded", IsOccludee, SetOccludee, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw Distance", GetDrawDistance, SetDrawDistance, float, 0.0f, AM_DEFAULT);

    URHO3D_ACCESSOR_ATTRIBUTE("Width", GetWidth, SetWidth, int, 0, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Horiz Alignment", GetHorizontalAlignment, SetHorizontalAlignment, HorizontalAlignment,
        horizontalAlignments, HA_LEFT, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Vert Alignment", GetVerticalAlignment, SetVerticalAlignment, VerticalAlignment, verticalAlignments,
        VA_TOP, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Opacity", GetOpacity, SetOpacity, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColor, Color, Color::WHITE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Top Left Color", Color, text_.color_[0], Color::WHITE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Top Right Color", Color, text_.color_[1], Color::WHITE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bottom Left Color", Color, text_.color_[2], Color::WHITE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bottom Right Color", Color, text_.color_[3], Color::WHITE, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Text Effect", text_.textEffect_, textEffects, TE_NONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Shadow Offset", IntVector2, text_.shadowOffset_, IntVector2(1, 1), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Stroke Thickness", int, text_.strokeThickness_, 1, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Round Stroke", bool, text_.roundStroke_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Effect Color", GetEffectColor, SetEffectColor, Color, Color::BLACK, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Effect Depth Bias", float, text_.effectDepthBias_, DEFAULT_EFFECT_DEPTH_BIAS, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Drawable2D);
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

bool Text2D::SetFontSize(int size)
{
    text_.SetFontSize2(size);

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
    MarkTextDirty();
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
    // C.VILLE : we need to fix width to correctly use word wrapping
    text_.SetFixedWidth(width);
//    text_.SetMinWidth(width);
//    text_.SetWidth(width);

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

IntVector2 Text2D::GetCharPosition(unsigned index)
{
    return text_.GetCharPosition(index);
}

IntVector2 Text2D::GetCharSize(unsigned index)
{
    return text_.GetCharSize(index);
}

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
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(value.name_));
}

void Text2D::SetFontAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    text_.font_ = cache->GetResource<Font>(value.name_);
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
    return GetResourceRef(text_.font_, Font::GetTypeStatic());
}

void Text2D::UpdateTextBatches()
{
    if (!enabled_)
        return;

    uiBatches_.Clear();
    uiVertexData_.Clear();

    Vector3 offset(Vector3::ZERO);

    if (text_.text_.Length())
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
                pass->SetDepthTestMode(CMP_ALWAYS);
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
                Pass* pass = tech ? tech->GetPass("alpha") : (Pass*)0;
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
    }
}

void CopyVerticesUIBatchToSourceBatch2D(const UIBatch& uibatch, SourceBatch2D& batch2d)
{
    const unsigned QUAD_STRIDE = 6 * UI_VERTEX_SIZE;
    const unsigned numquads = (uibatch.vertexEnd_ - uibatch.vertexStart_) / QUAD_STRIDE;
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

        memcpy(datas, &uibatch.vertexData_->At(ivertices+12), UI_VERTEX_SIZE * sizeof(float));      // V0
        memcpy(&datas[6], &uibatch.vertexData_->At(ivertices), 2*UI_VERTEX_SIZE * sizeof(float));   // V1 & V2
        memcpy(&datas[18], &uibatch.vertexData_->At(ivertices+24), UI_VERTEX_SIZE * sizeof(float)); // V3

//        for (unsigned v = 0; v < UI_VERTEX_SIZE; v++)
//        {
//            // V0 Bottom Left
//            datas[v] = uibatch.vertexData_->At(ivertices+v+2*UI_VERTEX_SIZE);
//            // V1 Top Left
//            datas[v+1*UI_VERTEX_SIZE] = uibatch.vertexData_->At(ivertices+v);
//            // V2 Top Right
//            datas[v+2*UI_VERTEX_SIZE] = uibatch.vertexData_->At(ivertices+v+1*UI_VERTEX_SIZE);
//            // V3 Bottom Right
//            datas[v+3*UI_VERTEX_SIZE] = uibatch.vertexData_->At(ivertices+v+4*UI_VERTEX_SIZE);
//        }
        ivertices += QUAD_STRIDE;
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

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();

    // Apply world transform
    for (unsigned i = 0; i < sourceBatches_.Size(); ++i)
    {
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

}
