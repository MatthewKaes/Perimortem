// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Ttx::Model::Callable;
using Ttx::Model::Type;

static constexpr Ttx::Model::Layouts::Named incomplete_layout;

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

static auto callable_receives_self(
    const Callable& callable,
    Option<const Type&> receiver = {}) -> Bool {
  auto first = callable.get_parameters().get_abstract(0);
  if (!first) {
    return False;
  }

  return first->visit<Ttx::Model::Addressable>(
      [&](const Ttx::Model::Addressable& parameter) {
        if (parameter.get_name() != "self"_view) {
          return False;
        }

        return receiver.visit(
            []() { return True; },
            [&](const Type& expected) {
              return Bool(&parameter.get_type() == &expected);
            });
      },
      [](const Abstract&) { return False; });
}

static auto resolve_externally_reachable_type(
    const Tetrodotoxin::Library::Language::Access::Type& access,
    const Abstract& external_context,
    const Monograph& source) -> const Abstract& {
  const Abstract& external = access.resolve(external_context);
  if (&external != &Invalid::get_invalid()) {
    return external;
  }

  const Abstract& intrinsic =
      source.get_library_host().resolve_intrinsic(access.get_root());
  return access.resolve_from(intrinsic);
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

        Bool receives_self =
            callable_receives_self(callable, function.get_host());

        for (Count i = 0; i < signature->get_parameter_size(); i++) {
          if (i == 0 && receives_self) {
            continue;
          }

          auto type = signature->get_parameter_type(i);
          auto access = signature->get_parameter_type_access(i);
          if (!type || !access) {
            return signature->get_parameter_type_anchor(i);
          }

          const Abstract& reachable = resolve_externally_reachable_type(
              *access, external_context, source);
          if (&reachable != &*type) {
            return signature->get_parameter_type_anchor(i);
          }
        }
        for (Count i = 0; i < signature->get_result_size(); i++) {
          auto type = signature->get_result_type(i);
          auto access = signature->get_result_type_access(i);
          if (!type || !access) {
            return signature->get_result_type_anchor(i);
          }

          const Abstract& reachable = resolve_externally_reachable_type(
              *access, external_context, source);
          if (&reachable != &*type) {
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
      addressable_bindings(domain),
      external_addressable_bindings(domain),
      layout_fields(domain),
      callables(domain),
      callable_observations(domain),
      public_callables(domain),
      callable_bindings(domain),
      external_callable_bindings(domain),
      type_bindings(domain),
      external_type_bindings(domain),
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
  structure.layout = domain.construct<Ttx::Model::Layouts::Named>();
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
      return Object::create_authored(
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
    if (contains_field_name(structure.field_sources, field->get_name())) {
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

static auto get_binding_target(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return get_binding_target(alias.get_target());
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

static auto contains_binding_name(
    View::Vector<Reference<const Abstract>> bindings,
    View::Bytes name) -> Bool {
  for (Count i = 0; i < bindings.get_size(); i++) {
    if (bindings.get_data()[i].get().get_name() == name) {
      return True;
    }
  }

  return False;
}

static auto contains_binding(
    View::Vector<Reference<const Abstract>> bindings,
    const Abstract& binding) -> Bool {
  for (Count i = 0; i < bindings.get_size(); i++) {
    if (&bindings.get_data()[i].get() == &binding) {
      return True;
    }
  }

  return False;
}

static auto find_binding(
    View::Vector<Reference<const Abstract>> bindings,
    View::Bytes name) -> const Abstract& {
  Count selected = bindings.get_size();
  for (Count i = 0; i < bindings.get_size(); i++) {
    const Abstract& candidate = bindings.get_data()[i].get();
    if (candidate.get_name() != name) {
      continue;
    }
    if (selected != bindings.get_size()) {
      return Invalid::get_invalid();
    }

    selected = i;
  }

  return selected == bindings.get_size() ? Invalid::get_invalid()
                                         : bindings.get_data()[selected].get();
}

static auto find_field(
    View::Vector<Reference<const Field>> fields,
    View::Bytes name) -> const Abstract& {
  for (Count i = 0; i < fields.get_size(); i++) {
    const Field& field = fields.get_data()[i].get();
    if (field.get_name() == name) {
      return field;
    }
  }

  return Invalid::get_invalid();
}

auto Types::Structure::can_bind_static(const Abstract& candidate) const
    -> Bool {
  if (!is_source() ||
      (stage != Stage::Authored && stage != Stage::InitializersLinked)) {
    return False;
  }

  return can_bind(candidate);
}

auto Types::Structure::can_bind(const Abstract& binding) const -> Bool {
  View::Bytes candidate = binding.get_name();
  if (candidate.is_empty() || candidate == "source"_view ||
      contains_binding(static_binding_order, binding)) {
    return False;
  }

  const Abstract& target = get_binding_target(binding);
  return target.visit<Callable>(
      [&](const Callable& callable) {
        if (&target == &binding && !callable_has_host(callable, *this)) {
          return False;
        }

        return True;
      },
      [&](const Abstract& possible_type_or_addressable) {
        return possible_type_or_addressable.visit<Type>(
            [&](const Type&) -> Bool {
              if (contains_binding_name(type_bindings, candidate)) {
                return False;
              }
              if (!is_source()) {
                return True;
              }

              const Abstract& outer =
                  source.get_interpretation_context().resolve_context(
                      candidate);
              const Abstract& intrinsic =
                  source.get_library_host().resolve_intrinsic(candidate);
              return Bool(
                  &outer == &Invalid::get_invalid() &&
                  &intrinsic == &Invalid::get_invalid());
            },
            [&](const Abstract& possible_addressable) {
              return possible_addressable.visit<Ttx::Model::Addressable>(
                  [&](const Ttx::Model::Addressable&) {
                    return Bool(
                        !contains_binding_name(
                            addressable_bindings, candidate) &&
                        !contains_field_name(field_sources, candidate));
                  },
                  [](const Abstract&) { return False; });
            });
      });
}

auto Types::Structure::bind_static(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  if (!is_source() ||
      (stage != Stage::Authored && stage != Stage::InitializersLinked)) {
    return False;
  }

  return bind(binding, binding_visibility);
}

auto Types::Structure::bind(Abstract& binding, Visibility binding_visibility)
    -> Bool {
  if (!can_bind(binding)) {
    return False;
  }

  const Abstract& target = get_binding_target(binding);
  Bool callable = target.is<Callable>();
  Bool type = target.is<Type>();
  Bool addressable = target.is<Ttx::Model::Addressable>();
  if (!callable && !type && !addressable) {
    return False;
  }

  Reference<const Abstract> identity(binding);
  if (callable) {
    callable_bindings.insert(identity);
    if (binding_visibility == Visibility::Public) {
      external_callable_bindings.insert(identity);
    }
  } else if (type) {
    type_bindings.insert(identity);
    if (binding_visibility == Visibility::Public) {
      external_type_bindings.insert(identity);
    }
  } else {
    addressable_bindings.insert(identity);
    if (binding_visibility == Visibility::Public) {
      external_addressable_bindings.insert(identity);
    }
  }

  static_binding_order.insert(binding);
  if (binding_visibility == Visibility::Public) {
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

auto Types::Structure::retain_field(Field::Source field) -> Bool {
  if (!is_source() ||
      (stage != Stage::Authored && stage != Stage::InitializersLinked) ||
      contains_field_name(field_sources, field.get_name()) ||
      contains_binding_name(addressable_bindings, field.get_name())) {
    return False;
  }

  field_sources.insert(field);
  stage = Stage::Authored;
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
    const Tetrodotoxin::Library::Language::Access::Type& access =
        field_source.get_type_access();
    const Abstract& root = resolve_internal_type_context(access.get_root());
    const Abstract& selected = access.resolve_from(root);
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

  // Every exact Type settles before the Structure publishes a Field. The
  // Layout and external observation borrow those same identities, so a failed
  // route cannot leave a second member model behind.
  for (Count i = 0; i < linked_fields.get_size(); i++) {
    Field& field = linked_fields[i].get();
    fields.insert(field);
    field_observations.insert(field);
    if (field.is_readable_externally()) {
      public_fields.insert(field);
    }

    if (is_source()) {
      addressable_bindings.insert(field);
      static_binding_order.insert(field);
      if (field.is_readable_externally()) {
        external_addressable_bindings.insert(field);
        external_static_binding_order.insert(field);
      }
    } else {
      layout_fields.insert(field);
    }
  }

  if (!is_source()) {
    layout =
        domain.construct<Ttx::Model::Layouts::Named>(layout_fields.get_view());
  }
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
    Callable& callable = callables[i].get();
    Bool linked = link_signature(callable);
    failed |= !linked;
    if (!linked || !is_source() || !callable_receives_self(callable, *this)) {
      continue;
    }

    auto callable_anchor = callable.visit<Function>(
        [](const Function& function) -> Option<Anchor> {
          Token name = function.get_name_token();
          return name ? Option<Anchor>(Anchor::create(Span(name)))
                      : Option<Anchor>();
        },
        [](const Abstract&) -> Option<Anchor> { return {}; });
    source.report(
        callable_anchor,
        "A top level Library Function cannot receive `self`."_view,
        "Remove `self` from the top level Function signature."_view);
    failed = True;
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
        field.get_type_access(), external_source, source);
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

auto Types::Structure::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& type = find_binding(external_type_bindings, route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  if (is_source()) {
    return find_binding(external_addressable_bindings, route);
  }

  if (stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  return find_field(public_fields, route);
}

auto Types::Structure::resolve_context(
    View::Bytes route,
    const Callable& requester) const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  if (!is_source()) {
    return resolve_internal_type_context(route);
  }

  // Root signatures consume Type routes before their linked state becomes
  // visible. Body Expressions enter afterward and may select source Fields,
  // preserving category coexistence without another lookup context object.
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

  return resolve_internal_type_context(route);
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

auto Types::Structure::grants_complete_access(const Abstract& requester) const
    -> Bool {
  return requester.visit<Callable>(
      [&](const Callable& callable) { return owns(callable); },
      [&](const Abstract& possible_field_or_source) {
        return possible_field_or_source.visit<Field>(
            [&](const Field& field) { return owns(field); },
            [&](const Abstract& possible_source) {
              return possible_source.visit<Monograph>(
                  [&](const Monograph& selected) {
                    return Bool(is_source() && &selected == &source);
                  },
                  [](const Abstract&) { return False; });
            });
      });
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
  const Abstract& member = is_source()
                               ? find_binding(addressable_bindings, route)
                               : find_field(field_observations, route);
  if (&member != &Invalid::get_invalid()) {
    return member;
  }

  const Abstract& type = find_binding(type_bindings, route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  if (source_scope) {
    return source_scope->get().visit<Structure>(
        [&](const Structure& structure) -> const Abstract& {
          return structure.resolve_internal_type_context(route);
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

auto Types::Structure::resolve_internal_type_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& type = find_binding(type_bindings, route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  if (source_scope) {
    return source_scope->get().visit<Structure>(
        [&](const Structure& structure) -> const Abstract& {
          return structure.resolve_internal_type_context(route);
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
auto Types::Structure::get_layout() const -> const Ttx::Model::Layouts::Named& {
  return layout.visit(
      []() -> const Ttx::Model::Layouts::Named& { return incomplete_layout; },
      [](const Ttx::Model::Layouts::Named& selected)
          -> const Ttx::Model::Layouts::Named& { return selected; });
}

auto Types::Structure::is_readable(
    const Field& field,
    const Abstract& requester) const -> Bool {
  if (!owns(field)) {
    return False;
  }

  return grants_complete_access(requester) || field.is_readable_externally();
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

auto Types::Structure::get_callable_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return external_callable_bindings;
}

auto Types::Structure::get_callable_bindings(const Abstract& requester) const
    -> View::Vector<Reference<const Abstract>> {
  if (grants_complete_access(requester)) {
    return callable_bindings;
  }

  return external_callable_bindings;
}

auto Types::Structure::get_static_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return static_binding_order;
}

auto Types::Structure::get_external_static_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return external_static_binding_order;
}
