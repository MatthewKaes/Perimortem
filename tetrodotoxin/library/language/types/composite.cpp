// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/composite.hpp"

#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Ttx::Model::Callable;
using Ttx::Model::Type;

static constexpr Ttx::Model::Layouts::Named incomplete_layout;

template <typename selected_type, typename visitor_type>
static auto visit_each(
    View::Vector<Reference<Abstract>> bindings,
    visitor_type visitor) -> Bool {
  Bool failed = False;
  for (Count i = 0; i < bindings.get_size(); i++) {
    failed |= !bindings.get_data()[i].get().visit<selected_type>(
        [&](selected_type& selected) { return visitor(selected); },
        [](Abstract&) { return True; });
  }

  return !failed;
}

static auto parse_type_visibility(
    Cursor& cursor,
    const Tetrodotoxin::Language::Definition& definition)
    -> Option<Visibility> {
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Type definitions require a Type shaped name."_view);
    return {};
  }

  Count publications = 0;
  Visibility visibility = Visibility::Private;
  auto modifiers = definition.get_modifiers();
  for (Count i = 0; i < modifiers.get_size(); i++) {
    Token modifier = modifiers.get_data()[i];
    if (modifier.get_code() == Code::Type::Public) {
      visibility = Visibility::Public;
      publications++;
      continue;
    }
    if (modifier.get_code() == Code::Type::Private) {
      visibility = Visibility::Private;
      publications++;
      continue;
    }

    cursor.create_token_error(
        modifier,
        "Library Type definitions accept only visibility modifiers."_view);
    return {};
  }

  if (publications != 1) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Type definitions require one visibility modifier."_view);
    return {};
  }

  return visibility;
}

static auto get_definition_visibility(
    const Tetrodotoxin::Language::Definition& definition) -> Visibility {
  auto modifiers = definition.get_modifiers();
  for (Count i = 0; i < modifiers.get_size(); i++) {
    if (modifiers.get_data()[i].get_code() == Code::Type::Public) {
      return Visibility::Public;
    }
  }

  return Visibility::Private;
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
  BAIL_IF(!first);

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
    const Types::Composite& external_context) -> const Abstract& {
  return external_context.resolve_exported_type(access);
}

