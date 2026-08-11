// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/composite.hpp"

#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parser/member.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Tetrodotoxin::Language::Visibility;
using Ttx::Model::Callable;
using Ttx::Model::Type;

static constexpr Ttx::Model::Layouts::Named incomplete_layout;

template <typename abstract_type>
static auto find_monograph(abstract_type& host)
    -> decltype(*host.template select<Monograph>()) {
  auto* current = &host;
  while (auto enclosing = current->template select<Types::Composite>()) {
    current = &enclosing->get_host();
  }

  return *current->template select<Monograph>();
}

template <typename selected_type>
static auto select_bindings(View::Vector<Reference<Abstract>> bindings) {
  return View::Selection(
      bindings, [](const Reference<Abstract>& binding) -> Bool {
        return binding.get().is<selected_type>();
      });
}

template <typename selected_type, typename visitor_type>
static auto visit_each(
    View::Vector<Reference<Abstract>> bindings,
    visitor_type visitor) -> Bool {
  Bool failed = False;
  for (const Reference<Abstract>& binding :
       select_bindings<selected_type>(bindings)) {
    failed |= !binding.get().visit<selected_type>(
        [&](selected_type& selected) { return visitor(selected); },
        [](Abstract&) { return False; });
  }

  return !failed;
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

auto Types::Composite::get_enclosing_scope() const -> Option<const Composite&> {
  return get_host().select<Composite>();
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
  Materializations& materializations =
      static_cast<Monograph&>(get_monograph()).get_materializations();
  auto member =
      Parser::Member::parse(domain, materializations, cursor, definition);
  BAIL_IF(!member);
  if (!retain_binding(*member)) {
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
  const Abstract* selected = nullptr;
  for (const auto& binding : bindings) {
    const Abstract& candidate = binding.get();
    if (candidate.get_name() != name) {
      continue;
    }
    if (selected) {
      return Invalid::get_invalid();
    }

    selected = &candidate;
  }

  if (!selected) {
    return Invalid::get_invalid();
  }

  return *selected;
}

auto Types::Composite::can_accept_definition() const -> Bool {
  return stage == Stage::Authored;
}

auto Types::Composite::can_bind_definition(const Abstract& binding) const
    -> Bool {
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

  const Abstract& target = Ttx::Model::Alias::get_represented(binding);
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
              return !types.get_view().contains(
                  [&](const Reference<Abstract>& existing) {
                    return existing.get().get_name() == candidate;
                  });
            },
            [&](const Abstract& possible_addressable) {
              return possible_addressable.visit<Ttx::Model::Addressable>(
                  [&](const Ttx::Model::Addressable&) {
                    return !addressables.get_view().contains(
                        [&](const Reference<Abstract>& existing) {
                          return existing.get().get_name() == candidate;
                        });
                  },
                  [](const Abstract&) { return False; });
            });
      });
}

auto Types::Composite::retain_binding(Abstract& binding) -> Bool {
  BAIL_IF(!can_accept_definition() || !can_bind_definition(binding));

  publish_binding(binding);
  return True;
}

auto Types::Composite::publish_binding(Abstract& binding) -> void {
  const Abstract& target = Ttx::Model::Alias::get_represented(binding);
  Bool callable = target.is<Callable>();
  Bool type = target.is<Type>();
  Bool addressable = target.is<Ttx::Model::Addressable>();
  if (!callable && !type && !addressable) {
    return;
  }

  if (callable) {
    callables.insert(binding);
  } else if (type) {
    types.insert(binding);
  } else {
    addressables.insert(binding);
  }
}

auto Types::Composite::resolve_type(const Access::Type& access) const
    -> const Abstract& {
  const Abstract& root = resolve_type_root(access.get_root(), *this);
  return access.resolve_from(root, *this);
}

