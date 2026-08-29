// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/type.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/bootstrap/model/layouts/fluid.hpp"
#include "ttx/bootstrap/model/layouts/named.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

Language::Model::Type::Type(Memory::Allocator::Arena& domain) {
  initialize_authorities(domain);
}

auto Language::Model::Type::initialize_authorities(
    Memory::Allocator::Arena& domain) -> void {
  if (static_authority) {
    return;
  }

  static_authority = Core::Option<Language::Types::Static*>(
      &domain.construct<Language::Types::Static>(domain));
  instance_authority = Core::Option<Language::Types::Instance*>(
      &domain.construct<Language::Types::Instance>(domain));
}

auto Language::Model::Type::edit_static_authority()
    -> Language::Types::Static& {
  return **static_authority;
}

auto Language::Model::Type::edit_instance_authority()
    -> Language::Types::Instance& {
  return **instance_authority;
}

auto Language::Model::Type::get_static_authority() const
    -> const Language::Types::Static& {
  return **static_authority;
}

auto Language::Model::Type::get_instance_authority() const
    -> const Language::Types::Instance& {
  return **instance_authority;
}

auto Language::Model::Type::resolve_concept(Core::View::Bytes route) const
    -> const Abstract& {
  if (route == "static"_view) {
    return static_authority ? static_cast<const Abstract&>(**static_authority)
                            : static_cast<const Abstract&>(None::get_none());
  }
  if (route == "instance"_view) {
    return instance_authority
               ? static_cast<const Abstract&>(**instance_authority)
               : static_cast<const Abstract&>(None::get_none());
  }
  return Ttx::Model::Type::resolve_concept(route);
}

auto Language::Model::Type::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  if (!static_authority) {
    return;
  }
  visit_concept(visitor, "static"_view, **static_authority);
  visit_concept(visitor, "instance"_view, **instance_authority);
}

auto Language::Model::Type::get_callable_bindings(
    Tetrodotoxin::Language::Visibility visibility) const -> CallableBindings {
  const auto& selected =
      visibility == Tetrodotoxin::Language::Visibility::Private
          ? callables
          : published_callables;
  return selected ? selected->get_view() : CallableBindings();
}

auto Language::Model::Type::get_callables(
    Tetrodotoxin::Language::Visibility visibility) const -> Callables {
  return Callables(get_callable_bindings(visibility));
}

auto Language::Model::Type::can_publish_callable(
    const Abstract& candidate) const -> Bool {
  auto callable = candidate.select<Language::Model::Callable>();
  if (!callable || candidate.get_name().is_empty()) {
    return False;
  }

  Bool self = callable->declares_self();
  const Bool route_available =
      self ? (!instance_authority || (**instance_authority).can_bind(candidate))
           : (!static_authority || (**static_authority).can_bind(candidate));
  BAIL_IF(!route_available);
  for (const Abstract* retained : get_callables()) {
    auto existing = retained->select<Language::Model::Callable>();
    if (retained == &candidate ||
        (retained->get_name() == candidate.get_name() && existing &&
         existing->declares_self() == self)) {
      return False;
    }
  }

  return True;
}

auto Language::Model::Type::publish_callable(
    Memory::Allocator::Arena& domain,
    Abstract& callable,
    Bool published) -> void {
  if (!can_publish_callable(callable)) {
    Core::Diagnostics::Log::fatal(
        "Library Type cannot publish a duplicate or invalid Callable."_view);
  }

  if (!callables) {
    initialize_authorities(domain);
    callables = Memory::Managed::Vector<Abstract*>(domain);
    published_callables = Memory::Managed::Vector<Abstract*>(domain);
  }

  auto selected = callable.select<Language::Model::Callable>();
  if (!selected) {
    Core::Diagnostics::Log::fatal(
        "Library Type cannot publish a non Callable identity."_view);
  }
  Bool bound = selected->declares_self()
                   ? edit_instance_authority().bind(callable, published)
                   : edit_static_authority().bind(callable, published);
  if (!bound) {
    Core::Diagnostics::Log::fatal(
        "Library Type cannot publish an occupied Callable concept."_view);
  }

  callables->insert(&callable);
  if (published) {
    published_callables->insert(&callable);
  }
}
