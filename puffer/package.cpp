// Tetrodotoxin
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
#include "tetrodotoxin/library/llvm/compiler.hpp"
#include "tetrodotoxin/library/llvm/header.hpp"
#include "tetrodotoxin/library/llvm/program.hpp"
#include "tetrodotoxin/library/llvm/symbol.hpp"
#include "tetrodotoxin/linker/manifest.hpp"
#include "tetrodotoxin/linker/provider.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/archive/writer.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

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
    -> Core::Option<Library::Llvm::Debug::Level> {
  if (text == "none"_view) {
    return Library::Llvm::Debug::Level::None;
  }
  if (text == "line"_view) {
    return Library::Llvm::Debug::Level::Line;
  }
  if (text == "full"_view) {
    return Library::Llvm::Debug::Level::Full;
  }
  return {};
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
  auto function = semantic.select<Library::Language::Function>();
  if (function) {
    BAIL_IF(!append_host_path(
        output, function->get_definition().get_host(), start));
    if (output.get_size() != start) {
      output.concat("::"_view);
    }
    output.concat(function->get_name());
    output.concat(function->declares_self() ? "[self]"_view : "[static]"_view);
    return True;
  }

  auto field = semantic.select<Library::Language::Field>();
  if (field) {
    BAIL_IF(
        !append_host_path(output, field->get_definition().get_host(), start));
    if (output.get_size() != start) {
      output.concat("::"_view);
    }
    output.concat(field->get_name());
    output.concat("[static]"_view);
    return True;
  }

  auto composite = semantic.select<Library::Language::Types::Composite>();
  if (composite) {
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
    const Ttx::Concept::Abstract& semantic) -> Core::Option<Core::View::Bytes> {
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
    selected = Ttx::Concept::Reference<const Ttx::Concept::Abstract>(
        selected.get().resolve_context(segment).resolve());
    BAIL_IF(selected.get().is<Ttx::Concept::Invalid>());
    if (separator) {
      index++;
    }
    start = index + 1;
  }
  return selected.get();
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
    Memory::Managed::Vector<Library::Llvm::Unit::TypeBinding>& bindings,
    Core::View::Bytes package,
    Core::View::Bytes member,
    Core::View::Bytes route,
    const Ttx::Concept::Abstract& selected) -> Bool {
  const Ttx::Concept::Abstract& resolved = selected.resolve();
  auto type = resolved.select<Library::Language::Model::Type>();
  if (!type) {
    return True;
  }

  for (const Library::Llvm::Unit::TypeBinding& binding : bindings.get_view()) {
    if (&binding.get_semantic() == &*type) {
      return True;
    }
    if (binding.get_package() == package && binding.get_route() == route) {
      return False;
    }
  }

  Core::View::Bytes retained_route = arena.proxy(route);
  bindings.insert(
      Library::Llvm::Unit::TypeBinding(*type, package, member, retained_route));
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
    Memory::Managed::Vector<Library::Llvm::Unit::TypeBinding>& bindings,
    Core::View::Bytes package,
    Core::View::Bytes member,
    const Library::Language::Monograph& library) -> Bool {
  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& selected :
       library.get_source().get_types()) {
    Memory::Managed::Bytes route(arena, member);
    route.concat("::"_view);
    route.concat(selected.get().get_name());
    if (!retain_type_binding(
            arena, bindings, package, member, route.get_view(),
            selected.get())) {
      return False;
    }
  }
  return True;
}

enum class RouteKind : U8 {
  Static,
  SelfCallable,
};

struct RouteTerminal {
  Core::View::Bytes name;
  RouteKind kind;
};

struct UnitSpecification {
  Core::View::Bytes source;
  Core::View::Bytes ir;
  Core::View::Bytes object;
};

static auto parse_import_kind(Core::View::Bytes value)
    -> Core::Option<Linker::Import::Kind> {
  if (value == "function"_view) {
    return Linker::Import::Kind::Function;
  }
  if (value == "readonly"_view) {
    return Linker::Import::Kind::ReadOnlyState;
  }
  if (value == "writable"_view) {
    return Linker::Import::Kind::WritableState;
  }
  return {};
}

static auto terminal(Core::View::Bytes segment) -> Core::Option<RouteTerminal> {
  constexpr Core::View::Bytes static_callable = "[static]"_view;
  constexpr Core::View::Bytes self_callable = "[self]"_view;
  if (segment.get_size() > static_callable.get_size() &&
      segment.slice(segment.get_size() - static_callable.get_size()) ==
          static_callable) {
    return RouteTerminal{
      segment.slice(0, segment.get_size() - static_callable.get_size()),
      RouteKind::Static,
    };
  }
  if (segment.get_size() > self_callable.get_size() &&
      segment.slice(segment.get_size() - self_callable.get_size()) ==
          self_callable) {
    return RouteTerminal{
      segment.slice(0, segment.get_size() - self_callable.get_size()),
      RouteKind::SelfCallable,
    };
  }
  return {};
}

