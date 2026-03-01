#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include "Event.h"

namespace se::core {

class EventBus {
   public:
    using HandlerId = uint64_t;

    struct Subscription {
        EventBus* bus = nullptr;
        EventType type = EventType::FramebufferResize;
        HandlerId id = 0;

        Subscription() = default;
        Subscription(EventBus* bus, EventType type, HandlerId id)
            : bus(bus), type(type), id(id) {}

        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        Subscription(Subscription&& other) noexcept {
            bus = other.bus;
            type = other.type;
            id = other.id;
            other.bus = nullptr;
            other.id = 0;
        }

        Subscription& operator=(Subscription&& other) noexcept {
            if (this != &other) {
                reset();
                bus = other.bus;
                type = other.type;
                id = other.id;
                other.bus = nullptr;
                other.id = 0;
            }
            return *this;
        }

        ~Subscription() { reset(); }

        void reset() {
            if (bus && id != 0) {
                bus->unsubscribe(*this);
            }
            bus = nullptr;
            id = 0;
        }
    };

    template <typename T>
    using Handler = std::function<void(const T&)>;

    template <typename T>
    HandlerId subscribe(Handler<T> handler) {
        HandlerId id = m_NextId.fetch_add(1);
        auto wrapper = [handler](const Event& e) {
            handler(static_cast<const T&>(e));
        };
        m_Handlers[T::kType].push_back({id, std::move(wrapper)});
        return id;
    }

    template <typename T>
    Subscription subscribeScoped(Handler<T> handler) {
        HandlerId id = subscribe<T>(std::move(handler));
        return Subscription(this, T::kType, id);
    }

    void unsubscribe(const Subscription& subscription) {
        auto it = m_Handlers.find(subscription.type);
        if (it == m_Handlers.end()) {
            return;
        }

        auto& handlers = it->second;
        handlers.erase(
            std::remove_if(
                handlers.begin(),
                handlers.end(),
                [&subscription](const auto& entry) { return entry.id == subscription.id; }),
            handlers.end());
    }

    template <typename T>
    void queue(const T& event) {
        m_Queue.push_back([this, event]() {
            dispatch(event);
        });
    }

    void dispatchQueued() {
        for (const auto& dispatch : m_Queue) {
            dispatch();
        }
        m_Queue.clear();
    }

    void dispatch(const Event& event) const {
        auto it = m_Handlers.find(event.type);
        if (it == m_Handlers.end()) {
            return;
        }
        for (const auto& handler : it->second) {
            handler.fn(event);
        }
    }

   private:
    using HandlerFn = std::function<void(const Event&)>;
    struct HandlerEntry {
        HandlerId id;
        HandlerFn fn;
    };

    std::unordered_map<EventType, std::vector<HandlerEntry>> m_Handlers;
    std::vector<std::function<void()>> m_Queue;
    std::atomic<HandlerId> m_NextId{1};
};

}  // namespace se::core
