// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Ttx::Model::Addressable;
using Ttx::Model::Callable;
using Ttx::Model::Type;
namespace Layouts = Ttx::Model::Layouts;

static const Layouts::Fluid incomplete_layout;

enum class DefinitionKind : Unsigned_8 {
  Structure,
  Object,
};

struct ParsedStructuredDefinition {
  Visibility visibility;
  View::Bytes name;
  DefinitionKind kind;
  Token opening;
  Token kind_token;
  Anchor name_anchor;
};

static auto parse_visibility(Cursor& cursor) -> Option<Visibility> {
  if (cursor.matches(Code::Type::Public)) {
    cursor.consume();
    return Visibility::Public;
  }
  if (cursor.matches(Code::Type::Private)) {
    cursor.consume();
    return Visibility::Private;
  }

  cursor.create_token_error(
      "Library Structure declarations require `public` or `private` "
      "visibility."_view);
  return {};
}

static auto parse_structured_definition(Cursor& cursor)
    -> Option<ParsedStructuredDefinition> {
  Token opening = cursor.current();
  auto visibility = parse_visibility(cursor);
  if (!visibility) {
    return {};
  }

  Token name_token = cursor.require(
      Code::Type::Type,
      "Library Structures require an authored Type shaped name."_view);
  if (!name_token) {
    return {};
  }
  if (!cursor.require(
          Code::Type::Define,
          "Library Structure names require `:` before their kind."_view)) {
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Addressable,
      "Library Structure declarations require `struct` or `object`."_view);
  if (!kind_token) {
    return {};
  }

  View::Bytes kind_text = kind_token.caculate_text(cursor.get_source_text());
  DefinitionKind kind = DefinitionKind::Structure;
  if (kind_text == "object"_view) {
    kind = DefinitionKind::Object;
  } else if (kind_text != "struct"_view) {
    cursor.create_token_error(
        kind_token,
        "Library Structure declarations require `struct` or `object`."_view);
    return {};
  }
  if (!cursor.require(
          Code::Type::ScopeStart,
          "Library Structure bodies require an opening `{`."_view)) {
    return {};
  }

  return ParsedStructuredDefinition{
    .visibility = *visibility,
    .name = name_token.caculate_text(cursor.get_source_text()),
    .kind = kind,
    .opening = opening,
    .kind_token = kind_token,
    .name_anchor = Anchor::create(Span(name_token)),
  };
}

static auto contains_field_name(
    View::Vector<Field::Source> fields,
    View::Bytes name) -> Bool {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields.get_data()[i].get_name() == name) {
      return True;
    }
  }

  return False;
}

static auto callable_has_host(
    const Callable& callable,
    const Type& expected_host) -> Bool {
  return callable.visit<Function>(
      [&](const Function& function) {
        return Bool(&function.get_host() == &expected_host);
      },
      [](const Abstract&) { return False; });
}

static auto resolve_externally_reachable_type(
    View::Bytes route,
    const Abstract& external_context,
    const Monograph& source) -> const Abstract& {
  const Abstract& external = external_context.resolve_context(route).resolve();
  if (&external != &Invalid::get_invalid()) {
    return external;
  }

  return source.get_library_host().resolve_intrinsic(route).resolve();
}

static auto callable_exposes_unreachable_type(
    const Callable& callable,
    const Abstract& external_context,
    const Monograph& source) -> Option<Anchor> {
  return callable.visit<Function>(
      [&](const Function& function) -> Option<Anchor> {
        auto signature = function.get_signature();
        if (!signature) {
          return function.get_name_token()
                     ? Option<Anchor>(
                           Anchor::create(Span(function.get_name_token())))
                     : Option<Anchor>();
        }

        for (Count i = 0; i < signature->get_parameter_size(); i++) {
          auto type = signature->get_parameter_type(i);
          const Abstract& reachable = resolve_externally_reachable_type(
              signature->get_parameter_type_route(i), external_context, source);
          if (!type || &reachable != &*type) {
            return signature->get_parameter_type_anchor(i);
          }
        }
        for (Count i = 0; i < signature->get_result_size(); i++) {
          auto type = signature->get_result_type(i);
          const Abstract& reachable = resolve_externally_reachable_type(
              signature->get_result_type_route(i), external_context, source);
          if (!type || &reachable != &*type) {
            return signature->get_result_type_anchor(i);
          }
        }

        return {};
      },
      [](const Abstract&) -> Option<Anchor> { return {}; });
}

