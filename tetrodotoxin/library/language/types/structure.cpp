// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/parser/declaration.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
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

static auto parse_structured_definition(Cursor& cursor)
    -> Option<ParsedStructuredDefinition> {
  Token opening = cursor.current();
  auto visibility = Parser::Declaration::parse_visibility(cursor);
  BAIL_IF(!visibility);

  Token name_token = cursor.require(
      Code::Type::Type,
      "Library Structures require an authored Type shaped name."_view);
  BAIL_IF(!name_token);
  BAIL_IF(!cursor.require(
      Code::Type::Define,
      "Library Structure names require `:` before their kind."_view));

  Token kind_token = cursor.require(
      Code::Type::Addressable,
      "Library Structure declarations require `struct` or `object`."_view);
  BAIL_IF(!kind_token);

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
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Structure bodies require an opening `{`."_view));

  return ParsedStructuredDefinition{
    .visibility = *visibility,
    .name = name_token.caculate_text(cursor.get_source_text()),
    .kind = kind,
    .opening = opening,
    .kind_token = kind_token,
    .name_anchor = Anchor::create(Span(name_token)),
  };
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
    const Types::Structure& external_context) -> const Abstract& {
  return external_context.resolve_exported_type(access);
}

static auto callable_exposes_unreachable_type(
    const Callable& callable,
    const Types::Structure& external_context) -> Option<Anchor> {
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

Types::Structure::Structure(
    Allocator::Arena& domain,
    View::Bytes name,
    const Documentation& documentation,
    Visibility visibility,
    Monograph& source,
    Materializations& materializations,
    Option<Reference<const Type>> enclosing_scope,
    Option<Anchor> anchor,
    Option<Anchor> name_anchor)
    : domain(domain),
      name(name),
      documentation(documentation),
      visibility(visibility),
      source(source),
      materializations(materializations),
      enclosing_scope(enclosing_scope),
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

auto Types::Structure::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Monograph& source,
    Materializations& materializations,
    const Structure& enclosing_scope) -> Option<Structure&> {
  auto transaction = cursor.branch();
  auto definition = parse_structured_definition(transaction);
  BAIL_IF(!definition);

  Anchor preliminary = Anchor::create(
      definition->kind_token,
      Span(definition->opening, definition->kind_token));
  auto reserve_exact_type = [&]() -> Structure& {
    if (definition->kind == DefinitionKind::Object) {
      return Object::create_authored(
          domain, definition->name, documentation, definition->visibility,
          source, materializations, enclosing_scope, preliminary,
          definition->name_anchor);
    }

    return domain.construct_from<Structure>([&]() -> Structure {
      return Structure(
          domain, definition->name, documentation, definition->visibility,
          source, materializations, Reference<const Type>(enclosing_scope),
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

    Bool parsed = Parser::Declaration::parse(
        domain, materializations, transaction, source, structure);
    BAIL_IF(!parsed);
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

  if (selected == bindings.get_size()) {
    return Invalid::get_invalid();
  }

  return bindings.get_data()[selected].get();
}

static auto find_binding(
    View::Vector<Reference<Abstract>> bindings,
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

auto Types::Structure::can_bind_member(const Abstract& candidate) const
    -> Bool {
  BAIL_IF(!can_accept_declaration());

  return can_bind_declaration(candidate);
}

auto Types::Structure::can_accept_declaration() const -> Bool {
  return stage == Stage::Authored;
}

auto Types::Structure::can_bind_declaration(const Abstract& binding) const
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
                        !field_sources.get_view().contains(
                            [&](const Field::Source& field) {
                              return field.get_name() == candidate;
                            }));
                  },
                  [](const Abstract&) { return False; });
            });
      });
}

auto Types::Structure::bind_member(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  BAIL_IF(!can_accept_declaration());

  return bind_declaration(binding, binding_visibility);
}

auto Types::Structure::bind_declaration(
    Abstract& binding,
    Visibility binding_visibility) -> Bool {
  BAIL_IF(!can_bind_declaration(binding));

  publish_binding(binding, binding_visibility);
  return True;
}

