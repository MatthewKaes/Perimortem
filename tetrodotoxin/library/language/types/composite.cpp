// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/composite.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/alias.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/parser/member.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
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

static auto select_definition_category(
    Token qualifier,
    Types::Composite::Category& category) -> Bool {
  switch (qualifier.get_code().get_type()) {
  case Code::Type::Type:
  case Code::Type::Assign:
    category = Types::Composite::Category::Addressable;
    return True;
  case Code::Type::Func:
    category = Types::Composite::Category::Callable;
    return True;
  case Code::Type::Alias:
  case Code::Type::Enum:
  case Code::Type::Struct:
  case Code::Type::Object:
    category = Types::Composite::Category::Type;
    return True;
  default:
    return False;
  }
}

template <typename bindings_type>
static auto retains_binding(bindings_type bindings, const Abstract& candidate)
    -> Bool {
  for (const Reference<Abstract>& binding : bindings) {
    if (&binding.get() == &candidate) {
      return True;
    }
  }
  return False;
}

static auto resolve_local_addressable(
    const Types::Composite& composite,
    const Abstract& host,
    View::Bytes route,
    Type::Access access) -> const Abstract& {
  for (const Reference<Abstract>& binding :
       composite.get_addressables(Visibility::Private)) {
    if (binding.get().get_name() != route) {
      continue;
    }

    const Abstract& resolved = binding.get().resolve();
    auto addressable = resolved.select<Model::Addressable>();
    if (!addressable || !addressable->supports_access(access)) {
      return Invalid::get_invalid();
    }

    auto caller = host.select<Type>();
    Bool published = retains_binding(
        composite.get_addressables(Visibility::Public), binding.get());
    if (published || (caller && caller->has_private_access_to(composite))) {
      return binding.get();
    }
    return Invalid::get_invalid();
  }

  return Invalid::get_invalid();
}

Types::Composite::Composite(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition)
    : definition(definition),
      domain(domain),
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