static auto link_signature(Callable& callable) -> Bool {
  return callable.visit<Function>(
      [](Function& function) { return function.link_signature(); },
      [](Abstract&) { return True; });
}

static auto link_body(Callable& callable) -> Bool {
  return callable.visit<Function>(
      [](Function& function) { return function.link_body(); },
      [](Abstract&) { return True; });
}

static auto finalize_callable(Callable& callable) -> Bool {
  return callable.visit<Function>(
      [](Function& function) { return function.finalize(); },
      [](Abstract&) { return True; });
}

Types::Structure::Structure(
    Allocator::Arena& domain,
    View::Bytes name,
    const Documentation& documentation,
    Visibility visibility,
    Monograph& source,
    Materializations& materializations,
    Option<Reference<const Type>> source_scope,
    Option<Anchor> anchor,
    Option<Anchor> name_anchor)
    : domain(domain),
      name(name),
      documentation(documentation),
      visibility(visibility),
      source(source),
      materializations(materializations),
      source_scope(source_scope),
      anchor(anchor),
      name_anchor(name_anchor),
      field_sources(domain),
      fields(domain),
      field_observations(domain),
      public_fields(domain),
      layout_fields(domain),
      callables(domain),
      callable_observations(domain),
      public_callables(domain),
      static_bindings(domain),
      external_static_bindings(domain),
      member_bindings(domain),
      external_member_bindings(domain),
      static_binding_order(domain),
      external_static_binding_order(domain) {}

auto Types::Structure::create_synthetic(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Monograph& source,
    Materializations& materializations) -> Structure& {
  Structure& structure = domain.construct_from<Structure>([&]() -> Structure {
    return Structure(
        domain, "source"_view, documentation, Visibility::Public, source,
        materializations, {}, {}, {});
  });

  // Source has no instance state, so its complete empty Layout exists before
  // any declaration receives it as a host. Static bindings may then accumulate
  // without creating a second scope or changing the Type identity.
  structure.layout = domain.construct<Layouts::Structured>();
  structure.stage = Stage::InitializersLinked;
  return structure;
}

auto Types::Structure::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Monograph& source,
    Materializations& materializations) -> Option<Structure&> {
  auto transaction = cursor.branch();
  auto definition = parse_structured_definition(transaction);
  if (!definition) {
    return {};
  }

  auto source_scope = source.get_source().visit<Structure>(
      [](Structure& structure) -> Option<Structure&> { return structure; },
      [](Abstract&) -> Option<Structure&> { return {}; });
  if (!source_scope) {
    transaction.create_token_error(
        "Library declarations require one synthetic source Structure."_view);
    return {};
  }

  Anchor preliminary = Anchor::create(
      definition->kind_token,
      Span(definition->opening, definition->kind_token));
  auto reserve_exact_type = [&]() -> Structure& {
    if (definition->kind == DefinitionKind::Object) {
      return Object::create(
          domain, definition->name, documentation, definition->visibility,
          source, materializations, *source_scope, preliminary,
          definition->name_anchor);
    }

    return domain.construct_from<Structure>([&]() -> Structure {
      return Structure(
          domain, definition->name, documentation, definition->visibility,
          source, materializations, Reference<const Type>(*source_scope),
          preliminary, definition->name_anchor);
    });
  };

  // The discriminator chooses the exact final Type before shared body grammar.
  // Nested Functions retain that Arena address as their host. A rejected body
  // leaves the private branch unreachable from the source inventory.
  Structure& structure = reserve_exact_type();
  while (!transaction.matches(Code::Type::ScopeEnd)) {
    if (transaction.matches(Code::Type::Terminal)) {
      transaction.create_token_error(
          "Library Structure body reached the end of source before `}`."_view);
      return {};
    }

    const Documentation& member_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(transaction);
    if ((transaction.matches(Code::Type::Public) ||
         transaction.matches(Code::Type::Private)) &&
        transaction.peek(1).get_code() == Code::Type::Func) {
      auto function = Function::reserve(
          domain, transaction, member_documentation, source, structure,
          materializations);
      if (!function || !function->complete(transaction)) {
        return {};
      }
      if (!structure.bind_callable(*function, function->get_visibility())) {
        transaction.create_token_error(
            function->get_name_token(),
            "Duplicate member name in one Library Structure."_view);
        return {};
      }

      continue;
    }

    auto field = Field::interpret(
        domain, materializations, transaction, member_documentation, structure);
    if (!field) {
      return {};
    }
    if (contains_field_name(structure.field_sources, field->get_name()) ||
        structure.static_bindings.contains(field->get_name())) {
      transaction.create_token_error(
          field->get_anchor().get_token(),
          "Duplicate member name in one Library Structure."_view);
      return {};
    }

    structure.field_sources.insert(*field);
  }

  Token closing = transaction.consume();
  structure.complete_declaration(
      Anchor::create(
          definition->kind_token, Span(definition->opening, closing)));
  cursor.join(transaction);
  return structure;
}

