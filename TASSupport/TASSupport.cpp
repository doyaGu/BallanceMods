#include "TASSupport.h"

#include <cstdio>
#include <ctime>
#include <thread>
#include <sys/stat.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Psapi.h>

#include <MinHook.h>

#include <BML/Bui.h>

#define BML_TAS_PATH "..\\ModLoader\\TASRecords\\"

TASSupport *g_Mod = nullptr;

IMod *BMLEntry(IBML *bml) {
    g_Mod = new TASSupport(bml);
    return g_Mod;
}

void BMLExit(IMod *mod) {
    delete mod;
}

template<typename T>
T ForceReinterpretCast(void *base, size_t offset) {
    void *p = static_cast<char *>(base) + offset;
    return *reinterpret_cast<T *>(&p);
}

static void *GetModuleBaseAddress(const char *modulePath) {
    if (!modulePath)
        return nullptr;

    int size = ::MultiByteToWideChar(CP_UTF8, 0, modulePath, -1, nullptr, 0);
    if (size == 0)
        return nullptr;

    auto ws = new wchar_t[size];
    ::MultiByteToWideChar(CP_UTF8, 0, modulePath, -1, ws, size);

    HMODULE hModule = ::GetModuleHandleW(ws);
    delete[] ws;
    if (!hModule)
        return nullptr;

    MODULEINFO moduleInfo;
    ::GetModuleInformation(::GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo));

    return moduleInfo.lpBaseOfDll;
}

static CKDWORD GetPhysicsRTVersion() {
    CKPluginEntry *entry = CKGetPluginManager()->FindComponent(CKGUID(0x6BED328B, 0x141F5148));
    if (entry)
        return entry->m_PluginInfo.m_Version;
    return 0;
}

static char *UncompressDataFromFile(const char *filename, size_t &size) {
    if (!filename)
        return nullptr;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return nullptr;

    fseek(fp, 0, SEEK_END);
    int nsize = ftell(fp) - 4;
    fseek(fp, 0, SEEK_SET);

    char *buffer = new char[nsize];
    fread(&size, sizeof(size_t), 1, fp);
    fread(buffer, nsize, 1, fp);
    fclose(fp);

    char *res = CKUnPackData((int) size, buffer, nsize);
    delete[] buffer;

    return res;
}

static bool CompressDataToFile(char *data, size_t size, const char *filename) {
    if (!data || size == 0 || !filename)
        return false;

    int nsize;
    char *res = CKPackData(data, (int) size, nsize, 9);

    FILE *fp = fopen(filename, "wb");
    if (!fp) return false;

    fwrite(&size, sizeof(size_t), 1, fp);
    fwrite(res, nsize, 1, fp);
    fclose(fp);

    CKDeletePointer(res);

    return true;
}

static double (*ivp_rand)() = nullptr;
static double (*ivp_rand_orig)() = nullptr;
static int (*qh_rand)() = nullptr;
static int (*qh_rand_orig)() = nullptr;

double IVP_Rand() { return 1; }
int QH_Rand() { return 1; }

typedef CKERROR (CKBaseManager::*PreProcessFunc)();

class TimeManagerHook : public CKTimeManager {
public:
    CKERROR PreProcessHook() {
        CKERROR ret = (this->*s_PreProcessFuncOrig)();
        g_Mod->OnPreProcessTime();
        return ret;
    }

    static PreProcessFunc s_PreProcessFunc;
    static PreProcessFunc s_PreProcessFuncOrig;
    static PreProcessFunc s_PreProcessFuncTarget;
};

PreProcessFunc TimeManagerHook::s_PreProcessFunc = reinterpret_cast<PreProcessFunc>(&TimeManagerHook::PreProcessHook);
PreProcessFunc TimeManagerHook::s_PreProcessFuncOrig = nullptr;
PreProcessFunc TimeManagerHook::s_PreProcessFuncTarget = nullptr;

class InputManagerHook : public CKInputManager {
public:
    CKERROR PreProcessHook() {
        CKERROR ret = (this->*s_PreProcessFuncOrig)();
        g_Mod->OnPreProcessInput();
        return ret;
    }

    static PreProcessFunc s_PreProcessFunc;
    static PreProcessFunc s_PreProcessFuncOrig;
    static PreProcessFunc s_PreProcessFuncTarget;
};

PreProcessFunc InputManagerHook::s_PreProcessFunc = reinterpret_cast<PreProcessFunc>(&InputManagerHook::PreProcessHook);
PreProcessFunc InputManagerHook::s_PreProcessFuncOrig = nullptr;
PreProcessFunc InputManagerHook::s_PreProcessFuncTarget = nullptr;

