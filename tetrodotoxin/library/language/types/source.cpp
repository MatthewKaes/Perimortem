// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/source.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Tetrodotoxin::Language::Visibility;
using Type = Model::Type;

static auto is_foreign_keyword(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Addressable) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "foreign"_view;
}

auto Types::Source::create_synthetic(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Abstract& host,
    const Anchor& source_anchor) -> Source& {
  // The root has no instance state, so its empty Layout exists before any
  // Function Definition names Source as its host. Static Fields cannot change
  // that Source owned value.
  auto& definition = Tetrodotoxin::Language::Definition::create_synthetic(
      domain, documentation, host, "<source>"_view, Visibility::Public,
      source_anchor);
  return domain.construct_from<Source>(
      [&]() -> Source { return Source(domain, definition); });
}

auto Types::Source::parse_definition(
    Cursor& cursor,
    const Documentation& documentation) -> Bool {
  auto definition =
      Tetrodotoxin::Language::Definition::parse(cursor, documentation, *this);
  BAIL_IF(!definition || !interpret_definition(cursor, *definition));
  return True;
}

auto Types::Source::parse(Cursor& cursor) -> Bool {
  while (!cursor.matches(Code::Type::Terminal)) {
    // Every root form begins with the same optional Documentation. Source
    // parses it once and passes that exact object to the selected owner so
    // Import, Foreign, and Definition never speculate over the prefix
    // independently.
    const Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);

    if (cursor.matches(Code::Type::Using)) {
      auto import = Import::parse(cursor, documentation);
      BAIL_IF(!import || imports_linked);
      import_routes.insert(*import);
      continue;
    }

    if (is_foreign_keyword(cursor)) {
      BAIL_IF(!foreign.parse(cursor, documentation));
      continue;
    }

    BAIL_IF(!parse_definition(cursor, documentation));
  }

  return True;
}

auto Types::Source::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::Source);
  BAIL_IF(
      !writer.write(get_documentation()) || import_routes.get_size() > U32(-1));

  writer.write(U32(import_routes.get_size()));
  for (const Import& import : import_routes.get_view()) {
    BAIL_IF(!import.persist(writer));
  }

  writer.write(U8(foreign.is_authored() ? 1 : 0));
  BAIL_IF(foreign.is_authored() && !foreign.persist(writer));

  Bool public_only = writer.get_profile() ==
                     Tetrodotoxin::Language::Persistence::Profile::Interface;
  BAIL_IF(!persist_declarations(writer, public_only));
  return writer.finish(record);
}

auto Types::Source::restore(
    Archive::Reader& contents,
    Tetrodotoxin::Language::Persistence::Profile profile) -> Bool {
  auto import_count = contents.read_u32();
  BAIL_IF(!import_count);
  for (Count index = 0; index < *import_count; index++) {
    auto import = Import::restore(contents, get_domain(), get_host());
    BAIL_IF(!import);
    import_routes.insert(*import);
  }

  auto has_foreign = contents.read_u8();
  BAIL_IF(!has_foreign || *has_foreign > 1);
  BAIL_IF(*has_foreign == 1 && !foreign.restore(contents));
  return restore_declarations(contents, profile);
}

auto Types::Source::link_types(Cursor& cursor) -> Bool {
  while (link_aliases() != 0) {
  }
  BAIL_IF(!validate_aliases(cursor));
  return Composite::link_types(cursor) && foreign.link_types(cursor);
}

auto Types::Source::link_fields(Cursor& cursor) -> Bool {
  BAIL_IF(!Composite::link_fields(cursor));
  return validate_layout(cursor);
}

auto Types::Source::link_initializers(Cursor& cursor) -> Bool {
  return Composite::link_initializers(cursor);
}

auto Types::Source::link_callable_signatures(Cursor& cursor) -> Bool {
  return foreign.link_callables(cursor) &&
         Composite::link_callable_signatures(cursor);
}

auto Types::Source::link_callable_bodies(Cursor& cursor) -> Bool {
  return Composite::link_callable_bodies(cursor);
}

