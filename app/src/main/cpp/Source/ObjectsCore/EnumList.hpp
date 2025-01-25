#pragma once


#include <cstring>
#include <Urho3D/Container/Str.h>

using namespace Urho3D;


#define ENUMSTRING( x ) #x

#define LIST(listName, listEnum, listString) \
struct listName \
{ \
private: \
    static const char* names_[]; \
    static const unsigned size_; \
public: \
    enum type { listEnum }; \
    static const char* GetListName() { static const char* listNameStatic(#listName); return listNameStatic; } \
    static const unsigned GetListSize() { return size_; } \
    static const char* GetName(unsigned i) { return names_[i]; } \
    static unsigned GetIndex(const char* name) { \
        for (unsigned i=0; i < size_; i++) if (strcmp(GetName(i), name) == 0) return i; \
        return -1; } \
};

#define LISTCONST(listName, listString, size) \
const char* listName::names_[] = { listString }; \
const unsigned listName::size_ = size;


#define LISTVALUE(listValueName, listEnum, listString, listValues) \
struct listValueName \
{ \
private: \
    static const char* names_[]; \
    static int values_[]; \
    static const unsigned size_; \
public: \
    enum type { listEnum }; \
    static const char* GetListName() { static const char* listNameStatic(#listValueName); return listNameStatic; } \
    static const unsigned GetListSize() { return size_; } \
    static const char* GetName(unsigned i) { return names_[i]; } \
    static int GetValue(unsigned i) { return values_[i]; } \
    static unsigned char GetIndex(const char* name) { \
        for (unsigned char i=0; i < size_; i++) if (strcmp(GetName(i), name) == 0) return i; \
        return 0; } \
    static unsigned char GetIndex(int value) { \
        for (unsigned char i=0; i < size_; i++) if (value == values_[i]) return i; \
        return 0; } \
    static int GetValue(const char* name) { \
        for (unsigned i=0; i < size_; i++) if (strcmp(GetName(i), name) == 0) return values_[i]; \
        return -1; } \
    static unsigned GetValue(const String& dataStr) { \
        int value = 0; int propvalue; \
        Vector<String> properties = dataStr.Split('|'); \
        for (unsigned i=0; i<properties.Size(); i++) { \
            propvalue = GetValue(properties[i].CString()); \
            if (propvalue != -1) value = value | propvalue; } \
        return (unsigned)value; } \
};

#define LISTVALUECONST(listValueName, listString, listValue, size) \
const char* listValueName::names_[] = { listString }; \
int listValueName::values_[] = { listValue }; \
const unsigned listValueName::size_ = size;
