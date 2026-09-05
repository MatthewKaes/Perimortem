// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_CROSS_LANGUAGE_CPP_TTX_HPP
#define VALIDATION_CROSS_LANGUAGE_CPP_TTX_HPP

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "ttx/abi.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace TtxTest {

using Route = std::vector<uint8_t>;
using Routes = std::vector<std::pair<Route, ttx_abstract>>;
using Invocation =
    std::function<void(ttx_abstract, ttx_pack, ttx_context, ttx_pack_result)>;

struct InterfaceModel {
  ttx_abstract requirement = {};
  ttx_abstract candidate = {};
  ttx_interface_relation relation = TTX_INTERFACE_REJECTED;
  Invocation invoke;
};

class AbstractModel : public Ttx::Abstract {
 public:
  using Ttx::Abstract::Abstract;

  virtual auto concepts() const -> Routes;
  void visit_concepts(ttx_concept_sink result) const override;
};

struct AliasBinding {
  std::shared_ptr<Ttx::Alias> owner;
  ttx_abstract abstract = {};
  ttx_abstract target = {};
};

auto retain_abstract(std::shared_ptr<Ttx::Abstract> model) -> ttx_abstract;
auto redispatch(ttx_abstract original) -> ttx_abstract;
auto make_alias(ttx_abstract target) -> AliasBinding;
auto alias_target(const AliasBinding& alias) -> ttx_abstract;
auto spot_resolve(ttx_abstract value) -> ttx_abstract;
auto resolve_concept(ttx_abstract value, ttx_borrowed_bytes route)
    -> ttx_abstract;
auto relation(ttx_abstract candidate, ttx_abstract requirement)
    -> ttx_interface_relation;
void answer_interface(InterfaceModel model, ttx_interface_sink result);
void forward_interface(
    ttx_abstract source,
    ttx_abstract visible_candidate,
    ttx_abstract requirement,
    ttx_interface_sink result);
void forward_bytes(
    ttx_abstract source,
    ttx_abstract candidate,
    ttx_bytes_result result);
void invoke(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result);
void return_pack(ttx_context context, Routes entries, ttx_pack_result result);
auto retain_pack(ttx_context context, Routes entries) -> ttx_pack;
auto pack_cardinality(ttx_pack pack) -> std::optional<uint64_t>;

}  // namespace TtxTest

#endif
