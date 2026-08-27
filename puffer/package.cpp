// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/package.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/version.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "perimortem/utility/table.hpp"

#include "puffer/dependencies.hpp"
#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/linker/elf/object.hpp"
#include "tetrodotoxin/linker/manifest.hpp"
#include "tetrodotoxin/linker/provider.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/archive/writer.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"
#include "tetrodotoxin/shader/dialect.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/terminal/abi/c/header.hpp"
#include "tetrodotoxin/terminal/abi/compiler.hpp"
#include "tetrodotoxin/terminal/abi/products.hpp"
#include "tetrodotoxin/terminal/abi/projection.hpp"
#include "tetrodotoxin/terminal/abi/resource_product.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/graphics/compiler.hpp"
#include "tetrodotoxin/terminal/llvm/compiler.hpp"
#include "tetrodotoxin/terminal/vulkan/compiler.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal;

using DebugEntry = Utility::Pair<Core::View::Bytes, Llvm::Module::Debug::Level>;
constexpr Core::Static::Vector<DebugEntry, 3> debug_entries = {{
  DebugEntry{"none"_view, Llvm::Module::Debug::Level::None},
  {"line"_view, Llvm::Module::Debug::Level::Line},
  {"full"_view, Llvm::Module::Debug::Level::Full},
}};
using DebugTable = Utility::Table<Llvm::Module::Debug::Level, debug_entries>;

using ImportKindEntry = Utility::Pair<Core::View::Bytes, Linker::Import::Kind>;
constexpr Core::Static::Vector<ImportKindEntry, 3> import_kind_entries = {{
  ImportKindEntry{"function"_view, Linker::Import::Kind::Function},
  {"readonly"_view, Linker::Import::Kind::ReadOnlyState},
  {"writable"_view, Linker::Import::Kind::WritableState},
}};
using ImportKindTable =
    Utility::Table<Linker::Import::Kind, import_kind_entries>;

using RouteAccess = Library::Language::Model::Type::Access;
using RouteAccessEntry = Utility::Pair<Core::View::Bytes, RouteAccess>;
constexpr Core::Static::Vector<RouteAccessEntry, 2> route_access_entries = {{
  RouteAccessEntry{"[self]"_view, RouteAccess::Self},
  {"[static]"_view, RouteAccess::Static},
}};
using RouteAccessTable = Utility::Table<RouteAccess, route_access_entries>;

static auto value(const System::Args::Values& arguments, Core::View::Bytes name)
    -> Core::View::Bytes {
  auto entry = arguments.find(name);
  if (!entry || entry->value->is_empty()) {
    return {};
  }

  return entry->value->get_view().get_data()[0];
}

static auto values(
    const System::Args::Values& arguments,
    Core::View::Bytes name) -> Core::View::Vector<Core::View::Bytes> {
  auto entry = arguments.find(name);
  return entry ? entry->value->get_view()
               : Core::View::Vector<Core::View::Bytes>();
}

static auto parse_debug(Core::View::Bytes text)
    -> Core::Option<Llvm::Module::Debug::Level> {
  return DebugTable::find(text);
}

static auto split(Core::View::Bytes value, U8 separator, Count index)
    -> Core::View::Bytes {
  Count selected = 0;
  Count start = 0;
  for (Count offset = 0; offset <= value.get_size(); offset++) {
    Bool terminal = offset == value.get_size();
    if (!terminal && value[offset] != separator) {
      continue;
    }

    if (selected == index) {
      return value.slice(start, offset - start);
    }

    selected++;
    start = offset + 1;
  }

  return {};
}

static auto split_count(Core::View::Bytes value, U8 separator) -> Count {
  Count count = value.is_empty() ? 0 : 1;
  for (Count index = 0; index < value.get_size(); index++) {
    count += value[index] == separator ? 1 : 0;
  }

  return count;
}

static auto package_paths(
    Core::View::Bytes manifest,
    Core::View::Bytes& root,
    Core::View::Bytes& logical) -> Bool {
  Count separator = Count(-1);
  for (Count index = 0; index < manifest.get_size(); index++) {
    if (manifest[index] == '/') {
      separator = index;
    }
  }

  BAIL_IF(separator == Count(-1) || separator + 1 == manifest.get_size());
  root = manifest.slice(0, separator);
  logical = manifest.slice(separator + 1);
  return True;
}

static auto append_host_path(
    Memory::Managed::Bytes& output,
    const Ttx::Concept::Abstract& semantic,
    Count start) -> Bool {
  if (auto function = semantic.select<Library::Language::Function>()) {
    BAIL_IF(!append_host_path(
        output, function->get_definition().get_host(), start));
    if (output.get_size() != start) {
      output.concat("::"_view);
    }

    output.concat(function->get_name());
    output.concat(function->declares_self() ? "[self]"_view : "[static]"_view);
    return True;
  } else if (auto field = semantic.select<Library::Language::Field>()) {
    BAIL_IF(
        !append_host_path(output, field->get_definition().get_host(), start));
    if (output.get_size() != start) {
      output.concat("::"_view);
    }

    output.concat(field->get_name());
    output.concat("[static]"_view);
    return True;
  } else if (
      auto composite = semantic.select<Library::Language::Types::Composite>()) {
    if (composite->is<Library::Language::Types::Source>()) {
      return True;
    }

    BAIL_IF(!append_host_path(
        output, composite->get_definition().get_host(), start));
    if (output.get_size() != start) {
      output.concat("::"_view);
    }

    output.concat(composite->get_name());
    return True;
  }

  return False;
}

static auto create_route(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes member,
    const Ttx::Concept::Abstract& semantic,
    const Tetrodotoxin::Terminal::Abi::Unit& unit)
    -> Core::Option<Core::View::Bytes> {
  const Library::Language::Model::Type* host = nullptr;
  if (auto function = semantic.select<Library::Language::Function>()) {
    host = &function->get_host();
  } else if (auto field = semantic.select<Library::Language::Field>()) {
    auto field_host = field->get_definition()
                          .get_host()
                          .select<Library::Language::Model::Type>();
    if (field_host) {
      host = &*field_host;
    }
  } else {
    auto type = semantic.select<Library::Language::Model::Type>();
    if (type) {
      host = &*type;
    }
  }

  auto binding =
      host ? unit.find_type(*host)
           : Core::Option<
                 const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding&>();
  if (binding && binding->get_package() == unit.get_package()) {
    Memory::Managed::Bytes output(arena, binding->get_route());
    if (auto function = semantic.select<Library::Language::Function>()) {
      output.concat("::"_view);
      output.concat(function->get_name());
      output.concat(
          function->declares_self() ? "[self]"_view : "[static]"_view);
    } else if (auto field = semantic.select<Library::Language::Field>()) {
      output.concat("::"_view);
      output.concat(field->get_name());
      output.concat("[static]"_view);
    }

    return output.get_view();
  }

  Memory::Managed::Bytes output(arena, member);
  output.concat("::"_view);
  Count start = output.get_size();
  BAIL_IF(!append_host_path(output, semantic, start));
  return output.get_view();
}

