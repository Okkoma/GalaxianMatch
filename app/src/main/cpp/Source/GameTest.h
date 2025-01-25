#pragma once

#include <Urho3D/Core/Timer.h>

#include "DefsJNI.h"

namespace Urho3D
{
    class Context;
    class Scene;
    class AnimatedSprite2D;
}

using namespace Urho3D;

URHO3D_EVENT(TESTPLAY_FINISHED, TestPlay_Finished) { }
URHO3D_EVENT(TESTRECORD_FINISHED, TestRecord_Finished) { }

enum
{
    IPLAYER_RECORD = 0,
    IPLAYER_RANDOM = 1,
    IPLAYER_FILE = 2
};

enum
{
    IRECORD_MOVE = 0,
    IRECORD_BUTTON,
    IRECORD_WHEEL,
    IRECORD_SETCURSOR
};

struct InputRecordEntry
{
    void Set(int type, int value, unsigned time, IntVector2 position);

    void Get(IntVector2& position) const;

    int type_, value_;
    unsigned time_;
    bool onscene_;

    Vector2 position_;
};

typedef PODVector<InputRecordEntry> InputRecordEntries;


class InputRecorder : public Object
{
    URHO3D_OBJECT(InputRecorder, Object);

public:
	InputRecorder(Context* context);
	~InputRecorder();

    void Start();
    void Stop();

    bool SaveRecord();

    bool IsRecording() const { return isRecording_; }
    const InputRecordEntries& GetRecordEntries() const { return recordentries_; }
    unsigned GetRecordIndex() const { return irecord_; }

    static InputRecorder* Get();
    static void Release();

private:
    void HandleRecordTest(StringHash eventType, VariantMap& eventData);

    bool isRecording_;
    unsigned irecord_;
    Timer timer_;
    InputRecordEntries recordentries_;
    unsigned lastmovetime_;

    static InputRecorder* recorder_;
};


class InputPlayer : public Object
{
    URHO3D_OBJECT(InputPlayer, Object);

public:
	InputPlayer(Context* context);
	~InputPlayer();

	void SetMode(int mode);
	void SetRecord(const InputRecordEntries& entries);
    void SetNumRandomEntries(unsigned num);
    void SetSmoothMove(bool enable) { cursorSmoothMove_ = enable; }

    void Start();
    void StartFile(const String& name);
    void Stop();

    bool HasRecordFile(int irecords) const;
    int GetLastRecordFile() const;
    bool LoadRecord(int irecord);
    bool LoadFile(const String& filename);
    void PlayRecord(int irecord=0);

    const IntVector2& GetPosition() const { return testposition_; }
    bool IsPlaying() const;

    static InputPlayer* Get();
    static void Release();

private:
    void HandleStartPlayTest(StringHash eventType, VariantMap& eventData);
    void HandlePlayTest(StringHash eventType, VariantMap& eventData);
    void HandleRandomTest(StringHash eventType, VariantMap& eventData);

    IntVector2 GetScreenPosition(unsigned ientry) const;
    IntVector2 GetScreenPositionForScene(unsigned ientry) const;

    void TestClickDown(const IntVector2& position, int button);
    void TestClickWheel(const IntVector2& position, int wheel);
    void TestMouseMove(const IntVector2& position, const IntVector2& position2=IntVector2::ZERO, unsigned delay=0U);
    void TestClickUp(const IntVector2& position);

    bool cursorSmoothMove_;
    bool clickdone_;
    int mode_, ientry_;
    int recordresync_;
    Timer timer_;
    unsigned delay_, numrandomentries_;
    InputRecordEntries recordentries_;
    float speed_;
    IntVector2 testposition_;
    WeakPtr<BorderImage> testCursor_;
    WeakPtr<AnimatedSprite2D> animatedCursor_;
    int animatedCursorEntity_;

    static InputPlayer* player_;
};

extern bool PlayInputRecordTest(int test);

