#include "logger.h"

#include <hk/hook/Trampoline.h>

#include <cstdio>
#include <nn/fs.h>
#include <nn/nifm.h>
#include <nn/oe.h>
#include <nn/socket.h>

HkTrampoline disableSocketInit = [](TrampolineStatic()) -> void { };

static constexpr s32 socketPoolSize = 0x600000;
static constexpr s32 socketAllocPoolSize = 0x20000;
char socketPool[socketPoolSize + socketAllocPoolSize] __attribute__((aligned(0x1000)));

void Logger::init(const char* serverIPStr, const u32 port) {
    if (sInstance.isInitialised)
        return;

    nn::socket::Initialize(socketPool, socketPoolSize, socketAllocPoolSize, 0xE);

    disableSocketInit.installAtSym<"_ZN2nn6socket6detail10InitializeEPvmmi">();

    nn::nifm::Initialize();
    nn::nifm::SubmitNetworkRequestAndWait();

    sInstance.sockfd = nn::socket::Socket(AF_INET, SOCK_DGRAM, 0);
    HK_ABORT_UNLESS(sInstance.sockfd >= 0, "failed to create UDP socket");

    sockaddr_in bindAddr;
    bindAddr.sin_family = nn::socket::InetHtons(AF_INET);
    bindAddr.sin_port = nn::socket::InetHtons(port);
    HK_ABORT_UNLESS(nn::socket::InetAton("0.0.0.0", &bindAddr.sin_addr), "failed to parse UDP bind IP");

    sInstance.serverAddr.sin_family = nn::socket::InetHtons(AF_INET);
    sInstance.serverAddr.sin_port = nn::socket::InetHtons(port);
    HK_ABORT_UNLESS(nn::socket::InetAton(serverIPStr, &sInstance.serverAddr.sin_addr), "failed to parse server IP");

    HK_ABORT_UNLESS(nn::socket::Bind(sInstance.sockfd, (sockaddr*)&bindAddr, sizeof(bindAddr)) >= 0, "failed to bind UDP socket");

    sInstance.isInitialised = true;
}

s32 Logger::sendUDPDatagram(hk::Span<const u8> data) {
    hk::Array<u8, 500> dataArr;
    memcpy(dataArr.data(), data.data(), data.size_bytes());
    s32 r = nn::socket::SendTo(sInstance.sockfd, &dataArr, data.size_bytes(), 0, (sockaddr*)&sInstance.serverAddr, sizeof(serverAddr));
    return 0;
}

void Logger::log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char message[0x400];
    size_t len = nn::util::VSNPrintf(message, sizeof(message), fmt, args);

    s32 r = sInstance.sendUDPDatagram({ (const u8*)message, len });

    va_end(args);
}

void Logger::log(hk::StringView message) {
    s32 r = sInstance.sendUDPDatagram({ (const u8*)message.data(), message.length() });
}

Logger Logger::sInstance;

extern "C" void hkLogSink(const char* msg, size len) {
    if (Logger::ready())
        Logger::sendUDPDatagram({ (const u8*)msg, len });
}
