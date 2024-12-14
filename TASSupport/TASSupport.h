#pragma once

#include <string>
#include <vector>

#include <BML/BMLAll.h>

#include "physics_RT.h"
#include "TASRecord.h"

MOD_EXPORT IMod *BMLEntry(IBML *bml);
MOD_EXPORT void BMLExit(IMod *mod);

typedef enum TASState {
    TAS_IDLE = 0,
    TAS_PLAYING = 0x1,
    TAS_RECORDING = 0x2,
} TASState;

class TASSupport : public IMod {
public:
    explicit TASSupport(IBML *bml) : IMod(bml) {}

    const char *GetID() override { return "TASSupport"; }
    const char *GetVersion() override { return BML_VERSION; }
    const char *GetName() override { return "TAS Support"; }
    const char *GetAuthor() override { return "Kakuty & Gamepiaynmo"; }
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
    void OnPreLoadLevel() override;
    void OnStartLevel() override;
    void OnPreResetLevel() override;
    void OnPreExitLevel() override;
    void OnLevelFinish() override;
    void OnBallOff() override;
    void OnPreCheckpointReached() override;
    void OnPostCheckpointReached() override;

#ifdef _DEBUG
    void OnPhysicalize(CK3dEntity *target, CKBOOL fixed, float friction, float elasticity, float mass,
                       const char *collGroup, CKBOOL startFrozen, CKBOOL enableColl, CKBOOL calcMassCenter,
                       float linearDamp, float rotDamp, const char *collSurface, VxVector massCenter,
                       int convexCnt, CKMesh **convexMesh, int ballCnt, VxVector *ballCenter, float *ballRadius,
                       int concaveCnt, CKMesh **concaveMesh) override;
#endif

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
    void SetNextMovementCheck(short count = 0);
    void SetupNewRecord();

    void RefreshRecords();
    void OpenTASMenu();
    void ExitTASMenu();

    InputState GetKeyboardState(const unsigned char *src) const;
    void SetKeyboardState(unsigned char *dest, const InputState &state) const;
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
};
