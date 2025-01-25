#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/File.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Camera.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/BorderImage.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include <SDL/SDL.h>

#include "TextMessage.h"
#include "DelayInformer.h"

#include "Game.h"
#include "GameEvents.h"
#include "GameStateManager.h"
#include "GameHelpers.h"
#include "GameStatics.h"

#include "sLevelMap.h"

#include "GameTest.h"

#define TESTRECORD_DELAYMAX 10000
#define TESTRANDOM_DELAYMIN 500
#define TESTRANDOM_DELAYMAX 2000
#define TESTCLICK_DELAYPRESSED 100
#define TESTCLICK_THRESHOLD 50
#define TESTMOVE_DELAYBETWEENENTRIES 100

WeakPtr<TextMessage> testText_;

extern int UISIZE[NUMUIELEMENTSIZE];

const char* GameTestModeNames_[] =
{
    "IPLAYER_RECORD",
    "IPLAYER_RANDOM",
    "IPLAYER_FILE"
};

const String TESTRECORDNAME("test");
const String TESTRECORDDIR("Data/Tests/");


bool PlayInputRecordTest(int test)
{
    InputPlayer* player = InputPlayer::Get();
    if (!player)
        return false;

    if (player->IsPlaying())
        player->Stop();

    player->PlayRecord(test);

    return true;
}

#ifdef __ANDROID__
JNIEXPORT void JNICALL GAME_JAVA_INTERFACE(PlayTest)(JNIEnv* env, jclass jcls, jint test)
{
    URHO3D_LOGINFOF("InputPlayer() - PlayTest : scenario=%d !", test);
    GameStatics::playTest_ = test;
}

void Android_FinishTest()
{
    URHO3D_LOGINFOF("Android_FinishTest ...");

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    jobject mActivityInstance = (jobject)SDL_AndroidGetActivity();
    jclass mActivityClass = env->GetObjectClass(mActivityInstance);
    jmethodID midfinishTest = env->GetMethodID(mActivityClass, "finishTest", "()V");

    URHO3D_LOGINFOF("Android_FinishTest : Launch : jobject=%u jclass=%u jmethod=%u", (void*)mActivityInstance, (void*)mActivityClass, (void*)midfinishTest);

    if (midfinishTest != NULL)
        env->CallNonvirtualVoidMethod(mActivityInstance, mActivityClass, midfinishTest, 0);
    else
        URHO3D_LOGERROR("Android_FinishTest : no finishTest method found !");

    env->DeleteLocalRef(mActivityInstance);
}
#endif

void LogInputRecordEntries(const InputRecordEntries& entries)
{
    URHO3D_LOGINFO("LogInputRecordEntries() - START ...");
    for (unsigned i=0; i < entries.Size(); i++)
        URHO3D_LOGINFOF("-> entries[%u] : type=%d value=%d delay=%u pos=%s", i, entries[i].type_, entries[i].value_, entries[i].time_, entries[i].position_.ToString().CString());
    URHO3D_LOGINFO("LogInputRecordEntries() - ... END !");
}


/// RecordEntry

void InputRecordEntry::Set(int type, int value, unsigned time, IntVector2 position)
{
    type_        = type;
    value_       = value;
    time_        = time;
    onscene_     = GameStatics::screenSceneRect_.IsInside(position);

    if (onscene_)
    {
        position_   = GameHelpers::ScreenToWorld2D(position);
//        URHO3D_LOGINFOF("InputRecordEntry() - Set : scenepos=%s", position_.ToString().CString());
    }
    else
    {
        position_.x_ = (float)position.x_ / GameStatics::graphics_->GetWidth();
        position_.y_ = (float)position.y_ / GameStatics::graphics_->GetHeight();
//        URHO3D_LOGINFOF("InputRecordEntry() - Set : screenpos=%s", position_.ToString().CString());
    }
}

void InputRecordEntry::Get(IntVector2& position) const
{
    if (onscene_)
    {
        GameHelpers::OrthoWorldToScreen(position, Vector3(position_));
//        URHO3D_LOGINFOF("InputRecordEntry() - Get : screenpos=%s from scene", position.ToString().CString());
    }
    else
    {
        position.x_ = (int)floor(position_.x_ * GameStatics::graphics_->GetWidth());
        position.y_ = (int)floor(position_.y_ * GameStatics::graphics_->GetHeight());
//        URHO3D_LOGINFOF("InputRecordEntry() - Get : screenpos=%s", position.ToString().CString());
    }
}


