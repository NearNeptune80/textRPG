#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "items/enchantmentAspects.h"

struct item;

struct EnchantmentGroupDef
{
    std::string id;
    std::string name;
    int defaultMaxEnchantments{ 999 };
    std::vector<std::string> allowedFocuses;
    std::vector<std::string> allowedProperties;
};

class EnchantmentRegistry
{
public:
    static EnchantmentRegistry& getInstance();

    bool load(const std::string& aspectsPath = "data/enchantments/aspects.json",
              const std::string& groupsPath = "data/enchantments/groups.json");

    bool isLoaded() const { return m_loaded; }

    const AspectDefinition* getFocusDef(const std::string& idOrEnum) const;
    const AspectDefinition* getPropertyDef(const std::string& idOrEnum) const;

    const EnchantmentGroupDef* getGroup(const std::string& groupId) const;
    const std::unordered_map<std::string, EnchantmentGroupDef>& getAllGroups() const { return m_groups; }

    std::vector<EnchantmentFocus> getCompatibleFocuses(const item* baseItem) const;
    std::vector<AspectProperty> getAvailablePropertiesForFocus(EnchantmentFocus focus, const item* baseItem) const;

    int getDefaultLimitForGroup(const std::string& groupId) const;

    // String / Enum bi-directional helpers
    static std::string focusToEnumString(EnchantmentFocus focus);
    static EnchantmentFocus enumStringToFocus(const std::string& str);

    static std::string propToEnumString(AspectProperty prop);
    static AspectProperty enumStringToProp(const std::string& str);

private:
    EnchantmentRegistry() = default;

    bool m_loaded{ false };
    std::unordered_map<std::string, AspectDefinition> m_focusesById;
    std::unordered_map<std::string, AspectDefinition> m_focusesByEnum;

    std::unordered_map<std::string, AspectDefinition> m_propsById;
    std::unordered_map<std::string, AspectDefinition> m_propsByEnum;

    std::unordered_map<std::string, std::string> m_propShortLabels;
    std::unordered_map<std::string, bool> m_propContinuous;
    std::unordered_map<std::string, std::vector<std::string>> m_propLimitSteps;

    std::unordered_map<std::string, std::vector<std::string>> m_focusToProps;

    std::unordered_map<std::string, EnchantmentGroupDef> m_groups;
};
