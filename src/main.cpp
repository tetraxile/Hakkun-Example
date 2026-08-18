#include "hk/hook/Trampoline.h"
#include "nn/fs/fs_mount.h"

#include <nn/fs.h>
#include <nn/oe.h>

class ProductSequence;
namespace al {
    struct SequenceInitInfo;
}

HkTrampoline exampleHook = [](TrampolineStatic(), ProductSequence* thisPtr, const al::SequenceInitInfo& initInfo) -> void {
    orig(thisPtr, initInfo);
    hk::diag::log("bunger bunger");
    HK_ABORT("bye");
};

extern "C" void hkMain() {
    hk::hook::a64::assemble<"add x20, x20, #0xfa8">().installAtMainOffset(0x3fcafc);
    exampleHook.installAtSym<"_ZN15ProductSequence4initERKN2al16SequenceInitInfoE">();

    nn::fs::MountSdCardForDebug("sd");

    nn::fs::FileHandle logHandle;
    nn::fs::OpenFile(&logHandle, "sd:/log.txt", nn::fs::OpenMode_Write);
    nn::fs::CloseFile(logHandle);
}