/// Recorder

InputRecorder* InputRecorder::recorder_ = 0;

InputRecorder::InputRecorder(Context* context) :
    Object(context),
    isRecording_(false),
    irecord_(1U),
    lastmovetime_(0)
{
    recordentries_.Reserve(1000);
}

InputRecorder::~InputRecorder() { }

InputRecorder* InputRecorder::Get()
{
    if (!recorder_)
        recorder_ = new InputRecorder(GameStatics::context_);

    return recorder_;
}

void InputRecorder::Release()
{
    if (!recorder_)
    {
        delete recorder_;
        recorder_ = 0;
    }
}

void InputRecorder::Start()
{
    URHO3D_LOGINFOF("InputRecorder() - Start()");

    while (InputPlayer::Get()->HasRecordFile(irecord_))
        irecord_++;

    testText_ = GameHelpers::AddUIMessage(ToString("Record Test %d", irecord_), false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE, IntVector2(0, -1), 1.5f);

    timer_.Reset();
    recordentries_.Clear();
    isRecording_ = true;

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(InputRecorder, HandleRecordTest));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(InputRecorder, HandleRecordTest));
    SubscribeToEvent(E_MOUSEWHEEL, URHO3D_HANDLER(InputRecorder, HandleRecordTest));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(InputRecorder, HandleRecordTest));
}

void InputRecorder::Stop()
{
    URHO3D_LOGINFOF("InputRecorder() - Stop()");

    testText_ = GameHelpers::AddUIMessage(ToString("Stop Rec Test %d", irecord_), false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE, IntVector2(0, -1), 1.5f);

    isRecording_ = false;
    UnsubscribeFromAllEvents();

    if (SaveRecord())
        irecord_++;

    SendEvent(TESTRECORD_FINISHED);
}

bool InputRecorder::SaveRecord()
{
    if (!recordentries_.Size())
        return false;

    URHO3D_LOGINFOF("InputRecorder() - SaveRecord() : irecord=%u", irecord_);
    LogInputRecordEntries(recordentries_);
    return GameHelpers::SaveData(context_, recordentries_.Buffer(), recordentries_.Size()*sizeof(InputRecordEntry),
                                 String(GameStatics::gameConfig_.saveDir_ + GameStatics::saveDir_ + TESTRECORDNAME + String(irecord_) + String(".bin")).CString());
}

