// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"
#include "IndexableName.hpp"

#include <functional>
#include <unordered_map>

FLOW_NAMESPACE_BEGIN

template<class... Args>
using Event = std::function<void(Args...)>;

/**
 * @brief Dispatches a series of bound events.
 * @tparam Args The argument types for the event.
 */
template<class... Args>
class EventDispatcher
{
    using EventType = Event<Args...>;

  public:
    /**
     * @brief Binds an event to the dispatcher.
     *
     * @details Uniquely binds an event to the dispatcher. Once bound, another event cannot rebind with the same name.
     *          The original event MUST first be unbound.
     *
     * @param name The unique identifier of the event to be bound.
     * @param event The event to be bound.
     */
    void Bind(IndexableName name, EventType&& event) noexcept { _events.try_emplace(name, std::move(event)); }

    /**
     * @brief Unbinds an event by name.
     *
     * @param name The name fo the event being unbound.
     */
    void Unbind(IndexableName name) { _events.erase(name); }

    /**
     * @brief Unbinds all events from the dispatcher.
     */
    void UnbindAll() { _events.clear(); }

    /**
     * @brief Broadcasts the given arguments to all bound events asynchronously.
     *
     * @param args Variadic arguments to pass to all bound events.
     */
    void Broadcast(Args&&... args) const
    {
        for (const auto& [_, event] : _events)
        {
            event(std::forward<Args>(args)...);
        }
    }

  private:
    /// Keyed list of bound events.
    std::unordered_map<IndexableName, EventType> _events;
};

FLOW_NAMESPACE_END
