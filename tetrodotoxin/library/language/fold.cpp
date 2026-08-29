// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/fold.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constants/aggregate.hpp"
#include "ttx/bootstrap/concept/constant.hpp"
#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/model/layouts/named.h"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

class FoldLayoutVisitor {
 public:
  explicit FoldLayoutVisitor(
      Memory::Managed::Vector<const ttx_abstract*>& entries)
      : callable{&operations}, operations{.call = append}, entries(entries) {}

  ttx_abstract_callable callable;
  ttx_abstract_callable_operations operations;
  Memory::Managed::Vector<const ttx_abstract*>& entries;

  static auto append(ttx_abstract_callable* callable, const ttx_abstract* entry)
      -> void {
    reinterpret_cast<FoldLayoutVisitor*>(callable)->entries.insert(entry);
  }
};

class FoldNamedLayoutVisitor {
 public:
  FoldNamedLayoutVisitor(
      Memory::Managed::Vector<const ttx_abstract*>& entries,
      Memory::Managed::Vector<Core::View::Bytes>& names)
      : callable{&operations},
        operations{.call = append},
        entries(entries),
        names(names) {}

  ttx_named_abstract_callable callable;
  ttx_named_abstract_callable_operations operations;
  Memory::Managed::Vector<const ttx_abstract*>& entries;
  Memory::Managed::Vector<Core::View::Bytes>& names;

  static auto append(
      ttx_named_abstract_callable* callable,
      perimortem_bytes name,
      const ttx_abstract* entry) -> void {
    auto& self = *reinterpret_cast<FoldNamedLayoutVisitor*>(callable);
    self.entries.insert(entry);
    self.names.insert(Core::View::Bytes(name.data, name.size));
  }
};

class FoldEntrySelector {
 public:
  explicit FoldEntrySelector(Count requested)
      : callable{&operations},
        operations{.call = select},
        requested(requested) {}

  ttx_abstract_callable callable;
  ttx_abstract_callable_operations operations;
  Count requested;
  Count current = 0;
  const ttx_abstract* selected = nullptr;

  static auto select(ttx_abstract_callable* callable, const ttx_abstract* entry)
      -> void {
    auto& self = *reinterpret_cast<FoldEntrySelector*>(callable);
    if (self.current == self.requested) {
      self.selected = entry;
    }
    self.current++;
  }
};

auto Language::query_fold(const Model::Pack& source) -> const Abstract& {
  auto identity = source.get_identity();
  if (!identity) {
    return Unknown::get_unknown();
  }
  if (Ttx::Concept::Constant::prove(*identity)) {
    return *identity;
  }
  return identity->resolve_concept("fold"_view);
}

auto Language::query_folded_pack(Model::Pack& source)
    -> Core::Option<Model::Pack&> {
  const Abstract& answer = query_fold(source);
  BAIL_IF(!Ttx::Concept::Constant::prove(answer));
  return Model::Pack::from(const_cast<Abstract&>(answer));
}

auto Language::query_folded_pack(const Model::Pack& source)
    -> Core::Option<const Model::Pack&> {
  const Abstract& answer = query_fold(source);
  BAIL_IF(!Ttx::Concept::Constant::prove(answer));
  return Model::Pack::from(answer);
}

auto Language::query_folded_pack(
    Memory::Allocator::Arena& domain,
    Model::Pack& source) -> Core::Option<Model::Pack&> {
  auto direct = query_folded_pack(source);
  if (direct) {
    return const_cast<Model::Pack&>(*direct);
  }

  const ttx_layout* layout = source.get_layout().get_abi();
  Memory::Managed::Vector<const ttx_abstract*> producers(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);
  ttx_named_layout_view named_layout;
  Bool named = Bool(ttx_named_layout_prove(layout, &named_layout));
  if (named) {
    FoldNamedLayoutVisitor visitor(producers, names);
    ttx_named_layout_visit(&named_layout, &visitor.callable);
  } else {
    FoldLayoutVisitor visitor(producers);
    ttx_layout_visit(layout, &visitor.callable);
  }
  Memory::Managed::Vector<Model::Pack*> constants(domain);
  constants.reset(producers.get_size());
  for (Count index = 0; index < producers.get_size(); index++) {
    const Abstract& producer = Abstract::from_abi(producers[index]);
    auto producer_pack = Model::Pack::from(const_cast<Abstract&>(producer));
    BAIL_IF(!producer_pack);
    auto folded = query_folded_pack(*producer_pack);
    BAIL_IF(!folded);

    Count occurrence = 0;
    for (Count prior = 0; prior < index; prior++) {
      occurrence += producers[prior] == producers[index] ? 1 : 0;
    }
    FoldEntrySelector selector(occurrence);
    ttx_layout_visit(folded->get_layout().get_abi(), &selector.callable);
    BAIL_IF(selector.selected == nullptr);
    const Abstract& selected = Abstract::from_abi(selector.selected);
    BAIL_IF(!Ttx::Concept::Constant::prove(selected));
    auto selected_pack = Model::Pack::from(const_cast<Abstract&>(selected));
    BAIL_IF(!selected_pack);
    FoldEntrySelector count(Count(-1));
    ttx_layout_visit(selected_pack->get_layout().get_abi(), &count.callable);
    BAIL_IF(count.current != 1);
    constants.insert(&*selected_pack);
  }
  auto aggregate = Constants::Aggregate::create(
      domain, constants.get_view(),
      named ? names.get_view() : Core::View::Vector<Core::View::Bytes>());
  return aggregate ? Core::Option<Model::Pack&>(*aggregate)
                   : Core::Option<Model::Pack&>();
}

auto Language::fold_answer(Core::Option<const Model::Pack&> result)
    -> const Abstract& {
  if (!result) {
    return Ttx::Concept::None::get_none();
  }
  auto identity = result->get_identity();
  return identity && Ttx::Concept::Constant::prove(*identity)
             ? *identity
             : static_cast<const Abstract&>(Ttx::Concept::None::get_none());
}

auto Language::fold_answer(Core::Option<Model::Pack&> result)
    -> const Abstract& {
  return result ? fold_answer(Core::Option<const Model::Pack&>(*result))
                : static_cast<const Abstract&>(Ttx::Concept::None::get_none());
}