static auto resolve_context_route(
    const Ttx::Concept::Abstract& root,
    Core::View::Bytes route) -> Core::Option<const Ttx::Concept::Abstract&> {
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> selected(root);
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool end = index == route.get_size();
    Bool separator = !end && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!end && !separator) {
      continue;
    }

    Core::View::Bytes segment = route.slice(start, index - start);
    BAIL_IF(segment.is_empty());
    const Ttx::Concept::Abstract& context = selected.get().resolve();
    auto monograph = context.select<Tetrodotoxin::Language::Monograph>();
    const Ttx::Concept::Abstract& queried =
        monograph ? monograph->resolve_lexical_context(segment)
                  : context.resolve_context(segment);
    selected = Ttx::Concept::Reference<const Ttx::Concept::Abstract>(
        queried.resolve());
    BAIL_IF(selected.get().is<Ttx::Concept::Invalid>());
    if (separator) {
      index++;
    }

    start = index + 1;
  }

  return selected.get();
}

static auto resolve_source_route(
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Package::Language::Monograph& package,
    Core::View::Bytes route) -> Core::Option<const Ttx::Concept::Abstract&> {
  Core::Option<const Ttx::Concept::Abstract&> selected =
      resolve_context_route(package, route);
  if (selected) {
    return *selected;
  }

  Count count = workspace.get_package_source_count(package);
  for (Count index = 0; index < count; index++) {
    auto source = workspace.get_package_source(package, index);
    Core::View::Bytes candidate_route = route;
    if (source && route.get_size() > source->get_name().get_size() + 2 &&
        route.slice(0, source->get_name().get_size()) == source->get_name() &&
        route[source->get_name().get_size()] == ':' &&
        route[source->get_name().get_size() + 1] == ':') {
      candidate_route = route.slice(source->get_name().get_size() + 2);
    }
    auto candidate =
        source ? resolve_context_route(source->get_monograph(), candidate_route)
               : Core::Option<const Ttx::Concept::Abstract&>();
    if (!candidate) {
      continue;
    }

    if (selected && &selected->resolve() != &candidate->resolve()) {
      return {};
    }

    selected = *candidate;
  }

  return selected;
}

static auto select_library(
    const Tetrodotoxin::Language::Monograph& member,
    const Ttx::Concept::Abstract& library_dialect)
    -> Core::Option<const Library::Language::Monograph&> {
  auto direct = member.select<Library::Language::Monograph>();
  if (direct) {
    return *direct;
  }

  auto layer = member.get_layer(library_dialect);
  return layer ? layer->select<Library::Language::Monograph>()
               : Core::Option<const Library::Language::Monograph&>();
}

static auto retain_type_binding(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>&
        bindings,
    Core::View::Bytes package,
    Core::View::Bytes member,
    Core::View::Bytes route,
    const Ttx::Concept::Abstract& selected) -> Bool {
  const Ttx::Concept::Abstract& resolved = selected.resolve();
  auto type = resolved.select<Library::Language::Model::Type>();
  if (!type) {
    return True;
  }

  for (const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding :
       bindings.get_view()) {
    if (&binding.get_semantic() == &*type) {
      return True;
    } else if (
        binding.get_package() == package && binding.get_route() == route) {
      return False;
    }
  }

  Core::View::Bytes retained_route = arena.proxy(route);
  bindings.insert(
      Tetrodotoxin::Terminal::Abi::Unit::TypeBinding(
          *type, package, member, retained_route));
  auto composite = type->select<Library::Language::Types::Composite>();
  if (!composite) {
    return True;
  }

  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& nested :
       composite->get_types()) {
    Memory::Managed::Bytes nested_route(arena, retained_route);
    nested_route.concat("::"_view);
    nested_route.concat(nested.get().get_name());
    if (!retain_type_binding(
            arena, bindings, package, member, nested_route.get_view(),
            nested.get())) {
      return False;
    }
  }

  return True;
}

static auto retain_library_types(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>&
        bindings,
    Core::View::Bytes package,
    Core::View::Bytes member,
    Core::View::Bytes route_prefix,
    const Library::Language::Monograph& library) -> Bool {
  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& selected :
       library.get_source().get_types()) {
    Memory::Managed::Bytes route(arena, route_prefix);
    if (!route.get_view().is_empty()) {
      route.concat("::"_view);
    }

    route.concat(selected.get().get_name());
    if (!retain_type_binding(
            arena, bindings, package, member, route.get_view(),
            selected.get())) {
      return False;
    }
  }

  return True;
}

static auto contains_type(
    const Ttx::Concept::Abstract& candidate,
    const Library::Language::Model::Type& target) -> Bool {
  const Ttx::Concept::Abstract& resolved = candidate.resolve();
  auto type = resolved.select<Library::Language::Model::Type>();
  BAIL_IF(!type);
  if (&*type == &target) {
    return True;
  }

  auto composite = type->select<Library::Language::Types::Composite>();
  BAIL_IF(!composite);
  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& nested :
       composite->get_types()) {
    if (contains_type(nested.get(), target)) {
      return True;
    }
  }

  return False;
}

static auto find_type_owner(
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Package::Language::Monograph& package,
    const Ttx::Concept::Abstract& library_dialect,
    const Library::Language::Model::Type& target)
    -> Core::Option<Core::View::Bytes> {
  Count count = workspace.get_package_source_count(package);
  for (Count index = 0; index < count; index++) {
    auto source = workspace.get_package_source(package, index);
    if (!source) {
      continue;
    }

    auto library = select_library(source->get_monograph(), library_dialect);
    if (!library) {
      continue;
    }

    for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& selected :
         library->get_source().get_types()) {
      if (contains_type(selected.get(), target)) {
        return source->get_name();
      }
    }
  }

  return {};
}

