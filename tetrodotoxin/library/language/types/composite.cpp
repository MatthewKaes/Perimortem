// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/composite.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/termination.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Tetrodotoxin::Language::Visibility;
using Type = Model::Type;

static constexpr Ttx::Model::Layouts::Named empty_layout;
static constexpr U8 addressable_publication = 1 << 0;
static constexpr U8 type_publication = 1 << 1;
static constexpr U8 static_callable_publication = 1 << 2;
static constexpr U8 self_callable_publication = 1 << 3;

auto Types::Composite::NameIndex::Entry::select(Category category, Bool self)
    const -> Option<Abstract&> {
  switch (category) {
  case Category::Addressable:
    return addressable;
  case Category::Callable:
    return self ? self_callable : static_callable;
  case Category::Type:
    return type;
  }

  return {};
}

auto Types::Composite::NameIndex::Entry::can_bind(
    const Abstract& binding,
    Category category,
    Bool self) const -> Bool {
  switch (category) {
  case Category::Addressable:
  case Category::Callable:
  case Category::Type:
    break;
  default:
    return False;
  }

  auto matches = [&](Option<Abstract&> candidate) {
    return candidate && &*candidate == &binding;
  };
  BAIL_IF(
      matches(addressable) || matches(type) || matches(static_callable) ||
      matches(self_callable));
  return !select(category, self);
}

auto Types::Composite::NameIndex::Entry::bind(
    Abstract& binding,
    Category category,
    Bool self,
    Bool published) -> Bool {
  BAIL_IF(!can_bind(binding, category, self));

  U8 flag = 0;
  switch (category) {
  case Category::Addressable:
    addressable = Option<Abstract&>(binding);
    flag = addressable_publication;
    break;
  case Category::Callable:
    if (self) {
      self_callable = Option<Abstract&>(binding);
      flag = self_callable_publication;
    } else {
      static_callable = Option<Abstract&>(binding);
      flag = static_callable_publication;
    }
    break;
  case Category::Type:
    type = Option<Abstract&>(binding);
    flag = type_publication;
    break;
  default:
    return False;
  }

  if (published) {
    publication |= flag;
  }
  return True;
}

auto Types::Composite::NameIndex::Entry::is_published(
    const Abstract& binding) const -> Bool {
  auto matches = [&](Option<Abstract&> candidate, U8 flag) {
    return candidate && &*candidate == &binding && (publication & flag) != 0;
  };
  return matches(addressable, addressable_publication) ||
         matches(type, type_publication) ||
         matches(static_callable, static_callable_publication) ||
         matches(self_callable, self_callable_publication);
}

auto Types::Composite::NameIndex::can_bind(
    const Abstract& binding,
    Category category) const -> Bool {
  View::Bytes name = binding.get_name();
  BAIL_IF(name.is_empty());

  Bool self = False;
  switch (category) {
  case Category::Addressable:
  case Category::Type:
    break;
  case Category::Callable: {
    auto callable = binding.select<Model::Callable>();
    BAIL_IF(!callable);
    self = callable->declares_self();
    break;
  }
  default:
    return False;
  }

  auto entry = entries.find(name);
  return !entry || entry->value.can_bind(binding, category, self);
}

auto Types::Composite::NameIndex::bind(
    Abstract& binding,
    Category category,
    Bool published) -> Bool {
  BAIL_IF(!can_bind(binding, category));

  Bool self = False;
  if (category == Category::Callable) {
    auto callable = binding.select<Model::Callable>();
    BAIL_IF(!callable);
    self = callable->declares_self();
  }

  auto selected = entries.find(binding.get_name());
  if (selected) {
    return selected->value.bind(binding, category, self, published);
  }

  auto created = entries.insert(binding.get_name(), Entry());
  return created && created->value.bind(binding, category, self, published);
}

