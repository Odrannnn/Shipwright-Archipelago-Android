#include "Archipelago.h"
#include "soh/util.h"
#include <apuuid.hpp>
#include <apclient.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>

#include "soh/Network/Archipelago/ArchipelagoConsoleWindow.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/context.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Notification/Notification.h"
#include "soh/ShipInit.hpp"
#include "soh/SaveManager.h"

extern "C" {
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
}

ArchipelagoClient::ArchipelagoClient() {
    uuid = ap_get_uuid("uuid");

    gameWon = false;
    itemQueued = false;
    disconnecting = false;
}

ArchipelagoClient& ArchipelagoClient::GetInstance() {
    static ArchipelagoClient Client;
    return Client;
}

bool ArchipelagoClient::StartClient() {
    if (apClient != NULL) {
        apClient.reset();
    }

    disconnecting = false;
    apClient = std::unique_ptr<APClient>(
        new APClient(uuid, AP_Client_consts::AP_GAME_NAME,
                     CVarGetString(CVAR_REMOTE_ARCHIPELAGO("ServerAddress"), "localhost:38281"), "cacert.pem"));

    apClient->set_room_info_handler([&]() {
        std::list<std::string> tags;
        // tags.push_back("DeathLink");     // todo, implement deathlink
        apClient->ConnectSlot(CVarGetString(CVAR_REMOTE_ARCHIPELAGO("SlotName"), ""),
                              CVarGetString(CVAR_REMOTE_ARCHIPELAGO("Password"), ""), 0b001, tags);
    });

    apClient->set_slot_connected_handler([&](const nlohmann::json data) {
        ArchipelagoConsole_SendMessage("[LOG] Connected.", true);
        ArchipelagoClient::StartLocationScouts();

        slotData = data;

        // if we are already in game when we connect
        // we won't have to request an itemSynch
        if (GameInteractor::IsSaveLoaded(true)) {
            if (!isRightSaveLoaded()) {
                disconnecting = true;
                ArchipelagoConsole_SendMessage("[ERROR] Connected to incorrect slot, disconnecting...");
                return;
            }

            SynchSentLocations();
            SynchReceivedLocations();
        }
    });

    apClient->set_items_received_handler([&](const std::list<APClient::NetworkItem>& items) {
        if (disconnecting) {
            return;
        }

        for (const APClient::NetworkItem& item : items) {
            ApItem apItem;
            const std::string game = apClient->get_player_game(item.player);
            apItem.itemName = apClient->get_item_name(item.item, AP_Client_consts::AP_GAME_NAME);
            apItem.locationName = apClient->get_location_name(item.location, game);
            apItem.playerName = apClient->get_player_alias(item.player);
            apItem.flags = item.flags;
            apItem.index = item.index;
            OnItemReceived(apItem);
        }
    });

    apClient->set_location_info_handler([&](const std::list<APClient::NetworkItem>& items) {
        if (disconnecting) {
            return;
        }

        scoutedItems.clear();

        for (const APClient::NetworkItem& item : items) {
            ApItem apItem;
            const std::string game = apClient->get_player_game(item.player);
            apItem.itemName = apClient->get_item_name(item.item, game);
            apItem.locationName = apClient->get_location_name(item.location, AP_Client_consts::AP_GAME_NAME);
            apItem.playerName = apClient->get_player_alias(item.player);
            apItem.flags = item.flags;
            apItem.index = item.index;
            scoutedItems.push_back(apItem);

            const std::string itemName = apItem.itemName;
            const std::string playerName = apItem.playerName;
            const std::string locationName = apItem.locationName;
            std::string logMessage =
                "[LOG] Location scouted: " + itemName + " for " + playerName + " in location " + locationName;
            ArchipelagoConsole_SendMessage(logMessage.c_str(), true);
        }

        ArchipelagoConsole_SendMessage("[LOG] Scouting finished.", true);
    }); // todo maybe move these functions to a lambda, since they don't have to be static anymore

    apClient->set_location_checked_handler([&](const std::list<int64_t> locations) {
        if (disconnecting) {
            return;
        }

        for (const int64_t apLoc : locations) {
            QueueExternalCheck(apLoc);
        }
    });

    apClient->set_print_json_handler([&](const APClient::PrintJSONArgs& arg) {
        if (disconnecting) {
            return;
        }

        std::vector<ColoredTextNode> coloredNodes;

        for (const APClient::TextNode& node : arg.data) {
            APClient* client = apClient.get();
            std::string color;
            std::string text;

            if (node.type == "player_id") {
                int id = std::stoi(node.text);
                if (color.empty() && id == client->get_player_number())
                    color = "magenta";
                else if (color.empty())
                    color = "yellow";
                text = client->get_player_alias(id);
            } else if (node.type == "item_id") {
                int64_t id = std::stoll(node.text);
                if (color.empty()) {
                    if (node.flags & APClient::ItemFlags::FLAG_ADVANCEMENT)
                        color = "plum";
                    else if (node.flags & APClient::ItemFlags::FLAG_NEVER_EXCLUDE)
                        color = "slateblue";
                    else if (node.flags & APClient::ItemFlags::FLAG_TRAP)
                        color = "salmon";
                    else
                        color = "cyan";
                }
                text = client->get_item_name(id, client->get_player_game(node.player));
            } else if (node.type == "location_id") {
                int64_t id = std::stoll(node.text);
                if (color.empty())
                    color = "blue";
                text = client->get_location_name(id, client->get_player_game(node.player));
            } else if (node.type == "hint_status") {
                text = node.text;
                if (node.hintStatus == APClient::HINT_FOUND)
                    color = "green";
                else if (node.hintStatus == APClient::HINT_UNSPECIFIED)
                    color = "grey";
                else if (node.hintStatus == APClient::HINT_NO_PRIORITY)
                    color = "slateblue";
                else if (node.hintStatus == APClient::HINT_AVOID)
                    color = "salmon";
                else if (node.hintStatus == APClient::HINT_PRIORITY)
                    color = "plum";
                else
                    color = "red"; // unknown status -> red
            } else if (node.type == "ERROR") {
                color = "ERROR";
                text = node.text;
            } else if (node.type == "LOG") {
                color = "LOG";
                text = node.text;
            } else {
                color = "white";
                text = node.text;
            }

            ColoredTextNode Colornode;
            Colornode.color = color;
            Colornode.text = text;
            coloredNodes.push_back(Colornode);
        }

        ArchipelagoConsole_PrintJson(coloredNodes);
    });

    return true;
}