auto Types::Source::finalize(Cursor& cursor) -> Bool {
  return Composite::finalize(cursor) && foreign.finalize(cursor);
}

auto Types::Source::link(Cursor& cursor, Abstract& interpretation_context)
    -> Bool {
  // Source owns the closure sequence because every phase mutates the same
  // declaration tree and its one Foreign context. Monograph only supplies the
  // package context that precedes this source graph.
  BAIL_IF(!link_imports(cursor, interpretation_context));
  BAIL_IF(!link_types(cursor));
  BAIL_IF(!link_callable_signatures(cursor));
  BAIL_IF(!link_fields(cursor));
  BAIL_IF(!link_initializers(cursor));
  return link_callable_bodies(cursor);
}

auto Types::Source::link_restored(Abstract& interpretation_context) -> Bool {
  if (!imports_linked) {
    for (const Import& import : import_routes.get_view()) {
      Option<const Abstract&> selected;
      import.get_type_reference()
          .resolve(interpretation_context)
          .visit(
              [&](const Abstract& resolved) { selected = resolved; },
              [](const TypeReference::Failure&) {});
      if (!selected || !retain_import(selected->resolve())) {
        Diagnostics::Log::error(
            "Restored Library Import did not resolve in Package context."_view);
        return False;
      }
    }
    imports_linked = True;
  }

  if (!Composite::link_restored_types()) {
    Diagnostics::Log::error("Restored Library Types failed linking."_view);
    return False;
  }
  if (!foreign.link_restored()) {
    Diagnostics::Log::error("Restored Library Foreign failed linking."_view);
    return False;
  }
  if (!Composite::link_restored_callable_signatures()) {
    Diagnostics::Log::error(
        "Restored Library Callable signatures failed linking."_view);
    return False;
  }
  if (!Composite::link_restored_fields()) {
    Diagnostics::Log::error("Restored Library Fields failed linking."_view);
    return False;
  }
  if (!validate_layout_restored()) {
    Diagnostics::Log::error(
        "Restored Library value Layout does not terminate."_view);
    return False;
  }
  if (!Composite::link_restored_initializers()) {
    Diagnostics::Log::error(
        "Restored Library initializers failed linking."_view);
    return False;
  }
  return True;
}

auto Types::Source::finalize_restored() -> Bool {
  return Composite::finalize_restored() && foreign.finalize_restored();
}

auto Types::Source::can_bind_static(const Abstract& binding, Category category)
    const -> Bool {
  // This query proves namespace and publication collisions independently from
  // lifecycle. Import discovery can therefore preflight future Addressables
  // before publishing any Type or Callable from the same transaction.
  BAIL_IF(
      is_finalized() || binding.get_name() == "foreign"_view ||
      !can_bind_definition(binding, category));

  if (category != Category::Type) {
    return True;
  }

  View::Bytes name = binding.get_name();
  return get_host().resolve_context(name).is<Invalid>();
}

auto Types::Source::retain_import(const Abstract& imported) -> Bool {
  const Abstract& context = imported.resolve();
  BAIL_IF(context.is<Invalid>() || &context == this);

  if (imports.get_view().contains(
          [&](const Reference<const Abstract>& retained) -> Bool {
            return &retained.get() == &context;
          })) {
    return True;
  }

  auto has_conflict = [&](auto bindings) -> Bool {
    for (const Reference<Abstract>& binding : bindings) {
      if (!context.resolve_context(binding.get().get_name()).is<Invalid>()) {
        return True;
      }
    }
    return False;
  };
  BAIL_IF(
      has_conflict(get_addressables()) || has_conflict(get_types()) ||
      has_conflict(get_callables()));

  imports.insert(context);
  return True;
}