static auto resolve_route(
    const Package::Language::Monograph& package,
    Core::View::Bytes route) -> Core::Option<const Ttx::Concept::Abstract&> {
  auto direct = resolve_context_route(package, route);
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

  Ttx::Concept::Reference<const Ttx::Concept::Abstract> selected(
      package.resolve_context(segments[0]).resolve());
  BAIL_IF(selected.get().is<Ttx::Concept::Invalid>());
  for (Count index = 1; index + 1 < segments.get_size(); index++) {
    selected = Ttx::Concept::Reference<const Ttx::Concept::Abstract>(
        selected.get().resolve_context(segments[index]).resolve());
    BAIL_IF(selected.get().is<Ttx::Concept::Invalid>());
  }

  auto selected_terminal = terminal(segments[segments.get_size() - 1]);
  BAIL_IF(!selected_terminal || selected_terminal->name.is_empty());
  auto type = selected.get().select<Library::Language::Model::Type>();
  Library::Language::Model::Type::Access access =
      selected_terminal->kind == RouteKind::SelfCallable
          ? Library::Language::Model::Type::Access::Self
          : Library::Language::Model::Type::Access::Static;
  const Ttx::Concept::Abstract& callable =
      type ? type->resolve_type_call(
                 selected.get(), selected_terminal->name, access)
           : selected.get().resolve_call(
                 selected.get(), selected_terminal->name);
  if (selected_terminal->kind == RouteKind::SelfCallable) {
    BAIL_IF(callable.resolve().is<Ttx::Concept::Invalid>());
    return callable.resolve();
  }

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

auto Puffer::Package::run() const -> S32 {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::plain_sink);

  Core::View::Bytes manifest = value(arguments, "manifest"_view);
  Core::View::Bytes identity = value(arguments, "name"_view);
  Core::View::Bytes complete_path = value(arguments, "complete"_view);
  Core::View::Bytes interface_path = value(arguments, "interface"_view);
  Core::View::Bytes header_path = value(arguments, "header"_view);
  Core::View::Bytes abi_manifest_path = value(arguments, "abi-manifest"_view);
  System::Version version =
      System::Version::parse(value(arguments, "version"_view));
  auto debug = parse_debug(value(arguments, "debug"_view));
  Core::View::Bytes artifact = value(arguments, "artifact"_view);
  if (manifest.is_empty() || identity.is_empty() || complete_path.is_empty() ||
      interface_path.is_empty() || header_path.is_empty() ||
      abi_manifest_path.is_empty() || version.is_null() || !debug ||
      artifact != "x86_64-sysv-linux"_view) {
    Core::Diagnostics::Log::error(
        "Puffer Package mode received an incomplete request."_view);
    return 2;
  }

  Memory::Allocator::Arena arena;
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
  if (!toolchain.install<Tetrodotoxin::Package::Dialect>("Package"_view) ||
      !toolchain.install<Library::Dialect>("Library"_view) ||
      !toolchain.install<App::Dialect>("App"_view)) {
    return 1;
  }
  Environment::Workspace workspace(toolchain);

  Memory::Dynamic::Vector<Memory::Dynamic::Bytes> dependency_bytes;
  Memory::Managed::Vector<Tetrodotoxin::Package::Archive::Archive>
      dependency_archives(arena);
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
        archive->get_profile() != Language::Persistence::Profile::Interface) {
      Core::Diagnostics::Log::error(
          "Puffer Package rejected one dependency Interface Archive."_view);
      return 1;
    }
    if (!workspace.restore_package(*archive, archive->get_identity())) {
      Core::Diagnostics::Log::Message<256> message(
          Core::Diagnostics::Log::Level::Error, Core::Diagnostics::Source());
      message << "Puffer Package could not restore dependency `"_view
              << archive->get_identity() << "`."_view;
      return 1;
    }
    dependency_archives.insert(*archive);
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
  for (const Tetrodotoxin::Package::Archive::Archive& archive :
       dependency_archives.get_view()) {
    Count matches = 0;
    for (const Linker::Manifest& manifest : dependency_manifests.get_view()) {
      matches +=
          manifest.get_artifact() == artifact && archive.matches(manifest) ? 1
                                                                           : 0;
    }
    if (matches != 1) {
      Core::Diagnostics::Log::error(
          "Puffer Package could not match one dependency ABI Manifest."_view);
      return 1;
    }
  }

  Core::View::Bytes package_root;
  Core::View::Bytes manifest_route;
  if (!package_paths(manifest, package_root, manifest_route)) {
    return 2;
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

  Memory::Managed::Vector<Library::Llvm::Unit::TypeBinding> type_bindings(
      arena);
  Memory::Managed::Vector<Core::View::Bytes> dependency_headers(arena);
  for (const Tetrodotoxin::Package::Archive::Archive& dependency :
       dependency_archives.get_view()) {
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
    for (const Tetrodotoxin::Package::Archive::Member& archived_member :
         dependency.get_members()) {
      auto selected = resolve_context_route(
          *dependency_root, archived_member.get_semantic_name());
      auto member =
          selected ? selected->select<Tetrodotoxin::Language::Monograph>()
                   : Core::Option<const Tetrodotoxin::Language::Monograph&>();
      auto library = member
                         ? select_library(*member, *library_dialect)
                         : Core::Option<const Library::Language::Monograph&>();
      if (library && !retain_library_types(
                         arena, type_bindings, dependency.get_identity(),
                         archived_member.get_semantic_name(), *library)) {
        Core::Diagnostics::Log::error(
            "Puffer Package found colliding dependency ABI Type routes."_view);
        return 1;
      }
    }
  }

  for (const Tetrodotoxin::Package::Language::Source& source :
       root->get_sources()) {
    auto selected = resolve_context_route(*root, source.get_local_name());
    auto member =
        selected ? selected->select<Tetrodotoxin::Language::Monograph>()
                 : Core::Option<const Tetrodotoxin::Language::Monograph&>();
    auto library = member ? select_library(*member, *library_dialect)
                          : Core::Option<const Library::Language::Monograph&>();
    if (library && !retain_library_types(
                       arena, type_bindings, identity, source.get_local_name(),
                       *library)) {
      Core::Diagnostics::Log::error(
          "Puffer Package found colliding local ABI Type routes."_view);
      return 1;
    }
  }

  Memory::Managed::Vector<Library::Llvm::Unit::Binding> external(arena);
  for (const Tetrodotoxin::Package::Archive::Archive& dependency :
       dependency_archives.get_view()) {
    auto dependency_root =
        workspace.resolve_context(dependency.get_identity())
            .resolve()
            .select<Tetrodotoxin::Package::Language::Monograph>();
    if (!dependency_root) {
      return 1;
    }
    for (const Tetrodotoxin::Package::Archive::Export& exported :
         dependency.get_exports()) {
      auto semantic =
          resolve_route(*dependency_root, exported.get_semantic_route());
      if (!semantic || exported.get_artifact_id() != artifact) {
        Core::Diagnostics::Log::Message<256> message(
            Core::Diagnostics::Log::Level::Error, Core::Diagnostics::Source());
        message << "Puffer Package could not bind export `"_view
                << exported.get_semantic_route() << "`."_view;
        return 1;
      }
      external.insert(
          Library::Llvm::Unit::Binding(
              *semantic, exported.get_symbol_locator()));
    }
  }

  Memory::Managed::Vector<Tetrodotoxin::Package::Archive::Export> exports(
      arena);
  Memory::Managed::Vector<Linker::Import> selected_imports(arena);
  Memory::Managed::Bytes abi_description(arena, artifact);
  Memory::Dynamic::Bytes combined_header;
  Memory::Dynamic::Vector<UnitSpecification> units;
  for (Core::View::Bytes specification : values(arguments, "unit"_view)) {
    if (split_count(specification, '|') != 3) {
      return 2;
    }
    units.insert(
        UnitSpecification{
          .source = split(specification, '|', 0),
          .ir = split(specification, '|', 1),
          .object = split(specification, '|', 2),
        });
  }
  if (units.get_size() != root->get_sources().get_size()) {
    return 2;
  }

  for (const Tetrodotoxin::Package::Language::Source& declared :
       root->get_sources()) {
    Memory::Managed::Bytes declared_source(arena, package_root);
    declared_source.append('/');
    declared_source.concat(declared.get_source_path());
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

    Core::View::Bytes member_name = declared.get_local_name();
    Core::View::Bytes source_path = unit_specification->source;
    Core::View::Bytes ir_path = unit_specification->ir;
    Core::View::Bytes object_path = unit_specification->object;
    auto selected = resolve_context_route(*root, member_name);
    auto member =
        selected ? selected->select<Tetrodotoxin::Language::Monograph>()
                 : Core::Option<const Tetrodotoxin::Language::Monograph&>();
    if (!member) {
      return 1;
    }

    auto source = System::File::read(source_path);
    if (!source) {
      return 1;
    }
    Library::Llvm::Unit unit(
        identity, member_name, artifact, external.get_view(),
        type_bindings.get_view(), dependency_headers.get_view());
    Library::Llvm::Products products = [&]() {
      auto library = member->select<Library::Language::Monograph>();
      if (library) {
        Library::Llvm::Request request(
            *library, errors, source_path, *source,
            Library::Llvm::Target::X86_64SysV, *debug, unit);
        Library::Llvm::Compiler compiler;
        return compiler.compile(arena, request)
            .visit(
                [](const Library::Llvm::Products& compiled) {
                  return compiled;
                },
                [](const Library::Llvm::Failure&) {
                  return Library::Llvm::Products({}, {}, {});
                });
      }

      Library::Llvm::Program empty(
          arena, errors, source_path, *source,
          Library::Llvm::Target::X86_64SysV, *debug, unit.bind(*member));
      if (!empty.initialize()) {
        return Library::Llvm::Products({}, {}, {});
      }
      return empty.compile().visit(
          [](const Library::Llvm::Products& compiled) { return compiled; },
          [](const Library::Llvm::Failure&) {
            return Library::Llvm::Products({}, {}, {});
          });
    }();
    if (products.get_object().is_empty() || products.get_llvm_ir().is_empty() ||
        !publish(ir_path, products.get_llvm_ir()) ||
        !publish(object_path, products.get_object())) {
      report_errors(errors);
      Core::Diagnostics::Log::error(
          "Puffer Package could not emit one member product."_view);
      return 1;
    }
    combined_header.concat(products.get_header());
    abi_description.concat(products.get_abi_fingerprint().render(arena));

    for (const Linker::Import& import : products.get_imports()) {
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

    for (const Library::Llvm::Publication& publication :
         products.get_publications()) {
      auto route = create_route(arena, member_name, publication.get_semantic());
      if (!route) {
        Core::Diagnostics::Log::error(
            "Puffer Package could not create one export route."_view);
        return 1;
      }
      Bool duplicate = False;
      for (const Tetrodotoxin::Package::Archive::Export& existing :
           exports.get_view()) {
        Bool same_route = existing.get_semantic_route() == *route;
        Bool same_symbol =
            existing.get_symbol_locator() == publication.get_symbol();
        if (same_route && same_symbol) {
          duplicate = True;
          break;
        }
        if (same_route || same_symbol) {
          Core::Diagnostics::Log::Message<512> message(
              Core::Diagnostics::Log::Level::Error,
              Core::Diagnostics::Source());
          message << "Puffer Package export `"_view << *route
                  << "` with symbol `"_view << publication.get_symbol()
                  << "` from member `"_view << member_name
                  << "` collides with `"_view << existing.get_semantic_route()
                  << "` and `"_view << existing.get_symbol_locator()
                  << "`."_view;
          return 1;
        }
      }
      if (duplicate) {
        continue;
      }
      exports.insert(
          Tetrodotoxin::Package::Archive::Export(
              *route, artifact, publication.get_symbol()));
    }
  }

  Linker::Fingerprint abi_fingerprint =
      Linker::Fingerprint::create(abi_description.get_view());
  auto identified_header = Library::Llvm::Header::identify(
      arena, combined_header.get_view(), identity, abi_fingerprint);
  if (!identified_header) {
    return 1;
  }
  Linker::Manifest native_manifest(
      identity, version, artifact, artifact, abi_fingerprint,
      selected_imports.get_view());
  auto manifest_bytes = Linker::Manifest::write(native_manifest);
  if (!manifest_bytes) {
    return 1;
  }

  Tetrodotoxin::Package::Archive::Artifact artifacts[] = {
    Tetrodotoxin::Package::Archive::Artifact(
        artifact, artifact, abi_fingerprint, selected_imports.get_view()),
  };
  auto complete = Tetrodotoxin::Package::Archive::Writer::write(
      *root, identity, version, Language::Persistence::Profile::Complete,
      artifacts, exports.get_view());
  auto interface = Tetrodotoxin::Package::Archive::Writer::write(
      *root, identity, version, Language::Persistence::Profile::Interface,
      artifacts, exports.get_view());
  if (!complete || !interface || !publish(complete_path, *complete) ||
      !publish(interface_path, *interface) ||
      !publish(header_path, identified_header->get_view()) ||
      !publish(abi_manifest_path, *manifest_bytes)) {
    Core::Diagnostics::Log::error(
        "Puffer Package could not publish its completed products."_view);
    return 1;
  }
  return 0;
}