bool TASRecord::Load() {
    if (m_Path.empty())
        return false;

    size_t size = 0;
    char *data = UncompressDataFromFile(m_Path.c_str(), size);
    if (data) {
        const size_t count = size / sizeof(FrameData);
        m_FrameData.resize(count);
        for (size_t i = 0; i < count; i++) {
            memcpy(&m_FrameData[i], &data[i * sizeof(FrameData)], sizeof(FrameData));
        }
        CKDeletePointer(data);
    }

    m_Loaded = true;
    return true;
}

bool TASRecord::Save() {
    if (m_Path.empty())
        return false;

    CompressDataToFile((char *) &m_FrameData[0], m_FrameData.size() * sizeof(FrameData), m_Path.c_str());
    return true;
}

void TASSupport::OnLoad() {
    GetConfig()->SetCategoryComment("Misc", "Miscellaneous");

    m_Enabled = GetConfig()->GetProperty("Misc", "Enable");
    m_Enabled->SetComment("Enable TAS Features");
    m_Enabled->SetDefaultBoolean(false);

    m_Record = GetConfig()->GetProperty("Misc", "Record");
    m_Record->SetComment("Record Actions");
    m_Record->SetDefaultBoolean(false);

    m_StopKey = GetConfig()->GetProperty("Misc", "StopKey");
    m_StopKey->SetComment("Key for stop playing");
    m_StopKey->SetDefaultKey(CKKEY_F3);

    m_ShowKeys = GetConfig()->GetProperty("Misc", "ShowKeysGui");
    m_ShowKeys->SetComment("Show realtime keyboard behavior for TAS records");
    m_ShowKeys->SetDefaultBoolean(true);

    m_ShowInfo = GetConfig()->GetProperty("Misc", "ShowInfoGui");
    m_ShowInfo->SetComment("Show realtime physics info");
    m_ShowInfo->SetDefaultBoolean(true);

    m_SkipRender = GetConfig()->GetProperty("Misc", "SkipRenderUntil");
    m_SkipRender->SetComment("Skip render until the given frame to speed up TAS playing");
    m_SkipRender->SetDefaultInteger(0);

    m_ExitOnDead = GetConfig()->GetProperty("Misc", "ExitOnDead");
    m_ExitOnDead->SetComment("Automatically exit game when ball fell");
    m_ExitOnDead->SetDefaultBoolean(false);

    m_ExitOnFinish = GetConfig()->GetProperty("Misc", "ExitOnFinish");
    m_ExitOnFinish->SetComment("Automatically exit game when TAS playing finished");
    m_ExitOnFinish->SetDefaultBoolean(false);

    m_ExitKey = GetConfig()->GetProperty("Misc", "ExitKey");
    m_ExitKey->SetComment("Press to exit game quickly");
    m_ExitKey->SetDefaultKey(CKKEY_DELETE);

    m_LoadTAS = GetConfig()->GetProperty("Misc", "AutoLoadTAS");
    m_LoadTAS->SetComment("Automatically load TAS record on game startup");
    m_LoadTAS->SetDefaultString("");

    m_LoadLevel = GetConfig()->GetProperty("Misc", "AutoLoadLevel");
    m_LoadLevel->SetComment("Automatically load given level on game startup");
    m_LoadLevel->SetDefaultInteger(0);

    m_LegacyMode = GetConfig()->GetProperty("Misc", "LegacyMode");
    m_LegacyMode->SetComment("Compatibility mode for older TAS records (Restart game to take effect)");
    m_LegacyMode->SetDefaultBoolean(false);
    m_Legacy = m_LegacyMode->GetBoolean();

    VxMakeDirectory((CKSTRING) BML_TAS_PATH);

    m_PhysicsRTVersion = GetPhysicsRTVersion();
    if (m_PhysicsRTVersion == 0x000001) {
        InitPhysicsMethodPointers();
    }

    m_IpionManager = (CKIpionManager *) m_BML->GetCKContext()->GetManagerByGuid(CKGUID(0x6bed328b, 0x141f5148));
    m_TimeManager = m_BML->GetTimeManager();
    m_InputHook = m_BML->GetInputManager();

    if (MH_Initialize() != MH_OK) {
        GetLogger()->Error("Failed to initialize MinHook");
    }

    if (m_Enabled->GetBoolean()) {
        InitHooks();
    }
}

void TASSupport::OnUnload() {
    if (m_Enabled->GetBoolean()) {
        ShutdownHooks();
    }

    MH_Uninitialize();
}

void TASSupport::OnModifyConfig(const char *category, const char *key, IProperty *prop) {
    if (prop == m_Enabled) {
        if (m_Enabled->GetBoolean()) {
            InitHooks();
        } else {
            ShutdownHooks();
        }
    }
}