void ArchipelagoClient::GameLoaded() {
    if (apClient == nullptr) {
        return;
    }

    // if its not an AP save, disconnect
    if (!IS_ARCHIPELAGO) {
        ArchipelagoConsole_SendMessage("[ERROR] Loaded save is not not an archipelago save, disconnecting...");
        disconnecting = true;
        return;
    }

    if (!isRightSaveLoaded()) {
        ArchipelagoConsole_SendMessage("[ERROR] Loaded save is not associated with connected slot, disconnecting...");
        disconnecting = true;
        return;
    }

    ArchipelagoConsole_SendMessage("[LOG] Synching Items and Locations.", true);

    SynchItems();
    SynchSentLocations();
    SynchReceivedLocations();
}

void ArchipelagoClient::StartLocationScouts() {
    std::set<int64_t> missing_loc_set = apClient->get_missing_locations();
    std::set<int64_t> found_loc_set = apClient->get_checked_locations();
    std::list<int64_t> location_list;
    for (const int64_t loc_id : missing_loc_set) {
        location_list.emplace_back(loc_id);
    }
    for (const int64_t loc_id : found_loc_set) {
        location_list.emplace_back(loc_id);
    }
    apClient->LocationScouts(location_list);
}

void ArchipelagoClient::SynchItems() {
    // Send a Synch request to get any items we may have missed
    ArchipelagoConsole_SendMessage("[LOG] Sending synch request", true);
    apClient->Sync();
}

void ArchipelagoClient::SynchSentLocations() {
    // send already checked locations
    std::list<int64_t> checkedLocations;
    for (const auto& loc : Rando::StaticData::GetLocationTable()) {
        const RandomizerCheck rc = loc.GetRandomizerCheck();
        if (Rando::Context::GetInstance()->GetItemLocation(rc)->HasObtained()) {
            const int64_t apLocation = apClient->get_location_id(loc.GetName());
            checkedLocations.emplace_back(apLocation);
        }
    }
    std::string locationLog =
        "[LOG] Synching " + std::to_string(checkedLocations.size()) + " checks already found in game";
    ArchipelagoConsole_SendMessage(locationLog.c_str(), true);

    apClient->LocationChecks(checkedLocations);
}

