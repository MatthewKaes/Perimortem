// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/source.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Ttx::Model::Addressable;
using Ttx::Model::Alias;
using Ttx::Model::Callable;
using Ttx::Model::Type;

static auto get_binding_target(const Abstract& binding) -> const Abstract& {
  return binding.visit<Alias>(
      [](const Alias& alias) -> const Abstract& {
        return get_binding_target(alias.get_target());
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

static auto receives_self(const Callable& callable, const Type& source)
    -> Bool {
  auto first = callable.get_parameters().get_abstract(0);
  BAIL_IF(!first);

  return first->visit<Addressable>(
      [&](const Addressable& parameter) {
        return Bool(
            parameter.get_name() == "self"_view &&
            &parameter.get_type() == &source);
      },
      [](const Abstract&) { return False; });
}

Types::Source::Source(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Monograph& source,
    Materializations& materializations)
    : Composite(domain, source, materializations),
      documentation(documentation),
      instance_layout(domain.construct<Ttx::Model::Layouts::Named>()) {}

auto Types::Source::create_synthetic(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Monograph& source,
    Materializations& materializations) -> Source& {
  // The root has no instance state, so its empty Layout exists before any
  // Function retains Source as its host. Static Fields cannot change that
  // Source owned value.
  return domain.construct_from<Source>([&]() -> Source {
    return Source(domain, documentation, source, materializations);
  });
}

auto Types::Source::parse(Cursor& cursor) -> Bool {
  Monograph& monograph = get_source_monograph();
  while (!cursor.matches(Code::Type::Terminal)) {
    auto extension = cursor.branch();
    Tetrodotoxin::Language::Parser::Comment::parse(extension);
    if (extension.matches(Code::Type::Addressable) &&
        extension.get_text() == "using"_view) {
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
    auto definition = Tetrodotoxin::Language::Definition::parse(transaction);
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

  const Abstract& target = get_binding_target(binding);
  Bool complete_type =
      target.is<Type>() && &target.resolve() != &Invalid::get_invalid();
  Bool imported_addressable = is_linked() && target.is<Addressable>();
  BAIL_IF(!can_accept_definition() && !complete_type && !imported_addressable);
  if (!target.is<Type>()) {
    return True;
  }

  View::Bytes name = binding.get_name();
  const Monograph& monograph = get_source_monograph();
  const Abstract& outer =
      monograph.get_interpretation_context().resolve_context(name);
  const Abstract& intrinsic =
      monograph.get_library_host().resolve_intrinsic(name);
  return Bool(
      &outer == &Invalid::get_invalid() &&
      &intrinsic == &Invalid::get_invalid());
}

auto Types::Source::can_retain_binding(const Abstract& binding) const -> Bool {
  return can_bind_static(binding);
}

auto Types::Source::bind_static(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  BAIL_IF(!can_bind_static(binding));

  publish_binding(binding, binding_visibility);
  return True;
}

auto Types::Source::retain_binding(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  return get_source_monograph().bind_static(binding, binding_visibility);
}

auto Types::Source::publish_linked_field(Field& field) -> void {
  Visibility visibility =
      field.is_readable_externally() ? Visibility::Public : Visibility::Private;
  publish_binding(field, visibility);
}

auto Types::Source::complete_field_layout() -> void {}

auto Types::Source::get_layout() const -> const Ttx::Model::Layouts::Named& {
  return instance_layout;
}

auto Types::Source::validate_linked_callable(const Callable& callable) -> Bool {
  if (!receives_self(callable, *this)) {
    return True;
  }

  auto callable_anchor = callable.visit<Function>(
      [](const Function& function) -> Option<Anchor> {
        Token name = function.get_definition().get_name_token();
        return name ? Option<Anchor>(Anchor::create(Span(name)))
                    : Option<Anchor>();
      },
      [](const Abstract&) -> Option<Anchor> { return {}; });
  get_source_monograph().report(
      callable_anchor,
      "A top level Library Function cannot receive `self`."_view,
      "Remove `self` from the top level Function signature."_view);
  return False;
}

auto Types::Source::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& type = resolve_external_type_binding(route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  return resolve_external_addressable_binding(route);
}

auto Types::Source::resolve_context(
    View::Bytes route,
    const Callable& requester) const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  // Signatures admit only Type context even though source Fields are already
  // exact. A completed signature can enter the ordinary body context where
  // bare Static Addressables are legal without becoming implicit receiver
  // Fields.
  return requester.visit<Function>(
      [&](const Function& function) -> const Abstract& {
        return function.is_signature_linked()
                   ? resolve_internal_context(route)
                   : resolve_internal_type_context(route);
      },
      [&](const Abstract&) -> const Abstract& {
        return resolve_internal_type_context(route);
      });
}

auto Types::Source::resolve_context(View::Bytes route, const Field& requester)
    const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  return resolve_internal_context(route);
}

auto Types::Source::resolve_context(
    View::Bytes route,
    const Monograph& requester) const -> const Abstract& {
  if (&requester != &get_source_monograph()) {
    return resolve_context(route);
  }

  return resolve_internal_type_context(route);
}

auto Types::Source::grants_complete_access(const Abstract& requester) const
    -> Bool {
  if (Composite::grants_complete_access(requester)) {
    return True;
  }

  return requester.visit<Monograph>(
      [&](const Monograph& selected) {
        return Bool(&selected == &get_source_monograph());
      },
      [](const Abstract&) { return False; });
}

auto Types::Source::resolve_internal_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& addressable = resolve_internal_addressable_binding(route);
  if (&addressable != &Invalid::get_invalid()) {
    return addressable;
  }

  return resolve_internal_type_context(route);
}

auto Types::Source::resolve_internal_type_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& type = resolve_internal_type_binding(route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  const Monograph& monograph = get_source_monograph();
  const Abstract& outer =
      monograph.get_interpretation_context().resolve_context(route);
  if (&outer != &Invalid::get_invalid()) {
    return outer;
  }

  return monograph.get_library_host().resolve_intrinsic(route);
}

auto Types::Source::resolve_external_type_context(View::Bytes route) const
    -> const Abstract& {
  return resolve_external_type_binding(route);
}