void TASSupport::OnLoadObject(const char *filename, CKBOOL isMap, const char *masterName, CK_CLASSID filterClass,
                              CKBOOL addToScene, CKBOOL reuseMeshes, CKBOOL reuseMaterials, CKBOOL dynamic,
                              XObjectArray *objArray, CKObject *masterObj) {
    if (!strcmp(filename, "3D Entities\\Gameplay.nmo")) {
        m_CurLevel = m_BML->GetArrayByName("CurrentLevel");
    }

    if (!strcmp(filename, "3D Entities\\Menu.nmo")) {
        m_Level01 = m_BML->Get2dEntityByName("M_Start_But_01");
        CKBehavior *menuStart = m_BML->GetScriptByName("Menu_Start");
        m_ExitStart = ScriptHelper::FindFirstBB(menuStart, "Exit");
        m_Keyboard = m_BML->GetArrayByName("Keyboard");
        CKBehavior *menuMain = m_BML->GetScriptByName("Menu_Main");
        m_ExitMain = ScriptHelper::FindFirstBB(menuMain, "Exit", false, 1, 0);
    }

    if (isMap) {
        m_MapName = filename;
        m_MapName = m_MapName.substr(m_MapName.find_last_of('\\') + 1);
        m_MapName = m_MapName.substr(0, m_MapName.find_last_of('.'));
    }
}

void TASSupport::OnLoadScript(const char *filename, CKBehavior *script) {
    if (m_Enabled->GetBoolean()) {
        if (!strcmp(script->GetName(), "Ball_Explosion_Wood")
            || !strcmp(script->GetName(), "Ball_Explosion_Paper")
            || !strcmp(script->GetName(), "Ball_Explosion_Stone")) {
            CKBehavior *beh = ScriptHelper::FindFirstBB(script, "Set Position");
            ScriptHelper::DeleteBB(script, beh);
        }
        if (!strcmp(script->GetName(), "Gameplay_Ingame")) {
            for (int i = 0; i < script->GetLocalParameterCount(); ++i) {
                CKParameter *param = script->GetLocalParameter(i);
                if (!strcmp(param->GetName(), "ActiveBall")) {
                    m_ActiveBall = param;
                    break;
                }
            }
        }
    }
}

void TASSupport::OnProcess() {
    if (m_Enabled->GetBoolean()) {
        Bui::ImGuiContextScope scope;

#ifndef _DEBUG
        if (m_BML->IsCheatEnabled() && IsRecording())
            OnStop();
#endif

        if (m_Level01 && m_Level01->IsVisible()) {
            const ImVec2 &vpSize = ImGui::GetMainViewport()->Size;
            ImGui::SetNextWindowPos(ImVec2(vpSize.x * 0.61f, vpSize.y * 0.88f));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            constexpr ImGuiWindowFlags ButtonFlags = ImGuiWindowFlags_NoDecoration |
                                                     ImGuiWindowFlags_NoBackground |
                                                     ImGuiWindowFlags_NoMove |
                                                     ImGuiWindowFlags_NoNav |
                                                     ImGuiWindowFlags_AlwaysAutoResize |
                                                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                                                     ImGuiWindowFlags_NoFocusOnAppearing |
                                                     ImGuiWindowFlags_NoSavedSettings;

            if (ImGui::Begin("Button_TAS", nullptr, ButtonFlags)) {
                if (Bui::SmallButton("TAS")) {
                    m_ExitStart->ActivateInput(0);
                    m_ExitStart->Activate();
                    OpenTASMenu();
                }
            }
            ImGui::End();

            ImGui::PopStyleVar(2);
        }

        if (m_ShowMenu)
            OnDrawMenu();

        if (IsPlaying()) {
            if (m_CurrentRecord->GetFrameIndex() < (size_t) m_SkipRender->GetInteger())
                m_BML->SkipRenderForNextTick();

            if (m_InputHook->IsKeyPressed(m_StopKey->GetKey()))
                OnStop();

            if (m_InputHook->IsKeyPressed(m_ExitKey->GetKey())) {
                OnStop();
                m_BML->ExitGame();
            }

            OnDrawKeys();
            OnDrawInfo();
        }
    }
}

void TASSupport::OnPostStartMenu() {
    static bool firstTime = true;

    if (firstTime) {
        std::string tasFile = m_LoadTAS->GetString();
        if (m_Enabled->GetBoolean() && !tasFile.empty()) {
            std::string tasPath = BML_TAS_PATH + tasFile + ".tas";
            struct stat buf = {0};
            if (stat(tasPath.c_str(), &buf) == 0) {
                m_RecordOnStartup.SetName(tasFile);
                m_RecordOnStartup.SetPath(tasPath);
                m_CurrentRecord = &m_RecordOnStartup;

                m_BML->SendIngameMessage(("Loading TAS Record: " + tasFile + ".tas").c_str());
                m_CurrentRecord->Load();

                int level = m_LoadLevel->GetInteger();
                if (level >= 1 && level <= 13) {
                    m_BML->AddTimer(2ul, [this, level]() {
                        m_CurLevel->SetElementValue(0, 0, (void *) &level);

                        CKContext *ctx = m_BML->GetCKContext();
                        CKMessageManager *mm = ctx->GetMessageManager();
                        CKMessageType loadLevel = mm->AddMessageType("Load Level");
                        CKMessageType loadMenu = mm->AddMessageType("Menu_Load");

                        mm->SendMessageSingle(loadLevel, ctx->GetCurrentLevel());
                        mm->SendMessageSingle(loadMenu, m_BML->GetGroupByName("All_Sound"));
                        m_BML->Get2dEntityByName("M_BlackScreen")->Show(CKHIDE);
                        m_ExitMain->ActivateInput(0);
                        m_ExitMain->Activate();
                    });
                }
            } else
                m_BML->SendIngameMessage(("TAS file " + tasFile + ".tas not found.").c_str());
        }

        firstTime = false;
    }
}