void InputRecorder::HandleRecordTest(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_MOUSEMOVE)
    {
        unsigned time = timer_.GetMSec(false);
        if (time - lastmovetime_ < TESTMOVE_DELAYBETWEENENTRIES)
            return;

        recordentries_.Resize(recordentries_.Size()+1);
        InputRecordEntry& entry = recordentries_.Back();
        entry.Set(IRECORD_MOVE, 0, timer_.GetMSec(false), GameStatics::input_->GetMousePosition());
        lastmovetime_ = entry.time_;
        URHO3D_LOGINFOF("InputRecorder() - HandleRecordTest : E_MOUSEMOVE entry[%d] delay=%u mousepos=%s recordedpos=%s scale(%F,%F) uiScale(%F)",
                        recordentries_.Size()-1, entry.time_, GameStatics::input_->GetMousePosition().ToString().CString(), entry.position_.ToString().CString(),
                        GameStatics::gameScale_.x_, GameStatics::gameScale_.y_, GameStatics::ui_->GetScale());
    }
    if (eventType == E_MOUSEBUTTONDOWN)
    {
        recordentries_.Resize(recordentries_.Size()+1);
        InputRecordEntry& entry = recordentries_.Back();
        entry.Set(IRECORD_BUTTON, eventData[MouseButtonDown::P_BUTTON].GetInt(), timer_.GetMSec(true), GameStatics::input_->GetMousePosition());
        URHO3D_LOGINFOF("InputRecorder() - HandleRecordTest : E_MOUSEBUTTONDOWN entry[%d] delay=%u mousepos=%s recordedpos=%s scale(%F,%F) uiScale(%F)",
                        recordentries_.Size()-1, entry.time_, GameStatics::input_->GetMousePosition().ToString().CString(), entry.position_.ToString().CString(),
                        GameStatics::gameScale_.x_, GameStatics::gameScale_.y_, GameStatics::ui_->GetScale());
    }
    else if (eventType == E_MOUSEWHEEL)
    {
        recordentries_.Resize(recordentries_.Size()+1);
        InputRecordEntry& entry = recordentries_.Back();
        entry.Set(IRECORD_WHEEL, eventData[MouseWheel::P_WHEEL].GetInt(), timer_.GetMSec(true), GameStatics::input_->GetMousePosition());
        URHO3D_LOGINFOF("InputRecorder() - HandleRecordTest : E_MOUSEWHEEL entry[%d] delay=%u mousepos=%s recordedpos=%s scale(%F,%F) uiScale(%F)",
                        recordentries_.Size()-1, entry.time_, GameStatics::input_->GetMousePosition().ToString().CString(), entry.position_.ToString().CString(),
                        GameStatics::gameScale_.x_, GameStatics::gameScale_.y_, GameStatics::ui_->GetScale());
    }
    else if (eventType == E_KEYUP)
    {
        int code = eventData[KeyUp::P_SCANCODE].GetInt();
        if (code >= SDL_SCANCODE_KP_1 && code <= SDL_SCANCODE_KP_0)
        {
            code = code == SDL_SCANCODE_KP_0 ? -1 : code - SDL_SCANCODE_KP_1;

            recordentries_.Resize(recordentries_.Size()+1);
            InputRecordEntry& entry = recordentries_.Back();
            entry.Set(IRECORD_SETCURSOR, code, timer_.GetMSec(false), GameStatics::input_->GetMousePosition());
            URHO3D_LOGINFOF("InputRecorder() - HandleRecordTest : E_KEYUP entry[%d] value=%d delay=%u mousepos=%s recordedpos=%s",
                            recordentries_.Size()-1, entry.value_, entry.time_, GameStatics::input_->GetMousePosition().ToString().CString(), entry.position_.ToString().CString());
        }
    }
}


/// Player

InputPlayer* InputPlayer::player_ = 0;

InputPlayer::InputPlayer(Context* context) :
    Object(context),
    cursorSmoothMove_(true),
    mode_(-1),
    speed_(1.f)
{
    AnimationSet2D* animationset = GameStatics::context_->GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>("UI/Companion/animatedcursors.scml");
    if (!animationset)
        return;

    Node* node = GameStatics::rootScene_->CreateChild("AnimatedCursor", LOCAL);
    animatedCursor_ = node->CreateComponent<AnimatedSprite2D>();
    animatedCursor_->SetAnimationSet(animationset);
    animatedCursor_->SetHotSpot(Vector2(0.5f,0.5f));
    animatedCursor_->SetUseHotSpot(true);
    animatedCursor_->SetLayer(10000);

    node->SetEnabled(false);
    animatedCursorEntity_ = -1;
}

InputPlayer::~InputPlayer()
{
    if (testText_)
        testText_->Free();
}

InputPlayer* InputPlayer::Get()
{
    if (!player_)
        player_ = new InputPlayer(GameStatics::context_);

    return player_;
}

void InputPlayer::Release()
{
    if (!player_)
    {
        delete player_;
        player_ = 0;
    }
}

void InputPlayer::SetMode(int mode)
{
    if (mode_ != mode)
    {
        mode_ = mode;
        if (testCursor_)
            testCursor_->SetVisible(false);
    }

    clickdone_ = false;
}

void InputPlayer::SetRecord(const InputRecordEntries& entries)
{
    recordentries_ = entries;
    SetMode(IPLAYER_RECORD);
}

void InputPlayer::SetNumRandomEntries(unsigned num)
{
    numrandomentries_ = num;
    SetMode(IPLAYER_RANDOM);
}

void InputPlayer::StartFile(const String& name)
{
    if (LoadFile(name))
        Start();
}

