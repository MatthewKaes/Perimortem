// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/model/type.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

static auto retains_binding(
    Language::Model::Type::Callables callables,
    const Abstract& candidate) -> Bool {
  for (const Reference<Abstract>& callable : callables) {
    if (&callable.get() == &candidate) {
      return True;
    }
  }

  return False;
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
  for (const Reference<Abstract>& retained : get_callables()) {
    auto existing = retained.get().select<Language::Model::Callable>();
    if (&retained.get() == &candidate ||
        (retained.get().get_name() == candidate.get_name() && existing &&
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
    callables = Memory::Managed::Vector<Reference<Abstract>>(domain);
    published_callables = Memory::Managed::Vector<Reference<Abstract>>(domain);
  }

  callables->insert(callable);
  if (published) {
    published_callables->insert(callable);
  }
}

auto Language::Model::Type::resolve_type_call(
    const Abstract& host,
    Core::View::Bytes route,
    Access access) const -> const Abstract& {
  Bool self = access == Access::Self;
  for (const Reference<Abstract>& binding : get_callables()) {
    if (binding.get().get_name() != route) {
      continue;
    }

    auto callable = binding.get().resolve().select<Language::Model::Callable>();
    if (!callable || callable->declares_self() != self) {
      continue;
    }

    auto caller = host.select<Type>();
    Bool published = retains_binding(
        get_callables(Tetrodotoxin::Language::Visibility::Public),
        binding.get());
    if (published || (caller && caller->has_private_access_to(*this))) {
      return binding.get();
    }

    return Invalid::get_invalid();
  }

  return Invalid::get_invalid();
}
