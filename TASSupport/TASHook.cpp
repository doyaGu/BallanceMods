#include "TASHook.h"

#include <ctime>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Psapi.h>

#include "CKAll.h"
#include "BML/Guids.h"

#include "physics_RT.h"

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

// qh_RANDOMmax
static const int QH_RAND_MAX = 2147483646;

static int (*qh_rand)() = nullptr;
static int (*qh_rand_orig)() = nullptr;

int QH_Rand() { return QH_RAND_MAX; }

static int (IVP_Environment::*must_perform_movement_check)();
static int (IVP_Environment::*must_perform_movement_check_orig)();

struct IVP_Environment_Hook {
    int MustPerformMovementCheck() {
        auto &next_movement_check = *reinterpret_cast<short *>(this + 0x140);
        next_movement_check = 0;
        return 1;
    }
};

bool HookPhysicsRT() {
    void *base = GetModuleBaseAddress("physics_RT.dll");
    if (!base) {
        return false;
    }

    qh_rand = ForceReinterpretCast<decltype(qh_rand)>(base, 0x52F50);

    must_perform_movement_check = ForceReinterpretCast<decltype(must_perform_movement_check)>(base, 0x13610);

    if (MH_CreateHook(*reinterpret_cast<LPVOID *>(&qh_rand),
                      reinterpret_cast<LPVOID>(QH_Rand),
                      reinterpret_cast<LPVOID *>(&qh_rand_orig)) != MH_OK ||
        MH_EnableHook(*reinterpret_cast<LPVOID *>(&qh_rand)) != MH_OK) {
        return false;
    }

    auto detour = &IVP_Environment_Hook::MustPerformMovementCheck;
    if (MH_CreateHook(*reinterpret_cast<LPVOID *>(&must_perform_movement_check),
                      *reinterpret_cast<LPVOID *>(&detour),
                      reinterpret_cast<LPVOID *>(&must_perform_movement_check_orig)) != MH_OK ||
        MH_EnableHook(*reinterpret_cast<LPVOID *>(&must_perform_movement_check)) != MH_OK) {
        return false;
    }

    return true;
}

void UnhookPhysicsRT() {
    MH_DisableHook(*reinterpret_cast<LPVOID *>(&qh_rand));
    MH_RemoveHook(*reinterpret_cast<LPVOID *>(&qh_rand));

    MH_DisableHook(*reinterpret_cast<LPVOID *>(&must_perform_movement_check));
    MH_RemoveHook(*reinterpret_cast<LPVOID *>(&must_perform_movement_check));
}

static int g_FixedRandomValue = RAND_MAX / 2;
static CKBEHAVIORFCT g_RandomOrig = nullptr;

#undef min
#undef max

int Random(const CKBehaviorContext &behcontext) {
    CKBehavior *beh = behcontext.Behavior;

    // Set IOs States
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKParameterIn *pin;
    CKParameterOut *pout = beh->GetOutputParameter(0);
    CKGUID guid = pout->GetGUID();

#define pincheck(index)                      \
    {                                        \
        pin = beh->GetInputParameter(index); \
        if (!pin)                            \
            return CKBR_OK;                  \
        if (pin->GetGUID() != guid)          \
            return CKBR_OK;                  \
    }

    CKParameterManager *pm = behcontext.ParameterManager;
    if (pm->IsDerivedFrom(guid, CKPGUID_FLOAT)) {
        pincheck(0);
        float min;
        pin->GetValue(&min);

        pincheck(1);
        float max;
        pin->GetValue(&max);

        float res = min + g_FixedRandomValue * (max - min) / RAND_MAX;
        pout->SetValue(&res);

        return CKBR_OK;
    }

    if (guid == CKPGUID_INT) {
        pincheck(0);
        int min;
        pin->GetValue(&min);

        pincheck(1);
        int max;
        pin->GetValue(&max);

        int res = min + g_FixedRandomValue * (max - min) / RAND_MAX;
        pout->SetValue(&res);

        return CKBR_OK;
    }

    if (guid == CKPGUID_VECTOR) {
        pincheck(0);
        VxVector min(0.0f);
        pin->GetValue(&min);

        pincheck(1);
        VxVector max(0.0f);
        pin->GetValue(&max);

        VxVector res;
        res.x = min.x + g_FixedRandomValue * (max.x - min.x) / RAND_MAX;
        res.y = min.y + g_FixedRandomValue * (max.y - min.y) / RAND_MAX;
        res.z = min.z + g_FixedRandomValue * (max.z - min.z) / RAND_MAX;

        pout->SetValue(&res);

        return CKBR_OK;
    }

    if (guid == CKPGUID_2DVECTOR) {
        pincheck(0);
        Vx2DVector min;
        pin->GetValue(&min);

        pincheck(1);
        Vx2DVector max;
        pin->GetValue(&max);

        Vx2DVector res;
        res.x = min.x + g_FixedRandomValue * (max.x - min.x) / RAND_MAX;
        res.y = min.y + g_FixedRandomValue * (max.y - min.y) / RAND_MAX;

        pout->SetValue(&res);

        return CKBR_OK;
    }

    if (guid == CKPGUID_RECT) {
        pincheck(0);
        VxRect min;
        pin->GetValue(&min);

        pincheck(1);
        VxRect max;
        pin->GetValue(&max);

        VxRect res;
        res.left = min.left + g_FixedRandomValue * (max.left - min.left) / RAND_MAX;
        res.top = min.top + g_FixedRandomValue * (max.top - min.top) / RAND_MAX;
        res.right = min.right + g_FixedRandomValue * (max.right - min.right) / RAND_MAX;
        res.bottom = min.bottom + g_FixedRandomValue * (max.bottom - min.bottom) / RAND_MAX;
        res.Normalize();
        pout->SetValue(&res);

        return CKBR_OK;
    }

    if (guid == CKPGUID_BOOL) {
        CKBOOL res = g_FixedRandomValue & 1;

        pout->SetValue(&res);

        return CKBR_OK;
    }

    if (guid == CKPGUID_COLOR) {
        pincheck(0);
        VxColor min;
        pin->GetValue(&min);

        pincheck(1);
        VxColor max;
        pin->GetValue(&max);

        VxColor res;
        res.r = min.r + g_FixedRandomValue * (max.r - min.r) / RAND_MAX;
        res.g = min.g + g_FixedRandomValue * (max.g - min.g) / RAND_MAX;
        res.b = min.b + g_FixedRandomValue * (max.b - min.b) / RAND_MAX;
        res.a = min.a + g_FixedRandomValue * (max.a - min.a) / RAND_MAX;
        pout->SetValue(&res);

        return CKBR_OK;
    }

    return CKBR_OK;
}

bool HookRandom() {
    CKBehaviorPrototype *randomProto = CKGetPrototypeFromGuid(VT_LOGICS_RANDOM);
    if (!randomProto) return false;
    if (!g_RandomOrig) g_RandomOrig = randomProto->GetFunction();
    randomProto->SetFunction(&Random);
    return true;
}

bool UnhookRandom() {
    CKBehaviorPrototype *randomProto = CKGetPrototypeFromGuid(VT_LOGICS_RANDOM);
    if (!randomProto) return false;
    randomProto->SetFunction(g_RandomOrig);
    return true;
}