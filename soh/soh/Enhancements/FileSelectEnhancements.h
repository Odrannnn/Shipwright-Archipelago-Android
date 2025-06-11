#ifndef FILE_SELECT_ENHANCEMENTS_H
#define FILE_SELECT_ENHANCEMENTS_H

#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif
const char* SohFileSelect_GetRandomizerSettingText(u8 optionIndex, u8 language);
const char* SohFileSelect_GetArchipelagoSettingText(u8 optionIndex, u8 language);
#ifdef __cplusplus
};
#endif

typedef enum {
    RSM_START_RANDOMIZER,
    RSM_GENERATE_RANDOMIZER,
    RSM_OPEN_RANDOMIZER_SETTINGS,
    RSM_GENERATING,
    RSM_NO_RANDOMIZER_GENERATED,
    RSM_MAX,
} RandomizerSettingsMenuEnums;

typedef enum {
    ASM_CHANGE_SETTINGS,
    ASM_CONNECT,
    ASM_CONNECTING,
    ASM_CONNECTION_ERROR,
    ASM_SERVER_ADDRESS,
    ASM_SLOT_NAME,
    ASM_MAX
} ArchipelagoSettingsMenuEnums;

#endif