auto Types::Composite::NameIndex::resolve(
    View::Bytes name,
    Category category,
    Visibility visibility,
    Bool self) const -> const Abstract& {
  auto entry = entries.find(name);
  if (!entry) {
    return Invalid::get_invalid();
  }

  auto selected = entry->value.select(category, self);
  if (!selected || (visibility != Visibility::Private &&
                    !entry->value.is_published(*selected))) {
    return Invalid::get_invalid();
  }
  return *selected;
}

auto Types::Composite::NameIndex::is_published(const Abstract& binding) const
    -> Bool {
  auto entry = entries.find(binding.get_name());
  return entry && entry->value.is_published(binding);
}

template <typename selected_type, typename visitor_type>
static auto visit_each(
    View::Vector<Reference<Abstract>> bindings,
    visitor_type visitor) -> Bool {
  // A category may retain an opaque Alias before its target closes. Only an
  // exact local contract receives this lifecycle step because the target owner
  // remains responsible for completing its own graph.
  Bool failed = False;
  for (const Reference<Abstract>& binding : bindings) {
    auto selected = binding.get().select<selected_type>();
    if (selected) {
      failed |= !visitor(*selected);
    }
  }

  return !failed;
}

template <typename selected_type>
static auto select_next(
    View::Vector<Reference<Abstract>> bindings,
    Count& index) -> Option<const selected_type&> {
  while (index < bindings.get_size()) {
    auto selected = bindings.get_data()[index].get().select<selected_type>();
    if (selected) {
      return *selected;
    }

    index++;
  }

  return {};
}

template <typename selected_type>
static auto declaration_offset(const Option<const selected_type&>& selected)
    -> Count {
  if (!selected) {
    return Count(-1);
  }

  auto anchor = selected->get_declaration_anchor();
  return anchor ? Count(anchor->get_span().get_offset()) : Count(-1);
}

Types::Composite::Composite(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition)
    : definition(definition),
      domain(domain),
      names(domain),
      addressables(domain),
      published_addressables(domain),
      types(domain),
      published_types(domain),
      declarations(domain) {}

auto Types::Composite::has_private_access_to(const Type& owner) const -> Bool {
  if (this == &owner) {
    return True;
  }

  // Authority walks outward from the caller. Asking the owner to walk its own
  // host would admit parents, siblings, and unrelated Aliases.
  auto enclosing = get_host().select<Type>();
  return enclosing && enclosing->has_private_access_to(owner);
}

auto Types::Composite::retain_authored_definition(
    Abstract& binding,
    Tetrodotoxin::Language::Definition& definition,
    Category category,
    Cursor& cursor) -> Bool {
  if (!retain_binding(binding, definition, category, cursor)) {
    if (category == Category::Addressable) {
      cursor.create_expression_error(
          definition.get_name_anchor(),
          "Library Addressable name is already occupied in this Composite."_view,
          "Static and state Fields share one Addressable namespace. Choose a "
          "unique name."_view);
    } else {
      cursor.create_expression_error(
          definition.get_name_anchor(),
          "Library member collides with an occupied Composite category."_view);
    }
    return False;
  }

  return True;
}

auto Types::Composite::retain_definition(
    Abstract& binding,
    Category category,
    Bool published) -> Bool {
  return publish_binding(binding, category, published);
}

auto Types::Composite::can_accept_definition() const -> Bool {
  return stage == Stage::Authored;
}

auto Types::Composite::can_bind_definition(
    const Abstract& binding,
    Category category) const -> Bool {
  return names.can_bind(binding, category);
}

auto Types::Composite::retain_binding(
    Abstract& binding,
    Tetrodotoxin::Language::Definition& definition,
    Category category,
    Cursor& cursor) -> Bool {
  BAIL_IF(!can_accept_definition() || !can_bind_definition(binding, category));

  BAIL_IF(!publish_binding(binding, category, definition.is_published()));
  cursor.get_associations().create(definition.get_name_anchor(), binding);
  return True;
}

