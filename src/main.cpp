#include "hk/hook/Trampoline.h"

HkTrampoline<void, void*> exampleHook = hk::hook::trampoline([](void*) -> void {});

extern "C" void hkMain() {}
