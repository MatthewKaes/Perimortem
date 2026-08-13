// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/source.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/foreign/surface.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Tetrodotoxin::Language::Visibility;

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
  Source& source = domain.construct_from<Source>(
      [&]() -> Source { return Source(domain, definition); });
  // One stable Foreign Surface belongs to the synthetic root even when the
  // source has no blocks. Later blocks append declarations to this identity.
  source.foreign = Foreign::Surface::create(domain, source);
  return source;
}

static auto is_foreign_keyword(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Addressable) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "foreign"_view;
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

    if (is_foreign_keyword(extension)) {
      BAIL_IF(!get_foreign().parse(monograph, extension));
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

auto Types::Source::link_types() -> Bool {
  BAIL_IF(!Composite::link_types());
  return get_foreign().link_types(static_cast<Monograph&>(get_monograph()));
}

auto Types::Source::link_fields() -> Bool {
  return Composite::link_fields();
}

auto Types::Source::link_initializers() -> Bool {
  return Composite::link_initializers();
}

auto Types::Source::link_callable_signatures() -> Bool {
  auto& monograph = static_cast<Monograph&>(get_monograph());
  BAIL_IF(!get_foreign().link_callables(monograph));
  return Composite::link_callable_signatures();
}

auto Types::Source::link_callable_bodies() -> Bool {
  return Composite::link_callable_bodies();
}

auto Types::Source::finalize() -> Bool {
  BAIL_IF(!Composite::finalize());
  return get_foreign().finalize(static_cast<Monograph&>(get_monograph()));
}

auto Types::Source::can_bind_static(const Abstract& binding, Category category)
    const -> Bool {
  // This query proves namespace and publication collisions independently from
  // lifecycle. Import discovery can therefore preflight future Addressables
  // before publishing any Type or Callable from the same transaction.
  BAIL_IF(is_finalized() || !can_bind_definition(binding, category, False));

  if (category != Category::Type) {
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

auto Types::Source::bind_static(Abstract& binding, Category category) -> Bool {
  // Types and Callables enter only while the source declaration is open.
  // Addressables also have one deliberate late phase after every provider
  // Field has settled, but before any initializer consumes source lookup.
  Bool addressable_phase = category == Category::Addressable && is_linked();
  BAIL_IF(
      (!can_accept_definition() && !addressable_phase) ||
      !can_bind_static(binding, category));

  publish_binding(binding, category);
  return True;
}

auto Types::Source::retain_binding(Abstract& binding, Category category)
    -> Bool {
  BAIL_IF(!can_accept_definition());

  if (category == Category::Callable) {
    auto function = binding.select<Function>();
    BAIL_IF(!function || &function->get_host() != this);
    if (function->declares_self()) {
      Token name = function->get_definition().get_name_token();
      get_monograph().report(
          name ? Option<Anchor>(Anchor::create(Span(name))) : Option<Anchor>(),
          "A top level Library Function cannot receive `self`."_view,
          "Remove `self` from the top level Function signature."_view);
      return False;
    }
  }

  return bind_static(binding, category);
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
