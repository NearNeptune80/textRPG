#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

enum class gameEvent
{
	timeAdvanced,
	entityMutated,
	combatEnded,
	questStageChanged,
	itemUsed,
	mapEntered
};

struct eventData
{
	gameEvent type;
	int numericValue = 0;
	std::string stringValue = "";
	void* entityRef = nullptr;
};

using callbackID = std::uint64_t;

class eventBus
{
public:
	using eventCallback = std::function<void(const eventData&)>;

	static eventBus& getInstance()
	{
		static eventBus instance;
		return instance;
	}

	callbackID subscribe(gameEvent type, eventCallback callback);
	void unsubscribe(gameEvent type, callbackID id);
	void publishEvent(const eventData& data);
	void clearAllListeners();

private:
	eventBus() = default;
	~eventBus() = default;

	eventBus(const eventBus&) = delete;
	eventBus& operator=(const eventBus&) = delete;

	callbackID nextID = 1;

	struct subscriber
	{
		callbackID id;
		eventCallback callback;
	};

	std::unordered_map<gameEvent, std::vector<subscriber>> listeners;
};