void TASSupport::OnExitGame() {
    m_Level01 = nullptr;
}

void TASSupport::OnPreLoadLevel() { OnStart(); }

void TASSupport::OnPreResetLevel() { OnStop(); }

void TASSupport::OnPreExitLevel() { OnStop(); }

void TASSupport::OnLevelFinish() { OnFinish(); }

void TASSupport::OnBallOff() {
    if (m_Enabled->GetBoolean() && IsPlaying() && m_ExitOnDead->GetBoolean())
        m_BML->ExitGame();
}

#ifdef _DEBUG
void TASSupport::OnPhysicalize(CK3dEntity *target, CKBOOL fixed, float friction, float elasticity, float mass,
                               const char *collGroup, CKBOOL startFrozen, CKBOOL enableColl, CKBOOL calcMassCenter,
                               float linearDamp, float rotDamp, const char *collSurface, VxVector massCenter,
                               int convexCnt, CKMesh **convexMesh, int ballCnt, VxVector *ballCenter, float *ballRadius,
                               int concaveCnt, CKMesh **concaveMesh) {
    auto *ball = GetActiveBall();
    if (ball && ball == target) {
        if (m_CurrentRecord)
            GetLogger()->Info("FrameIndex: %d", m_CurrentRecord->GetFrameIndex());

        VxVector position;
        ball->GetPosition(&position);
        GetLogger()->Info("Position: (%.3f, %.3f, %.3f)", position.x, position.y, position.z);

        if (m_PhysicsRTVersion == 0x000001) {
            auto *env = *reinterpret_cast<CKBYTE **>(reinterpret_cast<CKBYTE *>(m_IpionManager) + 0xC0);

            auto &base_time = *reinterpret_cast<double *>(*reinterpret_cast<CKBYTE **>(env + 0x4) + 0x18);
            GetLogger()->Info("time_manager->base_time: %f", base_time);

            auto &current_time = *reinterpret_cast<double *>(env + 0x120);
            GetLogger()->Info("current_time: %f", current_time);

            auto &time_of_last_psi = *reinterpret_cast<double *>(env + 0x130);
            GetLogger()->Info("time_of_last_psi: %f", time_of_last_psi);

            auto &time_of_next_psi = *reinterpret_cast<double *>(env + 0x128);
            GetLogger()->Info("time_of_next_psi: %f", time_of_next_psi);

            auto &deltaTime = *reinterpret_cast<float *>(reinterpret_cast<CKBYTE *>(m_IpionManager) + 0xC8);
            GetLogger()->Info("LastDeltaTime: %f", deltaTime);
        }
    }
}
#endif

void TASSupport::OnStart() {
    if (!m_Enabled->GetBoolean())
        return;

    m_BML->AddTimer(1ul, [this]() {
        ResetPhysicsTime();
    });

    AcquireKeyBindings();

    if (m_Record->GetBoolean()) {
        m_NewRecord.Clear();

        m_BML->SendIngameMessage("Start recording TAS.");
        m_State |= TAS_RECORDING;
    }

    if (m_CurrentRecord && m_CurrentRecord->IsLoaded()) {
        m_CurrentRecord->ResetFrame();

        m_BML->SendIngameMessage("Start playing TAS.");
        m_State |= TAS_PLAYING;
    }
}

void TASSupport::OnStop() {
    if (!m_Enabled->GetBoolean() || IsIdle())
        return;

    if (IsRecording()) {
        m_BML->SendIngameMessage("TAS recording stopped.");
        SetupNewRecord();
        m_BML->SendIngameMessage(("TAS record saved to " + m_NewRecord.GetName()).c_str());
        std::thread([this]() {
            m_NewRecord.Save();
            m_NewRecord.Clear();
        }).detach();
    }

    if (IsPlaying()) {
        ResetKeyboardState(m_InputHook->GetKeyboardState());
        m_CurrentRecord->Clear();
        m_BML->SendIngameMessage("TAS playing stopped.");
        if (m_ExitOnFinish->GetBoolean()) {
            m_State = TAS_IDLE;
            m_BML->ExitGame();
        }
    }

    if (!m_Legacy) {
        m_BML->AddTimer(1ul, [this]() {
            SetPhysicsTimeFactor();
        });
    }

    m_State = TAS_IDLE;
}

