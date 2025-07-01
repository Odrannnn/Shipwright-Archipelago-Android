#include "FileSelectEnhancements.h"

#include "soh/OTRGlobals.h"

#include <array>
#include <string>
#include <vector>

std::array<std::string, LANGUAGE_MAX> RandomizerSettingsMenuText[RSM_MAX] = {
    {
        // English
        "Start Randomizer",
        // German
        "Randomizer starten",
        // French
        "Commencer le Randomizer",
    },
    {
        // English
        "Generate New Randomizer Seed",
        // German
        "Neuen Randomizer Seed generieren",
        // French
        "Générer une nouvelle seed pour le Randomizer",
    },
    {
        // English
        "Open Randomizer Settings",
        // German
        "Randomizer Optionen öffnen",
        // French
        "Ouvrir les paramètres du Randomizer",
    },
    {
        // English
        "Generating...",
        // German
        "Generiere...",
        // French
        "Génération en cours...",
    },
    { // English
      "No randomizer seed loaded.\nPlease generate one first"
#if defined(__WIIU__) || defined(__SWITCH__)
      ".",
#else
      ",\nor drop a spoiler log on the game window.",
#endif
      // German
      "Kein Randomizer Seed gefunden.\nBitte generiere zuerst einen"
#if defined(__WIIU__) || defined(__SWITCH__)
      ".",
#else
      ",\noder ziehe ein Spoiler Log\nauf das Spielfenster.",
#endif
      // French
      "Aucune Seed de Randomizer actuellement disponible.\nGénérez-en une dans les \"Randomizer Settings\""
#if (defined(__WIIU__) || defined(__SWITCH__))
      "."
#else
      "\nou glissez un spoilerlog sur la fenêtre du jeu."
#endif
    },
};

std::array<std::string, LANGUAGE_MAX> ArchipelagoSettingsMenuText[ASM_MAX] {
    // ASM_START_ARCHIPELAGO
    {
        "Start Archipelago",
        "Todo",
        "Todo",
    },
    // ASM_CHANGE_CONNECTION_INFO
    {
        "Change Connection Info",
        "Todo",
        "Todo",
    },
    // ASM_SERVER_ADDRESS
    {
        "Server Address: ",
        "Todo",
        "Todo",
    },
    // ASM_SLOT_NAME
    {
        "Slot Name: ",
        "Todo",
        "Todo",
    },
    // ASM_NOT_CONNECTED
    {
        "Not Connected",
        "Todo",
        "Todo",
    },
    // ASM_CONNECTING
    {
        "Connecting...",
        "Todo",
        "Todo",
    },
    // ASM_CONNECTED
    {
        "Connected",
        "Todo",
        "Todo",
    },
    // ASM_STATUS
    {
        "Status:",
        "Todo",
        "Todo",
    },
};

const char* SohFileSelect_GetRandomizerSettingText(uint8_t optionIndex, uint8_t language) {
    return RandomizerSettingsMenuText[optionIndex][language].c_str();
}

const char* SohFileSelect_GetArchipelagoSettingText(uint8_t optionIndex, uint8_t language) {
    return ArchipelagoSettingsMenuText[optionIndex][language].c_str();
}