auto Types::Composite::publish_binding(
    Abstract& binding,
    Category category,
    Bool published,
    Bool persistent) -> Bool {
  // The parser or importing provider supplies the category before publication.
  // Alias resolution is deliberately absent here: delayed graph completion
  // cannot change which namespace owns the local name.
  BAIL_IF(!names.bind(binding, category, published));
  switch (category) {
  case Category::Addressable:
    addressables.insert(binding);
    if (persistent) {
      declarations.insert(binding);
    }
    if (published) {
      published_addressables.insert(binding);
    }
    return True;
  case Category::Callable:
    publish_callable(domain, binding, published);
    if (persistent) {
      declarations.insert(binding);
    }
    return True;
  case Category::Type:
    types.insert(binding);
    if (persistent) {
      declarations.insert(binding);
    }
    if (published) {
      published_types.insert(binding);
    }
    return True;
  }

  return False;
}

auto Types::Composite::is_published(const Abstract& declaration) const -> Bool {
  return names.is_published(declaration);
}

auto Types::Composite::link_aliases() -> Count {
  Count linked = 0;
  for (const Reference<Abstract>& binding : types.get_view()) {
    auto alias = binding.get().select<Alias>();
    if (alias && !alias->is_linked() && alias->link()) {
      linked++;
    }

    auto type = binding.get().select<Type>();
    if (type) {
      linked += type->link_aliases();
    }
  }
  return linked;
}

auto Types::Composite::validate_aliases(Cursor& cursor) const -> Bool {
  Bool valid = True;
  for (const Reference<Abstract>& binding : types.get_view()) {
    auto alias = binding.get().select<Alias>();
    if (alias && !alias->is_linked()) {
      alias->report_unresolved(cursor);
      valid = False;
    }

    auto type = binding.get().select<Type>();
    if (type && !type->validate_aliases(cursor)) {
      valid = False;
    }
  }
  return valid;
}

auto Types::Composite::link_types(Cursor& cursor) -> Bool {
  if (stage >= Stage::TypesLinked) {
    return True;
  }

  if (stage != Stage::Authored) {
    cursor.create_expression_error(
        get_anchor(),
        "Composite declaration Types cannot link from this lifecycle stage."_view,
        "Begin with the complete authored Composite declaration."_view);
    return False;
  }

  Bool failed = !visit_each<Type>(
      types.get_view(), [&](Type& type) { return type.link_types(cursor); });

  BAIL_IF(failed);

  stage = Stage::TypesLinked;
  return True;
}

