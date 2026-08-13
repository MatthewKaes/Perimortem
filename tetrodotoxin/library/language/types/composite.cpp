// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/composite.hpp"

#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parser/member.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Tetrodotoxin::Language::Visibility;
using Ttx::Model::Type;

static constexpr Ttx::Model::Layouts::Named empty_layout;

template <typename abstract_type>
static auto find_monograph(abstract_type& host)
    -> decltype(*host.template select<Monograph>()) {
  auto* current = &host;
  while (auto enclosing = current->template select<Types::Composite>()) {
    current = &enclosing->get_host();
  }

  return *current->template select<Monograph>();
}

template <typename selected_type, typename visitor_type>
static auto visit_each(
    View::Vector<Reference<Abstract>> bindings,
    visitor_type visitor) -> Bool {
  Bool failed = False;
  for (const Reference<Abstract>& binding : bindings) {
    auto selected = binding.get().select<selected_type>();
    if (selected) {
      failed |= !visitor(*selected);
    }
  }

  return !failed;
}

static auto definition_category(Token qualifier)
    -> Option<Types::Composite::Category> {
  switch (qualifier.get_code().get_type()) {
  case Code::Type::Type:
  case Code::Type::Assign:
    return Types::Composite::Category::Addressable;
  case Code::Type::Func:
    return Types::Composite::Category::Callable;
  case Code::Type::Alias:
  case Code::Type::Enum:
  case Code::Type::Struct:
  case Code::Type::Object:
    return Types::Composite::Category::Type;
  default:
    return {};
  }
}

static auto function_declares_self(const Abstract& binding) -> Bool {
  return binding.visit<Function>(
      [](const Function& function) -> Bool { return function.declares_self(); },
      [](const Abstract&) { return False; });
}

static auto resolve_local_addressable(
    const Types::Composite& composite,
    View::Bytes route,
    Visibility visibility) -> const Abstract& {
  for (const Reference<Abstract>& binding :
       composite.get_addressables(visibility)) {
    const Abstract& candidate = binding.get();
    if (!composite.is_linked() && candidate.is<Field>() &&
        &candidate.resolve() == &Invalid::get_invalid()) {
      continue;
    }

    auto field = candidate.select<Field>();
    if (field && field->get_writability() == Writability::Internal) {
      continue;
    }

    if (candidate.get_name() == route) {
      return candidate;
    }
  }

  return Invalid::get_invalid();
}

Types::Composite::Composite(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition)
    : Defined(definition),
      domain(domain),
      addressables(domain),
      callables(domain),
      types(domain) {}

auto Types::Composite::get_monograph() -> Tetrodotoxin::Language::Monograph& {
  return find_monograph(get_host());
}

auto Types::Composite::get_monograph() const
    -> const Tetrodotoxin::Language::Monograph& {
  return find_monograph(get_host());
}

auto Types::Composite::grants_private_access(const Type& caller_scope) const
    -> Bool {
  const Abstract* current = &caller_scope;
  while (auto hosted = current->select<Composite>()) {
    if (&*hosted == this) {
      return True;
    }

    // The caller walks outward through exact Definition hosts. The target never
    // walks its own hosts: doing so would turn containment into inherited
    // authority and admit parents, siblings, or unrelated Aliases.
    current = &hosted->get_host();
  }

  return False;
}

auto Types::Composite::interpret_definition(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto category = definition_category(definition.get_qualifier());
  if (!category) {
    cursor.create_token_error(
        definition.get_qualifier(),
        "Library members require a Type, `alias`, `enum`, `struct`, "
        "`object`, `func`, or inferred initializer qualifier."_view);
    return False;
  }

  // The Definition host chain already identifies the owning Library source.
  // Member grammar receives that owner and requests any source capability from
  // it directly. Composite does not relay individual caches through every
  // declaration parser.
  auto& source = static_cast<Monograph&>(get_monograph());
  auto member = Parser::Member::parse(domain, source, cursor, definition);
  BAIL_IF(!member);
  if (!retain_binding(*member, *category)) {
    cursor.create_expression_error(
        definition.get_name_anchor(),
        "Library member collides with an occupied Composite category."_view);
    return False;
  }

  return True;
}

template <typename bindings_type>
static auto find_binding(const bindings_type& bindings, View::Bytes name)
    -> const Abstract& {
  for (const auto& binding : bindings) {
    const Abstract& candidate = binding.get();
    if (candidate.get_name() == name) {
      return candidate;
    }
  }

  return Invalid::get_invalid();
}