auto Types::Source::link_imports(
    Cursor& cursor,
    Abstract& interpretation_context) -> Bool {
  if (imports_linked) {
    return True;
  }

  if (import_routes.is_empty()) {
    imports_linked = True;
    return True;
  }

  // Each using contributes one fallback context. Resolving the authored route
  // against the package context prevents local declarations from selecting
  // themselves while the Source is still incomplete.
  Bool failed = False;
  for (Count import_index = 0; import_index < import_routes.get_size();
       import_index++) {
    const Import& import = import_routes[import_index];
    Bool duplicate = import_routes.get_view()
                         .slice(0, import_index)
                         .contains([&](const Import& earlier) -> Bool {
                           return earlier.matches(import);
                         });
    if (duplicate) {
      cursor.create_expression_error(
          Anchor::create(import.get_span()),
          "Library source repeats one exact Import route."_view,
          "Keep one authored Import for each contextual route."_view);
      failed = True;
      continue;
    }

    auto selected = import.get_type_reference().resolve_authored(
        cursor, interpretation_context);
    if (!selected || selected->resolve().is<Invalid>()) {
      if (selected) {
        cursor.create_expression_error(
            Anchor::create(import.get_span()),
            "Library Import route did not resolve to one contextual object."_view,
            "Publish the selected context before linking this source."_view);
      }
      failed = True;
      continue;
    }

    if (!retain_import(selected->resolve())) {
      cursor.create_expression_error(
          Anchor::create(import.get_span()),
          "Library Import conflicts with this source context."_view,
          "Keep each visible name owned by only one local or imported "
          "context."_view);
      failed = True;
    }
  }

  BAIL_IF(failed);
  imports_linked = True;
  return True;
}

auto Types::Source::bind_static(Abstract& binding, Category category) -> Bool {
  // Types and Callables enter only while the source declaration is open.
  // Addressables also have one deliberate late phase after every provider
  // Field has settled, but before any initializer consumes source lookup.
  Bool addressable_phase = category == Category::Addressable && is_linked();
  BAIL_IF(
      (!can_accept_definition() && !addressable_phase) ||
      !can_bind_static(binding, category));

  // Synthetic bindings admitted through this path have no Definition and
  // therefore never enter this source's public lookup index.
  publish_binding(binding, category, False, False);
  return True;
}

auto Types::Source::retain_binding(
    Abstract& binding,
    Tetrodotoxin::Language::Definition& definition,
    Category category,
    Cursor& cursor) -> Bool {
  BAIL_IF(!can_accept_definition());

  if (category == Category::Addressable) {
    auto addressable = binding.select<Model::Addressable>();
    BAIL_IF(!addressable);

    // Source has no instance value. The retained Addressable declares whether
    // it contributes storage so Field does not inspect its concrete host.
    if (addressable->contributes_to_instance_layout()) {
      cursor.create_token_error(
          definition.get_name_token(),
          "Library Source rejects instance state Fields."_view,
          "Use an ordinary Static Field or move state into a Structure or "
          "Object."_view);
      return False;
    }
  }

  if (category == Category::Callable) {
    auto callable = binding.select<Model::Callable>();
    BAIL_IF(!callable);
    if (callable->declares_self()) {
      Token name = definition.get_name_token();
      cursor.create_expression_error(
          name ? Option<Anchor>(Anchor::create(Span(name))) : Option<Anchor>(),
          "A top level Library Function cannot receive `self`."_view,
          "Remove `self` from the top level Function signature."_view);
      return False;
    }
  }

  BAIL_IF(!can_bind_static(binding, category));
  publish_binding(binding, category, definition.is_published());
  cursor.get_associations().create(definition.get_name_anchor(), binding);
  return True;
}

auto Types::Source::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // Foreign is one reserved receiver context, while authored Source names use
  // the ordinary public categories. The Monograph fallback composes intrinsic,
  // outer Package, and using contexts without copying any of their bindings.
  if (route == "foreign"_view && foreign.is_authored()) {
    return foreign;
  }

  const Abstract& local = resolve_local(route, Visibility::Public);
  if (!local.is<Invalid>()) {
    return local;
  }

  return get_host().resolve_context(route);
}

auto Types::Source::create_default(Allocator::Arena&) const
    -> Option<Model::Pack&> {
  // Source is an empty contextual root and never enters value flow. Absence
  // keeps that fact distinct from a completed Pack that produces zero values.
  return {};
}

