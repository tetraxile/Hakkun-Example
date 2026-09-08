#pragma once

#include <Library/Nerve/NerveExecutor.h>

class GameDataHolder {
public:
    void setPlayingFileId(s32 fileId);
    void readFromSaveDataBuffer();
};

class ProductSequence : public al::NerveExecutor {
public:
    char filler[0xb0];
    GameDataHolder* gameDataHolder;
    char filler1[0xe8];
};

static_assert(sizeof(ProductSequence) == 0x1b0);
