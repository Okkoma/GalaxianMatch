//
// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2025 the U3D project.
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

#include "../Resource/Localization.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/JSONFile.h"
#include "../Resource/ResourceEvents.h"
#include "../IO/Log.h"

#include "../DebugNew.h"

namespace Urho3D
{

Localization::Localization(Context* context) :
    Object(context),
    languageIndex_(0U)
{
}

Localization::~Localization() = default;

int Localization::GetLanguageIndex(const String& language) const
{
    if (language.Empty())
    {
        URHO3D_LOGWARNING("Localization::GetLanguageIndex(language): language name is empty");
        return -1;
    }
    if (languages_.Empty())
    {
        URHO3D_LOGWARNING("Localization::GetLanguageIndex(language): no loaded languages");
        return -1;
    }
    const auto it = languages_.Find(language);
    return it != languages_.End() ? it-languages_.Begin() : -1;
}

const String& Localization::GetLanguage() const
{
    if (languageIndex_ >= languages_.Size())
    {
        URHO3D_LOGWARNING("Localization::GetLanguage(): no current language");
        return String::EMPTY;
    }
    return languages_[languageIndex_];
}

const String& Localization::GetLanguage(int index) const
{
    if (index >= (int)languages_.Size())
    {
        URHO3D_LOGWARNING("Localization::GetLanguage(index): index out of range");
        return String::EMPTY;
    }
    return languages_[index];
}

const String& Localization::Get(const String& id) const
{
    if (id.Empty())
        return String::EMPTY;
    if (languageIndex_ >= languages_.Size())
    {
        URHO3D_LOGWARNING("Localization::Get(id): no current language");
        return id;
    }

    const auto it = strings_.Find(StringHash(languages_[languageIndex_]));
    if (it != strings_.End())
    {
        const auto& translations = it->second_;
        const auto jt = translations.Find(StringHash(id));
        if (jt != translations.End())
            return jt->second_;
    }

    URHO3D_LOGWARNING("Localization::Get(\"" + id + "\") not found translation, language=\"" + GetLanguage() + "\"");
    return id;
}

void Localization::SetLanguage(int index)
{
    if (index >= languages_.Size())
    {
        URHO3D_LOGWARNING("Localization::SetLanguage(index): index out of range");
        return;
    }
    if (index != languageIndex_)
    {
        languageIndex_ = index;
        VariantMap& eventData = GetEventDataMap();
        SendEvent(E_CHANGELANGUAGE, eventData);
    }
}

void Localization::SetLanguage(const String& language)
{
    const auto it = languages_.Find(language);
    if (it == languages_.End())
    {
        URHO3D_LOGWARNING("Localization::SetLanguage(language): language not found");
        return;
    }
    unsigned index = it-languages_.Begin();
    if (index != languageIndex_)
    {
        languageIndex_ = index;
        VariantMap& eventData = GetEventDataMap();
        SendEvent(E_CHANGELANGUAGE, eventData);
    }
}


void Localization::Reset()
{
    languages_.Clear();
    languageIndex_ = 0U;
    strings_.Clear();
}


void Localization::LoadJSONFile(const String& name, const String& language)
{
    JSONFile* jsonFile = GetSubsystem<ResourceCache>()->GetResource<JSONFile>(name);
    if (jsonFile)
    {
        if (language.Empty())
            LoadMultipleLanguageJSON(jsonFile->GetRoot());
        else
            LoadSingleLanguageJSON(jsonFile->GetRoot(), language);
    }
}

void Localization::LoadMultipleLanguageJSON(const JSONValue& source)
{
    for (JSONObject::ConstIterator i = source.Begin(); i != source.End(); ++i)
    {
        const String& id = i->first_;
        if (id.Empty())
        {
            URHO3D_LOGWARNING("Localization::LoadMultipleLanguageJSON(source): string ID is empty");
            continue;
        }
        const JSONValue& value = i->second_;
        if (value.IsObject())
        {
            for (JSONObject::ConstIterator j = value.Begin(); j != value.End(); ++j)
            {
                const String& lang = j->first_;
                if (lang.Empty())
                {
                    URHO3D_LOGWARNING("Localization::LoadMultipleLanguageJSON(source): language name is empty, string ID=\"" + id + "\"");
                    continue;
                }
                const String& string = j->second_.GetString();
                if (string.Empty())
                {
                    URHO3D_LOGWARNING("Localization::LoadMultipleLanguageJSON(source): translation is empty, string ID=\""
                                       + id + "\", language=\"" + lang + "\"");
                    continue;
                }
                String& translation = strings_[StringHash(lang)][StringHash(id)];
                if (translation != String::EMPTY)
                {
                    URHO3D_LOGWARNING("Localization::LoadMultipleLanguageJSON(source): override translation, string ID=\""
                                      + id + "\", language=\"" + lang + "\"");
                }
                translation = string;

                if (!languages_.Contains(lang))
                    languages_.Push(lang);
            }
        }
        else
            URHO3D_LOGWARNING("Localization::LoadMultipleLanguageJSON(source): failed to load values, string ID=\"" + id + "\"");
    }
}

void Localization::LoadSingleLanguageJSON(const JSONValue& source, const String& language)
{
    if (!source.Size())
        return;

    bool updated = false;
    StringMap& translations = strings_[StringHash(language)];
    for (JSONObject::ConstIterator i = source.Begin(); i != source.End(); ++i)
    {
        const String& id = i->first_;
        if (id.Empty())
        {
            URHO3D_LOGWARNING("Localization::LoadSingleLanguageJSON(source, language): string ID is empty");
            continue;
        }
        const JSONValue& value = i->second_;
        if (value.IsString())
        {
            if (value.GetString().Empty())
            {
                URHO3D_LOGWARNING("Localization::LoadSingleLanguageJSON(source, language): translation is empty, string ID=\""
                                   + id + "\", language=\"" + language + "\"");
                continue;
            }

            String& translation = translations[StringHash(id)];
            if (translation != String::EMPTY)
            {
                URHO3D_LOGWARNING("Localization::LoadSingleLanguageJSON(source, language): override translation, string ID=\"" 
                                    + id + "\", language=\"" + language + "\"");
            }
            translation = value.GetString();
            updated = true;
        }
        else
            URHO3D_LOGWARNING("Localization::LoadSingleLanguageJSON(source, language): failed to load value, string ID=\""
                               + id + "\", language=\"" + language + "\"");
    }

    if (updated && !languages_.Contains(language))
        languages_.Push(language);
}

}
