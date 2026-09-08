#include <heap/seadHeapMgr.h>
#include <hk/gfx/DebugRenderer.h>
#include <hk/hook/Trampoline.h>
#include <nn/fs.h>
#include <nn/nifm.h>
#include <nn/oe.h>
#include <resource/seadResourceMgr.h>

#include <Library/Nerve/Nerve.h>
#include <Library/Nerve/NerveUtil.h>

#include "game/ProductSequence.h"

#include "hk/diag/ipclogger.h"
#include "logger.h"

namespace al {
    struct SequenceInitInfo;
}

namespace rc {
    class AssetLoadingThread {
    public:
        static AssetLoadingThread* sInstance;

        void cancelAndDelete3DWorldStationed();
        void disableFastLoad(bool);
        void startLoad(s32 loadType);
        bool isAllLoadingDone() const;
        bool waitLoadDone(s32 loadType);
        void waitAllLoadingDone() {
            while (!isAllLoadingDone())
                hk::svc::SleepThread(5'000'000);
        }
    };

    // static_assert(sizeof(AssetLoadingThread) == 0x68);
}

namespace al {
    struct GameFrameworkNx {
        char filler[0x27a];
        bool mrBool;
    };

    class SaveDataSequenceInitDir {
    public:
        static s32 threadFunc(const char* filename);
    };
}

class Application {
public:
    static Application* sInstance;

    char filler[0x28];
    al::GameFrameworkNx* framework;
};

typedef GameDataHolder* GameDataHolderWriter;

namespace PlayerEntryFunction {
    void entryPlayer(GameDataHolderWriter, s32, s32);
}

namespace SaveDataAccessFunction {
    void startSaveDataRead(GameDataHolder*, bool);
    void startSaveDataReadSync(GameDataHolder*);
}

namespace al {
    class MemorySystem;
    class ResourceSystem;

    class SaveDataSequenceRead {
    public:
        s32 threadFunc(const char*);
        void start(u8*, u32, u32);
    };

    class SaveDataDirector {
    public:
        char filler[0x18];
        SaveDataSequenceRead* sequenceRead;
        char filler1[0x18];
        u8* data;
    };

    class SystemKit {
    public:
        MemorySystem* memorySystem;
        void* _8;
        ResourceSystem* resourceSystem;
        SaveDataDirector* saveDataDirector;
        void* _20;
        sead::SafeString _28;
        sead::SafeString _38;
    };
}

namespace alProjectInterface {
    al::SystemKit* getSystemKit();
}

HkTrampoline productSequenceInitHook = [](TrampolineStatic(), ProductSequence* thisPtr, const al::SequenceInitInfo& initInfo) -> void {
    orig(thisPtr, initInfo);

    // set nerve to exeCourseSelect
    const al::Nerve* nerve = cast<const al::Nerve*>(hk::ro::getMainModule()->range().start() + 0x138cf98);
    al::setNerve(thisPtr, nerve);

    // make sure all of the async asset loading is done before trying to continue
    rc::AssetLoadingThread::sInstance->cancelAndDelete3DWorldStationed();
    rc::AssetLoadingThread::sInstance->disableFastLoad(false);
    rc::AssetLoadingThread::sInstance->startLoad(0);
    rc::AssetLoadingThread::sInstance->startLoad(1);
    rc::AssetLoadingThread::sInstance->startLoad(2);
    rc::AssetLoadingThread::sInstance->waitAllLoadingDone();
    Application::sInstance->framework->mrBool = true;
    nn::oe::FinishStartupLogo();

    // mount the save data filesystem
    al::SaveDataSequenceInitDir::threadFunc(nullptr);

    // read the save file and load it into the buffer, because this is normally done by one of the other nerves
    nn::fs::FileHandle saveFile;
    HK_ABORT_UNLESS_R(nn::fs::OpenFile(&saveFile, "save:/GameData.bin", nn::fs::OpenMode_Read).GetInnerValueForDebug());
    al::SaveDataDirector* saveDataDirector = alProjectInterface::getSystemKit()->saveDataDirector;
    HK_ABORT_UNLESS_R(nn::fs::ReadFile(saveFile, 0, saveDataDirector->data, 0x1000c).GetInnerValueForDebug());
    nn::fs::CloseFile(saveFile);
    thisPtr->gameDataHolder->readFromSaveDataBuffer();

    // force the first save file to be loaded by default
    thisPtr->gameDataHolder->setPlayingFileId(0);

    // force the player to be mario
    PlayerEntryFunction::entryPlayer(thisPtr->gameDataHolder, 0, 0);
};

