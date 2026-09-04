// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/cpp/root.hpp"

#include <algorithm>

class CppRoot final : public TtxTest::AbstractModel {
 public:
  explicit CppRoot(TtxTest::Routes routes) : routes(std::move(routes)) {}

  auto name() const -> ttx_borrowed_bytes override {
    static const uint8_t value[] = "C++ route host";
    return {value, sizeof(value) - 1};
  }

  auto resolve_concept(ttx_borrowed_bytes route) const
      -> ttx_abstract override {
    for (const auto& [name, value] : routes) {
      if (name.size() == route.size &&
          (route.size == 0 ||
           std::equal(name.begin(), name.end(), route.data))) {
        return value;
      }
    }
    return ttx_unknown();
  }

  auto concepts() const -> TtxTest::Routes override { return routes; }

 private:
  TtxTest::Routes routes;
};

auto TtxTest::create_root(Routes routes) -> ttx_abstract {
  return register_abstract(std::make_shared<CppRoot>(std::move(routes)));
}