void TASSupport::OnFinish() {
    if (m_Enabled->GetBoolean())
        OnStop();
}

void TASSupport::OnPreProcessInput() {
    if (IsPlaying()) {
        if (m_CurrentRecord->IsPlaying()) {
            auto state = m_CurrentRecord->GetFrameData().GetKeyState();
            SetKeyboardState(m_InputHook->GetKeyboardState(), state);
            m_CurrentRecord->NextFrame();
        } else {
            OnStop();
        }
    }

    if (IsRecording()) {
        auto state = GetKeyboardState(m_InputHook->GetKeyboardState());
        m_NewRecord.GetFrameData().SetKeyState(state);
    }
}

void TASSupport::OnPreProcessTime() {
    if (IsPlaying()) {
        if (m_CurrentRecord->IsPlaying()) {
            float delta = m_CurrentRecord->GetFrameData().GetDeltaTime();
            m_TimeManager->SetLastDeltaTime(delta);
        } else {
            OnStop();
        }
    }

    if (IsRecording()) {
        m_NewRecord.NewFrame(FrameData(m_TimeManager->GetLastDeltaTime()));
    }
}

void TASSupport::OnDrawMenu() {
    const ImVec2 &vpSize = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowPos(ImVec2(vpSize.x * 0.3f, 0.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(vpSize.x * 0.7f, vpSize.y), ImGuiCond_Appearing);

    constexpr auto TitleText = "TAS Records";

    constexpr ImGuiWindowFlags MenuWinFlags = ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoBackground |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoScrollWithMouse |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus |
                                              ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin(TitleText, nullptr, MenuWinFlags);

    {
        float oldScale = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale *= 1.5f;
        ImGui::PushFont(ImGui::GetFont());

        const auto titleSize = ImGui::CalcTextSize(TitleText);
        ImGui::GetWindowDrawList()->AddText(ImVec2((vpSize.x - titleSize.x) / 2.0f, vpSize.y * 0.07f), IM_COL32_WHITE,
                                            TitleText);

        ImGui::GetFont()->Scale = oldScale;
        ImGui::PopFont();
    }

    int recordCount = (int) m_Records.size();
    int maxPage = ((recordCount % 13) == 0) ? recordCount / 13 : recordCount / 13 + 1;

    if (m_CurrentPage > 0) {
        ImGui::SetCursorScreenPos(Bui::CoordToScreenPos(ImVec2(0.34f, 0.4f)));
        if (Bui::LeftButton("TASPrevPage")) {
            --m_CurrentPage;
        }
    }

    if (maxPage > 1 && m_CurrentPage < maxPage - 1) {
        ImGui::SetCursorScreenPos(ImVec2(vpSize.x * 0.6238f, vpSize.y * 0.4f));
        if (Bui::RightButton("TASNextPage")) {
            ++m_CurrentPage;
        }
    }

    bool v = true;
    const int n = m_CurrentPage * 13;
    for (int i = 0; i < 13 && n + i < recordCount; ++i) {
        auto &record = m_Records[n + i];

        ImGui::SetCursorScreenPos(Bui::CoordToScreenPos(ImVec2(0.4031f, 0.15f + (float) i * 0.06f)));
        if (Bui::LevelButton(record.GetName().c_str(), &v)) {
            ExitTASMenu();

            m_BML->SendIngameMessage(("Loading TAS Record: " + record.GetName()).c_str());
            m_CurrentRecord = &record;
            m_CurrentRecord->Load();
        }
    }

    ImGui::SetCursorScreenPos(Bui::CoordToScreenPos(ImVec2(0.4031f, 0.85f)));
    if (Bui::BackButton("TASBack") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_CurrentPage = 0;
        ExitTASMenu();
    }

    ImGui::End();
}

void TASSupport::OnDrawKeys() {
    if (m_ShowKeys->GetBoolean() && m_CurrentRecord->IsPlaying()) {
        const ImVec2 &vpSize = ImGui::GetMainViewport()->Size;

        ImGui::SetNextWindowPos(ImVec2(vpSize.x * 0.28f, vpSize.y * 0.7f));
        ImGui::SetNextWindowSize(ImVec2(vpSize.x * 0.45f, vpSize.y * 0.15f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.57f));

        constexpr ImGuiWindowFlags WinFlags = ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoResize |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoInputs |
                                              ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("TAS Keys", nullptr, WinFlags)) {
            ImDrawList *drawList = ImGui::GetWindowDrawList();

            KeyState state = m_CurrentRecord->GetFrameData().GetKeyState();

            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.56f, 0.76f)), Bui::BUTTON_SMALL, state.key_up ? 1 : 2, "^");
            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.56f, 0.8f)), Bui::BUTTON_SMALL, state.key_down ? 1 : 2, "v");
            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.48f, 0.8f)), Bui::BUTTON_SMALL, state.key_left ? 1 : 2, "<");
            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.64f, 0.8f)), Bui::BUTTON_SMALL, state.key_right ? 1 : 2, ">");
            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.30f, 0.8f)), Bui::BUTTON_SMALL, state.key_shift ? 1 : 2, "Shift");
            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.38f, 0.8f)), Bui::BUTTON_SMALL, state.key_space ? 1 : 2, "Space");
            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.38f, 0.76f)), Bui::BUTTON_SMALL, state.key_q ? 1 : 2, "Q");
            Bui::AddButtonImage(drawList, Bui::CoordToScreenPos(ImVec2(0.30f, 0.76f)), Bui::BUTTON_SMALL, state.key_esc ? 1 : 2, "ESC");

            sprintf(m_FrameCountText, "#%d", m_CurrentRecord->GetFrameIndex());
            const auto textSize = ImGui::CalcTextSize(m_FrameCountText);
            drawList->AddText(ImVec2((vpSize.x - textSize.x) / 2.0f, vpSize.y * 0.7f), IM_COL32_WHITE, m_FrameCountText);
        }
        ImGui::End();

        ImGui::PopStyleColor();
    }
}

