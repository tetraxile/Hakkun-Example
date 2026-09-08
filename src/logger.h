#pragma once

#include <hk/container/Span.h>
#include <hk/container/StringView.h>
#include <nn/socket.h>

class Logger {
public:
    static void init(const char* serverIPStr, const u32 port);
    static void log(const char* fmt, ...);
    static void log(hk::StringView message);
    static s32 sendUDPDatagram(hk::Span<const u8> data);
    static bool ready() { return sInstance.isInitialised; };

private:
    static Logger sInstance;

    s32 sockfd = -1;
    sockaddr_in serverAddr;
    bool isInitialised = false;
};