static auto retain_export_type(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>&
        bindings,
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Package::Language::Monograph& package,
    const Ttx::Concept::Abstract& library_dialect,
    Core::View::Bytes package_name,
    Core::View::Bytes route,
    const Ttx::Concept::Abstract& selected) -> Bool {
  auto type = selected.resolve().select<Library::Language::Model::Type>();
  if (!type) {
    return True;
  }

  auto owner = find_type_owner(workspace, package, library_dialect, *type);
  if (owner) {
    for (const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding :
         bindings.get_view()) {
      if (&binding.get_semantic() == &*type) {
        return True;
      } else if (
          binding.get_package() == package_name &&
          binding.get_route() == route) {
        return False;
      }
    }

    bindings.insert(
        Tetrodotoxin::Terminal::Abi::Unit::TypeBinding(
            *type, package_name, *owner, arena.proxy(route)));
  }

  auto composite = type->select<Library::Language::Types::Composite>();
  if (!composite) {
    return True;
  }

  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& nested :
       composite->get_types(Tetrodotoxin::Language::Visibility::Public)) {
    Memory::Managed::Bytes nested_route(arena, route);
    nested_route.concat("::"_view);
    nested_route.concat(nested.get().get_name());
    if (!retain_export_type(
            arena, bindings, workspace, package, library_dialect, package_name,
            nested_route.get_view(), nested.get())) {
      return False;
    }
  }

  return True;
}

static auto retain_package_exports(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>&
        bindings,
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Package::Language::Monograph& package,
    const Ttx::Concept::Abstract& library_dialect,
    Core::View::Bytes package_name) -> Bool {
  for (const Ttx::Concept::Reference<Ttx::Model::Type>& reachable :
       package.get_reachable_types()) {
    auto import = reachable.get().select<Tetrodotoxin::Language::Import>();
    if (!import || import->get_visibility() ==
                       Tetrodotoxin::Language::Visibility::Private) {
      continue;
    }

    if (!retain_export_type(
            arena, bindings, workspace, package, library_dialect, package_name,
            import->get_name(), *import)) {
      return False;
    }
  }

  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& selected :
       package.get_library().get_source().get_types(
           Tetrodotoxin::Language::Visibility::Public)) {
    if (!retain_export_type(
            arena, bindings, workspace, package, library_dialect, package_name,
            selected.get().get_name(), selected.get())) {
      return False;
    }
  }

  return True;
}

static auto find_archived_type_owner(
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Package::Language::Monograph& package,
    const Ttx::Concept::Abstract& library_dialect,
    const Library::Language::Model::Type& target)
    -> Core::Option<Core::View::Bytes> {
  Count count = workspace.get_package_source_count(package);
  for (Count index = 0; index < count; index++) {
    auto source = workspace.get_package_source(package, index);
    if (!source) {
      continue;
    }

    auto library = select_library(source->get_monograph(), library_dialect);
    if (!library) {
      continue;
    }

    for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& candidate :
         library->get_source().get_types()) {
      if (contains_type(candidate.get(), target)) {
        return source->get_name();
      }
    }
  }

  return {};
}

static auto retain_archived_export_type(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>&
        bindings,
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Package::Language::Monograph& package,
    const Tetrodotoxin::Package::Archive::Archive& archive,
    const Ttx::Concept::Abstract& library_dialect,
    Core::View::Bytes route,
    const Ttx::Concept::Abstract& selected) -> Bool {
  auto type = selected.resolve().select<Library::Language::Model::Type>();
  if (!type) {
    return True;
  }

  auto owner =
      find_archived_type_owner(workspace, package, library_dialect, *type);
  if (owner) {
    for (const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding :
         bindings.get_view()) {
      if (&binding.get_semantic() == &*type) {
        return True;
      } else if (
          binding.get_package() == archive.get_identity() &&
          binding.get_route() == route) {
        return False;
      }
    }

    bindings.insert(
        Tetrodotoxin::Terminal::Abi::Unit::TypeBinding(
            *type, archive.get_identity(), *owner, arena.proxy(route)));
  }

  auto composite = type->select<Library::Language::Types::Composite>();
  if (!composite) {
    return True;
  }

  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& nested :
       composite->get_types(Tetrodotoxin::Language::Visibility::Public)) {
    Memory::Managed::Bytes nested_route(arena, route);
    nested_route.concat("::"_view);
    nested_route.concat(nested.get().get_name());
    if (!retain_archived_export_type(
            arena, bindings, workspace, package, archive, library_dialect,
            nested_route.get_view(), nested.get())) {
      return False;
    }
  }

  return True;
}

static auto retain_archived_package_exports(
    Memory::Allocator::Arena& arena,
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>&
        bindings,
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Package::Language::Monograph& package,
    const Tetrodotoxin::Package::Archive::Archive& archive,
    const Ttx::Concept::Abstract& library_dialect) -> Bool {
  for (const Ttx::Concept::Reference<Ttx::Model::Type>& reachable :
       package.get_reachable_types()) {
    auto import = reachable.get().select<Tetrodotoxin::Language::Import>();
    if (!import || import->get_visibility() ==
                       Tetrodotoxin::Language::Visibility::Private) {
      continue;
    }

    if (!retain_archived_export_type(
            arena, bindings, workspace, package, archive, library_dialect,
            import->get_name(), *import)) {
      return False;
    }
  }

  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& selected :
       package.get_library().get_source().get_types(
           Tetrodotoxin::Language::Visibility::Public)) {
    if (!retain_archived_export_type(
            arena, bindings, workspace, package, archive, library_dialect,
            selected.get().get_name(), selected.get())) {
      return False;
    }
  }

  return True;
}

struct RouteTerminal {
  Core::View::Bytes name;
  RouteAccess access;
};

struct UnitSpecification {
  Core::View::Bytes source;
  Core::View::Bytes object;
};

static auto parse_import_kind(Core::View::Bytes value)
    -> Core::Option<Linker::Import::Kind> {
  return ImportKindTable::find(value);
}

static auto terminal(Core::View::Bytes segment) -> Core::Option<RouteTerminal> {
  Count suffix_start = segment.get_size();
  while (suffix_start != 0 && segment[suffix_start - 1] != '[') {
    suffix_start--;
  }

  BAIL_IF(suffix_start == 0);
  auto access = RouteAccessTable::find(segment.slice(suffix_start - 1));
  BAIL_IF(!access);
  return RouteTerminal{segment.slice(0, suffix_start - 1), *access};
}

