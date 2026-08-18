// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/fixed.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/builtins/get_access.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

Types::Fixed::Fixed(
    Allocator::Arena& domain,
    View::Bytes name,
    const Model::Type& element,
    ::Unsigned_64 extent,
    const Model::Type& access_type)
    : name(name),
      element(element),
      extent(extent),
      layout(element, Count(extent)) {
  auto& get_access = Builtins::GetAccess::create(domain, *this, access_type);
  publish_callable(domain, get_access, True);
}

auto Types::Fixed::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  BAIL_IF(get_extent() == 0 || get_extent() > Unsigned_64(Count(-1)));

  Managed::Vector<Reference<Model::Pack>> values(arena);
  values.reset(Count(get_extent()));
  for (Count index = 0; index < Count(get_extent()); index++) {
    auto value = get_element_type().create_default(arena);
    BAIL_IF(!value);
    values.insert(*value);
  }

  return Expressions::Initializer::create_synthetic(
      arena, *this, values.get_view());
}

static auto fold_output(Model::Pack& source, Count index)
    -> Option<Model::Pack&> {
  auto produced = source.get_produced(index);
  BAIL_IF(!produced);
  // Produced is an inspection edge, while constant fitting runs during the
  // mutable graph completion pass and may populate the Expression's fold
  // cache. The source Pack and every producer remain owned by this transaction.
  auto expression =
      const_cast<Ttx::Model::Pack&>(produced->producer).select<Expression>();
  BAIL_IF(!expression);

  Option<Model::Pack&> folded;
  expression->fold().visit(
      [&](const Option<Model::Pack&>& selected) { folded = selected; },
      [](const Expression::Error&) {});
  BAIL_IF(!folded);

  auto selected = folded->get_produced(produced->local_index);
  BAIL_IF(!selected);
  auto constant = selected->producer.select<Constant>();
  BAIL_IF(!constant);
  return const_cast<Constant&>(*constant);
}

auto Types::Fixed::create_fitted(Allocator::Arena& arena, Model::Pack& source)
    const -> Option<Model::Pack&> {
  BAIL_IF(!source.fits(*this));

  Managed::Vector<Reference<Model::Pack>> values(arena);
  values.reset(Count(get_extent()));
  for (Count index = 0; index < Count(get_extent()); index++) {
    auto value = fold_output(source, index);
    BAIL_IF(!value);
    values.insert(*value);
  }
  return Model::Pack::create_folded(arena, values.get_view());
}

auto Types::Fixed::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved = carriers.reserve_fixed(program, *this);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  Bool element_reserved = element.reserve(program);
  if (!element_reserved) {
    return False;
  }

  return reserve_callables(program);
}

auto Types::Fixed::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool completed = element.complete(program);
  if (!completed) {
    return False;
  }

  if (!complete_callables(program)) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete_fixed(program, *this, element, Count(extent));
  return carrier_completed && complete_debug(program);
}