auto Types::Source::resolve_imports(View::Bytes route) const
    -> const Abstract& {
  // Using contexts are composable query fallbacks, not an ordered shadowing
  // list. Context, access, and call queries all accept no answer as missing and
  // repeated answers only when they resolve to the same identity. Distinct
  // provider identities make the query ambiguous and therefore Invalid.
  Option<const Abstract&> selected;
  for (const Reference<const Abstract>& retained : imports.get_view()) {
    const Abstract& candidate = retained.get().resolve_context(route);
    if (candidate.is<Invalid>()) {
      continue;
    }
    if (selected && &selected->resolve() != &candidate.resolve()) {
      return Invalid::get_invalid();
    }
    selected = candidate;
  }

  return selected ? *selected : Invalid::get_invalid();
}

auto Types::Source::resolve_type_access(
    const Abstract& host,
    View::Bytes route,
    Type::Access access) const -> const Abstract& {
  const Abstract& local = Composite::resolve_type_access(host, route, access);
  if (!local.is<Invalid>()) {
    return local;
  }

  Option<const Abstract&> selected;
  for (const Reference<const Abstract>& retained : imports.get_view()) {
    const Abstract& context = retained.get();
    const Abstract& candidate = context.visit<Type>(
        [&](const Type& type) -> const Abstract& {
          return type.resolve_type_access(host, route, Type::Access::Static);
        },
        [&](const Abstract& provider) -> const Abstract& {
          return provider.resolve_access(host, route);
        });
    if (candidate.is<Invalid>()) {
      continue;
    }
    if (selected && &selected->resolve() != &candidate.resolve()) {
      return Invalid::get_invalid();
    }
    selected = candidate;
  }

  return selected ? *selected : Invalid::get_invalid();
}

auto Types::Source::resolve_type_call(
    const Abstract& host,
    View::Bytes route,
    Type::Access access) const -> const Abstract& {
  const Abstract& local = Model::Type::resolve_type_call(host, route, access);
  if (!local.is<Invalid>()) {
    return local;
  }

  Option<const Abstract&> selected;
  for (const Reference<const Abstract>& retained : imports.get_view()) {
    const Abstract& context = retained.get();
    const Abstract& candidate = context.visit<Type>(
        [&](const Type& type) -> const Abstract& {
          return type.resolve_type_call(host, route, Type::Access::Static);
        },
        [&](const Abstract& provider) -> const Abstract& {
          return provider.resolve_call(host, route);
        });
    if (candidate.is<Invalid>()) {
      continue;
    }
    if (selected && &selected->resolve() != &candidate.resolve()) {
      return Invalid::get_invalid();
    }
    selected = candidate;
  }

  return selected ? *selected : Invalid::get_invalid();
}

auto Types::Source::resolve_local(View::Bytes route, Visibility visibility)
    const -> const Abstract& {
  for (const Reference<Abstract>& binding : get_addressables(visibility)) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  for (const Reference<Abstract>& binding : get_types(visibility)) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  for (const Reference<Abstract>& binding : get_callables(visibility)) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  return Invalid::get_invalid();
}

auto Types::Source::reserve_carrier(Llvm::Program& program) const
    -> Option<Bool> {
  const auto& carriers = program.get_carriers();
  return carriers.reserve(program, *this, Llvm::Carriers::Kind::Context);
}

auto Types::Source::complete_carrier(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  return carriers.complete(program, *this, Llvm::Carriers::Kind::Context);
}

auto Types::Source::reserve(Llvm::Program& program) const -> Bool {
  Bool source_reserved = Composite::reserve(program);
  if (!source_reserved) {
    return False;
  }

  return foreign.reserve(program);
}

auto Types::Source::complete(Llvm::Program& program) const -> Bool {
  Bool source_completed = Composite::complete(program);
  if (!source_completed) {
    return False;
  }

  return foreign.complete(program);
}

auto Types::Source::lower(Llvm::Program& program) const -> Bool {
  Bool source_lowered = Composite::lower(program);
  if (!source_lowered) {
    return False;
  }

  return foreign.lower(program);
}
