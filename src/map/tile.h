#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class entity;
struct item;

enum TileType
{
	TILE_VOID = 0,
	TILE_FLOOR = 1,
	TILE_WALL = 2,
	TILE_DOOR = 3
};

enum DiscoveryState
{
	STATE_HIDDEN,
	STATE_PARTIAL,
	STATE_REVEALED
};

struct Tile
{
	TileType type{ TILE_VOID };
	DiscoveryState discovery{ STATE_HIDDEN };
	bool visited{ false };
};

struct MapWarp
{
	int x{ 0 };
	int y{ 0 };
	std::string targetMap;
	int targetX{ 0 };
	int targetY{ 0 };
	std::string label{ "" };
	std::string entranceDescription{ "" };
};

struct TemporarySafetyModifier
{
	std::string sourceId;
	int dangerDelta{ 0 };
	int durationTurns{ 0 };
};

struct DroppedItemEntry
{
	std::shared_ptr<item> itemPtr{ nullptr };
	int minutesRemaining{ 120 };
};

struct PersistentAmbushState
{
	std::shared_ptr<entity> npc{ nullptr };
	std::string templateId;
	std::vector<std::string> templatePool;
	bool isPermanentlyRemoved{ false };
	bool isDefeated{ false };
	int restockMinutesRemaining{ 0 };
	int ambushChance{ 50 };
};

struct TileCustomization
{
	bool isCustomizable{ false };
	std::string currentRoomId{ "" };       // Empty string = unbuilt foundation
	int upgradeTier{ 0 };                  // 0 = unbuilt, 1 = Tier 1, 2 = Tier 2, etc.
	std::string stationType{ "" };         // e.g. "alchemy", "milking_parlor", "nursery", "service_room"
	std::vector<std::string> roomTags;
};

struct TileRuntimeData
{
	std::shared_ptr<entity> persistentNPC{ nullptr };
	std::vector<std::shared_ptr<entity>> namedNPCs;
	PersistentAmbushState ambushState;
	std::vector<DroppedItemEntry> droppedItems;
	float droppedCurrency{ 0.0f };
	int currencyMinutesRemaining{ 120 };
	std::string iconId = "";
	int baseDangerLevel{ 0 };
	bool isStorageSafe{ false };
	TileCustomization customization;

	std::vector<TemporarySafetyModifier> safetyModifiers;

	[[nodiscard]] bool getIsEffectiveStorageSafe() const
	{
		if (isStorageSafe) return true;
		if (!customization.currentRoomId.empty()) return true; // Built rooms are always safe
		for (const auto& mod : safetyModifiers)
		{
			if (mod.dangerDelta < 0) return true;
		}
		return false;
	}

	[[nodiscard]] int getEffectiveDangerLevel() const
	{
		int effective = baseDangerLevel;
		for (const auto& mod : safetyModifiers)
		{
			effective += mod.dangerDelta;
		}
		return std::max(0, effective);
	}

	void addDroppedItem(std::shared_ptr<item> itemPtr, int lifespanMinutes = 120)
	{
		if (!itemPtr) return;
		droppedItems.push_back({ itemPtr, lifespanMinutes });
	}

	void addDroppedCurrency(float amount, int lifespanMinutes = 120)
	{
		if (amount <= 0.0f) return;
		droppedCurrency += amount;
		currencyMinutesRemaining = lifespanMinutes;
	}

	void processItemDecay(int minutesPassed)
	{
		if (getIsEffectiveStorageSafe()) return;
		if (minutesPassed <= 0) return;

		for (auto it = droppedItems.begin(); it != droppedItems.end();)
		{
			it->minutesRemaining -= minutesPassed;
			if (it->minutesRemaining <= 0)
			{
				it = droppedItems.erase(it);
			}
			else
			{
				++it;
			}
		}

		if (droppedCurrency > 0.0f)
		{
			currencyMinutesRemaining -= minutesPassed;
			if (currencyMinutesRemaining <= 0)
			{
				droppedCurrency = 0.0f;
			}
		}
	}
};

// Bit-packs 2D integer coordinates into a 64-bit unsigned hash key for runtime storage.
constexpr uint64_t makeTileKey(int32_t x, int32_t y) noexcept
{
	return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
}