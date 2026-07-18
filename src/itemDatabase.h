#pragma once
#include <string>
#include <unordered_map>
#include "item.h"

class itemDatabase
{
private:
    // The master dictionary storing our template items
    static std::unordered_map<std::string, item> registry;

public:
    // Reads the JSON file and populates the registry
    static bool loadDatabase(const std::string& filePath);

    // Retrieves a fresh copy of an item by its ID string
    static item getItem(const std::string& id);

    // Checks if an item template exists
    static bool exists(const std::string& id);
};