auto Types::Structure::publish_binding(
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

auto Types::Structure::retain_field(Field::Source field) -> Bool {
  BAIL_IF(!can_accept_declaration());

  return retain_declaration_field(field);
}

auto Types::Structure::retain_declaration_field(Field::Source field) -> Bool {
  View::Bytes name = field.get_name();
  if (field_sources.get_view().contains([&](const Field::Source& existing) {
        return existing.get_name() == name;
      }) ||
      addressable_bindings.get_view().contains(
          [&](const Reference<const Abstract>& existing) {
            return existing.get().get_name() == name;
          })) {
    return False;
  }

  field_sources.insert(field);
  stage = Stage::Authored;
  return True;
}

auto Types::Structure::resolve_type(const Access::Type& access) const
    -> const Abstract& {
  const Abstract& root = resolve_internal_type_context(access.get_root());
  return access.resolve_from(root);
}

auto Types::Structure::resolve_exported_type(const Access::Type& access) const
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

auto Types::Structure::link_types() -> Bool {
  if (stage >= Stage::TypesLinked) {
    return True;
  }
  if (stage != Stage::Authored) {
    source.report(
        anchor,
        "Structure declaration Types cannot link from this lifecycle stage."_view,
        "Begin with the complete authored Structure declaration."_view);
    return False;
  }

  Bool failed = !visit_each<Enumeration>(
      type_bindings,
      [](Enumeration& enumeration) { return enumeration.link_storage(); });
  failed |= !visit_each<Structure>(type_bindings, [](Structure& structure) {
    return structure.link_types();
  });

  BAIL_IF(failed);

  stage = Stage::TypesLinked;
  return True;
}

auto Types::Structure::link_fields() -> Bool {
  if (stage >= Stage::FieldsLinked) {
    return True;
  }
  if (stage != Stage::TypesLinked) {
    source.report(
        anchor, "Structure Fields require linked declaration Types."_view,
        "Settle every nested Type before constructing Fields."_view);
    return False;
  }

  Bool failed = !visit_each<Structure>(type_bindings, [](Structure& structure) {
    return structure.link_fields();
  });
  BAIL_IF(failed);

  Managed::Vector<Reference<Field>> available_fields(domain);
  available_fields.reset(field_sources.get_size());

  // Authored Type routes settle without evaluating initializers. Completing
  // those Fields first gives inference every exact declaration Type while the
  // candidate inventory remains private to this transaction.
  for (Count i = 0; i < field_sources.get_size(); i++) {
    Field::Source& field_source = field_sources[i];
    auto access = field_source.get_type_access();
    if (!access) {
      continue;
    }

    const Abstract& root = resolve_internal_type_context(access->get_root());
    const Abstract& selected = access->resolve_from(root);
    auto field = Field::link(domain, source, *this, field_source, selected);
    if (!field) {
      failed = True;
      continue;
    }

    available_fields.insert(*field);
  }

  // Explicit Fields are already safe lookup targets. Each inferred candidate
  // then authenticates the same private context it will own after publication,
  // while only completed candidates become visible to later inference.
  linking_fields = available_fields.get_view();
  for (Count i = 0; i < field_sources.get_size(); i++) {
    Field::Source& field_source = field_sources[i];
    if (!field_source.is_inferred()) {
      continue;
    }

    linking_source = field_source;
    auto field = Field::link_inferred(
        domain, source, materializations, *this, field_source);
    linking_source = {};
    if (!field) {
      failed = True;
      continue;
    }

    available_fields.insert(*field);
    linking_fields = available_fields.get_view();
  }
  linking_fields = {};
  linking_source = {};

  BAIL_IF(failed);

  Managed::Vector<Reference<Field>> linked_fields(domain);
  linked_fields.reset(field_sources.get_size());

  // Completion order puts explicit Fields before inferred dependencies. The
  // published Layout still follows authored order, so rebuild that order from
  // the unique Field names before exposing any identity.
  for (Count source_index = 0; source_index < field_sources.get_size();
       source_index++) {
    View::Bytes name = field_sources[source_index].get_name();
    for (Count field_index = 0; field_index < available_fields.get_size();
         field_index++) {
      Field& field = available_fields[field_index].get();
      if (field.get_name() == name) {
        linked_fields.insert(field);
        break;
      }
    }
  }
  if (linked_fields.get_size() != field_sources.get_size()) {
    source.report(
        anchor, "Structure Field completion lost one authored identity."_view,
        "Retain every completed Field in its authored order."_view);
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

    publish_linked_field(field);
  }

  complete_field_layout();
  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Structure::publish_linked_field(Field& field) -> void {
  layout_fields.insert(field);
}

auto Types::Structure::complete_field_layout() -> void {
  layout =
      domain.construct<Ttx::Model::Layouts::Named>(layout_fields.get_view());
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

  Bool failed = !visit_each<Structure>(type_bindings, [](Structure& structure) {
    return structure.link_initializers();
  });
  for (Count i = 0; i < fields.get_size(); i++) {
    failed |= !fields[i].get().link_initializer(source, materializations);
  }

  BAIL_IF(failed);

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

  Bool failed = !visit_each<Structure>(type_bindings, [](Structure& structure) {
    return structure.link_callable_signatures();
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

auto Types::Structure::validate_linked_callable(const Callable&) -> Bool {
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

  Bool failed = !visit_each<Structure>(type_bindings, [](Structure& structure) {
    return structure.link_callable_bodies();
  });
  for (Count i = 0; i < callables.get_size(); i++) {
    failed |= !link_body(callables[i].get());
  }

  BAIL_IF(failed);

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

  Bool failed = !visit_each<Enumeration>(
      type_bindings,
      [](Enumeration& enumeration) { return enumeration.finalize(); });
  failed |= !visit_each<Structure>(
      type_bindings, [](Structure& structure) { return structure.finalize(); });

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
        "Externally readable Structure Field publishes an unreachable Type."_view,
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

  BAIL_IF(failed);

  stage = Stage::Finalized;
  return True;
}

auto Types::Structure::resolve() const -> const Abstract& {
  if (stage < Stage::FieldsLinked) {
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

  return resolve_internal_type_context(route);
}

auto Types::Structure::resolve_context(
    View::Bytes route,
    const Field& requester) const -> const Abstract& {
  if (!owns(requester)) {
    return resolve_context(route);
  }

  return resolve_internal_context(route);
}

auto Types::Structure::owns(const Callable& requester) const -> Bool {
  BAIL_IF(!callable_has_host(requester, *this));

  return callable_observations.get_view().contains(
      [&](const Reference<const Callable>& existing) {
        return &existing.get() == &requester;
      });
}

auto Types::Structure::owns(const Field& requester) const -> Bool {
  const Type& expected_host = *this;
  BAIL_IF(&requester.get_host() != &expected_host);

  if (linking_source && requester.get_name() == linking_source->get_name() &&
      !requester.get_type_access()) {
    return True;
  }

  return field_observations.get_view().contains(
      [&](const Reference<const Field>& existing) {
        return &existing.get() == &requester;
      });
}

auto Types::Structure::is_externally_reachable(const Type& type) const -> Bool {
  for (Count i = 0; i < external_type_bindings.get_size(); i++) {
    const Abstract& target =
        get_binding_target(external_type_bindings.at(i).get()).resolve();
    if (&target == &type) {
      return True;
    }
  }

  if (enclosing_scope) {
    Bool reachable = enclosing_scope->get().visit<Structure>(
        [&](const Structure& structure) {
          return structure.is_externally_reachable(type);
        },
        [&](const Abstract& context) {
          return Bool(
              &context.resolve_context(type.get_name()).resolve() == &type);
        });
    if (reachable) {
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

auto Types::Structure::grants_complete_access(const Abstract& requester) const
    -> Bool {
  return requester.visit<Callable>(
      [&](const Callable& callable) { return owns(callable); },
      [&](const Abstract& possible_field) {
        return possible_field.visit<Field>(
            [&](const Field& field) { return owns(field); },
            [](const Abstract&) { return False; });
      });
}

auto Types::Structure::resolve_internal_addressable_binding(
    View::Bytes route) const -> const Abstract& {
  for (Count i = 0; i < linking_fields.get_size(); i++) {
    const Field& field = linking_fields.get_data()[i].get();
    if (field.get_name() == route) {
      return field;
    }
  }

  return find_binding(addressable_bindings, route);
}

auto Types::Structure::resolve_external_addressable_binding(
    View::Bytes route) const -> const Abstract& {
  return find_binding(external_addressable_bindings, route);
}

auto Types::Structure::resolve_internal_type_binding(View::Bytes route) const
    -> const Abstract& {
  return find_binding(type_bindings, route);
}

auto Types::Structure::resolve_external_type_binding(View::Bytes route) const
    -> const Abstract& {
  return find_binding(external_type_bindings, route);
}

auto Types::Structure::resolve_internal_context(View::Bytes route) const
    -> const Abstract& {
  for (Count i = 0; i < linking_fields.get_size(); i++) {
    const Field& field = linking_fields.get_data()[i].get();
    if (field.get_name() == route) {
      return field;
    }
  }

  const Abstract& member = find_field(field_observations, route);
  if (&member != &Invalid::get_invalid()) {
    return member;
  }

  const Abstract& type = find_binding(type_bindings, route);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  if (enclosing_scope) {
    return enclosing_scope->get().visit<Structure>(
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

  if (enclosing_scope) {
    return enclosing_scope->get().visit<Structure>(
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

auto Types::Structure::resolve_external_type_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& local = find_binding(external_type_bindings, route);
  if (&local != &Invalid::get_invalid()) {
    return local;
  }

  return enclosing_scope.visit(
      [&]() -> const Abstract& { return Invalid::get_invalid(); },
      [&](const Reference<const Type>& selected) -> const Abstract& {
        return selected.get().visit<Structure>(
            [&](const Structure& structure) -> const Abstract& {
              return structure.resolve_external_type_context(route);
            },
            [&](const Abstract& context) -> const Abstract& {
              return context.resolve_context(route);
            });
      });
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
  BAIL_IF(!owns(field));

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
