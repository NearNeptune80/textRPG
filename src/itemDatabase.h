#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "item.h"

equipSlot stringToEquipSlot(const std::string& str);
std::string equipSlotToString(equipSlot slot);

class itemDatabase
{
private:
    static std::unordered_map<std::string, item> registry;

public:
    static bool loadDatabase(const std::string& filePath);
    static bool exists(const std::string& id);
    static std::shared_ptr<item> getItem(const std::string& id);
};