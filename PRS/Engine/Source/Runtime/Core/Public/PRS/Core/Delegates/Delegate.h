#pragma once
#include "PRS/Core/Types.h"
#include <functional>
#include <vector>

namespace PRS
{
// Multicast delegate: handlers are stored by id so they can be removed while
// broadcasting is in progress (removal is deferred to the end of the broadcast).
template <typename... Args> class Delegate
{
public:
    using Handler = std::function<void(Args...)>;
    using Id = u64;

    Id Add(Handler handler)
    {
        const Id id = m_nextId++;
        m_handlers.push_back({id, std::move(handler)});
        return id;
    }

    bool Remove(Id id)
    {
        for (size_t i = 0; i < m_handlers.size(); ++i)
        {
            if (m_handlers[i].first != id) continue;
            if (m_broadcasting)
            {
                m_pendingRemoval.push_back(id);
                m_handlers[i].second = nullptr;
            }
            else
            {
                m_handlers.erase(m_handlers.begin() + static_cast<long>(i));
            }
            return true;
        }
        return false;
    }

    void Broadcast(Args... args) const
    {
        m_broadcasting = true;
        for (const auto& entry : m_handlers)
            if (entry.second) entry.second(args...);
        m_broadcasting = false;
        FlushRemovals();
    }

    void Clear()
    {
        m_handlers.clear();
        m_pendingRemoval.clear();
    }

    size_t Count() const { return m_handlers.size(); }
    bool Empty() const { return m_handlers.empty(); }

private:
    void FlushRemovals() const
    {
        if (m_pendingRemoval.empty()) return;
        for (Id id : m_pendingRemoval)
        {
            for (size_t i = 0; i < m_handlers.size(); ++i)
            {
                if (m_handlers[i].first != id) continue;
                m_handlers.erase(m_handlers.begin() + static_cast<long>(i));
                break;
            }
        }
        m_pendingRemoval.clear();
    }

    std::vector<std::pair<Id, Handler>> m_handlers;
    mutable std::vector<Id> m_pendingRemoval;
    mutable bool m_broadcasting = false;
    Id m_nextId = 1;
};
} // namespace PRS