static auto callable_exposes_unreachable_type(
    const Callable& callable,
    const Types::Composite& external_context) -> Option<Anchor> {
  return callable.visit<Function>(
      [&](const Function& function) -> Option<Anchor> {
        auto signature = function.get_signature();
        if (!signature) {
          return function.get_definition().get_name_token()
                     ? Option<Anchor>(Anchor::create(
                           Span(function.get_definition().get_name_token())))
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

          const Abstract& reachable =
              resolve_externally_reachable_type(*access, external_context);
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

          const Abstract& reachable =
              resolve_externally_reachable_type(*access, external_context);
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

Types::Composite::Composite(
    Allocator::Arena& domain,
    Monograph& source,
    Materializations& materializations,
    Option<const Composite&> selected_enclosing_scope)
    : domain(domain),
      source(source),
      materializations(materializations),
      enclosing_scope(selected_enclosing_scope),
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

static auto get_binding_target(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return get_binding_target(alias.get_target());
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

auto Types::Composite::interpret_alias(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto transaction = cursor.branch();
  auto visibility = parse_type_visibility(transaction, definition);
  BAIL_IF(!visibility);

  BAIL_IF(!transaction.require(
      Code::Type::Alias,
      "Library Alias definitions require the `alias` qualifier."_view));
  BAIL_IF(!transaction.require(
      Code::Type::Assign,
      "Library Alias qualifiers require `=` before their Type route."_view));

  auto route =
      Tetrodotoxin::Library::Language::Access::Type::parse(transaction);
  BAIL_IF(!route);
  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Alias definitions require one terminating `;`."_view);
  BAIL_IF(!terminator);

  const Abstract& resolved = resolve_type(*route);
  auto target = resolved.select<Type>();
  if (!target) {
    transaction.create_expression_error(
        route->get_anchor(),
        "Library Alias Type route did not resolve to one stable Type."_view);
    return False;
  }

  const Documentation* alias_documentation = &target->get_documentation();
  if (!definition.get_documentation().is_empty()) {
    alias_documentation = &domain.construct<Ttx::Model::Documentations::Merged>(
        definition.get_documentation(), target->get_documentation());
  }

  Ttx::Model::Alias candidate(
      definition.get_name(), *target, *alias_documentation);
  if (!can_retain_binding(candidate)) {
    transaction.create_expression_error(
        definition.get_name_anchor(),
        "Library Alias name collides with an occupied Type name."_view);
    return False;
  }

  auto& alias = domain.construct<Ttx::Model::Alias>(
      definition.get_name(), *target, *alias_documentation);
  if (!retain_binding(alias, *visibility)) {
    transaction.create_expression_error(
        definition.get_name_anchor(),
        "Library Alias could not enter its Composite Type surface."_view);
    return False;
  }

  cursor.join(transaction);
  return True;
}

auto Types::Composite::interpret_definition(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  Token qualifier = definition.get_qualifier();
  Code::Type qualifier_code = qualifier.get_code().get_type();
  if (qualifier_code == Code::Type::Type ||
      qualifier_code == Code::Type::Assign) {
    auto field =
        Field::interpret(domain, materializations, cursor, definition, *this);
    BAIL_IF(!field);
    if (!retain_authored_field(*field)) {
      cursor.create_expression_error(
          field->get_anchor(),
          "Duplicate Field name in this Library Composite."_view);
      return False;
    }

    return True;
  }

  if (qualifier_code == Code::Type::Alias) {
    return interpret_alias(cursor, definition);
  }

  if (qualifier_code == Code::Type::Func) {
    auto function = Function::reserve(
        domain, cursor, definition, source, *this, materializations);
    BAIL_IF(!function || !function->complete(cursor));
    if (!retain_binding(
            *function, get_definition_visibility(function->get_definition()))) {
      cursor.create_token_error(
          function->get_definition().get_name_token(),
          "Duplicate Callable name in this Library Composite."_view);
      return False;
    }

    return True;
  }

  if (qualifier_code == Code::Type::Addressable) {
    View::Bytes kind = qualifier.caculate_text(cursor.get_source_text());
    if (kind == "enum"_view) {
      auto enumeration =
          Enumeration::interpret(domain, cursor, definition, source, *this);
      BAIL_IF(!enumeration);
      if (!retain_binding(
              *enumeration, get_definition_visibility(definition))) {
        cursor.create_expression_error(
            definition.get_name_anchor(),
            "Duplicate Type name in this Library Composite."_view);
        return False;
      }

      return True;
    }

    if (kind == "struct"_view) {
      auto structure = Structure::interpret(
          domain, cursor, definition, source, materializations, *this);
      BAIL_IF(!structure);
      if (!retain_binding(*structure, get_definition_visibility(definition))) {
        cursor.create_expression_error(
            definition.get_name_anchor(),
            "Duplicate Type name in this Library Composite."_view);
        return False;
      }

      return True;
    }

    if (kind == "object"_view) {
      auto object = Object::interpret(
          domain, cursor, definition, source, materializations, *this);
      BAIL_IF(!object);
      if (!retain_binding(*object, get_definition_visibility(definition))) {
        cursor.create_expression_error(
            definition.get_name_anchor(),
            "Duplicate Type name in this Library Composite."_view);
        return False;
      }

      return True;
    }
  }

  cursor.create_token_error(
      qualifier,
      "Library definitions require a Type, `alias`, `enum`, `struct`, "
      "`object`, `func`, or inferred initializer qualifier."_view);
  return False;
}

template <typename abstract_type>
static auto find_binding(
    const Perimortem::Memory::Managed::Vector<Reference<abstract_type>>& source,
    View::Bytes name) -> const Abstract& {
  auto bindings = source.get_view();
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

  if (selected == bindings.get_size()) {
    return Invalid::get_invalid();
  }

  return bindings.get_data()[selected].get();
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

auto Types::Composite::can_bind_member(const Abstract& candidate) const
    -> Bool {
  BAIL_IF(!can_accept_definition());

  return can_bind_definition(candidate);
}

auto Types::Composite::can_accept_definition() const -> Bool {
  return stage == Stage::Authored;
}

auto Types::Composite::can_bind_definition(const Abstract& binding) const
    -> Bool {
  View::Bytes candidate = binding.get_name();
  if (candidate.is_empty() || candidate == "source"_view ||
      static_binding_order.get_view().contains(
          [&](const Reference<const Abstract>& existing) {
            return &existing.get() == &binding;
          })) {
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
              return !type_bindings.get_view().contains(
                  [&](const Reference<Abstract>& existing) {
                    return existing.get().get_name() == candidate;
                  });
            },
            [&](const Abstract& possible_addressable) {
              return possible_addressable.visit<Ttx::Model::Addressable>(
                  [&](const Ttx::Model::Addressable&) {
                    return Bool(
                        !addressable_bindings.get_view().contains(
                            [&](const Reference<const Abstract>& existing) {
                              return existing.get().get_name() == candidate;
                            }) &&
                        !field_observations.get_view().contains(
                            [&](const Reference<const Field>& field) {
                              return field.get().get_name() == candidate;
                            }));
                  },
                  [](const Abstract&) { return False; });
            });
      });
}

auto Types::Composite::bind_member(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  BAIL_IF(!can_accept_definition());

  return bind_definition(binding, binding_visibility);
}

auto Types::Composite::can_retain_binding(const Abstract& binding) const
    -> Bool {
  return can_bind_member(binding);
}

auto Types::Composite::retain_binding(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  return bind_member(binding, binding_visibility);
}

auto Types::Composite::retain_authored_field(Field& field) -> Bool {
  BAIL_IF(!can_accept_definition());

  return retain_definition_field(field);
}

auto Types::Composite::bind_definition(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  BAIL_IF(!can_bind_definition(binding));

  publish_binding(binding, binding_visibility);
  return True;
}

auto Types::Composite::publish_binding(
    Abstract& binding,
    Visibility binding_visibility) -> void {
  const Abstract& target = get_binding_target(binding);
  Bool callable = target.is<Callable>();
  Bool type = target.is<Type>();
  Bool addressable = target.is<Ttx::Model::Addressable>();
  if (!callable && !type && !addressable) {
    return;
  }

  Reference<const Abstract> identity(binding);
  if (callable) {
    callable_bindings.insert(identity);
    if (binding_visibility == Visibility::Public) {
      external_callable_bindings.insert(identity);
    }
  } else if (type) {
    type_bindings.insert(binding);
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
}

auto Types::Composite::retain_definition_field(Field& field) -> Bool {
  View::Bytes name = field.get_name();
  if (field_observations.get_view().contains(
          [&](const Reference<const Field>& existing) {
            return existing.get().get_name() == name;
          }) ||
      addressable_bindings.get_view().contains(
          [&](const Reference<const Abstract>& existing) {
            return existing.get().get_name() == name;
          })) {
    return False;
  }

  fields.insert(field);
  field_observations.insert(field);
  return True;
}

auto Types::Composite::resolve_type(const Access::Type& access) const
    -> const Abstract& {
  const Abstract& root = resolve_internal_type_context(access.get_root());
  return access.resolve_from(root);
}

auto Types::Composite::resolve_exported_type(const Access::Type& access) const
    -> const Abstract& {
  const Abstract& root = resolve_external_type_context(access.get_root());
  const Abstract& selected = access.resolve_from(root);
  if (&selected != &Invalid::get_invalid()) {
    return selected;
  }

  const Abstract& intrinsic =
      source.get_library_host().resolve_intrinsic(access.get_root());
  return access.resolve_from(intrinsic);
}

auto Types::Composite::link_types() -> Bool {
  if (stage >= Stage::TypesLinked) {
    return True;
  }
  if (stage != Stage::Authored) {
    source.report(
        get_anchor(),
        "Composite declaration Types cannot link from this lifecycle stage."_view,
        "Begin with the complete authored Composite declaration."_view);
    return False;
  }

  Bool failed = !visit_each<Enumeration>(
      type_bindings,
      [](Enumeration& enumeration) { return enumeration.link_storage(); });
  failed |= !visit_each<Composite>(type_bindings, [](Composite& composite) {
    return composite.link_types();
  });

  BAIL_IF(failed);

  stage = Stage::TypesLinked;
  return True;
}

auto Types::Composite::link_fields() -> Bool {
  if (stage >= Stage::FieldsLinked) {
    return True;
  }
  if (stage != Stage::TypesLinked) {
    source.report(
        get_anchor(), "Composite Fields require linked declaration Types."_view,
        "Settle every nested Type before completing Field Type edges."_view);
    return False;
  }

  Bool failed = !visit_each<Composite>(type_bindings, [](Composite& composite) {
    return composite.link_fields();
  });
  BAIL_IF(failed);

  Managed::Vector<Reference<Field>> available_fields(domain);
  available_fields.reset(fields.get_size());

  // Authored Type routes settle without evaluating initializers. Completing
  // those Fields first gives inference every exact declaration Type while
  // Composite keeps incomplete identities out of contextual lookup.
  for (Count i = 0; i < fields.get_size(); i++) {
    Field& field = fields[i].get();
    auto access = field.get_type_access();
    if (!access) {
      continue;
    }

    const Abstract& root = resolve_internal_type_context(access->get_root());
    const Abstract& selected = access->resolve_from(root);
    Bool linked = field.link_type(source, selected);
    if (!linked) {
      failed = True;
      continue;
    }

    available_fields.insert(field);
  }

  // Explicit Fields are already safe lookup targets. Each inferred candidate
  // then authenticates the same private context it will own after publication,
  // while only completed candidates become visible to later inference.
  linking_fields = available_fields.get_view();
  for (Count i = 0; i < fields.get_size(); i++) {
    Field& field = fields[i].get();
    if (!field.is_inferred()) {
      continue;
    }

    Bool linked = field.link_initializer(source, materializations);
    if (!linked) {
      failed = True;
      continue;
    }

    available_fields.insert(field);
    linking_fields = available_fields.get_view();
  }
  linking_fields = {};

  BAIL_IF(failed);

  if (available_fields.get_size() != fields.get_size()) {
    source.report(
        get_anchor(),
        "Composite Field completion lost one authored identity."_view,
        "Retain every completed Field in its authored order."_view);
    return False;
  }

  // Every exact Type settles before the Composite publishes a Field. The
  // Layout and public observations borrow the retained authored identities, so
  // completion never constructs or synchronizes another member model.
  for (Count i = 0; i < fields.get_size(); i++) {
    Field& field = fields[i].get();
    if (field.is_readable_externally()) {
      public_fields.insert(field);
    }

    publish_linked_field(field);
  }

  complete_field_layout();
  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Composite::publish_linked_field(Field& field) -> void {
  layout_fields.insert(field);
}

auto Types::Composite::complete_field_layout() -> void {
  layout =
      domain.construct<Ttx::Model::Layouts::Named>(layout_fields.get_view());
}

auto Types::Composite::link_initializers() -> Bool {
  if (stage >= Stage::InitializersLinked) {
    return True;
  }
  if (stage != Stage::FieldsLinked) {
    source.report(
        get_anchor(), "Composite initializers require linked Fields."_view,
        "Complete every Field Type before linking its initializer."_view);
    return False;
  }

  Bool failed = !visit_each<Composite>(type_bindings, [](Composite& composite) {
    return composite.link_initializers();
  });
  for (Count i = 0; i < fields.get_size(); i++) {
    failed |= !fields[i].get().link_initializer(source, materializations);
  }

  BAIL_IF(failed);

  stage = Stage::InitializersLinked;
  return True;
}

auto Types::Composite::link_callable_signatures() -> Bool {
  if (stage >= Stage::CallableSignaturesLinked) {
    return True;
  }
  if (stage != Stage::InitializersLinked) {
    source.report(
        get_anchor(),
        "Composite Callable signatures require linked initializers."_view,
        "Complete every retained Field Expression before Callables."_view);
    return False;
  }

  Bool failed = !visit_each<Composite>(type_bindings, [](Composite& composite) {
    return composite.link_callable_signatures();
  });
  for (Count i = 0; i < callables.get_size(); i++) {
    Callable& callable = callables[i].get();
    Bool linked = link_signature(callable);
    failed |= !linked;
    if (!linked) {
      continue;
    }

    failed |= !validate_linked_callable(callable);
  }

  BAIL_IF(failed);

  stage = Stage::CallableSignaturesLinked;
  return True;
}

auto Types::Composite::validate_linked_callable(const Callable&) -> Bool {
  return True;
}

auto Types::Composite::link_callable_bodies() -> Bool {
  if (stage >= Stage::CallablesLinked) {
    return True;
  }
  if (stage != Stage::CallableSignaturesLinked) {
    source.report(
        get_anchor(),
        "Composite Callable bodies require linked signatures."_view,
        "Complete every nested signature before linking its body."_view);
    return False;
  }

  Bool failed = !visit_each<Composite>(type_bindings, [](Composite& composite) {
    return composite.link_callable_bodies();
  });
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !link_body(callables[i].get());
  }

  BAIL_IF(failed);

  stage = Stage::CallablesLinked;
  return True;
}

auto Types::Composite::finalize() -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }
  if (stage != Stage::CallablesLinked) {
    source.report(
        get_anchor(), "An incomplete Composite cannot enter finalization."_view,
        "Link every Field, initializer, and Callable before finalizing."_view);
    return False;
  }

  Bool failed = !visit_each<Enumeration>(
      type_bindings,
      [](Enumeration& enumeration) { return enumeration.finalize(); });
  failed |= !visit_each<Composite>(
      type_bindings, [](Composite& composite) { return composite.finalize(); });

  // Explicit publication follows its authored route rather than eventual Type
  // visibility. Inference has no route, so it proves the exact Type through a
  // public binding or Library intrinsic before exposing that same identity.
  for (Count i = 0; i < public_fields.get_size(); i++) {
    const Field& field = public_fields[i].get();
    auto access = field.get_type_access();
    Bool reachable = access.visit(
        [&]() { return is_externally_reachable(field.get_type()); },
        [&](const Access::Type& selected) {
          return Bool(
              &resolve_externally_reachable_type(selected, *this) ==
              &field.get_type());
        });
    if (reachable) {
      continue;
    }

    source.report(
        field.get_type_anchor().visit(
            [&]() -> Option<Anchor> { return field.get_anchor(); },
            [](Anchor selected) -> Option<Anchor> { return selected; }),
        "Externally readable Composite Field publishes an unreachable Type."_view,
        "Keep the Field private or publish its exact Type."_view);
    failed = True;
  }
  for (Count i = 0; i < public_callables.get_size(); i++) {
    const Callable& callable = public_callables[i].get();
    auto exposure = callable_exposes_unreachable_type(callable, *this);
    if (!exposure) {
      continue;
    }

    source.report(
        exposure,
        "Externally readable Composite Callable publishes an unreachable Type "
        "route."_view,
        "Keep the Callable private or publish its authored Type route."_view);
    failed = True;
  }

  // Callable folding still runs when publication fails. Independent cache and
  // diagnostic facts therefore remain observable without admitting the Type.
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !finalize_callable(callables[i].get());
  }

  BAIL_IF(failed);

  stage = Stage::Finalized;
  return True;
}

auto Types::Composite::resolve() const -> const Abstract& {
  if (stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Types::Composite::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& type = find_binding(external_type_bindings, route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  if (stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  return find_field(public_fields, route);
}

auto Types::Composite::resolve_context(
    View::Bytes route,
    const Callable& requester) const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  return resolve_internal_type_context(route);
}

auto Types::Composite::resolve_context(
    View::Bytes route,
    const Field& requester) const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  return resolve_internal_context(route);
}

auto Types::Composite::owns(const Callable& requester) const -> Bool {
  BAIL_IF(!callable_has_host(requester, *this));

  return callable_observations.get_view().contains(
      [&](const Reference<const Callable>& existing) {
        return &existing.get() == &requester;
      });
}

auto Types::Composite::owns(const Field& requester) const -> Bool {
  const Type& expected_host = *this;
  BAIL_IF(&requester.get_host() != &expected_host);

  return field_observations.get_view().contains(
      [&](const Reference<const Field>& existing) {
        return &existing.get() == &requester;
      });
}

auto Types::Composite::is_externally_reachable(const Type& type) const -> Bool {
  for (Count i = 0; i < external_type_bindings.get_size(); i++) {
    const Abstract& target =
        get_binding_target(external_type_bindings.at(i).get()).resolve();
    if (&target == &type) {
      return True;
    }
  }

  if (enclosing_scope) {
    if (enclosing_scope->is_externally_reachable(type)) {
      return True;
    }
  }

  const Abstract& outer =
      source.get_interpretation_context().resolve_context(type.get_name());
  if (&outer.resolve() == &type) {
    return True;
  }

  const Abstract& intrinsic =
      source.get_library_host().resolve_intrinsic(type.get_name());
  return &intrinsic.resolve() == &type;
}

auto Types::Composite::grants_complete_access(const Abstract& requester) const
    -> Bool {
  return requester.visit<Callable>(
      [&](const Callable& callable) { return owns(callable); },
      [&](const Abstract& possible_field) {
        return possible_field.visit<Field>(
            [&](const Field& field) { return owns(field); },
            [](const Abstract&) { return False; });
      });
}

auto Types::Composite::resolve_internal_addressable_binding(
    View::Bytes route) const -> const Abstract& {
  for (Count i = 0; i < linking_fields.get_size(); i++) {
    const Field& field = linking_fields.get_data()[i].get();
    if (field.get_name() == route) {
      return field;
    }
  }

  return find_binding(addressable_bindings, route);
}

auto Types::Composite::resolve_external_addressable_binding(
    View::Bytes route) const -> const Abstract& {
  return find_binding(external_addressable_bindings, route);
}

auto Types::Composite::resolve_internal_type_binding(View::Bytes route) const
    -> const Abstract& {
  return find_binding(type_bindings, route);
}

auto Types::Composite::resolve_external_type_binding(View::Bytes route) const
    -> const Abstract& {
  return find_binding(external_type_bindings, route);
}

auto Types::Composite::resolve_internal_context(View::Bytes route) const
    -> const Abstract& {
  for (Count i = 0; i < linking_fields.get_size(); i++) {
    const Field& field = linking_fields.get_data()[i].get();
    if (field.get_name() == route) {
      return field;
    }
  }

  if (stage >= Stage::FieldsLinked) {
    const Abstract& member = find_field(field_observations, route);
    if (&member != &Invalid::get_invalid()) {
      return member;
    }
  }

  const Abstract& type = find_binding(type_bindings, route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  if (enclosing_scope) {
    return enclosing_scope->resolve_internal_type_context(route);
  }

  const Abstract& outer =
      source.get_interpretation_context().resolve_context(route);
  if (&outer != &Invalid::get_invalid()) {
    return outer;
  }

  return source.get_library_host().resolve_intrinsic(route);
}

auto Types::Composite::resolve_internal_type_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& type = find_binding(type_bindings, route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  if (enclosing_scope) {
    return enclosing_scope->resolve_internal_type_context(route);
  }

  const Abstract& outer =
      source.get_interpretation_context().resolve_context(route);
  if (&outer != &Invalid::get_invalid()) {
    return outer;
  }

  return source.get_library_host().resolve_intrinsic(route);
}

auto Types::Composite::resolve_external_type_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& local = find_binding(external_type_bindings, route);
  if (&local != &Invalid::get_invalid()) {
    return local;
  }

  return enclosing_scope.visit(
      [&]() -> const Abstract& { return Invalid::get_invalid(); },
      [&](const Composite& selected) -> const Abstract& {
        return selected.resolve_external_type_context(route);
      });
}

auto Types::Composite::get_layout() const -> const Ttx::Model::Layouts::Named& {
  return layout.visit(
      []() -> const Ttx::Model::Layouts::Named& { return incomplete_layout; },
      [](const Ttx::Model::Layouts::Named& selected)
          -> const Ttx::Model::Layouts::Named& { return selected; });
}

auto Types::Composite::is_readable(
    const Field& field,
    const Abstract& requester) const -> Bool {
  BAIL_IF(!owns(field));

  return grants_complete_access(requester) || field.is_readable_externally();
}

auto Types::Composite::get_fields() const
    -> View::Vector<Reference<const Field>> {
  return field_observations;
}

auto Types::Composite::get_public_fields() const
    -> View::Vector<Reference<const Field>> {
  return public_fields;
}

auto Types::Composite::get_callables() const
    -> View::Vector<Reference<const Callable>> {
  return callable_observations;
}

auto Types::Composite::get_public_callables() const
    -> View::Vector<Reference<const Callable>> {
  return public_callables;
}

auto Types::Composite::get_callable_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return external_callable_bindings;
}

auto Types::Composite::get_callable_bindings(const Abstract& requester) const
    -> View::Vector<Reference<const Abstract>> {
  if (grants_complete_access(requester)) {
    return callable_bindings;
  }

  return external_callable_bindings;
}

auto Types::Composite::get_static_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return static_binding_order;
}

auto Types::Composite::get_external_static_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return external_static_binding_order;
}