void TASSupport::OnDrawInfo() {
    if (!m_ShowInfo->GetBoolean())
        return;

    constexpr ImGuiWindowFlags WinFlags = ImGuiWindowFlags_AlwaysAutoResize |
                                          ImGuiWindowFlags_NoDecoration |
                                          ImGuiWindowFlags_NoNav |
                                          ImGuiWindowFlags_NoFocusOnAppearing |
                                          ImGuiWindowFlags_NoBringToFrontOnFocus |
                                          ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Info", nullptr, WinFlags)) {
        auto *ball = GetActiveBall();
        if (ball) {
            ImGui::Text("Active Ball: %s", ball->GetName());

            VxVector position, angles;
            VxVector velocity, angularVelocity;

            if (m_PhysicsRTVersion == 0x000001) {
                auto *obj = m_IpionManager->GetPhysicsObject0(ball);
                if (obj) {
                    obj->GetPosition(&position, &angles);
                    obj->GetVelocity(&velocity, &angularVelocity);
                }
            } else if (m_PhysicsRTVersion == 0x000002) {
                auto *obj = m_IpionManager->GetPhysicsObject(ball);
                if (obj) {
                    obj->GetPosition(&position, &angles);
                    obj->GetVelocity(&velocity, &angularVelocity);
                }
            }

            ImGui::Text("Position:");
            ImGui::Text("(%.3f, %.3f, %.3f)", position.x, position.y, position.z);
            ImGui::Text("Angles:");
            ImGui::Text("(%.3f, %.3f, %.3f)", angles.x, angles.y, angles.z);

            ImGui::Text("Velocity:");
            ImGui::Text("(%.3f, %.3f, %.3f)", velocity.x, velocity.y, velocity.z);
            ImGui::Text("Angular Velocity:");
            ImGui::Text("(%.3f, %.3f, %.3f)", angularVelocity.x, angularVelocity.y, angularVelocity.z);
        }
    }
    ImGui::End();
}

void TASSupport::InitHooks() {
    if (m_Hooked)
        return;

    if (!m_Legacy) {
        void *base = GetModuleBaseAddress("physics_RT.dll");
        if (!base) {
            GetLogger()->Error("Failed to get physics_RT.dll base address");
            return;
        }

        ivp_rand = ForceReinterpretCast<decltype(ivp_rand)>(base, 0x2FCD0);
        qh_rand = ForceReinterpretCast<decltype(qh_rand)>(base, 0x52F50);

        if (MH_CreateHook(*reinterpret_cast<LPVOID *>(&ivp_rand),
                          reinterpret_cast<LPVOID>(IVP_Rand),
                          reinterpret_cast<LPVOID *>(&ivp_rand_orig)) != MH_OK ||
            MH_EnableHook(*reinterpret_cast<LPVOID *>(&ivp_rand)) != MH_OK) {
            GetLogger()->Error("Failed to hook ivp_rand");
            return;
        }

        if (MH_CreateHook(*reinterpret_cast<LPVOID *>(&qh_rand),
                          reinterpret_cast<LPVOID>(QH_Rand),
                          reinterpret_cast<LPVOID *>(&qh_rand_orig)) != MH_OK ||
            MH_EnableHook(*reinterpret_cast<LPVOID *>(&qh_rand)) != MH_OK) {
            GetLogger()->Error("Failed to hook qh_rand");
            return;
        }
    }

    void **vtableTimeManager = reinterpret_cast<void **>(*reinterpret_cast<void **>(m_TimeManager));
    TimeManagerHook::s_PreProcessFuncTarget = *reinterpret_cast<PreProcessFunc *>(&vtableTimeManager[5]);
    if (MH_CreateHook(*reinterpret_cast<LPVOID *>(&TimeManagerHook::s_PreProcessFuncTarget),
                      *reinterpret_cast<LPVOID *>(&TimeManagerHook::s_PreProcessFunc),
                      reinterpret_cast<LPVOID *>(&TimeManagerHook::s_PreProcessFuncOrig)) != MH_OK ||
        MH_EnableHook(*reinterpret_cast<LPVOID *>(&TimeManagerHook::s_PreProcessFuncTarget)) != MH_OK) {
        GetLogger()->Error("Failed to hook Time Manager");
        return;
    }

    auto *inputManager = (CKInputManager *) m_BML->GetCKContext()->GetManagerByGuid(INPUT_MANAGER_GUID);
    void **vtableInputManager = reinterpret_cast<void **>(*reinterpret_cast<void **>(inputManager));
    InputManagerHook::s_PreProcessFuncTarget = *reinterpret_cast<PreProcessFunc *>(&vtableInputManager[5]);
    if (MH_CreateHook(*reinterpret_cast<LPVOID *>(&InputManagerHook::s_PreProcessFuncTarget),
                      *reinterpret_cast<LPVOID *>(&InputManagerHook::s_PreProcessFunc),
                      reinterpret_cast<LPVOID *>(&InputManagerHook::s_PreProcessFuncOrig)) != MH_OK ||
        MH_EnableHook(*reinterpret_cast<LPVOID *>(&InputManagerHook::s_PreProcessFuncTarget)) != MH_OK) {
        GetLogger()->Error("Failed to hook Input Manager");
    }
}