auto Types::Composite::resolve_exported_type(const Access::Type& access) const
    -> const Abstract& {
  const Abstract& root = resolve_exported_type_root(access.get_root());
  if (&root != &Invalid::get_invalid()) {
    // A published root owns the whole qualified route. A missing suffix must
    // not retry an intrinsic with the same spelling and bypass lexical
    // shadowing established by that root.
    return access.resolve_from(root);
  }

  const auto& source = static_cast<const Monograph&>(get_monograph());
  const Abstract& intrinsic =
      source.get_dialect().resolve_intrinsic(access.get_root());
  return access.resolve_from(intrinsic);
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
  Bool failed =
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
  if (stage != Stage::TypesLinked) {
    auto& source = get_monograph();
    source.report(
        get_anchor(), "Composite Fields require linked declaration Types."_view,
        "Settle every nested Type before completing Field Type edges."_view);
    return False;
  }

  auto& source = get_monograph();
  Materializations& materializations =
      static_cast<Monograph&>(source).get_materializations();
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

    return field.link_initializer(source, materializations);
  });

  BAIL_IF(failed);

  // Every exact Type settles before the Composite publishes a Field. The
  // Layout borrows the retained authored identities, so completion never
  // constructs or synchronizes another member model.
  complete_field_layout();
  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Composite::complete_field_layout() -> void {
  Managed::Vector<Reference<const Abstract>> fields(domain);
  fields.reset(addressables.get_size());
  for (const Reference<Abstract>& binding :
       select_bindings<Field>(addressables.get_view())) {
    fields.insert(binding.get());
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
  Materializations& materializations =
      static_cast<Monograph&>(source).get_materializations();
  Bool failed = !visit_each<Composite>(
      types.get_view(),
      [](Composite& composite) { return composite.link_initializers(); });
  failed |= !visit_each<Field>(addressables.get_view(), [&](Field& field) {
    return field.link_initializer(source, materializations);
  });

  BAIL_IF(failed);

  stage = Stage::InitializersLinked;
  return True;
}

auto Types::Composite::link_callable_signatures() -> Bool {
  if (stage >= Stage::CallableSignaturesLinked) {
    return True;
  }
  if (stage != Stage::InitializersLinked) {
    auto& source = get_monograph();
    source.report(
        get_anchor(),
        "Composite Callable signatures require linked initializers."_view,
        "Complete every retained Field Expression before Callables."_view);
    return False;
  }

  auto& source = get_monograph();
  Bool failed =
      !visit_each<Composite>(types.get_view(), [](Composite& composite) {
        return composite.link_callable_signatures();
      });
  failed |=
      !visit_each<Function>(callables.get_view(), [&](Function& function) {
        Bool linked = function.link_signature(source);
        if (!linked) {
          return False;
        }

        return validate_linked_callable(function);
      });

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
    auto& source = get_monograph();
    source.report(
        get_anchor(),
        "Composite Callable bodies require linked signatures."_view,
        "Complete every nested signature before linking its body."_view);
    return False;
  }

  auto& source = get_monograph();
  Materializations& materializations =
      static_cast<Monograph&>(source).get_materializations();
  Bool failed = !visit_each<Composite>(
      types.get_view(),
      [](Composite& composite) { return composite.link_callable_bodies(); });
  failed |=
      !visit_each<Function>(callables.get_view(), [&](Function& function) {
        return function.link_body(source, materializations);
      });

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
    const Abstract& target =
        Ttx::Model::Alias::get_represented(binding.get()).resolve();
    if (&target == &type) {
      return True;
    }
  }

  auto enclosing_scope = get_enclosing_scope();
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

  auto enclosing_scope = get_enclosing_scope();
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

  return get_enclosing_scope().visit(
      [&]() -> const Abstract& { return Invalid::get_invalid(); },
      [&](const Composite& selected) -> const Abstract& {
        return selected.resolve_exported_type_root(route);
      });
}

auto Types::Composite::get_layout() const -> const Ttx::Model::Layouts::Named& {
  return layout.visit(
      []() -> const Ttx::Model::Layouts::Named& { return incomplete_layout; },
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
