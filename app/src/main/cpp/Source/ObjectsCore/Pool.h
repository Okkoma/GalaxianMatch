#pragma once

#include <Urho3D/Container/List.h>

namespace Urho3D
{
    class Context;
    class Object;
}

using namespace Urho3D;


template <class T> class Pool
{
public:
    Pool()  { }
    ~Pool() { Delete(); }

    void Resize(Context* context, unsigned size)
    {
        context_ = context;
        if (size)
            Allocate(size);
        else
            Delete();
    }

    T* Get()
    {
        if (!freeinstances_.Size())
            Allocate(instances_.Size());

        if (freeinstances_.Size())
        {
            T* instance = freeinstances_.Back();
            freeinstances_.Pop();
            return instance;
        }

        return 0;
    }

    void Free(T* instance)
    {
        if (!instances_.Size())
            return;
        freeinstances_.PushFront(instance);
    }

    void FreeAll()
    {
        freeinstances_.Clear();
        for (unsigned i = 0; i < instances_.Size(); i++)
            instances_[i]->Free();
    }

private:
    void Allocate(unsigned size)
    {
        if (!context_)
            return;

        for (unsigned i = 0; i < size; i++)
        {
            instances_.Push(new T(context_));
            freeinstances_.Push(instances_.Back());
        }
    }
    void Delete()
    {
        for (unsigned i=0; i < instances_.Size(); i++)
            delete instances_[i];

        instances_.Clear();
        freeinstances_.Clear();
    }

    Context* context_;
    Vector<T* > instances_;
    List<T* > freeinstances_;
};
