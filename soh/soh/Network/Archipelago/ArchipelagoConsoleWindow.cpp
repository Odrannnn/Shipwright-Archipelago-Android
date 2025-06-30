#include "ArchipelagoConsoleWindow.h"

#include "soh/SohGui/UIWidgets.hpp"
#include "soh/SohGui/SohGui.hpp"
#include "soh/OTRGlobals.h"

std::vector<std::vector<ArchipelagoClient::ColoredTextNode>> Items;
bool autoScroll = true;

using namespace UIWidgets;

void ArchipelagoConsole_SendMessage(const char* fmt, bool debugMessage, ...) {
    if (debugMessage && CVarGetInteger(CVAR_REMOTE_ARCHIPELAGO("DebugEnabled"), 0) == 0) {
        return;
    }
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    ArchipelagoClient::ColoredTextNode node;
    node.text = std::string(buf);
    node.color = "white";
    if (strstr(buf, "[ERROR]")) {
        node.color = "ERROR";
    } else if (strstr(buf, "[LOG]")) {
        node.color = "LOG";
    }
    std::vector<ArchipelagoClient::ColoredTextNode> line;
    line.push_back(node);
    Items.push_back(line);
}

void ArchipelagoConsole_PrintJson(const std::vector<ArchipelagoClient::ColoredTextNode> nodes) {
    Items.push_back(nodes);
}

void ArchipelagoConsoleWindow::DrawElement() {
    ImGui::SeparatorText("Archipelago Log");

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));

    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 400), ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_HorizontalScrollbar)) {

        for (const std::vector<ArchipelagoClient::ColoredTextNode>& line : Items) {
            for (const ArchipelagoClient::ColoredTextNode& node : line) {
                ImGui::PushStyleColor(ImGuiCol_Text, getColorVal(node.color));
                ImGui::TextUnformatted(node.text.c_str());
                ImGui::SameLine();
                ImGui::PopStyleColor();
            }
            ImGui::NewLine();
        }

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

    static char textEntryBuf[1024];
    static bool keepFocus = false;

    if (keepFocus) {
        ImGui::SetKeyboardFocusHere();
        keepFocus = false;
    }
    if (ImGui::InputText("##AP_MessageField", textEntryBuf, 1023, ImGuiInputTextFlags_EnterReturnsTrue)) {
        ArchipelagoClient::GetInstance().SendMessageToConsole(std::string(textEntryBuf));
        textEntryBuf[0] = '\0';
        keepFocus = true;
    }
    // keepFocus = ImGui::IsItemActive();
    ImGui::SameLine();
    if (ImGui::Button("Send")) {
        ArchipelagoClient::GetInstance().SendMessageToConsole(std::string(textEntryBuf));
        textEntryBuf[0] = '\0';
        keepFocus = true;
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
};

ImVec4 getColorVal(const std::string& color) { // TODO change color strings to an enum
    if (color == "ERROR") {
        return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    } else if (color == "LOG") {
        return ImVec4(0.7f, 0.7f, 1.0f, 1.0f);
    } else if (color == "black") {
        return ImVec4(0.000f, 0.000f, 0.000f, 1.00f);
    } else if (color == "red") {
        return ImVec4(0.933f, 0.000f, 0.000f, 1.00f);
    } else if (color == "green") {
        return ImVec4(0.000f, 1.000f, 0.498f, 1.00f);
    } else if (color == "yellow") {
        return ImVec4(0.980f, 0.980f, 0.824f, 1.00f);
    } else if (color == "blue") {
        return ImVec4(0.392f, 0.584f, 0.929f, 1.00f);
    } else if (color == "cyan") {
        return ImVec4(0.000f, 0.933f, 0.933f, 1.00f);
    } else if (color == "magenta") {
        return ImVec4(0.933f, 0.000f, 0.933f, 1.00f);
    } else if (color == "slateblue") {
        return ImVec4(0.427f, 0.545f, 0.910f, 1.00f);
    } else if (color == "plum") {
        return ImVec4(0.686f, 0.600f, 0.937f, 1.00f);
    } else if (color == "salmon") {
        return ImVec4(0.980f, 0.502f, 0.447f, 1.00f);
    } else if (color == "white") {
        return ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    } else if (color == "orange") {
        return ImVec4(1.000, 0.467f, 0.000f, 1.000f);
    }
    return ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
}