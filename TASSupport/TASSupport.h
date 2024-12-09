#pragma once

#include <string>
#include <vector>

#include <BML/BMLAll.h>

#include "physics_RT.h"

MOD_EXPORT IMod *BMLEntry(IBML *bml);
MOD_EXPORT void BMLExit(IMod *mod);

typedef enum TASState {
    TAS_IDLE = 0,
    TAS_PLAYING = 0x1,
    TAS_RECORDING = 0x2,
} TASState;

struct KeyState {
    unsigned key_up: 1;
    unsigned key_down: 1;
    unsigned key_left: 1;
    unsigned key_right: 1;
    unsigned key_shift: 1;
    unsigned key_space: 1;
    unsigned key_q: 1;
    unsigned key_esc: 1;
    unsigned key_enter: 1;
};

class FrameData {
public:
    FrameData() = default;
    explicit FrameData(float deltaTime) : m_DeltaTime(deltaTime) {}

    float GetDeltaTime() const {
        return m_DeltaTime;
    }

    void SetDeltaTime(float delta) {
        m_DeltaTime = delta;
    }

    const KeyState &GetKeyState() const {
        return m_KeyState;
    }

    void SetKeyState(const KeyState &state) {
        m_KeyState = state;
    }

private:
    float m_DeltaTime = 0.0f;
    KeyState m_KeyState = {};
};

struct PhysicsData {
    PhysicsData() = default;
    explicit PhysicsData(float deltaTime) : deltaTime(deltaTime) {}

    float deltaTime = 0.0f;
    VxVector position;
    VxVector angles;
    VxVector velocity;
    VxVector angularVelocity;
};

class TASRecord {
public:
    TASRecord() = default;
    TASRecord(std::string name, std::string path) : m_Name(std::move(name)), m_Path(std::move(path)) {}

    bool operator==(const TASRecord &rhs) const { return m_Name == rhs.m_Name; }
    bool operator!=(const TASRecord &rhs) const { return !(rhs == *this); }

    bool operator<(const TASRecord &o) const { return m_Name < o.m_Name; }
    bool operator>(const TASRecord &rhs) const { return rhs < *this; }
    bool operator<=(const TASRecord &rhs) const { return !(rhs < *this); }
    bool operator>=(const TASRecord &rhs) const { return !(*this < rhs); }

    const std::string &GetName() const { return m_Name; }
    void SetName(const std::string &name) { m_Name = name; }

    const std::string &GetPath() const { return m_Path; }
    void SetPath(const std::string &path) { m_Path = path; }

    bool IsLoaded() const { return m_Loaded; }
    bool Load();
    bool Save();

    bool IsPlaying() const { return m_FrameIndex < m_FrameData.size(); }
    bool IsFinished() const { return m_FrameIndex == m_FrameData.size(); }

    size_t GetLength() const { return m_FrameData.size(); }
    size_t GetFrameIndex() const { return m_FrameIndex; }
    FrameData &GetFrameData() { return m_FrameData[m_FrameIndex]; }

    void NextFrame() { ++m_FrameIndex; }
    void PrevFrame() { --m_FrameIndex; }
    void ResetFrame() { m_FrameIndex = 0; }

    void NewFrame(const FrameData &frame) {
        if (!m_FrameData.empty())
            ++m_FrameIndex;
        m_FrameData.emplace_back(frame);
    }

    void Clear() {
        m_Loaded = false;
        m_FrameIndex = 0;
        m_FrameData.clear();
    }

private:
    std::string m_Name;
    std::string m_Path;
    bool m_Loaded = false;
    std::size_t m_FrameIndex = 0;
    std::vector<FrameData> m_FrameData;
};

class TASSupport : public IMod {
public:
    explicit TASSupport(IBML *bml) : IMod(bml) {}

    const char *GetID() override { return "TASSupport"; }
    const char *GetVersion() override { return BML_VERSION; }
    const char *GetName() override { return "TAS Support"; }
    const char *GetAuthor() override { return "Gamepiaynmo & Kakuty"; }
    const char *GetDescription() override { return "Make TAS possible in Ballance (WIP)."; }
    DECLARE_BML_VERSION;

