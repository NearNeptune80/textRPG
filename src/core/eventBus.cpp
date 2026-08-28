#include "core/eventBus.h"

#include <algorithm>

/**
 * Subscribes a listener callback to a specific game event type.
 * Returns a unique callbackID used to unsubscribe later.
 */
callbackID eventBus::subscribe(gameEvent type, eventCallback callback)
{
	callbackID assignedID = nextID++;
	listeners[type].push_back({ assignedID, std::move(callback) });
	return assignedID;
}

/**
 * Unsubscribes a listener by its unique callback ID.
 */
void eventBus::unsubscribe(gameEvent type, callbackID id)
{
	auto it = listeners.find(type);
	if (it == listeners.end()) return;

	std::erase_if(it->second, [id](const subscriber& sub) { return sub.id == id; });
}

/**
 * Dispatches an event payload synchronously to all registered listeners.
 */
void eventBus::publishEvent(const eventData& data)
{
	auto it = listeners.find(data.type);
	if (it == listeners.end() || it->second.empty()) return;

	// Use indexed iteration to prevent iterator invalidation if listeners modify subscriptions
	size_t count = it->second.size();
	for (size_t i = 0; i < count && i < it->second.size(); ++i)
	{
		if (it->second[i].callback)
		{
			it->second[i].callback(data);
		}
	}
}

/**
 * Removes all active event listeners.
 */
void eventBus::clearAllListeners()
{
	listeners.clear();
}