auto Types::Composite::can_accept_definition() const -> Bool {
  return stage == Stage::Authored;
}

auto Types::Composite::can_bind_definition(
    const Abstract& binding,
    Category category,
    Bool callable_declares_self) const -> Bool {
  View::Bytes candidate = binding.get_name();
  auto retains_identity = [&](const auto& category) {
    return category.get_view().contains(
        [&](const Reference<Abstract>& existing) {
          return &existing.get() == &binding;
        });
  };
  if (candidate.is_empty() || retains_identity(addressables) ||
      retains_identity(callables) || retains_identity(types)) {
    return False;
  }

  switch (category) {
  case Category::Addressable:
    return !addressables.get_view().contains(
        [&](const Reference<Abstract>& existing) {
          return existing.get().get_name() == candidate;
        });
  case Category::Type:
    return !types.get_view().contains([&](const Reference<Abstract>& existing) {
      return existing.get().get_name() == candidate;
    });
  case Category::Callable:
    // Signature entry zero already separates Self from Static, and the left
    // Expression result selects that role before Call performs name lookup. A
    // future overload contract could additionally discriminate complete
    // argument Layouts, but their Types bind later and fitting can overlap.
    // Registration therefore rejects one spelling only within the same role so
    // it avoids a staged overload registry while guaranteeing one Call target.
    return !callables.get_view().contains(
        [&](const Reference<Abstract>& existing) {
          return existing.get().get_name() == candidate &&
                 function_declares_self(existing.get()) ==
                     callable_declares_self;
        });
  }

  return False;
}

auto Types::Composite::retain_binding(Abstract& binding, Category category)
    -> Bool {
  // Only authored retention proves the Function's Definition host. Import
  // preflight deliberately observes the provider Function before constructing
  // the local opaque Alias, so category collision checks cannot impose this
  // ownership rule.
  if (category == Category::Callable) {
    auto function = binding.select<Function>();
    BAIL_IF(!function || &function->get_host() != this);
  }

  Bool callable_declares_self =
      category == Category::Callable && function_declares_self(binding);
  BAIL_IF(
      !can_accept_definition() ||
      !can_bind_definition(binding, category, callable_declares_self));

  publish_binding(binding, category);
  return True;
}

auto Types::Composite::publish_binding(Abstract& binding, Category category)
    -> void {
  // The parser or importing provider supplies the category before publication.
  // Alias resolution is deliberately absent here: delayed graph completion
  // cannot change which namespace owns the local name.
  switch (category) {
  case Category::Addressable:
    addressables.insert(binding);
    return;
  case Category::Callable:
    callables.insert(binding);
    return;
  case Category::Type:
    types.insert(binding);
    return;
  }
}

auto Types::Composite::resolve_type(const TypeReference& reference) const
    -> const Abstract& {
  const Abstract& root = resolve_type_root(reference.get_root(), *this);
  const Abstract& selected = reference.resolve_route_from(root, *this);
  return reference.resolve_type(selected, *this);
}

auto Types::Composite::resolve_exported_type(
    const TypeReference& reference) const -> const Abstract& {
  const Abstract& root = resolve_exported_type_root(reference.get_root());
  if (&root != &Invalid::get_invalid()) {
    // A published root owns the whole qualified route. A missing suffix must
    // not retry an intrinsic with the same spelling and bypass lexical
    // shadowing established by that root.
    const Abstract& selected = reference.resolve_route_from(root);
    return reference.resolve_exported_type(selected, *this);
  }

  const auto& source = static_cast<const Monograph&>(get_monograph());
  const Abstract& intrinsic =
      source.get_dialect().resolve_intrinsic(reference.get_root());
  const Abstract& selected = reference.resolve_route_from(intrinsic);
  return reference.resolve_exported_type(selected, *this);
}

