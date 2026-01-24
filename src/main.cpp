#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/hook/a64/Assembler.h"

#include <Library/Placement/PlacementFunction.h>
#include <Library/LiveActor/ActorInitUtil.h>

namespace al {
void initActorImpl(LiveActor* actor, const ActorInitInfo& initInfo, const sead::SafeString& folderName, const sead::SafeString& fileName, const char* suffix);
}

void newInitActor(al::LiveActor* actor, const al::ActorInitInfo& initInfo, const sead::SafeString& archiveName, const char* suffix) {
    const char* file = archiveName.cstr();
    alPlacementFunction::tryGetModelName(&file, initInfo);
    al::initActorImpl(actor, initInfo, "ObjectData", file, suffix);
}

extern "C" void hkMain() {
    hk::hook::writeBranchLinkAtSym<"$grow_plant_seed_init">(newInitActor);
    // hk::hook::writeBranchAtSym<"_ZN2al24initActorWithArchiveNameEPNS_9LiveActorERKNS_13ActorInitInfoERKN4sead14SafeStringBaseIcEEPKc">(newInitActor);
}