auto Types::Structure::complete_declaration(Anchor complete_anchor) -> void {
  anchor = complete_anchor;
}

auto Types::Structure::can_bind_static(View::Bytes candidate) const -> Bool {
  if (!is_source() || stage != Stage::InitializersLinked) {
    return False;
  }

  return can_bind(candidate);
}

auto Types::Structure::can_bind(View::Bytes candidate) const -> Bool {
  if (candidate.is_empty() || candidate == "source"_view ||
      static_bindings.contains(candidate) ||
      contains_field_name(field_sources, candidate)) {
    return False;
  }

  if (!is_source()) {
    return True;
  }

  const Abstract& outer =
      source.get_interpretation_context().resolve_context(candidate);
  const Abstract& intrinsic =
      source.get_library_host().resolve_intrinsic(candidate);
  return &outer == &Invalid::get_invalid() &&
         &intrinsic == &Invalid::get_invalid();
}

auto Types::Structure::bind_static(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  if (!is_source() || stage != Stage::InitializersLinked) {
    return False;
  }

  return bind(binding, binding_visibility);
}

auto Types::Structure::bind(Abstract& binding, Visibility binding_visibility)
    -> Bool {
  View::Bytes binding_name = binding.get_name();
  Bool host_authenticated = binding.visit<Callable>(
      [&](const Callable& callable) {
        return callable_has_host(callable, *this);
      },
      [](const Abstract&) { return True; });
  if (!host_authenticated || !can_bind(binding_name)) {
    return False;
  }

  // Host authentication and collision checks finish before either view moves.
  // Both views can therefore retain the same identity without partial binding.
  static_bindings.launder(binding_name, Reference<const Abstract>(binding));
  static_binding_order.insert(binding);
  if (binding_visibility == Visibility::Public) {
    external_static_bindings.launder(
        binding_name, Reference<const Abstract>(binding));
    external_static_binding_order.insert(binding);
  }

  binding.visit<Callable>(
      [&](Callable& callable) {
        callables.insert(callable);
        callable_observations.insert(callable);
        if (binding_visibility == Visibility::Public) {
          public_callables.insert(callable);
        }
      },
      [](Abstract&) {});
  return True;
}

auto Types::Structure::bind_callable(
    Callable& callable,
    Visibility callable_visibility) -> Bool {
  if (is_source() || stage != Stage::Authored ||
      !callable_has_host(callable, *this)) {
    return False;
  }

  return bind(callable, callable_visibility);
}