auto Types::Composite::link_types() -> Bool {
  if (stage >= Stage::TypesLinked) {
    return True;
  }
  if (stage != Stage::Authored) {
    auto& source = get_monograph();
    source.report(
        get_anchor(),
        "Composite declaration Types cannot link from this lifecycle stage."_view,
        "Begin with the complete authored Composite declaration."_view);
    return False;
  }

  auto& source = get_monograph();
  Bool failed = !visit_each<Alias>(
      types.get_view(), [&](Alias& alias) { return alias.link_target(); });
  failed |=
      !visit_each<Enumeration>(types.get_view(), [&](Enumeration& enumeration) {
        return enumeration.link_storage(source);
      });
  failed |= !visit_each<Composite>(types.get_view(), [](Composite& composite) {
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
  if (stage != Stage::CallableSignaturesLinked) {
    auto& source = get_monograph();
    source.report(
        get_anchor(),
        "Composite Fields require linked Callable signatures."_view,
        "Settle every signature before a Field Expression can invoke it."_view);
    return False;
  }

  auto& source = get_monograph();
  Bool failed = !visit_each<Composite>(
      types.get_view(),
      [](Composite& composite) { return composite.link_fields(); });
  BAIL_IF(failed);

  // Authored Type routes settle without evaluating initializers. Completing
  // those Fields first gives inference every exact declaration Type while
  // Composite keeps incomplete identities out of contextual lookup.
  failed |= !visit_each<Field>(addressables.get_view(), [&](Field& field) {
    return Bool(field.is_inferred() || field.link_type(source));
  });

  // Explicit Fields are already safe lookup targets. Each inferred candidate
  // then authenticates the same private context it will own after publication,
  // while only completed candidates become visible to later inference.
  failed |= !visit_each<Field>(addressables.get_view(), [&](Field& field) {
    if (!field.is_inferred()) {
      return True;
    }

    return field.link_initializer(source);
  });

  BAIL_IF(failed);

  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Composite::complete_field_layout() -> void {
  Managed::Vector<Reference<const Abstract>> fields(domain);
  fields.reset(addressables.get_size());
  for (const Reference<Abstract>& binding : addressables.get_view()) {
    auto field = binding.get().select<Field>();
    if (field && field->get_writability() == Writability::Internal) {
      fields.insert(*field);
    }
  }
  layout = domain.construct<Ttx::Model::Layouts::Named>(fields.get_view());
}

auto Types::Composite::link_initializers() -> Bool {
  if (stage >= Stage::InitializersLinked) {
    return True;
  }
  if (stage != Stage::FieldsLinked) {
    auto& source = get_monograph();
    source.report(
        get_anchor(), "Composite initializers require linked Fields."_view,
        "Complete every Field Type before linking its initializer."_view);
    return False;
  }

  auto& source = get_monograph();
  Bool failed = !visit_each<Composite>(
      types.get_view(),
      [](Composite& composite) { return composite.link_initializers(); });
  failed |= !visit_each<Field>(addressables.get_view(), [&](Field& field) {
    return field.link_initializer(source);
  });

  BAIL_IF(failed);

  // Every initializer Expression is linked before const folding begins. A
  // const Field may therefore depend on any other acyclic const Field in this
  // Composite without declaration order becoming semantic.
  failed |= !visit_each<Field>(addressables.get_view(), [&](Field& field) {
    return field.link_constant(source);
  });

  BAIL_IF(failed);

  stage = Stage::InitializersLinked;
  return True;
}

auto Types::Composite::link_callable_signatures() -> Bool {
  if (stage >= Stage::CallableSignaturesLinked) {
    return True;
  }
  if (stage != Stage::TypesLinked) {
    auto& source = get_monograph();
    source.report(
        get_anchor(),
        "Composite Callable signatures require linked declaration Types."_view,
        "Settle every nested Type before completing Callable signatures."_view);
    return False;
  }

  auto& source = get_monograph();
  Bool failed =
      !visit_each<Composite>(types.get_view(), [](Composite& composite) {
        return composite.link_callable_signatures();
      });
  failed |= !visit_each<Function>(
      callables.get_view(),
      [&](Function& function) { return function.link_signature(source); });

  BAIL_IF(failed);

  stage = Stage::CallableSignaturesLinked;
  return True;
}

auto Types::Composite::link_callable_bodies() -> Bool {
  if (stage >= Stage::CallablesLinked) {
    return True;
  }
  if (stage != Stage::InitializersLinked) {
    auto& source = get_monograph();
    source.report(
        get_anchor(),
        "Composite Callable bodies require linked initializers."_view,
        "Complete every Field Expression before linking Callable bodies."_view);
    return False;
  }

  auto& source = get_monograph();
  Bool failed = !visit_each<Composite>(
      types.get_view(),
      [](Composite& composite) { return composite.link_callable_bodies(); });
  failed |= !visit_each<Function>(
      callables.get_view(),
      [&](Function& function) { return function.link_body(source); });

  BAIL_IF(failed);

  stage = Stage::CallablesLinked;
  return True;
}

auto Types::Composite::finalize() -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }
  if (stage != Stage::CallablesLinked) {
    auto& source = get_monograph();
    source.report(
        get_anchor(), "An incomplete Composite cannot enter finalization."_view,
        "Link every Field, initializer, and Callable before finalizing."_view);
    return False;
  }

  auto& source = get_monograph();
  Bool failed = !visit_each<Enumeration>(
      types.get_view(),
      [&](Enumeration& enumeration) { return enumeration.finalize(source); });
  failed |= !visit_each<Composite>(types.get_view(), [](Composite& composite) {
    return composite.finalize();
  });

  failed |= !visit_each<Field>(addressables.get_view(), [&](Field& field) {
    field.finalize();
    return field.validate_publication(source);
  });

  // Callable folding still runs when publication fails. Independent cache and
  // diagnostic facts therefore remain observable without admitting the Type.
  failed |= !visit_each<Function>(
      callables.get_view(),
      [&](Function& function) { return function.finalize(source); });

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
  return find_binding(get_types(Visibility::Public), route);
}

auto Types::Composite::is_externally_reachable(const Type& type) const -> Bool {
  for (const Reference<Abstract>& binding : get_types(Visibility::Public)) {
    const Abstract& target = binding.get().resolve();
    if (&target == &type) {
      return True;
    }
  }

  auto enclosing_scope = get_host().select<Composite>();
  if (enclosing_scope) {
    if (enclosing_scope->is_externally_reachable(type)) {
      return True;
    }
  }

  const auto& source = static_cast<const Monograph&>(get_monograph());
  const Abstract& outer =
      source.get_interpretation_context().resolve_context(type.get_name());
  if (&outer.resolve() == &type) {
    return True;
  }

  const Abstract& intrinsic =
      source.get_dialect().resolve_intrinsic(type.get_name());
  return &intrinsic.resolve() == &type;
}

auto Types::Composite::resolve_lexical_addressable(
    View::Bytes route,
    const Type& caller_scope) const -> const Abstract& {
  Visibility visibility = grants_private_access(caller_scope)
                              ? Visibility::Private
                              : Visibility::Public;
  return resolve_local_addressable(*this, route, visibility);
}

auto Types::Composite::resolve_type_root(
    View::Bytes route,
    const Type& caller_scope) const -> const Abstract& {
  // Lexical ascent preserves one caller. An enclosing Composite may grant
  // private access because it occurs in that caller's Definition host chain,
  // but the ascent never changes which scope requested the lookup.
  const Abstract& type = resolve_type(route, caller_scope);
  if (&type != &Invalid::get_invalid()) {
    return type;
  }

  auto enclosing_scope = get_host().select<Composite>();
  if (enclosing_scope) {
    return enclosing_scope->resolve_type_root(route, caller_scope);
  }

  const auto& source = static_cast<const Monograph&>(get_monograph());
  const Abstract& outer =
      source.get_interpretation_context().resolve_context(route);
  if (&outer != &Invalid::get_invalid()) {
    return outer;
  }

  return source.get_dialect().resolve_intrinsic(route);
}

auto Types::Composite::resolve_type(View::Bytes route, const Type& caller_scope)
    const -> const Abstract& {
  Visibility visibility = grants_private_access(caller_scope)
                              ? Visibility::Private
                              : Visibility::Public;
  return find_binding(get_types(visibility), route);
}

auto Types::Composite::resolve_exported_type_root(View::Bytes route) const
    -> const Abstract& {
  const Abstract& local = find_binding(get_types(Visibility::Public), route);
  if (&local != &Invalid::get_invalid()) {
    return local;
  }

  return get_host().select<Composite>().visit(
      [&]() -> const Abstract& { return Invalid::get_invalid(); },
      [&](const Composite& selected) -> const Abstract& {
        return selected.resolve_exported_type_root(route);
      });
}

auto Types::Composite::get_layout() const -> const Ttx::Model::Layouts::Named& {
  return layout.visit(
      []() -> const Ttx::Model::Layouts::Named& { return empty_layout; },
      [](const Ttx::Model::Layouts::Named& selected)
          -> const Ttx::Model::Layouts::Named& { return selected; });
}

auto Types::Composite::is_visible(
    const Abstract& binding,
    Visibility visibility) const -> Bool {
  if (visibility == Visibility::Private) {
    return True;
  }

  auto authorship = binding.get_authorship();
  BAIL_IF(!authorship);

  return authorship->is_published() && (!binding.is<Field>() || is_linked());
}
