#include <string>

#include "af/resources/ResourceCache.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

struct Resource {
    explicit Resource(int v) : value(v) {}
    int value = 0;
};

}  // namespace

AF_TEST("cache deduplicates identical keys") {
    ResourceCache<Resource> cache;
    int loads = 0;
    auto loader = [&]() { ++loads; return Resource(7); };

    const auto a = cache.GetOrLoad("mesh/cube", loader);
    const auto b = cache.GetOrLoad("mesh/cube", loader);
    AF_CHECK(a.get() == b.get());  // same instance
    AF_CHECK_EQ(loads, 1);         // loader ran once
    AF_CHECK_EQ(a->value, 7);
    AF_CHECK_EQ(cache.Size(), 1u);
    AF_CHECK(cache.Contains("mesh/cube"));
}

AF_TEST("cache keeps distinct keys independent") {
    ResourceCache<Resource> cache;
    int loads = 0;
    auto loader = [&]() { return Resource(++loads); };

    const auto a = cache.GetOrLoad("a", loader);
    const auto b = cache.GetOrLoad("b", loader);
    AF_CHECK(a.get() != b.get());
    AF_CHECK_EQ(loads, 2);
    AF_CHECK_EQ(a->value, 1);
    AF_CHECK_EQ(b->value, 2);
}

AF_TEST("resources free when the last handle drops, and reload lazily") {
    ResourceCache<Resource> cache;
    int loads = 0;
    auto loader = [&]() { return Resource(++loads); };

    {
        const auto handle = cache.GetOrLoad("k", loader);
        AF_CHECK_EQ(loads, 1);
        AF_CHECK(cache.Contains("k"));
    }  // handle dropped → resource freed, weak entry expires

    const auto again = cache.GetOrLoad("k", loader);
    AF_CHECK_EQ(loads, 2);  // loader re-ran: the old instance was freed
    AF_CHECK_EQ(again->value, 2);
}

AF_TEST("a strong handle pins the resource while it exists") {
    ResourceCache<Resource> cache;
    int loads = 0;
    auto loader = [&]() { return Resource(++loads); };

    const auto pinned = cache.GetOrLoad("k", loader);
    {
        const auto second = cache.GetOrLoad("k", loader);
        AF_CHECK_EQ(loads, 1);  // still alive: no reload
        AF_CHECK(second.get() == pinned.get());
    }
    AF_CHECK_EQ(loads, 1);
}

AF_TEST("release drops the entry immediately") {
    ResourceCache<Resource> cache;
    int loads = 0;
    auto loader = [&]() { return Resource(++loads); };

    auto h1 = cache.GetOrLoad("k", loader);
    (void)h1;
    cache.Release("k");
    AF_CHECK(!cache.Contains("k"));
    const auto h2 = cache.GetOrLoad("k", loader);
    AF_CHECK_EQ(loads, 2);  // fresh load after explicit release
}

AF_TEST("clear drops all entries") {
    ResourceCache<Resource> cache;
    auto loader = []() { return Resource(1); };
    cache.GetOrLoad("a", loader);
    cache.GetOrLoad("b", loader);
    cache.Clear();
    AF_CHECK_EQ(cache.Size(), 0u);
    AF_CHECK(!cache.Contains("a"));
    AF_CHECK(!cache.Contains("b"));
}
