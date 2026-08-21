// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/package.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/version.hpp"

#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/construction.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/llvm/compiler.hpp"
#include "tetrodotoxin/library/llvm/program.hpp"
#include "tetrodotoxin/library/llvm/symbol.hpp"
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

static auto split(Core::View::Bytes value, Unsigned_8 separator, Count index)
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

static auto split_count(Core::View::Bytes value, Unsigned_8 separator)
    -> Count {
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
  auto construction = semantic.select<Library::Language::Construction>();
  if (construction) {
    BAIL_IF(!append_host_path(output, construction->get_owner(), start));
    if (output.get_size() != start) {
      output.concat("::"_view);
    }
    output.concat(construction->get_name());
    output.concat("[static]"_view);
    return True;
  }

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

enum class RouteKind : Unsigned_8 {
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

auto Puffer::Package::run() const -> Signed_32 {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
  Core::Diagnostics::Log::set_disable_header(True);

  Core::View::Bytes manifest = value(arguments, "manifest"_view);
  Core::View::Bytes identity = value(arguments, "name"_view);
  Core::View::Bytes complete_path = value(arguments, "complete"_view);
  Core::View::Bytes interface_path = value(arguments, "interface"_view);
  Core::View::Bytes header_path = value(arguments, "header"_view);
  System::Version version =
      System::Version::parse(value(arguments, "version"_view));
  auto debug = parse_debug(value(arguments, "debug"_view));
  Core::View::Bytes artifact = value(arguments, "artifact"_view);
  if (manifest.is_empty() || identity.is_empty() || complete_path.is_empty() ||
      interface_path.is_empty() || header_path.is_empty() ||
      version.is_null() || !debug || artifact != "x86_64-sysv-linux"_view) {
    Core::Diagnostics::Log::error(
        "Puffer Package mode received an incomplete request."_view);
    return 2;
  }

  Memory::Allocator::Arena arena;
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
    const Ttx::Concept::Abstract& selected =
        root->resolve_context(member_name).resolve();
    auto member = selected.select<Tetrodotoxin::Language::Monograph>();
    if (!member) {
      return 1;
    }

    auto source = System::File::read(source_path);
    if (!source) {
      return 1;
    }
    Library::Llvm::Unit unit(
        identity, member_name, artifact, external.get_view());
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

  Core::View::Bytes artifacts[] = {artifact};
  auto complete = Tetrodotoxin::Package::Archive::Writer::write(
      *root, identity, version, Language::Persistence::Profile::Complete,
      artifacts, exports.get_view());
  auto interface = Tetrodotoxin::Package::Archive::Writer::write(
      *root, identity, version, Language::Persistence::Profile::Interface,
      artifacts, exports.get_view());
  if (!complete || !interface || !publish(complete_path, *complete) ||
      !publish(interface_path, *interface) ||
      !publish(header_path, combined_header)) {
    Core::Diagnostics::Log::error(
        "Puffer Package could not publish its completed products."_view);
    return 1;
  }
  return 0;
}
