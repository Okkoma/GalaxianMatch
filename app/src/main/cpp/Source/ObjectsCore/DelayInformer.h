#pragma once

#include "Pool.h"

class DelayInformer : public Object
{
    URHO3D_OBJECT(DelayInformer, Object);

public :
    static void Reset(int size=0);
    static DelayInformer* Get();
    static DelayInformer* Get(Object* object, float delay, StringHash eventType);
    static DelayInformer* Get(Object* object, float delay, StringHash eventType, const VariantMap& eventData);

    DelayInformer();
    DelayInformer(Context* context);
    DelayInformer(const DelayInformer& timer);
    ~DelayInformer();

    DelayInformer* Set(Object* object, float delay, StringHash eventType);
    DelayInformer* Set(Object* object, float delay, StringHash eventType, const VariantMap& eventData);

    bool Expired() const;
    void Free();

private :
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    WeakPtr<Object> object_;
    float timer_;
	float expirationTime_;
	StringHash eventType_;
	VariantMap eventData_;

	static Pool<DelayInformer> pool_;
};