static auto resolve_route(
    const Environment::Workspace& workspace,
    const Package::Language::Monograph& package,
    Core::View::Bytes route) -> Core::Option<const Ttx::Concept::Abstract&> {
  auto direct = resolve_source_route(workspace, package, route);
  if (direct && direct->resolve().is<Library::Language::Model::Type>()) {
    return direct->resolve();
  }

  Memory::Dynamic::Vector<Core::View::Bytes> segments;
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool end = index == route.get_size();
    Bool separator = !end && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!end && !separator) {
      continue;
    }

    segments.insert(route.slice(start, index - start));
    if (separator) {
      index++;
    }

    start = index + 1;
  }

  BAIL_IF(segments.get_size() < 2);

  Memory::Dynamic::Bytes receiver_route(segments[0]);
  for (Count index = 1; index + 1 < segments.get_size(); index++) {
    receiver_route.concat("::"_view);
    receiver_route.concat(segments[index]);
  }

  auto receiver =
      resolve_source_route(workspace, package, receiver_route.get_view());
  BAIL_IF(!receiver);
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> selected(
      receiver->resolve());

  auto selected_terminal = terminal(segments[segments.get_size() - 1]);
  BAIL_IF(!selected_terminal || selected_terminal->name.is_empty());
  auto type = selected.get().select<Library::Language::Model::Type>();
  RouteAccess access = selected_terminal->access;
  const Ttx::Concept::Abstract& callable =
      type ? type->resolve_type_call(
                 selected.get(), selected_terminal->name, access)
           : selected.get().resolve_call(
                 selected.get(), selected_terminal->name);
  if (access == RouteAccess::Self) {
    BAIL_IF(callable.resolve().is<Ttx::Concept::Invalid>());
    return callable.resolve();
  } else {
    const Ttx::Concept::Abstract& addressable =
        type ? type->resolve_type_access(
                   selected.get(), selected_terminal->name, access)
             : selected.get().resolve_access(
                   selected.get(), selected_terminal->name);
    Bool has_callable = !callable.resolve().is<Ttx::Concept::Invalid>();
    Bool has_addressable = !addressable.resolve().is<Ttx::Concept::Invalid>();
    BAIL_IF(has_callable == has_addressable);
    return has_callable ? callable.resolve() : addressable.resolve();
  }
}

static auto publish(Core::View::Bytes path, Core::View::Bytes contents)
    -> Bool {
  return System::File::write(contents, path);
}

static auto report_errors(const Ttx::Lexical::Errors& errors) -> void {
  Memory::Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    Core::Diagnostics::Log::error(errors.render_message(arena, index));
    arena.reset();
  }
}

static auto retain_export(
    Memory::Managed::Vector<Tetrodotoxin::Package::Archive::Export>& exports,
    Core::View::Bytes route,
    Core::View::Bytes artifact,
    Core::View::Bytes symbol) -> Bool {
  for (const Tetrodotoxin::Package::Archive::Export& existing :
       exports.get_view()) {
    Bool same_route = existing.get_semantic_route() == route;
    Bool same_symbol = existing.get_symbol_locator() == symbol;
    if (same_route && same_symbol) {
      return True;
    } else if (same_route || same_symbol) {
      Core::Diagnostics::Log::Message<512> message(
          Core::Diagnostics::Log::Level::Error, Core::Diagnostics::Source());
      message << "Puffer Package export `"_view << route
              << "` with symbol `"_view << symbol << "` collides with `"_view
              << existing.get_semantic_route() << "` and `"_view
              << existing.get_symbol_locator() << "`."_view;
      return False;
    }
  }

  exports.insert(
      Tetrodotoxin::Package::Archive::Export(route, artifact, symbol));
  return True;
}