void ArchipelagoClient::SynchReceivedLocations() {
    // Open checks that have been found previously but went unsaved
    for (const int64_t apLoc : apClient->get_checked_locations()) {
        QueueExternalCheck(apLoc);
    }
}

void ArchipelagoClient::QueueExternalCheck(const int64_t apLocation) {
    const std::string checkName = apClient->get_location_name(apLocation, AP_Client_consts::AP_GAME_NAME);
    const uint32_t RC = static_cast<uint32_t>(Rando::StaticData::locationNameToEnum[checkName]);

    if (RC == RC_UNKNOWN_CHECK) {
        ArchipelagoConsole_SendMessage("[ERROR] Attempting to queue RC_UNKOWN_CHECK, skipping", false);
        return;
    }

    // Don't queue checks we already have
    if (Rando::Context::GetInstance()->GetItemLocation(RC)->HasObtained()) {
        return;
    }

    std::string locationLog = "[LOG] Externaly checking: " + checkName;
    ArchipelagoConsole_SendMessage(locationLog.c_str(), true);

    GameInteractor_ExecuteOnRandomizerExternalCheck(RC);
}

bool ArchipelagoClient::IsConnected() {
    if (apClient == nullptr) {
        return false;
    }

    return apClient->get_state() == APClient::State::SLOT_CONNECTED;
}

void ArchipelagoClient::CheckLocation(RandomizerCheck sohCheckId) {
    if (sohCheckId == RC_UNKNOWN_CHECK) {
        ArchipelagoConsole_SendMessage("[ERROR] trying to send RC_UNKNOWN_CHECK, skipping", false);
        return;
    }

    if (!IsConnected()) {
        return;
    }

    std::string apName = Rando::StaticData::GetLocation(sohCheckId)->GetName();
    if (apName.empty()) {
        return;
    }

    int64_t apItemId = apClient->get_location_id(std::string(apName));
    std::string logMessage = "[LOG] Checked: " + apName + "(" + std::to_string(apItemId) + "), sending to AP server";
    ArchipelagoConsole_SendMessage(logMessage.c_str(), true);

    apClient->LocationChecks({ apItemId });
}

void ArchipelagoClient::OnItemReceived(const ApItem apItem) {
    if (!GameInteractor::IsSaveLoaded(true)) {
        // Don't queue up any items when we aren't in game
        // Any Items missed this way will get synched when we load the save
        return;
    }

    std::string logMessage = "[LOG] Received " + apItem.itemName;
    ArchipelagoConsole_SendMessage(logMessage.c_str(), true);

    if (apItem.index < gSaveContext.ship.quest.data.archipelago.lastReceivedItemIndex) {
        // Skip queueing any items we already have
        std::string logMessage = "[LOG] Skipping giving " + apItem.itemName + ". We received this previously.";
        ArchipelagoConsole_SendMessage(logMessage.c_str(), true);
        return;
    }

    // add item to the queue
    receiveQueue.push(apItem);
}

void ArchipelagoClient::QueueItem(const ApItem item) {
    std::string logMessage = "[LOG] Giving " + item.itemName;
    ArchipelagoConsole_SendMessage(logMessage.c_str(), true);
    const RandomizerGet RG = Rando::StaticData::itemNameToEnum[item.itemName];
    if (RG == RG_NONE) {
        return;
    }

    itemQueued = true;
    GameInteractor_ExecuteOnArchipelagoItemReceived(static_cast<int32_t>(RG));
}

void ArchipelagoClient::SendGameWon() {
    if (!gameWon) {
        apClient->StatusUpdate(APClient::ClientStatus::GOAL);
        gameWon = true;
    }
}

void ArchipelagoClient::SendMessageToConsole(const std::string message) {
    // local commands not implemented yet
    if (message.starts_with("/")) {
        ArchipelagoConsole_SendMessage(
            "Ship of Harkinian does not have any local commands yet.\nUse \"!help\" to see server commands instead",
            false);
        return;
    }

    if (apClient == nullptr) {
        ArchipelagoConsole_SendMessage("[ERROR] Could not send message. Please Connect to your slot.", false);
        return;
    }

    apClient->Say(message);
}