auto Types::Structure::link_fields() -> Bool {
  if (stage >= Stage::FieldsLinked) {
    return True;
  }
  if (stage != Stage::Authored) {
    source.report(
        anchor, "Structure Fields cannot link from this lifecycle stage."_view,
        "Begin with the complete authored Structure declaration."_view);
    return False;
  }

  Managed::Vector<Reference<Field>> linked_fields(domain);
  linked_fields.reset(field_sources.get_size());
  Bool failed = False;
  for (Count i = 0; i < field_sources.get_size(); i++) {
    Field::Source& field_source = field_sources[i];
    const Abstract& selected =
        resolve_internal_context(field_source.get_type_route());
    auto field = Field::link(domain, source, *this, field_source, selected);
    if (!field) {
      failed = True;
      continue;
    }

    linked_fields.insert(*field);
  }

  if (failed) {
    return False;
  }

  // Every exact Type settles before the Structure publishes a Field. Layout
  // and both lookup views can then borrow the same Field identities without a
  // partial graph surviving one rejected route.
  for (Count i = 0; i < linked_fields.get_size(); i++) {
    Field& field = linked_fields[i].get();
    fields.insert(field);
    field_observations.insert(field);
    layout_fields.insert(field);
    member_bindings.launder(field.get_name(), Reference<const Abstract>(field));
    if (field.is_readable_externally()) {
      public_fields.insert(field);
      external_member_bindings.launder(
          field.get_name(), Reference<const Abstract>(field));
    }
  }

  layout = domain.construct<Layouts::Structured>(layout_fields.get_view());
  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Structure::link_initializers() -> Bool {
  if (stage >= Stage::InitializersLinked) {
    return True;
  }
  if (stage != Stage::FieldsLinked) {
    source.report(
        anchor, "Structure initializers require linked Fields."_view,
        "Complete every Field Type before linking its initializer."_view);
    return False;
  }

  Bool failed = False;
  for (Count i = 0; i < fields.get_size(); i++) {
    failed |= !fields[i].get().link_initializer(source, materializations);
  }

  if (failed) {
    return False;
  }

  stage = Stage::InitializersLinked;
  return True;
}

auto Types::Structure::link_callable_signatures() -> Bool {
  if (stage >= Stage::CallableSignaturesLinked) {
    return True;
  }
  if (stage != Stage::InitializersLinked) {
    source.report(
        anchor,
        "Structure Callable signatures require linked initializers."_view,
        "Complete every retained Field Expression before Callables."_view);
    return False;
  }

  Bool failed = False;
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !link_signature(callables[i].get());
  }

  if (failed) {
    return False;
  }

  stage = Stage::CallableSignaturesLinked;
  return True;
}

auto Types::Structure::link_callable_bodies() -> Bool {
  if (stage >= Stage::CallablesLinked) {
    return True;
  }
  if (stage != Stage::CallableSignaturesLinked) {
    source.report(
        anchor, "Structure Callable bodies require linked signatures."_view,
        "Complete every nested signature before linking its body."_view);
    return False;
  }

  Bool failed = False;
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !link_body(callables[i].get());
  }

  if (failed) {
    return False;
  }

  stage = Stage::CallablesLinked;
  return True;
}

auto Types::Structure::finalize() -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }
  if (stage != Stage::CallablesLinked) {
    source.report(
        anchor, "An incomplete Structure cannot enter finalization."_view,
        "Link every Field, initializer, and Callable before finalizing."_view);
    return False;
  }

  Bool failed = False;
  const Abstract& external_source = get_external_source_context();

  // Publication follows the authored route rather than eventual Type
  // visibility. A private import may reach a public provider internally while
  // only direct public bindings and Library intrinsics remain externally legal.
  for (Count i = 0; i < public_fields.get_size(); i++) {
    const Field& field = public_fields[i].get();
    const Abstract& reachable = resolve_externally_reachable_type(
        field.get_type_route(), external_source, source);
    if (&reachable == &field.get_type()) {
      continue;
    }

    source.report(
        field.get_type_anchor(),
        "Externally readable Structure Field publishes an unreachable Type "
        "route."_view,
        "Keep the Field private or publish its authored Type route."_view);
    failed = True;
  }
  for (Count i = 0; i < public_callables.get_size(); i++) {
    const Callable& callable = public_callables[i].get();
    auto exposure =
        callable_exposes_unreachable_type(callable, external_source, source);
    if (!exposure) {
      continue;
    }

    source.report(
        exposure,
        "Externally readable Structure Callable publishes an unreachable Type "
        "route."_view,
        "Keep the Callable private or publish its authored Type route."_view);
    failed = True;
  }

  // Callable folding still runs when publication fails. Independent cache and
  // diagnostic facts therefore remain observable without admitting the Type.
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !finalize_callable(callables[i].get());
  }

  if (failed) {
    return False;
  }

  stage = Stage::Finalized;
  return True;
}

