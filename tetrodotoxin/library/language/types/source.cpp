// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/source.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Tetrodotoxin::Language::Visibility;
using Ttx::Model::Addressable;
using Ttx::Model::Alias;
using Ttx::Model::Callable;
using Ttx::Model::Type;

auto Types::Source::create_synthetic(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Tetrodotoxin::Language::Monograph& host,
    const Anchor& source_anchor) -> Source& {
  // The root has no instance state, so its empty Layout exists before any
  // Function Definition names Source as its host. Static Fields cannot change
  // that Source owned value.
  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      domain, documentation, host, "<source>"_view, Visibility::Public,
      source_anchor);
  return domain.construct_from<Source>(
      [&]() -> Source { return Source(domain, definition); });
}

auto Types::Source::parse(Cursor& cursor) -> Bool {
  auto& monograph = static_cast<Monograph&>(get_monograph());
  while (!cursor.matches(Code::Type::Terminal)) {
    auto extension = cursor.branch();
    Tetrodotoxin::Language::Parser::Comment::parse(extension);
    if (extension.matches(Code::Type::Using)) {
      auto import = Import::parse(extension);
      if (!import || !monograph.retain_import(*import)) {
        if (import) {
          extension.create_expression_error(
              import->get_span(),
              "Library Imports cannot enter a source after linking begins."_view);
        }
        return False;
      }

      cursor.join(extension);
      continue;
    }

    auto transaction = cursor.branch();
    auto definition =
        Tetrodotoxin::Language::Definition::parse(transaction, *this);
    BAIL_IF(!definition || !interpret_definition(transaction, *definition));
    cursor.join(transaction);
  }

  return True;
}

auto Types::Source::can_bind_static(const Abstract& binding) const -> Bool {
  // A complete Type can repair a failed route without repeating Type linking.
  // Import replay similarly adds provider Fields after every local Field is
  // exact. Source owns both late Static cases without reopening an incomplete
  // declaration or changing its empty instance Layout.
  BAIL_IF(is_finalized() || !can_bind_definition(binding));

  const Abstract& target = Alias::get_represented(binding);
  Bool complete_type =
      target.is<Type>() && &target.resolve() != &Invalid::get_invalid();
  Bool imported_addressable = is_linked() && target.is<Addressable>();
  BAIL_IF(!can_accept_definition() && !complete_type && !imported_addressable);
  if (!target.is<Type>()) {
    return True;
  }

  View::Bytes name = binding.get_name();
  const auto& monograph = static_cast<const Monograph&>(get_monograph());
  const Abstract& outer =
      monograph.get_interpretation_context().resolve_context(name);
  const Abstract& intrinsic = monograph.get_dialect().resolve_intrinsic(name);
  return Bool(
      &outer == &Invalid::get_invalid() &&
      &intrinsic == &Invalid::get_invalid());
}

auto Types::Source::bind_static(Abstract& binding) -> Bool {
  BAIL_IF(!can_bind_static(binding));

  publish_binding(binding);
  return True;
}

auto Types::Source::retain_binding(Abstract& binding) -> Bool {
  return bind_static(binding);
}

auto Types::Source::complete_field_layout() -> void {}

auto Types::Source::get_layout() const -> const Ttx::Model::Layouts::Named& {
  return instance_layout;
}

auto Types::Source::validate_linked_callable(const Callable& callable) -> Bool {
  if (!callable.is_type_bound(*this)) {
    return True;
  }

  auto callable_anchor = callable.visit<Function>(
      [](const Function& function) -> Option<Anchor> {
        Token name = function.get_definition().get_name_token();
        return name ? Option<Anchor>(Anchor::create(Span(name)))
                    : Option<Anchor>();
      },
      [](const Abstract&) -> Option<Anchor> { return {}; });
  get_monograph().report(
      callable_anchor,
      "A top level Library Function cannot receive `self`."_view,
      "Remove `self` from the top level Function signature."_view);
  return False;
}

auto Types::Source::resolve_context(View::Bytes route) const
    -> const Abstract& {
  for (const Reference<Abstract>& binding : get_types(Visibility::Public)) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  for (const Reference<Abstract>& binding :
       get_addressables(Visibility::Public)) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  return Invalid::get_invalid();
}