void InputPlayer::Start()
{
    UnsubscribeFromAllEvents();

    if (mode_ == IPLAYER_RANDOM && numrandomentries_ > 0)
        GameStatics::playingTest_ = true;
    else if (mode_ == IPLAYER_RECORD && recordentries_.Size() > 0 && GameStatics::playTest_)
        GameStatics::playingTest_ = true;
    else if (mode_ == IPLAYER_FILE && recordentries_.Size() > 0)
    {
        GameStatics::playTest_ = 1;
        GameStatics::playingTest_ = true;
    }
    else
        GameStatics::playingTest_ = false;

    if (GameStatics::playingTest_)
    {
        URHO3D_LOGINFOF("InputPlayer() - Start this=%u mode=%s", this, GameTestModeNames_[mode_]);

        GameStatics::input_->ResetStates();
        timer_.Reset();
        ientry_ = 0;
        clickdone_ = false;
        recordresync_ = 0;

        if (mode_ == IPLAYER_FILE)
            GameStatics::SetTouchEmulation(true);

        if (mode_ != IPLAYER_FILE)
            testText_ = GameHelpers::AddUIMessage(ToString("Start Test %d", GameStatics::playTest_), false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE, IntVector2(0, -1), 1.5f);

        if (mode_ == IPLAYER_RECORD && !testCursor_)
        {
            testCursor_ = GameStatics::ui_->GetRoot()->CreateChild<BorderImage>();
            testCursor_->SetTexture(GetSubsystem<ResourceCache>()->GetResource<Texture2D>("UI/LevelMap/missionSelected2.png"));
            testCursor_->SetPivot(0.5f, 0.5f);
            testCursor_->SetSize(UISIZE[TRIESUISIZE], UISIZE[TRIESUISIZE]);
            testCursor_->SetOpacity(0.85f);
            GameStatics::ui_->AddElementAlwaysOnTop(testCursor_);
            testCursor_->SetVisible(false);
        }
        if (mode_ == IPLAYER_RECORD || mode_ == IPLAYER_FILE)
        {
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(InputPlayer, HandlePlayTest));
            SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(InputPlayer, HandlePlayTest));
            SubscribeToEvent(E_INPUTRESETED, URHO3D_HANDLER(InputPlayer, HandlePlayTest));
        }
        else if (mode_ == IPLAYER_RANDOM)
        {
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(InputPlayer, HandleRandomTest));
            SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(InputPlayer, HandleRandomTest));
            delay_ = Random(TESTRANDOM_DELAYMIN, TESTRANDOM_DELAYMAX);
        }
    }
    else
    {
        URHO3D_LOGWARNINGF("InputPlayer() - Start : this=%u can't start !", this);
    }
}

void InputPlayer::Stop()
{
    URHO3D_LOGINFOF("InputPlayer() - Stop : this=%u ", this);

    UnsubscribeFromAllEvents();

    GameStatics::playingTest_ = false;
    GameStatics::input_->ResetStates();

    GameStatics::SetTouchEmulation(false);

    if (mode_ != IPLAYER_FILE)
        testText_ = GameHelpers::AddUIMessage("Stop Tests", false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE, IntVector2(0, -1), 1.5f);

    SetMode(-1);

    SendEvent(TESTPLAY_FINISHED);
}

bool InputPlayer::HasRecordFile(int irecord) const
{
    FileSystem* fs = context_->GetSubsystem<FileSystem>();
    if (fs->FileExists(GameStatics::gameConfig_.appDir_ + TESTRECORDDIR + TESTRECORDNAME + String(irecord) + String(".bin")))
        return true;
    if (fs->FileExists(GameStatics::gameConfig_.saveDir_ + String(GameStatics::saveDir_) + TESTRECORDNAME + String(irecord) + String(".bin")))
        return true;
    return false;
}

int InputPlayer::GetLastRecordFile() const
{
    int irecord = 1;
    while (HasRecordFile(irecord))
        irecord++;
    return irecord-1;
}