auto Types::Structure::resolve() const -> const Abstract& {
  if (!is_source() && stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Types::Structure::resolve_external_static_context(View::Bytes route) const
    -> const Abstract& {
  return external_static_bindings.visit(
      route,
      [](const Reference<const Abstract>& binding) -> const Abstract& {
        return binding.get();
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}

auto Types::Structure::resolve_external_member_context(View::Bytes route) const
    -> const Abstract& {
  if (!is_source() && stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  return external_member_bindings.visit(
      route,
      [](const Reference<const Abstract>& binding) -> const Abstract& {
        return binding.get();
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}

auto Types::Structure::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& static_binding = resolve_external_static_context(route);
  if (&static_binding != &Invalid::get_invalid()) {
    return static_binding;
  }

  return resolve_external_member_context(route);
}

auto Types::Structure::resolve_context(
    View::Bytes route,
    const Callable& requester) const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  return resolve_internal_context(route);
}

auto Types::Structure::resolve_context(
    View::Bytes route,
    const Field& requester) const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  return resolve_internal_context(route);
}

auto Types::Structure::resolve_context(
    View::Bytes route,
    const Monograph& requester) const -> const Abstract& {
  if (!is_source() || &requester != &source) {
    return resolve_context(route);
  }

  return resolve_internal_context(route);
}

auto Types::Structure::owns(const Callable& requester) const -> Bool {
  if (!callable_has_host(requester, *this)) {
    return False;
  }

  View::Vector<Reference<const Callable>> hosted = callable_observations;
  for (Count i = 0; i < hosted.get_size(); i++) {
    if (&hosted.get_data()[i].get() == &requester) {
      return True;
    }
  }

  return False;
}

auto Types::Structure::owns(const Field& requester) const -> Bool {
  const Type& expected_host = *this;
  if (&requester.get_host() != &expected_host) {
    return False;
  }

  View::Vector<Reference<const Field>> hosted = field_observations;
  for (Count i = 0; i < hosted.get_size(); i++) {
    if (&hosted.get_data()[i].get() == &requester) {
      return True;
    }
  }

  return False;
}

auto Types::Structure::get_external_source_context() const -> const Abstract& {
  return source_scope.visit(
      [&]() -> const Abstract& { return *this; },
      [](const Reference<const Type>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Types::Structure::resolve_internal_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& static_binding = static_bindings.visit(
      route,
      [](const Reference<const Abstract>& binding) -> const Abstract& {
        return binding.get();
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
  if (&static_binding != &Invalid::get_invalid()) {
    return static_binding;
  }

  const Abstract& member_binding = member_bindings.visit(
      route,
      [](const Reference<const Abstract>& binding) -> const Abstract& {
        return binding.get();
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
  if (&member_binding != &Invalid::get_invalid()) {
    return member_binding;
  }

  if (source_scope) {
    return source_scope->get().visit<Structure>(
        [&](const Structure& structure) -> const Abstract& {
          return structure.resolve_internal_context(route);
        },
        [](const Abstract&) -> const Abstract& {
          return Invalid::get_invalid();
        });
  }

  const Abstract& outer =
      source.get_interpretation_context().resolve_context(route);
  if (&outer != &Invalid::get_invalid()) {
    return outer;
  }

  return source.get_library_host().resolve_intrinsic(route);
}

auto Types::Structure::get_layout() const -> const Layout& {
  return layout.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Layouts::Structured& selected) -> const Layout& {
        return selected;
      });
}

auto Types::Structure::get_fields() const
    -> View::Vector<Reference<const Field>> {
  return field_observations;
}

auto Types::Structure::get_public_fields() const
    -> View::Vector<Reference<const Field>> {
  return public_fields;
}

auto Types::Structure::get_field_sources() const
    -> View::Vector<Field::Source> {
  return field_sources;
}

auto Types::Structure::get_callables() const
    -> View::Vector<Reference<const Callable>> {
  return callable_observations;
}

auto Types::Structure::get_public_callables() const
    -> View::Vector<Reference<const Callable>> {
  return public_callables;
}

auto Types::Structure::get_static_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return static_binding_order;
}

auto Types::Structure::get_external_static_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return external_static_binding_order;
}