    void OnLoad() override;
    void OnUnload() override;
    void OnModifyConfig(const char *category, const char *key, IProperty *prop) override;
    void OnLoadObject(const char *filename, CKBOOL isMap, const char *masterName, CK_CLASSID filterClass,
                      CKBOOL addToScene, CKBOOL reuseMeshes, CKBOOL reuseMaterials, CKBOOL dynamic,
                      XObjectArray *objArray, CKObject *masterObj) override;
    void OnLoadScript(const char *filename, CKBehavior *script) override;

    void OnProcess() override;

    void OnPostStartMenu() override;
    void OnExitGame() override;

    void OnPreLoadLevel() override {
        if (m_Legacy)
            OnStart();
    }
    void OnPreResetLevel() override { OnStop(); }
    void OnPreExitLevel() override { OnStop(); }
    void OnStartLevel() override {
        if (!m_Legacy)
            OnStart();
    }
    void OnLevelFinish() override { OnFinish(); }

    void OnBallOff() override;

    void OnStart();
    void OnStop();
    void OnFinish();

    void OnPreProcessInput();
    void OnPreProcessTime();

    void OnDrawMenu();
    void OnDrawKeys();
    void OnDrawInfo();

    bool IsIdle() const { return m_State == 0; }
    bool IsPlaying() const { return (m_State & TAS_PLAYING) != 0; }
    bool IsRecording() const { return (m_State & TAS_RECORDING) != 0; }

    void InitHooks();
    void ShutdownHooks();

    void AcquireKeyBindings();
    void ResetPhysicsTime();
    void SetPhysicsTimeFactor(float factor = 1.0f);
    void SetupNewRecord();

    void RefreshRecords();
    void OpenTASMenu();
    void ExitTASMenu();

    KeyState GetKeyboardState(const unsigned char *src) const;
    void SetKeyboardState(unsigned char *dest, const KeyState &state) const;
    void ResetKeyboardState(unsigned char *dest) const;

    CK3dEntity *GetActiveBall() const;

    CKDWORD m_PhysicsRTVersion = 0;
    CKIpionManager *m_IpionManager = nullptr;
    CKTimeManager *m_TimeManager = nullptr;
    InputHook *m_InputHook = nullptr;

    CKDataArray *m_CurLevel = nullptr;
    CKDataArray *m_Keyboard = nullptr;
    CKKEYBOARD m_KeyUp = CKKEY_UP;
    CKKEYBOARD m_KeyDown = CKKEY_DOWN;
    CKKEYBOARD m_KeyLeft = CKKEY_LEFT;
    CKKEYBOARD m_KeyRight = CKKEY_RIGHT;
    CKKEYBOARD m_KeyShift = CKKEY_LSHIFT;
    CKKEYBOARD m_KeySpace = CKKEY_SPACE;

    int m_State = TAS_IDLE;
    int m_CurrentPage = 0;
    bool m_ShowMenu = false;
    bool m_Hooked = false;
    bool m_Legacy = false;

    TASRecord m_NewRecord;
    TASRecord m_RecordOnStartup;
    std::vector<TASRecord> m_Records;
    TASRecord *m_CurrentRecord = nullptr;

    std::string m_MapName;
    CK2dEntity *m_Level01 = nullptr;
    CKBehavior *m_ExitStart = nullptr;
    CKBehavior *m_ExitMain = nullptr;
    CKParameter *m_ActiveBall = nullptr;

    IProperty *m_ShowKeys = nullptr;
    IProperty *m_ShowInfo = nullptr;
    char m_FrameCountText[100] = {};

    IProperty *m_Enabled = nullptr;
    IProperty *m_Record = nullptr;
    IProperty *m_StopKey = nullptr;
    IProperty *m_SkipRender = nullptr;
    IProperty *m_ExitOnDead = nullptr;
    IProperty *m_ExitOnFinish = nullptr;
    IProperty *m_ExitKey = nullptr;
    IProperty *m_LoadTAS = nullptr;
    IProperty *m_LoadLevel = nullptr;
    IProperty *m_LegacyMode = nullptr;
    IProperty *m_Transcript = nullptr;
};