bool InputPlayer::LoadRecord(int irecord)
{
    File f(context_);
    {
        bool opened = false;
        FileSystem* fs = context_->GetSubsystem<FileSystem>();
        if (fs->FileExists(GameStatics::gameConfig_.appDir_ + TESTRECORDDIR + TESTRECORDNAME + String(irecord) + String(".bin")))
            opened = f.Open(GameStatics::gameConfig_.appDir_ + TESTRECORDDIR + TESTRECORDNAME + String(irecord) + String(".bin"), FILE_READ);
        if (!opened && fs->FileExists(GameStatics::gameConfig_.saveDir_ + String(GameStatics::saveDir_) + TESTRECORDNAME + String(irecord) + String(".bin")))
            opened = f.Open(GameStatics::gameConfig_.saveDir_ + String(GameStatics::saveDir_) + TESTRECORDNAME + String(irecord) + String(".bin"), FILE_READ);
        if (!opened)
            return false;
    }

    unsigned size = f.GetSize();
    unsigned numentries = size / sizeof(InputRecordEntry);
    recordentries_.Resize(numentries);

    URHO3D_LOGINFOF("InputPlayer() - LoadRecord : this=%u irecord=%d ... filename=%s filesize=%u numentries=%u ...", this, irecord, f.GetName().CString(), size, numentries);

    if (f.Read((void*)recordentries_.Buffer(), size))
    {
        f.Close();
        if (recordentries_.Size() == numentries)
        {
            URHO3D_LOGINFOF("InputPlayer() - LoadRecord : irecord=%d OK", irecord);
            LogInputRecordEntries(recordentries_);
            SetMode(IPLAYER_RECORD);

            return true;
        }
        else
        {
            URHO3D_LOGERRORF("InputPlayer() - LoadRecord : irecord=%d NOK", irecord);
            return false;
        }
    }

    return false;
}

bool InputPlayer::LoadFile(const String& filename)
{
    File f(context_);
    {
        bool opened = false;
        FileSystem* fs = context_->GetSubsystem<FileSystem>();
        if (fs->FileExists(GameStatics::gameConfig_.appDir_ + TESTRECORDDIR + filename + String(".bin")))
            opened = f.Open(GameStatics::gameConfig_.appDir_ + TESTRECORDDIR + filename + String(".bin"), FILE_READ);
        if (!opened && fs->FileExists(GameStatics::gameConfig_.saveDir_ + String(GameStatics::saveDir_) + filename + String(".bin")))
            opened = f.Open(GameStatics::gameConfig_.saveDir_ + String(GameStatics::saveDir_) + filename + String(".bin"), FILE_READ);
        if (!opened)
            return false;
    }

    unsigned size = f.GetSize();
    unsigned numentries = size / sizeof(InputRecordEntry);
    recordentries_.Resize(numentries);

    URHO3D_LOGINFOF("InputPlayer() - LoadFile : this=%u ... filename=%s filesize=%u numentries=%u ...", this, f.GetName().CString(), size, numentries);

    if (f.Read((void*)recordentries_.Buffer(), size))
    {
        f.Close();
        if (recordentries_.Size() == numentries)
        {
            URHO3D_LOGINFO("InputPlayer() - LoadFile ... OK !");
            LogInputRecordEntries(recordentries_);
            SetMode(IPLAYER_FILE);
            return true;
        }
    }

    URHO3D_LOGERROR("InputPlayer() - LoadFile ... NOK !");
    return false;
}

bool InputPlayer::IsPlaying() const
{
    return GameStatics::playingTest_;
}


void InputPlayer::TestClickDown(const IntVector2& position, int button)
{
    TestMouseMove(position);

//    URHO3D_LOGINFOF("InputPlayer() - TestClickDown(%s) buttondown=%d buttonpressed=%d",
//                    position.ToString().CString(), GameStatics::input_->GetMouseButtonDown(button), GameStatics::input_->GetMouseButtonPress(button));

    // Set Mouse Buttons
    if (!GameStatics::gameConfig_.touchEnabled_)
        GameStatics::input_->SetMouseButton(button, true, 1);
    else
    {
        if (!GameStatics::input_->GetTouchEmulation())
        {
            GameStatics::input_->AddTouch(0, position, 1.f);

            using namespace TouchBegin;
            VariantMap& eventData = GameStatics::context_->GetEventDataMap();
            eventData[P_TOUCHID] = 0;
            eventData[P_X] = position.x_;
            eventData[P_Y] = position.y_;
            eventData[P_PRESSURE] = 1.f;
            GameStatics::input_->SendEvent(E_TOUCHBEGIN, eventData);

            URHO3D_LOGINFOF("GameHelpers() - TestClickDown(%s) TOUCH_BEGIN", position.ToString().CString());
        }
        else
        {
            SDL_Event event;
            event.type = SDL_FINGERDOWN;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = 0;
            event.tfinger.pressure = 1.0f;
            event.tfinger.x = (float)position.x_ / (float)GameStatics::graphics_->GetWidth();
            event.tfinger.y = (float)position.y_ / (float)GameStatics::graphics_->GetHeight();
            event.tfinger.dx = 0;
            event.tfinger.dy = 0;
            SDL_PushEvent(&event);
            URHO3D_LOGINFOF("GameHelpers() - TestClickDown(%s) TOUCH_BEGIN Emulation", position.ToString().CString());
        }
    }

    testCursor_->BringToFront();
}

