#include "eventBus.h"
#include <algorithm>

callbackID eventBus::subscribe(gameEvent type, eventCallback callback)
{
	callbackID assignedID = nextID++;
	listeners[type].push_back({ assignedID, std::move(callback) });
	return assignedID;
}

void eventBus::unsubscribe(gameEvent type, callbackID id)
{
	auto it = listeners.find(type);
	if (it == listeners.end()) return;

	auto& subscriberList = it->second;
	subscriberList.erase(
		std::remove_if(subscriberList.begin(), subscriberList.end(),
			[id](const subscriber& sub) { return sub.id == id; }),
		subscriberList.end()
	);
}

void eventBus::publishEvent(const eventData& data)
{
	auto it = listeners.find(data.type);
	if (it == listeners.end()) return;

	// Iterate over a copy of the list so callbacks can safely subscribe/unsubscribe during publish
	const auto subscriberListCopy = it->second;
	for (const auto& listener : subscriberListCopy)
	{
		if (listener.callback)
		{
			listener.callback(data);
		}
	}
}

void eventBus::clearAllListeners()
{
	listeners.clear();
}