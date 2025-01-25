#pragma once

#include <Urho3D/Core/Timer.h>

using namespace Urho3D;


/// Timer in MSec with expiration
class TimerSimple
{

public:
	TimerSimple(unsigned expirationTime, bool startActive = false);
	~TimerSimple();

	void Reset();

	bool Active();
	bool Expired();

	unsigned GetCurrentTime();

	void SetExpirationTime(unsigned expirationTime);
	unsigned GetExpirationTime() const { return expirationTime_; }

private:
	Timer timer_;
	unsigned expirationTime_;
};