void InputPlayer::TestClickWheel(const IntVector2& position, int wheel)
{
    TestMouseMove(position);

    // Don't Work with TouchEmulation
    // TODO : Add a gesture and SDL_PushEvent ?

    // Send Mouse Wheel
    VariantMap& eventData = GameStatics::context_->GetEventDataMap();
    eventData[MouseWheel::P_WHEEL] = wheel;
    eventData[MouseWheel::P_BUTTONS] = 0;
    eventData[MouseWheel::P_QUALIFIERS] = 0;
    GameStatics::input_->SendEvent(E_MOUSEWHEEL, eventData);
}

void InputPlayer::TestMouseMove(const IntVector2& position, const IntVector2& position2, unsigned delay)
{
    // Reset Buttons
    if (!GameStatics::gameConfig_.touchEnabled_)
    {
        GameStatics::input_->SetMouseButton(MOUSEB_LEFT, false, 0);
        GameStatics::input_->SetMouseButton(MOUSEB_RIGHT, false, 0);
        GameStatics::input_->SetMouseButton(MOUSEB_MIDDLE, false, 0);
    }

    IntVector2 uiposition;
    uiposition.x_ = floor((float)position.x_ / GameStatics::ui_->GetScale());
    uiposition.y_ = floor((float)position.y_ / GameStatics::ui_->GetScale());

    // Set Mouse Position
    if (!GameStatics::gameConfig_.touchEnabled_)
    {
        if (GameStatics::cursor_ && GameStatics::cursor_->IsVisible())
            GameStatics::cursor_->SetPosition(uiposition);

        GameStatics::input_->SetMousePosition(position);

        VariantMap& eventData = GameStatics::context_->GetEventDataMap();
        eventData[MouseMove::P_X] = position.x_;
        eventData[MouseMove::P_Y] = position.y_;
        eventData[MouseMove::P_DX] = 0;
        eventData[MouseMove::P_DY] = 0;
        eventData[MouseMove::P_BUTTONS] = 0;
        GameStatics::input_->SendEvent(E_MOUSEMOVE, eventData);
    }

    if (delay > 0U && cursorSmoothMove_)
    {
        IntVector2 uiposition2;
        uiposition2.x_ = floor((float)position2.x_ / GameStatics::ui_->GetScale());
        uiposition2.y_ = floor((float)position2.y_ / GameStatics::ui_->GetScale());

        if (testCursor_)
        {
            SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
            SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(context_));
            moveAnimation->SetKeyFrame(0.f, testCursor_->GetPosition());
            moveAnimation->SetKeyFrame(delay / 1000.f, uiposition2);
            objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
            testCursor_->SetObjectAnimation(objectAnimation);
            testCursor_->SetAnimationEnabled(true);
        }
        if (animatedCursor_ && animatedCursorEntity_ != -1)
        {
            SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
            SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(context_));
            moveAnimation->SetKeyFrame(0.f, animatedCursor_->GetNode()->GetPosition());
            moveAnimation->SetKeyFrame(delay / 1000.f, Vector3(GameHelpers::ScreenToWorld2D(position2)));
            objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
            animatedCursor_->GetNode()->SetObjectAnimation(objectAnimation);
            animatedCursor_->GetNode()->SetAnimationEnabled(true);
        }
    }
    else
    {
        if (testCursor_)
        {
            testCursor_->SetAnimationEnabled(false);
            testCursor_->SetPosition(uiposition);
        }
        if (animatedCursor_)
        {
            animatedCursor_->GetNode()->SetAnimationEnabled(false);
            animatedCursor_->GetNode()->SetPosition2D(GameHelpers::ScreenToWorld2D(position));
        }
    }

    if (testCursor_)
        testCursor_->SetVisible(true);
    if (animatedCursor_ && animatedCursorEntity_ != -1)
    {
        animatedCursor_->GetNode()->SetEnabled(true);
        animatedCursor_->GetNode()->SetScale2D(Vector2(0.5f,0.5f) / GameStatics::camera_->GetZoom());
    }
}