HkTrampoline startSeHook = [](TrampolineStatic(), void* ptr, const sead::SafeString& name, void* ptr1) -> void {
    hk::diag::logLine("startSe: %s", name.cstr());
    orig(ptr, name, ptr1);
};

HkTrampoline tryStartSeHook = [](TrampolineStatic(), void* ptr, const sead::SafeString& name, void* ptr1) -> void {
    hk::diag::logLine("tryStartSe: %s", name.cstr());
    orig(ptr, name, ptr1);
};

HkTrampoline tryLoadHook = [](TrampolineStatic(), sead::ResourceMgr* thisPtr, const sead::ResourceMgr::LoadArg& loadArg, const sead::SafeString& str, void* decompressor) -> void {
    sead::Heap* heap = sead::HeapMgr::instance()->getCurrentHeap();
    if (!heap)
        heap = loadArg.instance_heap;
    hk::diag::logLine("load: %-50s | heap: %30s %10zx/%10zx", loadArg.path.cstr(), heap->getName().cstr(), heap->getFreeSize(), heap->getSize());
    orig(thisPtr, loadArg, str, decompressor);
};

HkTrampoline startSeSetSeqHook = [](TrampolineStatic(), void* p1, const sead::SafeString& name, s32 p3, u16 p4) -> void {
    hk::diag::logLine("startSeSetSeq: %s", name.cstr());
    orig(p1, name, p3, p4);
};

HkTrampoline requestPlaySeFromName1Hook = [](TrampolineStatic(), void* thisPtr, const char* name, void* ptr, bool b1) -> void {
    hk::diag::logLine("requestPlaySeFromName: %s", name);
    orig(thisPtr, name, ptr, b1);
};

HkTrampoline requestPlaySeHook = [](TrampolineStatic(), void* thisPtr, const char* name, void* p2, bool p3) -> void {
    hk::diag::logLine("requestPlaySe: %s", name);
    orig(thisPtr, name, p2, p3);
};

namespace al {
    template <typename T>
    struct AudioInfoList {
        sead::PtrArray<T> array;
        AudioInfoList<T>* next;

        T* tryFindInfo(const char* name) const;
        s32 getInfoNum() const;
    };

    struct SeResourceInfo { };

    struct SePlayInfo {
        const char* name;
        bool isLoop;
        s32 fadeOutFrameNum;
        AudioInfoList<SeResourceInfo>* resourceInfoList;
        const char* requestKeeperName;
    };
};

HkTrampoline tryFindInfoHook = [](TrampolineStatic(), void* thisPtr, const char* name) -> al::SePlayInfo* {
    al::SePlayInfo* out = orig(thisPtr, name);
    hk::diag::logLine("%c tryFindInfo: %s (%d)", out == nullptr ? 'X' : ' ', name, out->resourceInfoList->getInfoNum());
    return out;
};

extern "C" void hkMain() {
    hk::diag::ipclogger::IpcLogger::initialize();

    Logger::init("192.168.178.114", 3085);
    hk::diag::logLine("mod started");

    productSequenceInitHook.installAtSym<"_ZN15ProductSequence4initERKN2al16SequenceInitInfoE">();
    startSeHook.installAtSym<"_ZN2al7startSeEPKNS_15IUseAudioKeeperERKN4sead14SafeStringBaseIcEEPNS_6MeInfoE">();
    tryLoadHook.installAtSym<"_ZN4sead11ResourceMgr7tryLoadERKNS0_7LoadArgERKNS_14SafeStringBaseIcEEPNS_12DecompressorE">();
    requestPlaySeHook.installAtSym<"_ZN2al8SeKeeper13requestPlaySeEPKcPNS_6MeInfoEb">();
    tryStartSeHook.installAtSym<"_ZN2al10tryStartSeEPKNS_15IUseAudioKeeperERKN4sead14SafeStringBaseIcEEPNS_6MeInfoE">();
    startSeSetSeqHook.installAtSym<"_ZN2al27startSeSetSeqLoacalVariableEPKNS_15IUseAudioKeeperERKN4sead14SafeStringBaseIcEEis">();
    requestPlaySeFromName1Hook.installAtSym<"_ZN2al8SeKeeper33requestPlaySeFromNameGetParamListEPKcPNS_6MeInfoEb">();
    tryFindInfoHook.installAtSym<"_ZNK2al13AudioInfoListINS_10SePlayInfoEE11tryFindInfoEPKc">();
}
