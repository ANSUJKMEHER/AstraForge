#pragma once
// ResourceCache<T> — named, deduplicated, lazily freed resources.
//
// Semantics (the interesting part — worth explaining in an interview):
//   - GetOrLoad(key, loader): returns a shared handle. The loader runs only
//     when the key is absent OR its previous instance has been released by
//     every user. Identical keys always resolve to the same instance while
//     any handle pins it.
//   - The cache holds weak_ptr, so it never keeps resources alive on its own:
//     dropping the last strong handle frees the resource; a later GetOrLoad
//     re-runs the loader (lazy reload). This gives manual-lifetime semantics
//     without manual "free" bookkeeping or dangling-cache bugs.
//   - Release(key) drops the cache entry immediately (next load re-creates).
//   - Clear() drops all entries.
//
// Not thread-safe (single-threaded engine; see context.md Decision 8).

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace af {

template <typename T>
class ResourceCache {
public:
    using Handle = std::shared_ptr<const T>;

    template <typename Fn>
    Handle GetOrLoad(const std::string& key, Fn&& loader) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (Handle strong = it->second.lock()) return strong;  // still alive
        }
        Handle handle = std::make_shared<const T>(std::forward<Fn>(loader)());
        cache_[key] = handle;
        return handle;
    }

    void Release(const std::string& key) { cache_.erase(key); }
    void Clear() { cache_.clear(); }
    std::size_t Size() const { return cache_.size(); }
    bool Contains(const std::string& key) const {
        return cache_.find(key) != cache_.end();
    }

private:
    std::unordered_map<std::string, std::weak_ptr<const T>> cache_;
};

}  // namespace af
