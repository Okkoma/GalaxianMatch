#pragma once

#include <Urho3D/UI/Text.h>

#include "Drawable2D.h"

namespace Urho3D
{
    class Text;
}

using namespace Urho3D;

/// 2D text component.
class Text2D : public Drawable2D
{
    URHO3D_OBJECT(Text2D, Drawable2D);

public:
    /// Construct.
    Text2D(Context* context);
    /// Destruct.
    ~Text2D();
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately.
    void ApplyAttributes() override;
    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Set font by looking from resource cache by name and font size. Return true if successful.
    bool SetFont(const String& fontName, int size = DEFAULT_FONT_SIZE);
    /// Set font and font size. Return true if successful.
    bool SetFont(Font* font, int size = DEFAULT_FONT_SIZE);
    /// Set font size only while retaining the existing font. Return true if successful.
    bool SetFontSize(int size);
    void SetFontSizeAttr(int size);
    /// Set material.
    void SetMaterial(Material* material);
    /// Set text. Text is assumed to be either ASCII or UTF8-encoded.
    void SetText(const String& text);
    /// Set horizontal and vertical alignment.
    void SetAlignment(HorizontalAlignment hAlign, VerticalAlignment vAlign);
    /// Set horizontal alignment.
    void SetHorizontalAlignment(HorizontalAlignment align);
    /// Set vertical alignment.
    void SetVerticalAlignment(VerticalAlignment align);
    /// Set row alignment.
    void SetTextAlignment(HorizontalAlignment align);
    /// Set row spacing, 1.0 for original font spacing.
    void SetRowSpacing(float spacing);
    /// Set wordwrap. In wordwrap mode the text element will respect its current width. Otherwise it resizes itself freely.
    void SetWordwrap(bool enable);
    /// The text will be automatically translated. The text value used as string identifier.
    void SetAutoLocalizable(bool enable);
    /// Set text effect.
    void SetTextEffect(TextEffect textEffect);
    /// Set shadow offset.
    void SetEffectShadowOffset(const IntVector2& offset);
    /// Set stroke thickness.
    void SetEffectStrokeThickness(int thickness);
    /// Set stroke rounding. Corners of the font will be rounded off in the stroke so the stroke won't have corners.
    void SetEffectRoundStroke(bool roundStroke);
    /// Set effect color.
    void SetEffectColor(const Color& effectColor);
    /// Set effect Z bias.
    void SetEffectDepthBias(float bias);
    /// Set text width. Only has effect in word wrap mode.
    void SetWidth(int width);
    /// Set color on all corners.
    void SetColor(const Color& color);
    /// Set color on one corner.
    void SetColor(Corner corner, const Color& color);
    /// Set opacity.
    void SetOpacity(float opacity);

    /// Return font.
    Font* GetFont() const;
    /// Return font size.
    int GetFontSize() const;
    /// Return material.
    Material* GetMaterial() const;
    /// Return text.
    const String& GetText() const;
    /// Return row alignment.
    HorizontalAlignment GetTextAlignment() const;
    /// Return horizontal alignment.
    HorizontalAlignment GetHorizontalAlignment() const;
    /// Return vertical alignment.
    VerticalAlignment GetVerticalAlignment() const;
    /// Return row spacing.
    float GetRowSpacing() const;
    /// Return wordwrap mode.
    bool GetWordwrap() const;
    /// Return auto localizable mode.
    bool GetAutoLocalizable() const { return text_.GetAutoLocalizable(); }
    /// Return text effect.
    TextEffect GetTextEffect() const;
    /// Return effect shadow offset.
    const IntVector2& GetEffectShadowOffset() const;
    /// Return effect stroke thickness.
    int GetEffectStrokeThickness() const;
    /// Return effect round stroke.
    bool GetEffectRoundStroke() const;
    /// Return effect color.
    const Color& GetEffectColor() const;
    /// Return effect depth bias.
    float GetEffectDepthBias() const;
    /// Return text width.
    int GetWidth() const;
    /// Return text height.
    int GetHeight() const;
    /// Return row height.
    int GetRowHeight() const;
    /// Return number of rows.
    unsigned GetNumRows() const;
    /// Return number of characters.
    unsigned GetNumChars() const;
    /// Return width of row by index.
    int GetRowWidth(unsigned index) const;
/*
    /// Return position of character by index relative to the text element origin.
    IntVector2 GetCharPosition(unsigned index);
    /// Return size of character by index.
    IntVector2 GetCharSize(unsigned index);
*/
    /// Return corner color.
    const Color& GetColor(Corner corner) const;
    /// Return opacity.
    float GetOpacity() const;

    /// Set font attribute.
    void SetFontAttr(const ResourceRef& value);
    /// Return font attribute.
    ResourceRef GetFontAttr() const;
    /// Set material attribute.
    void SetMaterialAttr(const ResourceRef& value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;
    /// Set text attribute.
    void SetTextAttr(const String& value);
    /// Return text attribute.
    String GetTextAttr() const;

    /// Get color attribute. Uses just the top-left color.
    const Color& GetColorAttr() const { return text_.GetColorAttr(); }

    /// Only Used by Renderer2D
    const BoundingBox& GetWorldBoundingBox2D() override;

protected:

    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;
    /// Handle draw order changed.
    void OnDrawOrderChanged() override;
    /// Update source batches.
    void UpdateSourceBatches() override;

    void HandleChangeLanguage(StringHash eventType, VariantMap& eventData);

    /// Mark text dirty.
    void MarkTextDirty();
    /// Update text %UI batches.
    void UpdateTextBatches();
    /// Create materials for text rendering. May only be called from the main thread. Text %UI batches must be up-to-date.
    void UpdateTextMaterials(bool forceUpdate = false);

    /// Internally used text element.
    Text text_;
    /// Text UI batches.
    PODVector<UIBatch> uiBatches_;
    /// Text vertex data.
    PODVector<float> uiVertexData_;
    /// Text needs update flag.
    bool textDirty_;
    /// Flag for whether currently using SDF shader defines in the generated material.
    bool usingSDFShader_;
    /// Font texture data lost flag.
    bool fontDataLost_;

    /// Material to use as a base for the text material(s).
    SharedPtr<Material> material_;
};