void TASSupport::ShutdownHooks() {
    if (!m_Hooked)
        return;

    if (!m_Legacy) {
        MH_DisableHook(*reinterpret_cast<LPVOID *>(&ivp_rand));
        MH_RemoveHook(*reinterpret_cast<LPVOID *>(&ivp_rand));

        MH_DisableHook(*reinterpret_cast<LPVOID *>(&qh_rand));
        MH_RemoveHook(*reinterpret_cast<LPVOID *>(&qh_rand));
    }

    MH_DisableHook(*reinterpret_cast<void **>(&TimeManagerHook::s_PreProcessFuncTarget));
    MH_RemoveHook(*reinterpret_cast<void **>(&TimeManagerHook::s_PreProcessFuncTarget));

    MH_DisableHook(*reinterpret_cast<void **>(&InputManagerHook::s_PreProcessFuncTarget));
    MH_RemoveHook(*reinterpret_cast<void **>(&InputManagerHook::s_PreProcessFuncTarget));
}

void TASSupport::AcquireKeyBindings() {
    if (m_Keyboard) {
        m_Keyboard->GetElementValue(0, 0, &m_KeyUp);
        m_Keyboard->GetElementValue(0, 1, &m_KeyDown);
        m_Keyboard->GetElementValue(0, 2, &m_KeyLeft);
        m_Keyboard->GetElementValue(0, 3, &m_KeyRight);
        m_Keyboard->GetElementValue(0, 4, &m_KeyShift);
        m_Keyboard->GetElementValue(0, 5, &m_KeySpace);
    }
}

void TASSupport::ResetPhysicsTime() {
    // Reset physics time in order to sync with TAS records
    if (m_PhysicsRTVersion == 0x000001) {
        // IVP_Environment
        auto *env = *reinterpret_cast<CKBYTE **>(reinterpret_cast<CKBYTE *>(m_IpionManager) + 0xC0);

        auto &base_time = *reinterpret_cast<double *>(*reinterpret_cast<CKBYTE **>(env + 0x4) + 0x18);
#ifdef _DEBUG
        GetLogger()->Info("time_manager->base_time: %f", base_time);
#endif
        base_time = 0;

        auto &current_time = *reinterpret_cast<double *>(env + 0x120);
#ifdef _DEBUG
        GetLogger()->Info("current_time: %f", current_time);
#endif
        current_time = 0;

        auto &time_of_last_psi = *reinterpret_cast<double *>(env + 0x130);
#ifdef _DEBUG
        GetLogger()->Info("time_of_last_psi: %f", time_of_last_psi);
#endif
        time_of_last_psi = 0;

        auto &time_of_next_psi = *reinterpret_cast<double *>(env + 0x128);
#ifdef _DEBUG
        GetLogger()->Info("time_of_next_psi: %f", time_of_next_psi);
#endif
        time_of_next_psi = time_of_last_psi + 1.0 / 66;

        // auto &current_time_code = *reinterpret_cast<double *>(env + 0x138);
        // GetLogger()->Info("current_time_code: %f", current_time_code);
        // current_time_code = 1;

        auto &deltaTime = *reinterpret_cast<float *>(reinterpret_cast<CKBYTE *>(m_IpionManager) + 0xC8);
        deltaTime = m_TimeManager->GetLastDeltaTime();
    } else if (m_PhysicsRTVersion == 0x000002) {
        m_IpionManager->ResetSimulationClock();
        m_IpionManager->SetDeltaTime(m_TimeManager->GetLastDeltaTime());
    }
}

