#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "items/item.h"

equipSlot stringToEquipSlot(const std::string& str);
std::string equipSlotToString(equipSlot slot);

class itemDatabase
{
public:
    static bool loadDatabase(const std::string& filePath);
    static bool exists(std::string_view id);
    static std::shared_ptr<item> getItem(std::string_view id);

private:
    static std::unordered_map<std::string, item> registry;
};