void ArchipelagoClient::Poll() {
    if (apClient == nullptr) {
        return;
    }

    if (disconnecting) {
        apClient->reset();
        apClient = nullptr;
        return;
    }

    // queue another item to be received
    if (!itemQueued && receiveQueue.size() > 0) {

        const ApItem item = receiveQueue.front();
        receiveQueue.pop();
        QueueItem(item);
    }

    apClient->poll();
}

bool ArchipelagoClient::isRightSaveLoaded() const {
    const bool seedMatch = apClient->get_seed().compare(gSaveContext.ship.quest.data.archipelago.roomHash) == 0;
    const bool slotMatch = GetSlotName().compare(gSaveContext.ship.quest.data.archipelago.slotName) == 0;
    return seedMatch && slotMatch;
}

const std::string ArchipelagoClient::GetSlotName() const {
    if (apClient == NULL) {
        return "";
    }

    return apClient->get_slot();
}

const nlohmann::json ArchipelagoClient::GetSlotData() {
    return slotData;
}

const std::vector<ArchipelagoClient::ApItem>& ArchipelagoClient::GetScoutedItems() {
    return scoutedItems;
}

const char* ArchipelagoClient::GetConnectionStatus() {
    if (!apClient) {
        return "Disconnected!";
    }

    APClient::State clientStatus = apClient->get_state();

    switch (clientStatus) {
        case APClient::State::DISCONNECTED: {
            return "Disconnected!";
        }
        case APClient::State::SOCKET_CONNECTING: {
            return "Socket Connecting!";
        }
        case APClient::State::SOCKET_CONNECTED: {
            return "Socket Connected!";
        }
        case APClient::State::ROOM_INFO: {
            return "Room info Received!";
        }
        case APClient::State::SLOT_CONNECTED: {
            return "Slot Connected!";
        }
        default:
            return "";
    }
}

extern "C" void Archipelago_InitSaveFile() {
    gSaveContext.ship.quest.data.archipelago.isArchipelago = 1;

    nlohmann::json slotData = ArchipelagoClient::GetInstance().GetSlotData();
    gSaveContext.ship.quest.data.archipelago.deathLink = slotData["death_link"];

    std::vector<ArchipelagoClient::ApItem> scoutedItems = ArchipelagoClient::GetInstance().GetScoutedItems();

    ArchipelagoClient& client = ArchipelagoClient::GetInstance();
    SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.roomHash, client.apClient->get_seed(),
                                    ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.roomHash));
    SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.slotName, client.apClient->get_slot(),
                                    ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.slotName));

    for (uint32_t i = 0; i < scoutedItems.size(); i++) {
        RandomizerCheck rc = Rando::StaticData::locationNameToEnum[scoutedItems[i].locationName];

        SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.locations[rc].itemName,
                                        scoutedItems[i].itemName,
                                        ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations[rc].itemName));
        SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.locations[rc].playerName,
                                        scoutedItems[i].playerName,
                                        ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations[rc].playerName));
    }
}

void LoadArchipelagoData() {
    SaveManager::Instance->LoadData("isArchipelago", gSaveContext.ship.quest.data.archipelago.isArchipelago);
    SaveManager::Instance->LoadData("lastReceivedItemIndex",
                                    gSaveContext.ship.quest.data.archipelago.lastReceivedItemIndex);
    SaveManager::Instance->LoadData("deathLink", gSaveContext.ship.quest.data.archipelago.deathLink);

    SaveManager::Instance->LoadCharArray("roomHash", gSaveContext.ship.quest.data.archipelago.roomHash,
                                         ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.roomHash));
    SaveManager::Instance->LoadCharArray("slotName", gSaveContext.ship.quest.data.archipelago.slotName,
                                         ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.slotName));

    SaveManager::Instance->LoadArray(
        "locations", ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations), [](size_t i) {
            SaveManager::Instance->LoadStruct("", [&i]() {
                SaveManager::Instance->LoadCharArray(
                    "itemName", gSaveContext.ship.quest.data.archipelago.locations[i].itemName,
                    ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations[i].itemName));
                SaveManager::Instance->LoadCharArray(
                    "playerName", gSaveContext.ship.quest.data.archipelago.locations[i].playerName,
                    ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations[i].playerName));
            });
        });
}

