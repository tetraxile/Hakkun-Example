#include "hk/hook/Trampoline.h"

#include <sead/math/seadMatrix.h>
#include <sead/controller/seadControllerBase.h>
#include <sead/controller/seadControllerMgr.h>

#include <al/Library/Controller/InputFunction.h>
#include <al/Library/Controller/PadReplayFunction.h>
#include <al/Library/Controller/ReplayController.h>

#include "Player/PlayerActorHakoniwa.h"
#include "Sequence/HakoniwaSequence.h"

bool gIsUseAbsoluteJoystick = false;

inline sead::ControllerBase* getController(s32 port) {
    if (port == -1)
        port = al::getMainControllerPort();

    if (al::isValidReplayController(port))
        return al::getReplayController(port);

    return sead::ControllerMgr::instance()->getController(port);
}

bool isPadTrigger(s32 port, s32 button) {
    return getController(port)->isTrig(button);
}

bool isPadTriggerPressLeftStick(s32 port = -1) {
    return isPadTrigger(port, 1 << 7);
}

HkTrampoline<const sead::Matrix34f&, PlayerActorHakoniwa*> getPlayerViewMtxHook = hk::hook::trampoline([](PlayerActorHakoniwa* player) -> const sead::Matrix34f& {
    if (gIsUseAbsoluteJoystick)
        return sead::Matrix34f::ident;
    else
        return *player->getViewMtx();
});

HkTrampoline<void, HakoniwaSequence*> hakoniwaSequenceUpdateHook = hk::hook::trampoline([](HakoniwaSequence* thiz) -> void {
    if (isPadTriggerPressLeftStick())
        gIsUseAbsoluteJoystick = !gIsUseAbsoluteJoystick;
    
    hakoniwaSequenceUpdateHook.orig(thiz);
});


extern "C" void hkMain() {
    getPlayerViewMtxHook.installAtSym<"_ZN14PlayerFunction16getPlayerViewMtxEPKN2al9LiveActorE">();
    hakoniwaSequenceUpdateHook.installAtSym<"_ZN16HakoniwaSequence6updateEv">();
}
