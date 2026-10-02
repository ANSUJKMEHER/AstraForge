#include "af/ecs/SparseSet.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

Entity MakeEntity(uint32_t id, uint32_t generation = 1) {
    return Entity{id, generation};
}

}  // namespace

AF_TEST("sparse set add, get, has") {
    SparseSet<float> s;
    AF_CHECK_EQ(s.Size(), 0u);
    s.Add(MakeEntity(0), 1.5f);
    s.Add(MakeEntity(5), 2.5f);
    AF_CHECK(s.Has(0));
    AF_CHECK(s.Has(5));
    AF_CHECK(!s.Has(1));
    AF_CHECK_NEAR(s.Get(0), 1.5f, 0.0f);
    AF_CHECK_NEAR(s.Get(5), 2.5f, 0.0f);
    AF_CHECK_EQ(s.Size(), 2u);
}

AF_TEST("sparse set overwrites on re-add without growing") {
    SparseSet<float> s;
    s.Add(MakeEntity(3), 1.0f);
    s.Add(MakeEntity(3), 9.0f);
    AF_CHECK_EQ(s.Size(), 1u);
    AF_CHECK_NEAR(s.Get(3), 9.0f, 0.0f);
}

AF_TEST("sparse set removal keeps remaining elements intact") {
    SparseSet<float> s;
    for (uint32_t i = 0; i < 5; ++i) s.Add(MakeEntity(i), static_cast<float>(i));
    s.Remove(1);  // middle removal triggers swap-remove
    AF_CHECK(!s.Has(1));
    AF_CHECK_EQ(s.Size(), 4u);
    for (uint32_t i : {0u, 2u, 3u, 4u}) {
        AF_CHECK_NEAR(s.Get(i), static_cast<float>(i), 0.0f);
    }
    // entities_ and dense_ must stay in sync after swap-remove.
    AF_CHECK_EQ(s.Entities().size(), 4u);
    for (std::size_t j = 0; j < s.Entities().size(); ++j) {
        AF_CHECK_NEAR(s.Get(s.Entities()[j].id), s.Dense()[j], 0.0f);
    }
}

AF_TEST("sparse set removal is idempotent and re-add works after remove") {
    SparseSet<int> s;
    s.Add(MakeEntity(2), 7);
    s.Remove(2);
    s.Remove(2);  // no-op
    AF_CHECK_EQ(s.Size(), 0u);
    s.Add(MakeEntity(2), 8);
    AF_CHECK_EQ(s.Get(2), 8);
}

AF_TEST("sparse set stores full entity handles") {
    SparseSet<float> s;
    s.Add(MakeEntity(4, 3), 0.5f);
    AF_CHECK_EQ(s.EntityAt(0).id, 4u);
    AF_CHECK_EQ(s.EntityAt(0).generation, 3u);
}