auto Types::Composite::interpret_definition(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  Category category = Category::Addressable;
  if (!select_definition_category(definition.get_qualifier(), category)) {
    cursor.create_token_error(
        definition.get_qualifier(),
        "Library members require a Type, `alias`, `enum`, `struct`, "
        "`object`, `func`, or inferred initializer qualifier."_view);
    return False;
  }

  auto member = Parser::Member::parse(cursor, definition);
  BAIL_IF(!member);
  if (!retain_binding(*member, definition, category, cursor)) {
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
    Category category) const -> Bool {
  View::Bytes candidate = binding.get_name();
  auto retains_identity = [&](auto category) {
    return category.contains([&](const Reference<Abstract>& existing) {
      return &existing.get() == &binding;
    });
  };
  if (candidate.is_empty() || retains_identity(addressables.get_view()) ||
      retains_identity(get_callable_bindings()) ||
      retains_identity(types.get_view())) {
    return False;
  }

  auto contains_name = [&](auto bindings) {
    return bindings.contains([&](const Reference<Abstract>& existing) {
      return existing.get().get_name() == candidate;
    });
  };
  switch (category) {
  case Category::Addressable:
    return !contains_name(addressables.get_view());
  case Category::Type:
    return !contains_name(types.get_view());
  case Category::Callable:
    return can_publish_callable(binding);
  }

  return False;
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
  return retains_binding(published_addressables.get_view(), declaration) ||
         retains_binding(published_types.get_view(), declaration) ||
         retains_binding(
             get_callable_bindings(Visibility::Public), declaration);
}

auto Types::Composite::persist_declarations(
    Archive::Writer& writer,
    Bool public_only) const -> Bool {
  Count hidden_slot = 0;
  for (const Reference<Abstract>& retained : declarations.get_view()) {
    const Abstract& declaration = retained.get();
    if (public_only && !is_published(declaration)) {
      auto field = declaration.select<Language::Field>();
      if (field && field->contributes_to_instance_layout()) {
        BAIL_IF(!field->persist_slot(writer, hidden_slot));
        hidden_slot++;
      }
      continue;
    }

    auto alias = declaration.select<Language::Alias>();
    if (alias) {
      BAIL_IF(!alias->persist(writer));
      continue;
    }

    auto addressable = declaration.select<Model::Addressable>();
    if (addressable) {
      BAIL_IF(!addressable->persist(writer));
      continue;
    }

    auto callable = declaration.select<Model::Callable>();
    if (callable) {
      BAIL_IF(!callable->persist(writer));
      continue;
    }

    auto type = declaration.select<Model::Type>();
    BAIL_IF(!type || !type->persist(writer));
  }
  return True;
}

auto Types::Composite::restore_declarations(
    Archive::Reader& reader,
    Tetrodotoxin::Language::Persistence::Profile profile) -> Bool {
  Count hidden_slot = 0;
  while (!reader.is_complete()) {
    Archive::Reader probe = reader;
    auto record = probe.read_record();
    BAIL_IF(!record);

    if (record->is_optional()) {
      BAIL_IF(!reader.read_record());
      continue;
    }

    Option<Abstract&> restored;
    Category category = Category::Addressable;
    Archive::Tag tag = Archive::Tag(record->get_tag());
    switch (tag) {
    case Archive::Tag::Alias: {
      auto selected = Language::Alias::restore(reader, domain, *this);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    case Archive::Tag::Field: {
      auto selected = Language::Field::restore(reader, domain, *this);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Addressable;
      break;
    }
    case Archive::Tag::FieldSlot: {
      BAIL_IF(
          profile != Tetrodotoxin::Language::Persistence::Profile::Interface);
      auto selected =
          Language::Field::restore_slot(reader, domain, *this, hidden_slot);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Addressable;
      hidden_slot++;
      break;
    }
    case Archive::Tag::Function: {
      auto selected = Language::Function::restore(reader, domain, *this);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Callable;
      break;
    }
    case Archive::Tag::Structure: {
      auto selected = Structure::restore(reader, domain, *this, profile);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    case Archive::Tag::Object: {
      auto selected = Object::restore(reader, domain, *this, profile);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    case Archive::Tag::Enumeration: {
      auto selected = Enumeration::restore(reader, domain, *this);
      BAIL_IF(!selected);
      restored = *selected;
      category = Category::Type;
      break;
    }
    default:
      return False;
    }

    BAIL_IF(!restored);
    Bool published = restored->visit<Language::Alias>(
        [](const Language::Alias& selected) {
          return selected.get_definition().is_published();
        },
        [](const Abstract& selected) -> Bool {
          auto addressable = selected.select<Model::Addressable>();
          if (addressable) {
            auto field = addressable->select<Language::Field>();
            return field && field->get_definition().is_published();
          }
          auto callable = selected.select<Language::Function>();
          if (callable) {
            return callable->get_definition().is_published();
          }
          auto composite = selected.select<Composite>();
          if (composite) {
            return composite->get_definition().is_published();
          }
          auto enumeration = selected.select<Enumeration>();
          return enumeration && enumeration->get_definition().is_published();
        });
    BAIL_IF(!publish_binding(*restored, category, published));
  }
  return True;
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

auto Types::Composite::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& type = find_binding(get_types(Visibility::Public), route);
  return !type.is<Invalid>() ? type : get_host().resolve_context(route);
}

auto Types::Composite::resolve_lexical_context(View::Bytes route) const
    -> const Abstract& {
  const Abstract& addressable =
      resolve_local_addressable(*this, *this, route, Type::Access::Static);
  if (!addressable.is<Invalid>()) {
    return addressable;
  }

  const Abstract& type = find_binding(get_types(Visibility::Private), route);
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
  return resolve_local_addressable(*this, host, route, access);
}

auto Types::Composite::is_externally_reachable(const Type& type) const -> Bool {
  const Abstract& local =
      find_binding(get_types(Visibility::Public), type.get_name());
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

auto Types::Composite::reserve(Llvm::Program& program) const -> Bool {
  auto reserved = reserve_carrier(program);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  Bool types_reserved = visit_each<Model::Type>(
      types.get_view(),
      [&](const Model::Type& type) { return type.reserve(program); });
  if (!types_reserved) {
    return False;
  }

  Bool addressables_reserved = visit_each<Model::Addressable>(
      addressables.get_view(), [&](const Model::Addressable& addressable) {
        return addressable.reserve_declaration(program);
      });
  if (!addressables_reserved) {
    return False;
  }

  return visit_each<Model::Callable>(
      get_callable_bindings(), [&](const Model::Callable& callable) {
        return callable.reserve_declaration(program);
      });
}

auto Types::Composite::reserve_value(Llvm::Program& program) const -> Bool {
  auto reserved = reserve_carrier(program);
  if (!reserved) {
    return False;
  }
  if (!*reserved) {
    return True;
  }

  for (const Reference<Abstract>& candidate : addressables.get_view()) {
    auto addressable = candidate.get().select<Model::Addressable>();
    if (addressable && addressable->contributes_to_instance_layout() &&
        !addressable->get_type().reserve_value(program)) {
      return False;
    }
  }
  return True;
}

auto Types::Composite::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool types_completed = visit_each<Model::Type>(
      types.get_view(),
      [&](const Model::Type& type) { return type.complete(program); });
  if (!types_completed) {
    return False;
  }

  Bool addressables_completed = visit_each<Model::Addressable>(
      addressables.get_view(), [&](const Model::Addressable& addressable) {
        return addressable.complete_declaration(program);
      });
  if (!addressables_completed) {
    return False;
  }

  Bool callables_completed = visit_each<Model::Callable>(
      get_callable_bindings(), [&](const Model::Callable& callable) {
        return callable.complete_declaration(program);
      });
  if (!callables_completed) {
    return False;
  }

  Bool carrier_completed = complete_carrier(program);
  return carrier_completed && complete_debug(program);
}

auto Types::Composite::complete_value(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }
  if (!*began) {
    return True;
  }

  for (const Reference<Abstract>& candidate : addressables.get_view()) {
    auto addressable = candidate.get().select<Model::Addressable>();
    if (addressable && addressable->contributes_to_instance_layout() &&
        !addressable->get_type().complete_value(program)) {
      return False;
    }
  }
  return complete_carrier(program) && complete_debug(program);
}

auto Types::Composite::lower(Llvm::Program& program) const -> Bool {
  Count type_index = 0;
  Count addressable_index = 0;
  Count callable_index = 0;
  auto type_bindings = types.get_view();
  auto addressable_bindings = addressables.get_view();
  auto callable_bindings = get_callable_bindings();
  while (True) {
    auto type = select_next<Model::Type>(type_bindings, type_index);
    auto addressable = select_next<Model::Addressable>(
        addressable_bindings, addressable_index);
    auto callable =
        select_next<Model::Callable>(callable_bindings, callable_index);
    if (!type && !addressable && !callable) {
      return True;
    }

    Count type_offset = declaration_offset(type);
    Count addressable_offset = declaration_offset(addressable);
    Count callable_offset = declaration_offset(callable);
    if (type_offset <= addressable_offset && type_offset <= callable_offset) {
      Bool lowered = type->lower(program);
      if (!lowered) {
        Perimortem::Core::Diagnostics::Log::error(
            "Library LLVM lowering rejected one Type declaration."_view);
        return False;
      }

      type_index++;
    } else if (addressable_offset <= callable_offset) {
      Bool lowered = addressable->lower_declaration(program);
      if (!lowered) {
        Perimortem::Core::Diagnostics::Log::error(
            "Library LLVM lowering rejected one Addressable declaration."_view);
        return False;
      }

      addressable_index++;
    } else {
      Bool lowered = callable->lower_declaration(program);
      if (!lowered) {
        Perimortem::Core::Diagnostics::Log::error(
            "Library LLVM lowering rejected one Callable declaration."_view);
        return False;
      }

      callable_index++;
    }
  }
}