auto Types::Composite::link_fields(Cursor& cursor) -> Bool {
  if (stage >= Stage::FieldsLinked) {
    return True;
  }

  if (stage != Stage::CallableSignaturesLinked) {
    cursor.create_expression_error(
        get_anchor(),
        "Composite Fields require linked Callable signatures."_view,
        "Settle every signature before a Field Expression can invoke it."_view);
    return False;
  }

  Bool failed = !visit_each<Type>(
      types.get_view(), [&](Type& type) { return type.link_fields(cursor); });
  BAIL_IF(failed);

  // Authored Type routes settle without evaluating initializers. Completing
  // those declarations first gives inference every exact Type while each
  // Addressable decides whether it owns an authored route.
  failed |= !visit_each<Model::Addressable>(
      addressables.get_view(), [&](Model::Addressable& addressable) {
        return addressable.link_declaration_type(cursor);
      });

  // Explicit declarations are now safe lookup targets. Each inferred owner
  // then authenticates its final context, while only completed candidates
  // become visible to later inference.
  failed |= !visit_each<Model::Addressable>(
      addressables.get_view(), [&](Model::Addressable& addressable) {
        return addressable.link_inferred_declaration_type(cursor);
      });

  BAIL_IF(failed);

  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Composite::validate_layout(Cursor& cursor) const -> Bool {
  Bool valid = True;
  for (const Reference<Abstract>& binding : types.get_view()) {
    auto type = binding.get().select<Type>();
    if (type && !type->validate_layout(cursor)) {
      valid = False;
    }
  }

  const Layout& selected_layout = get_layout();
  if (selected_layout.is_empty() ||
      Ttx::Model::Layouts::is_terminating(*this)) {
    return valid;
  }

  // Every stored Addressable Type is settled before this closure check.
  // Rejecting an endless shape here keeps default construction and lowering
  // free from separate defensive cycle protocols.
  cursor.create_expression_error(
      get_anchor(), "Library Type Layout does not terminate."_view,
      "Break recursive value storage with one terminal Type Layout."_view);
  return False;
}

auto Types::Composite::validate_layout_restored() const -> Bool {
  for (const Reference<Abstract>& binding : types.get_view()) {
    auto composite = binding.get().select<Composite>();
    BAIL_IF(composite && !composite->validate_layout_restored());
  }

  const Layout& selected_layout = get_layout();
  return selected_layout.is_empty() ||
         Ttx::Model::Layouts::is_terminating(*this);
}

auto Types::Composite::complete_field_layout() -> void {
  Managed::Vector<Reference<const Abstract>> fields(domain);
  fields.reset(addressables.get_size());
  for (const Reference<Abstract>& binding : addressables.get_view()) {
    auto addressable = binding.get().select<Model::Addressable>();
    if (addressable && addressable->contributes_to_instance_layout()) {
      fields.insert(*addressable);
    }
  }
  layout = domain.construct<Ttx::Model::Layouts::Named>(fields.get_view());
}

auto Types::Composite::link_initializers(Cursor& cursor) -> Bool {
  if (stage >= Stage::InitializersLinked) {
    return True;
  }

  if (stage != Stage::FieldsLinked) {
    cursor.create_expression_error(
        get_anchor(), "Composite initializers require linked Fields."_view,
        "Complete every Field Type before linking its initializer."_view);
    return False;
  }

  Bool failed = !visit_each<Type>(types.get_view(), [&](Type& type) {
    return type.link_initializers(cursor);
  });
  failed |= !visit_each<Model::Addressable>(
      addressables.get_view(), [&](Model::Addressable& addressable) {
        return addressable.link_declaration_initializer(cursor);
      });

  BAIL_IF(failed);

  // Every initializer Expression is linked before const folding begins. A
  // const declaration may therefore depend on any other acyclic const
  // declaration in this Composite without source order becoming semantic.
  failed |= !visit_each<Model::Addressable>(
      addressables.get_view(), [&](Model::Addressable& addressable) {
        return addressable.link_declaration_constant(cursor);
      });

  BAIL_IF(failed);

  stage = Stage::InitializersLinked;
  return True;
}

auto Types::Composite::link_callable_signatures(Cursor& cursor) -> Bool {
  if (stage >= Stage::CallableSignaturesLinked) {
    return True;
  }

  if (stage != Stage::TypesLinked) {
    cursor.create_expression_error(
        get_anchor(),
        "Composite Callable signatures require linked declaration Types."_view,
        "Settle every nested Type before completing Callable signatures."_view);
    return False;
  }

  Bool failed = !visit_each<Type>(types.get_view(), [&](Type& type) {
    return type.link_callable_signatures(cursor);
  });
  failed |= !visit_each<Model::Callable>(
      get_callable_bindings(), [&](Model::Callable& callable) {
        return callable.link_declaration_signature(cursor);
      });

  BAIL_IF(failed);

  stage = Stage::CallableSignaturesLinked;
  return True;
}

auto Types::Composite::link_callable_bodies(Cursor& cursor) -> Bool {
  if (stage >= Stage::CallablesLinked) {
    return True;
  }

  if (stage != Stage::InitializersLinked) {
    cursor.create_expression_error(
        get_anchor(),
        "Composite Callable bodies require linked initializers."_view,
        "Complete every Field Expression before linking Callable bodies."_view);
    return False;
  }

  Bool failed = !visit_each<Type>(types.get_view(), [&](Type& type) {
    return type.link_callable_bodies(cursor);
  });
  failed |= !visit_each<Model::Callable>(
      get_callable_bindings(), [&](Model::Callable& callable) {
        return callable.link_declaration_body(cursor);
      });

  BAIL_IF(failed);

  stage = Stage::CallablesLinked;
  return True;
}

auto Types::Composite::finalize(Cursor& cursor) -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }

  if (stage != Stage::CallablesLinked) {
    cursor.create_expression_error(
        get_anchor(), "An incomplete Composite cannot enter finalization."_view,
        "Link every Field, initializer, and Callable before finalizing."_view);
    return False;
  }

  Bool failed = !visit_each<Type>(
      types.get_view(), [&](Type& type) { return type.finalize(cursor); });

  failed |= !visit_each<Model::Addressable>(
      addressables.get_view(), [&](Model::Addressable& addressable) {
        return addressable.finalize_declaration(cursor);
      });

  // Callable folding still runs when publication fails. Independent cache and
  // diagnostic facts therefore remain observable without admitting the Type.
  failed |= !visit_each<Model::Callable>(
      get_callable_bindings(), [&](Model::Callable& callable) {
        return callable.finalize_declaration(cursor);
      });

  BAIL_IF(failed);

  stage = Stage::Finalized;
  return True;
}