void InputPlayer::TestClickUp(const IntVector2& position)
{
    if (GameStatics::gameConfig_.touchEnabled_)
    {
        if (!GameStatics::input_->GetTouchEmulation())
        {
            URHO3D_LOGINFOF("InputPlayer() - TestClickUp(%s) TOUCH_END", position.ToString().CString());

            using namespace TouchEnd;
            VariantMap& eventData = GameStatics::context_->GetEventDataMap();
            eventData[P_TOUCHID] = 0;
            eventData[P_X] = position.x_;
            eventData[P_Y] = position.y_;
            GameStatics::input_->SendEvent(E_TOUCHEND, eventData);

            GameStatics::input_->RemoveTouch(0);
        }
        else
        {
            URHO3D_LOGINFOF("InputPlayer() - TestClickUp(%s) TOUCH_END Emulation ", position.ToString().CString());

            SDL_Event event;
            event.type = SDL_FINGERUP;
            event.tfinger.touchId = 0;
            event.tfinger.fingerId = 0;
            event.tfinger.pressure = 0.0f;
            event.tfinger.x = (float)position.x_ / (float)GameStatics::graphics_->GetWidth();
            event.tfinger.y = (float)position.y_ / (float)GameStatics::graphics_->GetHeight();
            event.tfinger.dx = 0;
            event.tfinger.dy = 0;
            SDL_PushEvent(&event);
        }
    }
    else
    {
        // Reset Buttons
        GameStatics::input_->SetMouseButton(MOUSEB_LEFT, false, 0);
        GameStatics::input_->SetMouseButton(MOUSEB_RIGHT, false, 0);
        GameStatics::input_->SetMouseButton(MOUSEB_MIDDLE, false, 0);
    }

    IntVector2 uiposition;
    uiposition.x_ = floor((float)position.x_ / GameStatics::ui_->GetScale());
    uiposition.y_ = floor((float)position.y_ / GameStatics::ui_->GetScale());

    if (testCursor_)
    {
        testCursor_->SetPosition(uiposition);
        testCursor_->SetVisible(true);
    }
}