void TASSupport::SetPhysicsTimeFactor(float factor) {
    // Reset physics time factor in case it was changed
    if (m_PhysicsRTVersion == 0x000001) {
        auto &physicsTimeFactor = *reinterpret_cast<float *>(reinterpret_cast<CKBYTE *>(m_IpionManager) + 0xD0);
        physicsTimeFactor = factor * 0.001f;
    } else if (m_PhysicsRTVersion == 0x000002) {
        m_IpionManager->SetTimeFactor(1);
    }
}

void TASSupport::SetupNewRecord() {
    char filename[MAX_PATH];
    time_t stamp = time(nullptr);
    tm *curTime = localtime(&stamp);
    sprintf(filename, "%s_%04d%02d%02d_%02d%02d%02d.tas", m_MapName.c_str(), curTime->tm_year + 1900,
            curTime->tm_mon + 1, curTime->tm_mday, curTime->tm_hour, curTime->tm_min, curTime->tm_sec);

    char filepath[MAX_PATH];
    sprintf(filepath, BML_TAS_PATH "%s", filename);

    m_NewRecord.SetName(filename);
    m_NewRecord.SetPath(filepath);
}

void TASSupport::RefreshRecords() {
    m_Records.clear();

    CKDirectoryParser tasTraverser((CKSTRING) BML_TAS_PATH, "*.tas", TRUE);
    for (char *tasPath = tasTraverser.GetNextFile(); tasPath != nullptr; tasPath = tasTraverser.GetNextFile()) {
        std::string tasFile = tasPath;
        auto start = tasFile.find_last_of('\\') + 1;
        auto count = tasFile.find_last_of('.') - start;
        std::string name = tasFile.substr(start, count);
        std::string path = BML_TAS_PATH + name + ".tas";
        m_Records.emplace_back(name, path);
    }

    std::sort(m_Records.begin(), m_Records.end());
}

void TASSupport::OpenTASMenu() {
    m_ShowMenu = true;
    m_InputHook->Block(CK_INPUT_DEVICE_KEYBOARD);

    RefreshRecords();
}

void TASSupport::ExitTASMenu() {
    m_ShowMenu = false;

    CKBehavior *beh = m_BML->GetScriptByName("Menu_Start");
    m_BML->GetCKContext()->GetCurrentScene()->Activate(beh, true);

    m_BML->AddTimerLoop(1ul, [this] {
        if (m_InputHook->oIsKeyDown(CKKEY_ESCAPE) || m_InputHook->oIsKeyDown(CKKEY_RETURN))
            return true;
        m_InputHook->Unblock(CK_INPUT_DEVICE_KEYBOARD);
        return false;
    });
}

KeyState TASSupport::GetKeyboardState(const unsigned char *src) const {
    KeyState state = {};
    if (src) {
        state.key_up = src[m_KeyUp];
        state.key_down = src[m_KeyDown];
        state.key_left = src[m_KeyLeft];
        state.key_right = src[m_KeyRight];
        state.key_q = src[CKKEY_Q];
        state.key_shift = src[m_KeyShift];
        state.key_space = src[m_KeySpace];
        state.key_esc = src[CKKEY_ESCAPE];
        state.key_enter = src[CKKEY_RETURN];
    }

    return state;
}

void TASSupport::SetKeyboardState(unsigned char *dest, const KeyState &state) const {
    dest[m_KeyUp] = state.key_up;
    dest[m_KeyDown] = state.key_down;
    dest[m_KeyLeft] = state.key_left;
    dest[m_KeyRight] = state.key_right;
    dest[CKKEY_Q] = state.key_q;
    dest[m_KeyShift] = state.key_shift;
    dest[m_KeySpace] = state.key_space;
    dest[CKKEY_ESCAPE] = state.key_esc;
    dest[CKKEY_RETURN] = state.key_enter;
}

void TASSupport::ResetKeyboardState(unsigned char *dest) const {
    if (dest) {
        dest[m_KeyUp] = KS_IDLE;
        dest[m_KeyDown] = KS_IDLE;
        dest[m_KeyLeft] = KS_IDLE;
        dest[m_KeyRight] = KS_IDLE;
        dest[CKKEY_Q] = KS_IDLE;
        dest[m_KeyShift] = KS_IDLE;
        dest[m_KeySpace] = KS_IDLE;
        dest[CKKEY_ESCAPE] = KS_IDLE;
    }
}

CK3dEntity *TASSupport::GetActiveBall() const {
    if (m_ActiveBall)
        return (CK3dEntity *) m_ActiveBall->GetValueObject();
    return nullptr;
}
