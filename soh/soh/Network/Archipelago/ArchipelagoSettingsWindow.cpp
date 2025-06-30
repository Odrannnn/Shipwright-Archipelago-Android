#include "ArchipelagoSettingsWindow.h"
#include "Archipelago.h"

#include "soh/SohGui/UIWidgets.hpp"
#include "soh/SohGui/SohGui.hpp"
#include "soh/Network/Archipelago/ArchipelagoConsoleWindow.h"
#include "soh/SaveManager.h"

void ArchipelagoSettingsWindow::DrawElement() {
    ArchipelagoClient& AP_client = ArchipelagoClient::GetInstance();

    ImGui::SeparatorText("Connection info");

    UIWidgets::PushStyleCombobox(THEME_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Border, UIWidgets::ColorValues.at(THEME_COLOR));

    ImGui::Text("Server Address");
    UIWidgets::CVarInputString("##ArchipelagoServerAddress", CVAR_REMOTE_ARCHIPELAGO("ServerAddress"),
                               UIWidgets::InputOptions()
                                   .Color(THEME_COLOR)
                                   .PlaceholderText("archipelago.gg:38281")
                                   .DefaultValue("archipelago.gg:38281")
                                   .Size(ImVec2(ImGui::GetFontSize() * 15, 0))
                                   .LabelPosition(UIWidgets::LabelPositions::None));
    ImGui::Text("Slot Name");
    UIWidgets::CVarInputString("##ArchipelagoSlotName", CVAR_REMOTE_ARCHIPELAGO("SlotName"),
                               UIWidgets::InputOptions()
                                   .Color(THEME_COLOR)
                                   .Size(ImVec2(ImGui::GetFontSize() * 15, 0))
                                   .LabelPosition(UIWidgets::LabelPositions::None));
    ImGui::Text("Password (leave blank for no password)");
    UIWidgets::CVarInputString("##ArchipelagoPassword", CVAR_REMOTE_ARCHIPELAGO("Password"),
                               UIWidgets::InputOptions()
                                   .Color(THEME_COLOR)
                                   .Size(ImVec2(ImGui::GetFontSize() * 15, 0))
                                   .LabelPosition(UIWidgets::LabelPositions::None));
    ImGui::PopStyleColor();
    UIWidgets::PopStyleCombobox();

    if (UIWidgets::Button("Connect", UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.0, 0.0)))) {
        bool success = AP_client.StartClient();
        ArchipelagoConsole_SendMessage("[LOG] Trying to connect...", true);
    }

    ImGui::SameLine();
    ImGui::Text(ArchipelagoClient::GetInstance().GetConnectionStatus());

    UIWidgets::CVarCheckbox(
        "Debug Enabled", CVAR_REMOTE_ARCHIPELAGO("DebugEnabled"),
        UIWidgets::CheckboxOptions().Color(THEME_COLOR).Tooltip("Enable Archipelago debug tools and extra logging."));

    // Temporary developer helpers
    UIWidgets::Separator();
    if (CVarGetInteger(CVAR_REMOTE_ARCHIPELAGO("DebugEnabled"), 0)) {
        ImGui::SeparatorText("Developer Tools");
        if (UIWidgets::Button("Scout", UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.0, 0.0)))) {
            AP_client.StartLocationScouts();
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Link up", UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.0, 0.0)))) {
            CVarSetInteger(CVAR_REMOTE_ARCHIPELAGO("Connected"), 1);
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Give Blue Rupee",
                              UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.0, 0.0)))) {
            ArchipelagoClient::ApItem apItem;
            apItem.itemName = "Blue Rupee";
            apItem.locationName = "Nowhere";
            apItem.playerName = "Nobody";
            apItem.flags = 0b001;
            apItem.index = 999999;
            ArchipelagoClient::GetInstance().OnItemReceived(apItem);
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Send Game Won", UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.0, 0.0)))) {
            ArchipelagoClient::GetInstance().SendGameWon();
        }
        if (UIWidgets::Button("Get Mido br chest",
                              UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.0, 0.0)))) {
            ArchipelagoClient::GetInstance().QueueExternalCheck(16711707);
        }
    }

    static bool sArchipelagoTexturesLoaded = false;
    if (!sArchipelagoTexturesLoaded) {
        Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadTextureFromRawImage(
            "Archipelago Progressive Icon", "textures/parameter_static/gArchipelagoProgressive.png");
        Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadTextureFromRawImage(
            "Archipelago Useful Icon", "textures/parameter_static/gArchipelagoUseful.png");
        Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadTextureFromRawImage(
            "Archipelago Junk Icon", "textures/parameter_static/gArchipelagoJunk.png");

        sArchipelagoTexturesLoaded = true;
    }
};

void ArchipelagoSettingsWindow::InitElement() {
    SaveManager::Instance->AddLoadFunction("archipelagoData", 1, LoadArchipelagoData);
    SaveManager::Instance->AddSaveFunction("archipelagoData", 1, SaveArchipelagoData, true, SECTION_PARENT_NONE);
    SaveManager::Instance->AddInitFunction(InitArchipelagoData);
}