void InputPlayer::HandlePlayTest(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_INPUTRESETED)
    {
        if (testCursor_)
            testCursor_->SetVisible(false);
        if (animatedCursor_)
            animatedCursor_->GetNode()->SetEnabled(false);
        animatedCursorEntity_ = -1;

        if (clickdone_)
            clickdone_ = false;

        if (!IsPlaying())
        {
            UnsubscribeFromAllEvents();
            URHO3D_LOGINFOF("InputPlayer() - HandlePlayTest : this=%u No Playing !", this);
        }
//        else
//            URHO3D_LOGINFOF("InputPlayer() - HandlePlayTest : this=%u Input Reseted => next record=%d", this, ientry_);

        return;
    }

    unsigned time = timer_.GetMSec(false);

    if (ientry_ < recordentries_.Size())
    {
        if (!clickdone_)
        {
            const InputRecordEntry& entry = recordentries_[ientry_];

            if (entry.type_ == IRECORD_MOVE && time >= entry.time_)
            {
                entry.Get(testposition_);
                IntVector2 testposition2;

                unsigned delay = 0U;
                if (ientry_+1 < recordentries_.Size())
                {
                    recordentries_[ientry_+1].Get(testposition2);
                    delay = recordentries_[ientry_+1].time_ - entry.time_;
                }

                TestMouseMove(testposition_, testposition2, delay);

                ientry_++;
            }
            else if (entry.type_ == IRECORD_SETCURSOR && animatedCursor_)
            {
                animatedCursorEntity_ = entry.value_;
                if (animatedCursorEntity_ != -1)
                {
                    animatedCursor_->SetSpriterEntity(animatedCursorEntity_);
                    animatedCursor_->SetAnimation("play");
                }
                else
                {
                    animatedCursor_->GetNode()->SetEnabled(false);
                }

                URHO3D_LOGERRORF("InputPlayer() - HandlePlayTest : animatedCursor=%d !", animatedCursorEntity_);

                entry.Get(testposition_);
                TestMouseMove(testposition_);

                ientry_++;
            }
            else if (entry.type_ != IRECORD_MOVE && time >= entry.time_ - TESTCLICK_THRESHOLD)
            {
                entry.Get(testposition_);

                if (entry.type_ == IRECORD_BUTTON)
                    TestClickDown(testposition_, entry.value_);
                else if (entry.type_ == IRECORD_WHEEL)
                    TestClickWheel(testposition_, entry.value_);

                clickdone_ = true;
                timer_.Reset();
            }
        }
        else if (time >= TESTCLICK_DELAYPRESSED)
        {
            TestClickUp(testposition_);

            clickdone_ = false;
            ientry_++;
        }
    }
    else if (time >= TESTCLICK_DELAYPRESSED)
    {
        TestClickUp(testposition_);

        GameStatics::SetTouchEmulation(false);
        GameStatics::input_->ResetStates();
        GameStatics::playingTest_ = false;

        URHO3D_LOGINFOF("InputPlayer() - HandlePlayTest : No more entries in the record => Stop the record !");

        UnsubscribeFromAllEvents();

        if (mode_ == IPLAYER_RECORD)
        {
            URHO3D_LOGINFOF("InputPlayer() - HandlePlayTest : IPLAYER_RECORD go back to mainmenu !");

            if (GameStateManager::Get()->GetActiveState()->GetStateId() != "MainMenu")
            {
                // Always Go Back to MainMenu
                GameStateManager::Get()->ClearStack();
                GameStateManager::Get()->PushToStack("MainMenu");
                // Be sure to not have the SplashScreen in close state
                SendEvent(SPLASHSCREEN_STOP);
            }

            // Next test
            GameStatics::playTest_++;

            DelayInformer* timer3 = DelayInformer::Get(this, SWITCHSCREENTIME + 1.5f, GAME_PLAYTEST);
            SubscribeToEvent(this, GAME_PLAYTEST, URHO3D_HANDLER(InputPlayer, HandleStartPlayTest));
        }

        SetMode(-1);
    }
}

void InputPlayer::PlayRecord(int irecord)
{
    if (LoadRecord(irecord))
    {
        GameStatics::playTest_ = irecord;
        Start();
    }
    else
    {
        testText_ = GameHelpers::AddUIMessage("Stop Tests", false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE, IntVector2(0, -1), 1.5f);

    #ifdef __ANDROID__
        Android_FinishTest();
//        GameStatics::Exit();
    #endif
    }
}

void InputPlayer::HandleStartPlayTest(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, GAME_PLAYTEST);
    PlayRecord(GameStatics::playTest_);
}

void InputPlayer::HandleRandomTest(StringHash eventType, VariantMap& eventData)
{
    unsigned time = timer_.GetMSec(false);

    if (ientry_ < numrandomentries_)
    {
        if (time >= delay_)
        {
            if (!GameStatics::input_->GetMouseButtonDown(MOUSEB_LEFT))
            {
                // TODO ui scaling
                IntVector2 position(Random(0, GameStatics::targetwidth_), Random(0, GameStatics::targetheight_));
                URHO3D_LOGINFOF("InputPlayer() - HandleRandomTest : delay=%u pos=%s", delay_, position.ToString().CString());
                TestClickDown(position, MOUSEB_LEFT);
                return;
            }
            if (time >= delay_ + TESTCLICK_DELAYPRESSED)
            {
//                URHO3D_LOGINFOF("InputPlayer() - HandleRandomTest : buttondown=%d buttonpressed=%d reset after press delay ...",
//                                GameStatics::input_->GetMouseButtonDown(MOUSEB_LEFT), GameStatics::input_->GetMouseButtonPress(MOUSEB_LEFT));
                timer_.Reset();
                delay_ = Random(TESTRANDOM_DELAYMIN, TESTRANDOM_DELAYMAX);
                ientry_++;
                GameStatics::input_->ResetStates();
            }
        }
    }
    else if (time >= TESTCLICK_DELAYPRESSED)
    {
        GameStatics::playingTest_ = false;
        UnsubscribeFromAllEvents();
        SetMode(-1);
    }
}