auto Types::Composite::link_restored_types() -> Bool {
  if (stage >= Stage::TypesLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::Authored);

  while (link_aliases() != 0) {
  }
  for (const Reference<Abstract>& binding : types.get_view()) {
    auto alias = binding.get().select<Language::Alias>();
    BAIL_IF(alias && !alias->is_linked());
  }
  BAIL_IF(!visit_each<Model::Type>(types.get_view(), [](Model::Type& type) {
    return type.link_restored_types();
  }));

  stage = Stage::TypesLinked;
  return True;
}

auto Types::Composite::link_restored_callable_signatures() -> Bool {
  if (stage >= Stage::CallableSignaturesLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::TypesLinked);

  BAIL_IF(!visit_each<Model::Type>(types.get_view(), [](Model::Type& type) {
    return type.link_restored_callable_signatures();
  }));
  BAIL_IF(!visit_each<Model::Callable>(
      get_callable_bindings(), [](Model::Callable& callable) {
        return callable.link_restored_declaration_signature();
      }));

  stage = Stage::CallableSignaturesLinked;
  return True;
}

auto Types::Composite::link_restored_fields() -> Bool {
  if (stage >= Stage::FieldsLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::CallableSignaturesLinked);

  BAIL_IF(!visit_each<Model::Type>(types.get_view(), [](Model::Type& type) {
    return type.link_restored_fields();
  }));
  BAIL_IF(!visit_each<Model::Addressable>(
      addressables.get_view(), [](Model::Addressable& addressable) {
        return addressable.link_restored_declaration_type();
      }));

  complete_field_layout();
  stage = Stage::FieldsLinked;
  return True;
}

auto Types::Composite::link_restored_initializers() -> Bool {
  if (stage >= Stage::InitializersLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::FieldsLinked);

  for (const Reference<Abstract>& binding : types.get_view()) {
    auto type = binding.get().select<Model::Type>();
    if (type && !type->link_restored_initializers()) {
      Diagnostics::Log::Message<256> message(Diagnostics::Log::Level::Error);
      message << "Restored Type initializer closure failed for '"_view
              << type->get_name() << "'."_view;
      return False;
    }
  }

  for (const Reference<Abstract>& binding : addressables.get_view()) {
    auto addressable = binding.get().select<Model::Addressable>();
    if (addressable && !addressable->link_restored_declaration_initializer()) {
      Diagnostics::Log::Message<256> message(Diagnostics::Log::Level::Error);
      message << "Restored Addressable initializer failed for '"_view
              << addressable->get_name() << "'."_view;
      return False;
    }
  }

  stage = Stage::CallablesLinked;
  return True;
}