auto Puffer::Package::run() const -> S32 {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::plain_sink);

  Core::View::Bytes manifest = value(arguments, "manifest"_view);
  Core::View::Bytes identity = value(arguments, "name"_view);
  Core::View::Bytes complete_path = value(arguments, "complete"_view);
  Core::View::Bytes contract_path = value(arguments, "contract"_view);
  Core::View::Bytes header_path = value(arguments, "header"_view);
  Core::View::Bytes cpp_header_path = value(arguments, "cpp-header"_view);
  Core::View::Bytes cpp_source_path = value(arguments, "cpp-source"_view);
  Core::View::Bytes cpp_include = value(arguments, "cpp-include"_view);
  Core::View::Bytes c_include = value(arguments, "c-include"_view);
  Core::View::Bytes abi_manifest_path = value(arguments, "abi-manifest"_view);
  Core::View::Bytes resources_object_path =
      value(arguments, "resources-object"_view);
  System::Version version =
      System::Version::parse(value(arguments, "version"_view));
  auto debug = parse_debug(value(arguments, "debug"_view));
  Core::View::Bytes artifact = value(arguments, "artifact"_view);
  Core::View::Bytes spirv_target = value(arguments, "spirv-target"_view);
  Core::View::Bytes graphics_placement_route =
      value(arguments, "graphics-placement"_view);
  if (manifest.is_empty() || identity.is_empty() || complete_path.is_empty() ||
      contract_path.is_empty() || header_path.is_empty() ||
      abi_manifest_path.is_empty() || version.is_null() || !debug ||
      artifact != "x86_64-sysv-linux"_view ||
      spirv_target != "vulkan1.0"_view) {
    Core::Diagnostics::Log::error(
        "Puffer Package mode received an incomplete request."_view);
    return 2;
  }

  Bool cpp_api = !cpp_header_path.is_empty() || !cpp_source_path.is_empty();
  if (cpp_header_path.is_empty() != cpp_source_path.is_empty() ||
      cpp_api != (!cpp_include.is_empty() && !c_include.is_empty())) {
    Core::Diagnostics::Log::error(
        "Puffer Package needs both generated C++ product paths."_view);
    return 2;
  }

  Memory::Allocator::Arena arena;
  Memory::Managed::Bytes native_target(arena, artifact);
  native_target.concat("+"_view);
  native_target.concat(spirv_target);
  Memory::Dynamic::Vector<Linker::Provider> providers;
  for (Core::View::Bytes specification :
       values(arguments, "native-provider"_view)) {
    if (split_count(specification, '|') != 4) {
      return 2;
    }

    auto kind = parse_import_kind(split(specification, '|', 2));
    if (!kind) {
      return 2;
    }

    providers.insert(
        Linker::Provider(
            split(specification, '|', 0), split(specification, '|', 1), *kind,
            "C"_view, split(specification, '|', 3)));
  }

  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto render = toolchain.install<Render::Dialect>("Pipeline"_view);
  if (!library || !render ||
      !toolchain.install<Tetrodotoxin::Package::Dialect>(
          "Package"_view, *library) ||
      !toolchain.install<App::Dialect>("App"_view) ||
      !toolchain.install<Scene::Dialect>("Scene"_view, *library) ||
      !toolchain.install<Shader::Dialect>("Shader"_view, *library, *render)) {
    return 1;
  }

  Core::View::Bytes package_root;
  Core::View::Bytes manifest_route;
  if (!package_paths(manifest, package_root, manifest_route)) {
    return 2;
  }

  Memory::Dynamic::Vector<Memory::Dynamic::Bytes> dependency_bytes;
  Dependencies selected_dependencies(arena, repository);
  for (Core::View::Bytes dependency_path : values(arguments, "dep"_view)) {
    auto bytes = System::File::read(dependency_path);
    if (!bytes) {
      return 1;
    }

    dependency_bytes.emplace(static_cast<Memory::Dynamic::Bytes&&>(*bytes));
    auto decoded = Tetrodotoxin::Package::Archive::Reader::read(
        arena, dependency_bytes[dependency_bytes.get_size() - 1]);
    Core::Option<Tetrodotoxin::Package::Archive::Archive> archive;
    decoded.visit(
        [&](const Tetrodotoxin::Package::Archive::Archive& selected) {
          archive = selected;
        },
        [](const Tetrodotoxin::Package::Archive::Reader::Error&) {});
    if (!archive ||
        archive->get_profile() != Language::Persistence::Profile::Contract) {
      Core::Diagnostics::Log::error(
          "Puffer Package rejected one dependency Contract Archive."_view);
      return 1;
    }

    if (!selected_dependencies.retain(*archive)) {
      Core::Diagnostics::Log::error(
          "Puffer Package received a duplicate dependency Contract."_view);
      return 1;
    }
  }

  Environment::Workspace discovery(toolchain);
  for (const Tetrodotoxin::Package::Archive::Archive& archive :
       selected_dependencies.get_archives()) {
    if (!discovery.restore_package(archive, archive.get_identity())) {
      return 1;
    }
  }

  Ttx::Lexical::Errors discovery_errors;
  discovery.import_package(
      discovery_errors, package_root, "RootPackage"_view, manifest_route,
      identity, version);
  for (const Ttx::Concept::Reference<Tetrodotoxin::Language::Import>& pending :
       discovery.get_pending_package_imports()) {
    if (!selected_dependencies.acquire(
            pending.get().get_locator(), pending.get().get_version())) {
      Core::Diagnostics::Log::error(
          "Puffer Package could not acquire one dependency Contract."_view);
      return 1;
    }
  }

  Environment::Workspace workspace(toolchain);
  for (const Tetrodotoxin::Package::Archive::Archive& archive :
       selected_dependencies.get_archives()) {
    if (!workspace.restore_package(archive, archive.get_identity())) {
      Core::Diagnostics::Log::Message<256> message(
          Core::Diagnostics::Log::Level::Error, Core::Diagnostics::Source());
      message << "Puffer Package could not restore dependency `"_view
              << archive.get_identity() << "`."_view;
      return 1;
    }
  }

  Memory::Dynamic::Vector<Memory::Dynamic::Bytes> dependency_manifest_bytes;
  Memory::Managed::Vector<Linker::Manifest> dependency_manifests(arena);
  for (Core::View::Bytes manifest_path : values(arguments, "dep-abi"_view)) {
    auto bytes = System::File::read(manifest_path);
    if (!bytes) {
      return 1;
    }

    dependency_manifest_bytes.emplace(
        static_cast<Memory::Dynamic::Bytes&&>(*bytes));
    auto decoded = Linker::Manifest::read(
        arena,
        dependency_manifest_bytes[dependency_manifest_bytes.get_size() - 1]);
    Bool retained = decoded.visit(
        [&](const Linker::Manifest& manifest) -> Bool {
          dependency_manifests.insert(manifest);
          return True;
        },
        [](const Linker::Manifest::Error&) -> Bool { return False; });
    if (!retained) {
      return 1;
    }
  }

  auto dependency_archives = selected_dependencies.get_archives();
  for (const Tetrodotoxin::Package::Archive::Archive& archive :
       dependency_archives) {
    Count matches = 0;
    for (const Linker::Manifest& manifest : dependency_manifests.get_view()) {
      matches += manifest.get_artifact() == artifact &&
                         manifest.get_target() == native_target.get_view() &&
                         archive.matches(manifest)
                     ? 1
                     : 0;
    }

    if (matches == 0) {
      repository
          .select_manifest(
              archive.get_identity(), archive.get_version(), artifact)
          .visit(
              [&](const Linker::Manifest& manifest) {
                if (manifest.get_target() == native_target.get_view()) {
                  dependency_manifests.insert(manifest);
                  matches = 1;
                }
              },
              [](Tetrodotoxin::Package::Repository::Repository::Error) {});
    }

    if (matches != 1) {
      Core::Diagnostics::Log::error(
          "Puffer Package could not match one dependency ABI Manifest."_view);
      return 1;
    }
  }

  Ttx::Lexical::Errors errors;
  auto imported = workspace.import_package(
      errors, package_root, "RootPackage"_view, manifest_route, identity,
      version);
  auto root =
      imported ? imported->select<Tetrodotoxin::Package::Language::Monograph>()
               : Core::Option<Tetrodotoxin::Package::Language::Monograph&>();
  if (!root) {
    report_errors(errors);
    return 1;
  }

  auto library_dialect = toolchain.find("Library"_view);
  if (!library_dialect) {
    return 1;
  }

  Core::Option<const Ttx::Model::Type&> graphics_placement;
  Memory::Managed::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>>
      graphics_types(arena);
  if (!graphics_placement_route.is_empty()) {
    auto selected =
        resolve_source_route(workspace, *root, graphics_placement_route);
    graphics_placement = selected
                             ? selected->resolve().select<Ttx::Model::Type>()
                             : Core::Option<const Ttx::Model::Type&>();
    if (!graphics_placement) {
      Core::Diagnostics::Log::error(
          "Puffer Package could not resolve its configured graphics Placement2D."_view);
      return 1;
    }

    for (Core::View::Bytes route : values(arguments, "graphics-type"_view)) {
      auto selected_type = resolve_source_route(workspace, *root, route);
      auto resolved = selected_type
                          ? selected_type->resolve().select<Ttx::Model::Type>()
                          : Core::Option<const Ttx::Model::Type&>();
      if (!resolved) {
        Core::Diagnostics::Log::error(
            "Puffer Package could not resolve one configured graphics Type."_view);
        return 1;
      }

      graphics_types.insert(*resolved);
    }

    if (graphics_types.is_empty()) {
      return 2;
    }
  } else if (!values(arguments, "graphics-type"_view).is_empty()) {
    return 2;
  }

  Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::TypeBinding>
      type_bindings(arena);
  Memory::Managed::Vector<Core::View::Bytes> dependency_headers(arena);
  for (const Tetrodotoxin::Package::Archive::Archive& dependency :
       dependency_archives) {
    Memory::Managed::Bytes header_path(arena);
    Serialization::Stream::Textual<Memory::Managed::Bytes> header_stream(
        header_path);
    header_stream << dependency.get_identity() << "/"_view
                  << dependency.get_version().get_major() << "."_view
                  << dependency.get_version().get_minor() << "/c_abi.h"_view;
    dependency_headers.insert(header_path.get_view());
    auto dependency_root =
        workspace.resolve_context(dependency.get_identity())
            .resolve()
            .select<Tetrodotoxin::Package::Language::Monograph>();
    if (!dependency_root) {
      return 1;
    }

    if (!retain_archived_package_exports(
            arena, type_bindings, workspace, *dependency_root, dependency,
            *library_dialect)) {
      Core::Diagnostics::Log::error(
          "Puffer Package found colliding dependency export Type routes."_view);
      return 1;
    }

    Count dependency_source_count =
        workspace.get_package_source_count(*dependency_root);
    for (Count source_index = 0; source_index < dependency_source_count;
         source_index++) {
      auto source =
          workspace.get_package_source(*dependency_root, source_index);
      auto library =
          source ? select_library(source->get_monograph(), *library_dialect)
                 : Core::Option<const Library::Language::Monograph&>();
      if (library && !retain_library_types(
                         arena, type_bindings, dependency.get_identity(),
                         source->get_name(), source->get_name(), *library)) {
        Core::Diagnostics::Log::error(
            "Puffer Package found colliding dependency ABI Type routes."_view);
        return 1;
      }
    }
  }

  Count local_source_count = workspace.get_package_source_count(*root);
  if (!retain_package_exports(
          arena, type_bindings, workspace, *root, *library_dialect, identity)) {
    Core::Diagnostics::Log::error(
        "Puffer Package found colliding public ABI Type routes."_view);
    return 1;
  }

  for (Count source_index = 0; source_index < local_source_count;
       source_index++) {
    auto source = workspace.get_package_source(*root, source_index);
    if (!source) {
      return 1;
    }

    const auto& member = source->get_monograph();
    auto library = select_library(member, *library_dialect);
    if (library && !retain_library_types(
                       arena, type_bindings, identity, source->get_name(),
                       source->get_name(), *library)) {
      Core::Diagnostics::Log::error(
          "Puffer Package found colliding local ABI Type routes."_view);
      return 1;
    }
  }

  Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Unit::Binding> external(
      arena);
  for (const Tetrodotoxin::Package::Archive::Archive& dependency :
       dependency_archives) {
    auto dependency_root =
        workspace.resolve_context(dependency.get_identity())
            .resolve()
            .select<Tetrodotoxin::Package::Language::Monograph>();
    if (!dependency_root) {
      return 1;
    }

    for (const Tetrodotoxin::Package::Archive::Export& exported :
         dependency.get_exports()) {
      auto semantic = resolve_route(
          workspace, *dependency_root, exported.get_semantic_route());
      if (!semantic || exported.get_artifact_id() != artifact) {
        Core::Diagnostics::Log::Message<256> message(
            Core::Diagnostics::Log::Level::Error, Core::Diagnostics::Source());
        message << "Puffer Package could not bind export `"_view
                << exported.get_semantic_route() << "`."_view;
        return 1;
      }

      external.insert(
          Tetrodotoxin::Terminal::Abi::Unit::Binding(
              *semantic, exported.get_symbol_locator()));
    }
  }

  auto resources = Tetrodotoxin::Terminal::Abi::ResourceProduct::compile(
      arena, root->get_resources(), identity, artifact);
  if (!resources) {
    Core::Diagnostics::Log::error(
        "Puffer Package could not derive its Resource product."_view);
    return 1;
  }

  for (const Tetrodotoxin::Terminal::Abi::Unit::Binding& binding :
       resources->get_bindings()) {
    external.insert(binding);
  }

  if (!resources_object_path.is_empty()) {
    if (!publish(resources_object_path, resources->get_object())) {
      Core::Diagnostics::Log::error(
          "Puffer Package could not emit its Resource product."_view);
      return 1;
    }
  } else if (!resources->get_bindings().is_empty()) {
    Core::Diagnostics::Log::error(
        "Puffer Package has Resources without a target product path."_view);
    return 2;
  }

  Memory::Managed::Vector<Tetrodotoxin::Package::Archive::Export> exports(
      arena);
  Memory::Managed::Vector<Linker::Import> selected_imports(arena);
  Memory::Managed::Bytes abi_description(arena, native_target.get_view());
  Memory::Dynamic::Bytes combined_header;
  Memory::Dynamic::Vector<UnitSpecification> units;
  for (Core::View::Bytes specification : values(arguments, "unit"_view)) {
    if (split_count(specification, '|') != 2) {
      return 2;
    }

    units.insert(
        UnitSpecification{
          .source = split(specification, '|', 0),
          .object = split(specification, '|', 1),
        });
  }

  if (units.get_size() != local_source_count) {
    return 2;
  }

  Memory::Dynamic::Vector<UnitSpecification> product_units;
  for (Core::View::Bytes specification :
       values(arguments, "product-unit"_view)) {
    if (split_count(specification, '|') != 2) {
      return 2;
    }

    product_units.insert(
        UnitSpecification{
          .source = split(specification, '|', 0),
          .object = split(specification, '|', 1),
        });
  }

  if (product_units.get_size() != local_source_count) {
    return 2;
  }

  if (cpp_api && units.get_size() != 1) {
    Core::Diagnostics::Log::error(
        "Generated C++ Package publication currently needs one source member."_view);
    return 2;
  }

  Core::View::Bytes cpp_header;
  Core::View::Bytes cpp_source;

  for (Count source_index = 0; source_index < local_source_count;
       source_index++) {
    auto declared = workspace.get_package_source(*root, source_index);
    if (!declared) {
      return 1;
    }

    Memory::Managed::Bytes declared_source(arena, package_root);
    declared_source.append('/');
    declared_source.concat(declared->get_logical_route());
    Core::Option<const UnitSpecification&> unit_specification;
    auto retained_units = units.get_view();
    for (Count unit_index = 0; unit_index < retained_units.get_size();
         unit_index++) {
      const UnitSpecification& candidate =
          retained_units.get_data()[unit_index];
      if (candidate.source == declared_source) {
        if (unit_specification) {
          return 2;
        }

        unit_specification = candidate;
      }
    }

    if (!unit_specification) {
      return 2;
    }

    Core::Option<const UnitSpecification&> product_specification;
    auto retained_products = product_units.get_view();
    for (Count product_index = 0; product_index < retained_products.get_size();
         product_index++) {
      const UnitSpecification& candidate =
          retained_products.get_data()[product_index];
      if (candidate.source == declared_source) {
        if (product_specification) {
          return 2;
        }

        product_specification = candidate;
      }
    }

    if (!product_specification) {
      return 2;
    }

    Core::View::Bytes member_name = declared->get_name();
    Core::View::Bytes source_path = unit_specification->source;
    Core::View::Bytes object_path = unit_specification->object;
    Core::View::Bytes product_path = product_specification->object;
    const Tetrodotoxin::Language::Monograph& member = declared->get_monograph();

    auto source = System::File::read(source_path);
    if (!source) {
      return 1;
    }

    Tetrodotoxin::Terminal::Abi::Unit unit(
        identity, member_name, artifact, external.get_view(),
        type_bindings.get_view(), dependency_headers.get_view(), {}, c_include,
        cpp_include);
    Core::Option<Tetrodotoxin::Terminal::Abi::Products> native_interface;
    Core::Option<Llvm::Products> cpu_products;
    Core::Option<Tetrodotoxin::Terminal::Graphics::Products> graphics_products;
    Memory::Dynamic::Bytes member_object;
    Memory::Dynamic::Bytes product_object;
    auto shader = member.select<Shader::Language::Monograph>();
    Memory::Managed::Vector<Tetrodotoxin::Terminal::Abi::Projection>
        projections(arena);
    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Library::Language::Model::Callable>>
        excluded(arena);
    Linker::Elf::Object product_builder;
    Bool products_complete = True;
    if (shader) {
      Vulkan::Compiler compiler;
      for (const Ttx::Concept::Reference<Shader::Language::Program>& retained :
           shader->get_programs()) {
        const Shader::Language::Program& program = retained.get();
        for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& callable :
             program.get_callables()) {
          auto selected_callable =
              callable.get().select<Library::Language::Model::Callable>();
          if (!selected_callable) {
            return 1;
          }

          excluded.insert(*selected_callable);
        }

        Spirv::Request request(
            *shader, program, errors, source_path, *source,
            Spirv::Target::Vulkan1_0);
        Core::Option<Spirv::Products> module;
        compiler.compile_module(arena, request)
            .visit(
                [&](const Spirv::Products& compiled) { module = compiled; },
                [](Spirv::Failure) {});
        if (!module) {
          products_complete = False;
          break;
        }

        Tetrodotoxin::Terminal::Abi::Symbol symbol(
            arena, program, Tetrodotoxin::Terminal::Abi::Symbol::Kind::ReadOnly,
            unit);
        Memory::Managed::Bytes end_symbol(arena, symbol.get_view());
        end_symbol.concat("_end"_view);
        if (!product_builder.add_read_only(
                symbol.get_view(), end_symbol.get_view(),
                module->get_module())) {
          products_complete = False;
          break;
        }

        auto projection = Tetrodotoxin::Terminal::Abi::Projection::create(
            arena, program, symbol.get_view(), unit);
        if (!projection) {
          products_complete = False;
          break;
        }

        projections.insert(*projection);

        Memory::Managed::Bytes route(arena, member_name);
        route.concat("::"_view);
        route.concat(program.get_name());
        if (!retain_export(
                exports, route.get_view(), artifact, symbol.get_view())) {
          return 1;
        }

        abi_description.concat(spirv_target);
        abi_description.concat(symbol.get_view());
      }
    }

    auto embedded = products_complete ? product_builder.build()
                                      : Core::Option<Memory::Dynamic::Bytes>();
    if (embedded) {
      product_object = static_cast<Memory::Dynamic::Bytes&&>(*embedded);
    }

    auto library_layer = select_library(member, *library_dialect);
    if (library_layer) {
      auto scene = member.select<Scene::Language::Monograph>();
      if (scene && graphics_placement) {
        Tetrodotoxin::Terminal::Graphics::Compiler graphics_compiler;
        graphics_products = graphics_compiler.compile(
            arena, *scene, *graphics_placement, graphics_types.get_view());
        if (!graphics_products) {
          Core::Diagnostics::Log::error(
              "Puffer Package could not derive the Scene graphics product."_view);
          return 1;
        }
      }

      Tetrodotoxin::Terminal::Abi::Compiler interface_compiler;
      native_interface = interface_compiler.compile(
          arena, *library_layer, unit, errors, source_path, *source,
          excluded.get_view(), projections.get_view());
      if (native_interface) {
        Llvm::Request request(
            *library_layer, errors, source_path, *source,
            Llvm::Target::X86_64SysV, *debug, unit, *native_interface,
            graphics_products
                ? Core::Option<
                      const Tetrodotoxin::Terminal::Graphics::Products&>(
                      *graphics_products)
                : Core::Option<
                      const Tetrodotoxin::Terminal::Graphics::Products&>(),
            excluded.get_view());
        Llvm::Compiler compiler;
        compiler.compile(arena, request)
            .visit(
                [&](const Llvm::Products& compiled) {
                  cpu_products = compiled;
                },
                [](const Llvm::Failure&) {});
      }

      if (cpu_products) {
        member_object = cpu_products->get_object();
      }
    } else {
      Linker::Elf::Object empty_builder;
      auto empty = empty_builder.build();
      if (empty) {
        member_object = static_cast<Memory::Dynamic::Bytes&&>(*empty);
      }
    }

    if (member_object.is_empty() || product_object.is_empty() ||
        !publish(object_path, member_object) ||
        !publish(product_path, product_object)) {
      report_errors(errors);
      Core::Diagnostics::Log::error(
          "Puffer Package could not emit one member product."_view);
      return 1;
    }

    if (native_interface) {
      combined_header.concat(native_interface->get_c_header());
    }

    if (cpp_api && !native_interface) {
      Core::Diagnostics::Log::error(
          "The generated C++ interface requires a Library member."_view);
      return 1;
    }

    if (cpp_api) {
      cpp_header = native_interface->get_cpp_header();
      cpp_source = native_interface->get_cpp_source();
      if (cpp_header.is_empty() || cpp_source.is_empty()) {
        Core::Diagnostics::Log::error(
            "Puffer Package could not emit its generated C++ interface."_view);
        return 1;
      }
    }

    if (cpu_products) {
      abi_description.concat(cpu_products->get_abi_fingerprint().render(arena));
    }

    for (const Linker::Import& import :
         cpu_products ? cpu_products->get_imports()
                      : Core::View::Vector<Linker::Import>()) {
      Core::Option<Linker::Import> selected_import;
      Core::Option<Linker::Provider::Error> selection_error;
      Linker::Provider::select(providers.get_view(), artifact, import)
          .visit(
              [&](const Linker::Import& selected) {
                selected_import = selected;
              },
              [&](Linker::Provider::Error error) { selection_error = error; });
      if (selection_error || !selected_import) {
        Core::Diagnostics::Log::Message<384> message(
            Core::Diagnostics::Log::Level::Error, Core::Diagnostics::Source());
        message << "Puffer Package native provider selection failed for `"_view
                << import.get_symbol() << "` with reason `"_view
                << (selection_error && *selection_error ==
                                           Linker::Provider::Error::Ambiguous
                        ? "ambiguous"_view
                        : "missing"_view)
                << "`."_view;
        return 1;
      }

      Linker::Import selected = *selected_import;
      Bool duplicate = False;
      for (const Linker::Import& existing : selected_imports.get_view()) {
        if (existing.get_symbol() != selected.get_symbol()) {
          continue;
        }

        if (!(existing == selected)) {
          Core::Diagnostics::Log::error(
              "Puffer Package selected conflicting providers for one import."_view);
          return 1;
        }

        duplicate = True;
        break;
      }

      if (!duplicate) {
        selected_imports.insert(selected);
        abi_description.concat(selected.get_provider());
        abi_description.concat(selected.get_abi());
        abi_description.concat(selected.get_symbol());
      }
    }

    if (native_interface) {
      for (const Tetrodotoxin::Terminal::Abi::Publication& publication :
           native_interface->get_publications()) {
        auto route =
            create_route(arena, member_name, publication.get_semantic(), unit);
        if (!route) {
          Core::Diagnostics::Log::error(
              "Puffer Package could not create one export route."_view);
          return 1;
        }

        if (!retain_export(
                exports, *route, artifact, publication.get_symbol())) {
          return 1;
        }
      }
    }
  }

  Linker::Fingerprint abi_fingerprint =
      Linker::Fingerprint::create(abi_description.get_view());
  auto identified_header = Tetrodotoxin::Terminal::Abi::C::Header::identify(
      arena, combined_header.get_view(), identity, abi_fingerprint);
  if (!identified_header) {
    return 1;
  }

  Linker::Manifest native_manifest(
      identity, version, artifact, native_target.get_view(), abi_fingerprint,
      selected_imports.get_view());
  auto manifest_bytes = Linker::Manifest::write(native_manifest);
  if (!manifest_bytes) {
    return 1;
  }

  Tetrodotoxin::Package::Archive::Artifact artifacts[] = {
    Tetrodotoxin::Package::Archive::Artifact(
        artifact, native_target.get_view(), abi_fingerprint,
        selected_imports.get_view()),
  };
  Memory::Managed::Vector<Tetrodotoxin::Package::Archive::Writer::GraphMember>
      archive_members(arena);
  Memory::Managed::Vector<Tetrodotoxin::Package::Archive::GraphImport>
      archive_imports(arena);
  for (Count source_index = 0; source_index < local_source_count;
       source_index++) {
    auto source = workspace.get_package_source(*root, source_index);
    if (!source) {
      return 1;
    }

    archive_members.insert(
        Tetrodotoxin::Package::Archive::Writer::GraphMember(
            source->get_name(), source->get_monograph()));
  }

  auto retain_graph_imports =
      [&](Core::View::Bytes importer_name,
          const Tetrodotoxin::Language::Monograph& importer) -> Bool {
    for (const Ttx::Concept::Reference<Ttx::Model::Type>& retained :
         importer.get_reachable_types()) {
      auto import = retained.get().select<Tetrodotoxin::Language::Import>();
      if (!import) {
        continue;
      }

      Core::View::Bytes target = import->get_locator();
      if (import->get_kind() == Tetrodotoxin::Language::Import::Kind::Source) {
        target = {};
        auto acquired = import->get_acquired();
        for (Count source_index = 0; source_index < local_source_count;
             source_index++) {
          auto source = workspace.get_package_source(*root, source_index);
          if (source && acquired &&
              &source->get_monograph().get_root() == &*acquired) {
            target = source->get_name();
            break;
          }
        }

        if (target.is_empty()) {
          return False;
        }
      }

      archive_imports.insert(
          Tetrodotoxin::Package::Archive::GraphImport(
              importer_name, import->get_name(), import->get_visibility(),
              import->get_kind(), target, import->get_version(),
              import->get_route()));
    }

    return True;
  };
  if (!retain_graph_imports("PackageSurface"_view, *root)) {
    return 1;
  }

  for (Count source_index = 0; source_index < local_source_count;
       source_index++) {
    auto source = workspace.get_package_source(*root, source_index);
    if (!source ||
        !retain_graph_imports(source->get_name(), source->get_monograph())) {
      return 1;
    }
  }

  auto complete = Tetrodotoxin::Package::Archive::Writer::write(
      *root, identity, version, Language::Persistence::Profile::Complete,
      archive_members.get_view(), archive_imports.get_view(), artifacts,
      exports.get_view());
  auto contract = Tetrodotoxin::Package::Archive::Writer::write(
      *root, identity, version, Language::Persistence::Profile::Contract,
      archive_members.get_view(), archive_imports.get_view(), artifacts,
      exports.get_view());
  if (!complete) {
    Core::Diagnostics::Log::error(
        "Puffer Package could not encode its Complete Archive."_view);
    return 1;
  }

  if (!contract) {
    Core::Diagnostics::Log::error(
        "Puffer Package could not encode its Contract Archive."_view);
    return 1;
  }

  if (!complete || !contract || !publish(complete_path, *complete) ||
      !publish(contract_path, *contract) ||
      !publish(header_path, identified_header->get_view()) ||
      (cpp_api && (!publish(cpp_header_path, cpp_header) ||
                   !publish(cpp_source_path, cpp_source))) ||
      !publish(abi_manifest_path, *manifest_bytes)) {
    Core::Diagnostics::Log::error(
        "Puffer Package could not publish its completed products."_view);
    return 1;
  }

  return 0;
}
