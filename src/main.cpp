#include <hk/container/FixedString.h>
#include <hk/hook/Trampoline.h>

#include <cstring>

#include <nn/account.h>
#include <nn/fs.h>
#include <sead/filedevice/nin/seadNinSaveFileDeviceNin.h>
#include <sead/filedevice/seadFileDevice.h>
#include <sead/gfx/seadColor.h>
#include <sead/heap/seadExpHeap.h>
#include <sead/heap/seadHeapMgr.h>

#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/LiveActor.h"
#include "Library/Nerve/NerveUtil.h"
#include "Project/SaveData/SaveDataSequenceRead.h"
#include "Project/SaveData/SaveDataSequenceWrite.h"

#include "MapObj/CapMessageShowInfo.h"
#include "Util/PlayerHackFunction.h"

namespace al {
    bool calcFindFluidSurface(sead::Vector3f* surfacePos, sead::Vector3f* surfaceNormal, const al::LiveActor* actor, const sead::Vector3f& trans, const sead::Vector3f& gravity, const char* type, bool isFlat, bool isDisplacement, f32 radius);
}

static void initHeap() {
    nn::fs::MountSdCardForDebug("sd");
}

HkTrampoline motorcycleCollider = [](TrampolineStatic(), al::LiveActor* actor) -> void {
    orig(actor);
    rs::trySnapTransToSnapMoveArea(actor, true, { 0.0f, 1.0f, 0.0f }, 100.0f);
};

bool calcFindWaterSurfaceFlat(sead::Vector3f* surfacePos, sead::Vector3f* surfaceNormal,
    const al::LiveActor* actor, const sead::Vector3f& trans,
    const sead::Vector3f& gravity, f32 radius) {
    if (!surfacePos || !surfaceNormal)
        return false;
    return al::calcFindFluidSurface(surfacePos, surfaceNormal, actor, trans, gravity, "Water", true, false, radius);
}

void getUsername(hk::FixedString<0x20>* out) {
    char username[0x20] = "default";

    nn::account::Uid uid;
    nn::account::Nickname nickname;
    if (nn::account::GetLastOpenedUser(&uid).IsSuccess() && nn::account::GetNickname(&nickname, uid).IsSuccess()) {
        strncpy(username, nickname.m_Buffer, sizeof(username));
    }

    for (s32 i = 0; i < sizeof(username); i++) {
        const char* disallowedChars = "<>*?:|/\\";
        for (s32 j = 0; j < strlen(disallowedChars); j++)
            if (username[i] == disallowedChars[j])
                username[i] = '_';
    }

    *out = username;
}

HkTrampoline saveReadHook = [](TrampolineStatic(), void* thisPtr, sead::FileDevice* device, const char* name, void* flags) -> s32 {
    hk::FixedString<0x20> username;
    getUsername(&username);

    hk::FixedString<0x40> path("smo/floaty/save/");
    path.append(username.cstr());
    path.append("/");
    path.append(name);

    sead::NinSaveFileDevice newDevice("sd");
    return orig(thisPtr, &newDevice, path.cstr(), flags);
};

HkTrampoline saveWriteHook = [](TrampolineStatic(), void* thisPtr, sead::FileDevice* device, const char* filename) -> s32 {
    hk::FixedString<0x20> username;
    getUsername(&username);

    hk::FixedString<0x40> sdDirPath("sd:/smo/floaty/save/");
    sdDirPath.append(username.cstr());
    sdDirPath.append("/");

    hk::FixedString<0x40> path("smo/floaty/save/");
    path.append(username.cstr());
    path.append("/");
    path.append(filename);

    hk::FixedString<0x40> sdPath("sd:/");
    sdPath.append(path.cstr());

    nn::fs::DirectoryEntryType type;
    if (nn::fs::GetEntryType(&type, sdDirPath).IsFailure() || type != nn::fs::DirectoryEntryType_Directory) {
        nn::fs::CreateDirectory("sd:/smo/");
        nn::fs::CreateDirectory("sd:/smo/floaty/");
        nn::fs::CreateDirectory("sd:/smo/floaty/save/");
        nn::fs::CreateDirectory(sdDirPath.cstr());
    }

    sead::NinSaveFileDevice newDevice("sd");
    return orig(thisPtr, &newDevice, path.cstr());
};

HkTrampoline jumpingRopeNpcInitHook = [](TrampolineStatic(), al::LiveActor* thisPtr, const al::ActorInitInfo& initInfo) -> void {
    orig(thisPtr, initInfo);
    *(u32*)((ptr)thisPtr + 0x200) = 50;
    *(u32*)((ptr)thisPtr + 0x204) = 150;
};

extern "C" void hkMain() {
    hk::hook::writeBranchAtSym<"$heap_create_hook">(initHeap);

    // make motorcycle interact with SnapMoveArea
    motorcycleCollider.installAtSym<"_ZN10Motorcycle14updateColliderEv">();

    // fix motorcycle crashing when it lands in water
    hk::hook::writeBranchLinkAtSym<"$water_motorcycle">(calcFindWaterSurfaceFlat);

    // change number of jumps required in jump rope
    jumpingRopeNpcInitHook.installAtSym<"_ZN14JumpingRopeNpc4initERKN2al13ActorInitInfoE">();

    // saveReadHook.installAtPtr(&al::SaveDataSequenceRead::read);
    // saveWriteHook.installAtPtr(&al::SaveDataSequenceWrite::write);
}
