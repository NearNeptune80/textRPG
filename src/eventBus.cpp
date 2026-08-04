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
	if (it == listeners.end() || it->second.empty()) return;

	// Iterate by index over size to prevent vector copy memory allocation crashes during dispatch
	size_t count = it->second.size();
	for (size_t i = 0; i < count && i < it->second.size(); ++i)
	{
		if (it->second[i].callback)
		{
			it->second[i].callback(data);
		}
	}
}

void eventBus::clearAllListeners()
{
	listeners.clear();
}