void SaveArchipelagoData(SaveContext* saveContext, int sectionID, bool fullSave) {
    SaveManager::Instance->SaveData("isArchipelago", saveContext->ship.quest.data.archipelago.isArchipelago);
    SaveManager::Instance->SaveData("lastReceivedItemIndex",
                                    saveContext->ship.quest.data.archipelago.lastReceivedItemIndex);
    SaveManager::Instance->SaveData("deathLink", saveContext->ship.quest.data.archipelago.deathLink);

    SaveManager::Instance->SaveData("roomHash", saveContext->ship.quest.data.archipelago.roomHash);
    SaveManager::Instance->SaveData("slotName", saveContext->ship.quest.data.archipelago.slotName);

    SaveManager::Instance->SaveArray(
        "locations", ARRAY_COUNT(saveContext->ship.quest.data.archipelago.locations), [&](size_t i) {
            SaveManager::Instance->SaveStruct("", [&]() {
                SaveManager::Instance->SaveData("itemName",
                                                saveContext->ship.quest.data.archipelago.locations[i].itemName);
                SaveManager::Instance->SaveData("playerName",
                                                saveContext->ship.quest.data.archipelago.locations[i].playerName);
            });
        });
}

void InitArchipelagoData(bool isDebug) {
    gSaveContext.ship.quest.data.archipelago.isArchipelago = 0;
    gSaveContext.ship.quest.data.archipelago.lastReceivedItemIndex = 0;
    gSaveContext.ship.quest.data.archipelago.deathLink = 0;

    SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.roomHash, "",
                                    ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.roomHash));
    SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.slotName, "",
                                    ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.slotName));

    for (uint32_t i = 0; i < ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations); i++) {
        SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.locations[i].itemName, "",
                                        ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations[i].itemName));
        SohUtils::CopyStringToCharArray(gSaveContext.ship.quest.data.archipelago.locations[i].playerName, "",
                                        ARRAY_COUNT(gSaveContext.ship.quest.data.archipelago.locations[i].playerName));
    }
}

void RegisterArchipelago() {
    // make sure the client is constructed
    ArchipelagoClient::GetInstance();

    CVarSetInteger(CVAR_REMOTE_ARCHIPELAGO("Connected"), 0);

    COND_HOOK(GameInteractor::OnGameFrameUpdate, true, []() { ArchipelagoClient::GetInstance().Poll(); });
    COND_HOOK(GameInteractor::PostLoadGame, true,
              [](int32_t file_id) { ArchipelagoClient::GetInstance().GameLoaded(); });
    COND_HOOK(
        GameInteractor::OnRandomizerItemGivenHooks, IS_ARCHIPELAGO,
        [](uint32_t rc, GetItemEntry gi, uint8_t isGiSkipped) {
            if (rc == RC_ARCHIPELAGO_RECEIVED_ITEM) {
                gSaveContext.ship.quest.data.archipelago.lastReceivedItemIndex++;
                ArchipelagoClient::GetInstance().itemQueued = false;
            } else {
                ArchipelagoClient::GetInstance().CheckLocation((RandomizerCheck)rc);

                if (isGiSkipped && gi.modIndex == MOD_RANDOMIZER &&
                    (gi.getItemId == RG_ARCHIPELAGO_ITEM_PROGRESSIVE || gi.getItemId == RG_ARCHIPELAGO_ITEM_USEFUL ||
                     gi.getItemId == RG_ARCHIPELAGO_ITEM_JUNK)) {

                    const char* itemIcon = "";
                    switch (gi.getItemId) {
                        case RG_ARCHIPELAGO_ITEM_PROGRESSIVE:
                            itemIcon = "Archipelago Progressive Icon";
                            break;
                        case RG_ARCHIPELAGO_ITEM_USEFUL:
                            itemIcon = "Archipelago Useful Icon";
                            break;
                        case RG_ARCHIPELAGO_ITEM_JUNK:
                            itemIcon = "Archipelago Junk Icon";
                            break;
                    }

                    Notification::Emit(
                        { .itemIcon = itemIcon,
                          .prefix = std::string(gSaveContext.ship.quest.data.archipelago.locations[rc].itemName),
                          .message = " for ",
                          .suffix = std::string(gSaveContext.ship.quest.data.archipelago.locations[rc].playerName) });
                }
            }
        });
}

static RegisterShipInitFunc initFunc(RegisterArchipelago, { "IS_ARCHIPELAGO" });
