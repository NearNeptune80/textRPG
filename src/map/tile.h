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
	TileType type;
	DiscoveryState discovery;
};

struct MapWarp
{
	int x;
	int y;
	std::string targetMap;
	int targetX;
	int targetY;
};

struct TemporarySafetyModifier
{
	std::string sourceId;
	int dangerDelta;
	int durationTurns;
};

struct TileRuntimeData
{
	std::shared_ptr<entity> persistentNPC = nullptr;
	std::vector<std::shared_ptr<item>> droppedItems;
	std::string iconId = "";
	int baseDangerLevel = 0;
	bool isStorageSafe = false;

	std::vector<TemporarySafetyModifier> safetyModifiers;

	int getEffectiveDangerLevel() const
	{
		int effective = baseDangerLevel;
		for (const auto& mod : safetyModifiers)
		{
			effective += mod.dangerDelta;
		}
		return std::max(0, effective);
	}
};

constexpr uint64_t makeTileKey(int32_t x, int32_t y) noexcept
{
	return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
}