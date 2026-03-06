#pragma once
#include <unordered_map>
#include <asp/time.hpp>

namespace asp {

/// A Cache is a key-value map that has properties like automatic eviction, TTL, etc.
/// If max capacity is set, elements may be evicted using the LRU policy.
template <typename K, typename V, template <typename, typename> typename Storage = std::unordered_map>
class Cache {
public:
    Cache() = default;

    void setMaxEntries(size_t m) {
        m_maxEntries = m;
    }

    void setTimeToLive(Duration dur) {
        m_ttl = dur;
    }

    /// Set how often certain cleanup is performed (e.g. expiring old entries)
    void setWorkInterval(Duration dur) {
        m_workInterval = dur;
    }

    void insert(K key, V value) {
        if (m_maxEntries > 0 && m_storage.size() >= m_maxEntries) {
            this->evictOne();
        }

        auto now = Instant::now();
        m_storage.emplace(std::move(key), Entry{std::move(value), now, now});

        if (now.durationSince(m_lastWork) > m_workInterval) {
            this->doWork();
        }
    }

    size_t size() const {
        return m_storage.size();
    }

    /// Returns the value at the given key, or null if not present
    V* get(const K& key) {
        auto now = asp::Instant::now();
        if (now.durationSince(m_lastWork) > m_workInterval) {
            this->doWork();
        }

        auto it = m_storage.find(key);

        if (it == m_storage.end()) {
            return nullptr;
        }

        if (!m_ttl.isZero() && now.durationSince(it->second.m_insertedAt) > m_ttl) {
            m_storage.erase(it);
            return nullptr;
        }

        it->second.m_usedAt = Instant::now();
        return &it->second.m_value;
    }

private:
    struct Entry {
        V m_value;
        Instant m_insertedAt;
        Instant m_usedAt;
    };

    Storage<K, Entry> m_storage;
    Duration m_ttl{};
    Duration m_workInterval{};
    Instant m_lastWork{};
    size_t m_maxEntries = 0;

    void evictOne() {
        if (m_storage.empty()) return;

        auto oldest = m_storage.begin();
        for (auto it = m_storage.begin(); it != m_storage.end(); ++it) {
            if (it->second.m_usedAt < oldest->second.m_usedAt) {
                oldest = it;
            }
        }
        m_storage.erase(oldest);
    }

    void doWork() {
        m_lastWork = Instant::now();
        if (m_ttl.isZero()) return;

        for (auto it = m_storage.begin(); it != m_storage.end();) {
            if (m_lastWork.durationSince(it->second.m_insertedAt) > m_ttl) {
                it = m_storage.erase(it);
                continue;
            }
            ++it;
        }
    }
};

}
