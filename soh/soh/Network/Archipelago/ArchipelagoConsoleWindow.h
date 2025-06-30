#pragma once
#ifndef ARCHIPELAGO_CONSOLE_WINDOW_H
#define ARCHIPELAGO_CONSOLE_WINDOW_H

#include <libultraship/libultraship.h>
#include "Archipelago.h"
#include <vector>
#include <list>

class ArchipelagoConsoleWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~ArchipelagoConsoleWindow(){};

  protected:
    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};

void ArchipelagoConsole_SendMessage(const char* fmt, bool debugMessage = false, ...);
void ArchipelagoConsole_PrintJson(const std::vector<ArchipelagoClient::ColoredTextNode> nodes);
ImVec4 getColorVal(const std::string& color);

#endif // ARCHIPELAGO_CONSOLE_WINDOW_H