auto Types::Composite::finalize_restored() -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }
  BAIL_IF(stage != Stage::CallablesLinked);

  BAIL_IF(!visit_each<Model::Type>(types.get_view(), [](Model::Type& type) {
    return type.finalize_restored();
  }));
  stage = Stage::Finalized;
  return True;
}

auto Types::Composite::resolve() const -> const Abstract& {
  if (stage < Stage::FieldsLinked) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Types::Composite::resolve_binding(
    View::Bytes route,
    Category category,
    Visibility visibility,
    Bool self) const -> const Abstract& {
  return names.resolve(route, category, visibility, self);
}

auto Types::Composite::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& local = resolve_public_context(route);
  return !local.is<Invalid>() ? local : get_host().resolve_context(route);
}

auto Types::Composite::resolve_public_context(View::Bytes route) const
    -> const Abstract& {
  return resolve_binding(route, Category::Type, Visibility::Public);
}

auto Types::Composite::resolve_lexical_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& addressable =
      resolve_type_access(*this, route, Type::Access::Static);
  if (!addressable.is<Invalid>()) {
    return addressable;
  }

  const Abstract& type =
      resolve_binding(route, Category::Type, Visibility::Private);
  if (!type.is<Invalid>()) {
    return type;
  }

  auto enclosing = get_host().select<Type>();
  return enclosing ? enclosing->resolve_lexical_context(route)
                   : get_host().resolve_context(route);
}

auto Types::Composite::resolve_type_access(
    const Abstract& host,
    View::Bytes route,
    Type::Access access) const -> const Abstract& {
  const Abstract& binding =
      resolve_binding(route, Category::Addressable, Visibility::Private);
  if (binding.is<Invalid>()) {
    return binding;
  }

  const Abstract& resolved = binding.resolve();
  auto addressable = resolved.select<Model::Addressable>();
  if (!addressable || !addressable->supports_access(access)) {
    return Invalid::get_invalid();
  }

  auto caller = host.select<Type>();
  if (names.is_published(binding) ||
      (caller && caller->has_private_access_to(*this))) {
    return binding;
  }
  return Invalid::get_invalid();
}

auto Types::Composite::resolve_type_call(
    const Abstract& host,
    View::Bytes route,
    Type::Access access) const -> const Abstract& {
  Bool self = access == Type::Access::Self;
  const Abstract& binding =
      resolve_binding(route, Category::Callable, Visibility::Private, self);
  if (binding.is<Invalid>()) {
    return binding;
  }

  auto callable = binding.resolve().select<Model::Callable>();
  if (!callable || callable->declares_self() != self) {
    return Invalid::get_invalid();
  }

  auto caller = host.select<Type>();
  if (names.is_published(binding) ||
      (caller && caller->has_private_access_to(*this))) {
    return binding;
  }
  return Invalid::get_invalid();
}

auto Types::Composite::is_externally_reachable(const Type& type) const -> Bool {
  const Abstract& local =
      resolve_binding(type.get_name(), Category::Type, Visibility::Public);
  if (!local.is<Invalid>()) {
    return &local.resolve() == &type;
  }

  auto enclosing = get_host().select<Type>();
  if (enclosing) {
    return enclosing->is_externally_reachable(type);
  }

  return &get_host().resolve_context(type.get_name()).resolve() == &type;
}

auto Types::Composite::get_layout() const -> const Ttx::Model::Layouts::Named& {
  return layout.visit(
      []() -> const Ttx::Model::Layouts::Named& { return empty_layout; },
      [](const Ttx::Model::Layouts::Named& selected)
          -> const Ttx::Model::Layouts::Named& { return selected; });
}
