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
};

struct MapWarp
{
	int x{ 0 };
	int y{ 0 };
	std::string targetMap;
	int targetX{ 0 };
	int targetY{ 0 };
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
	int minutesRemaining{ 120 }; // Minutes until despawn on unsafe tile (default 2 hours)
};

struct TileRuntimeData
{
	std::shared_ptr<entity> persistentNPC{ nullptr };
	std::vector<DroppedItemEntry> droppedItems;
	std::string iconId = "";
	int baseDangerLevel{ 0 };
	bool isStorageSafe{ false };

	std::vector<TemporarySafetyModifier> safetyModifiers;

	[[nodiscard]] bool getIsEffectiveStorageSafe() const
	{
		if (isStorageSafe) return true;

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

	/**
	 * Places an item on the tile ground with a time-to-decay lifespan in minutes.
	 */
	void addDroppedItem(std::shared_ptr<item> itemPtr, int lifespanMinutes = 120)
	{
		if (!itemPtr) return;
		droppedItems.push_back({ itemPtr, lifespanMinutes });
	}

	/**
	 * Decrements lifespans of dropped items on unsafe tiles and despawns expired loot.
	 */
	void processItemDecay(int minutesPassed)
	{
		if (getIsEffectiveStorageSafe() || minutesPassed <= 0) return;

		std::erase_if(droppedItems, [minutesPassed](DroppedItemEntry& entry) {
			entry.minutesRemaining -= minutesPassed;
			return entry.minutesRemaining <= 0;
		});
	}
};

/**
 * Bit-packs 2D integer coordinates into a 64-bit unsigned hash key for runtime storage.
 */
constexpr uint64_t makeTileKey(int32_t x, int32_t y) noexcept
{
	return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
}