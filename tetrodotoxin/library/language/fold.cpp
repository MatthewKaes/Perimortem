// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/fold.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constants/aggregate.hpp"
#include "ttx/concept/constant.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

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

  const Layout& layout = source.get_layout();
  Memory::Managed::Vector<const Abstract*> producers(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);
  Bool named = False;
  for (Count index = 0; index < layout.get_size(); ++index) {
    auto producer = layout.get_abstract(index);
    BAIL_IF(!producer);
    producers.insert(&*producer);
    auto name = layout.get_name(index);
    if (name) {
      named = True;
      names.insert(*name);
    } else if (named) {
      return {};
    }
  }
  BAIL_IF(named && names.get_size() != producers.get_size());
  Memory::Managed::Vector<Model::Pack*> constants(domain);
  constants.reset(producers.get_size());
  for (Count index = 0; index < producers.get_size(); index++) {
    const Abstract& producer = *producers[index];
    auto producer_pack = Model::Pack::from(const_cast<Abstract&>(producer));
    BAIL_IF(!producer_pack);
    auto folded = query_folded_pack(*producer_pack);
    BAIL_IF(!folded);

    Count occurrence = 0;
    for (Count prior = 0; prior < index; prior++) {
      occurrence += producers[prior] == &producer ? 1 : 0;
    }
    auto selected = folded->get_layout().get_abstract(occurrence);
    BAIL_IF(!selected);
    BAIL_IF(!Ttx::Concept::Constant::prove(*selected));
    auto selected_pack = Model::Pack::from(const_cast<Abstract&>(*selected));
    BAIL_IF(!selected_pack);
    BAIL_IF(selected_pack->get_layout().get_size() != 1);
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
