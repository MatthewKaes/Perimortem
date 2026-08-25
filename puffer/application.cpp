// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/application.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/linker/manifest.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"
#include "tetrodotoxin/shader/dialect.hpp"
#include "tetrodotoxin/shader/language/program.hpp"
#include "tetrodotoxin/terminal/application/generator.hpp"
#include "tetrodotoxin/terminal/vulkan/compiler.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal;

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

static auto decode(Memory::Allocator::Arena& arena, Core::View::Bytes bytes)
    -> Core::Option<Package::Archive::Archive> {
  auto decoded = Package::Archive::Reader::read(arena, bytes);
  Core::Option<Package::Archive::Archive> archive;
  decoded.visit(
      [&](const Package::Archive::Archive& selected) { archive = selected; },
      [](const Package::Archive::Reader::Error&) {});
  return archive;
}

static auto publish(Core::View::Bytes path, Core::View::Bytes contents)
    -> Bool {
  return System::File::write(contents, path);
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

static auto find_program_symbol(
    const Environment::Workspace& workspace,
    const Package::Archive::Archive& archive,
    const Shader::Language::Program& program,
    Core::View::Bytes artifact) -> Core::Option<Core::View::Bytes> {
  auto package = workspace.resolve_context(archive.get_identity())
                     .resolve()
                     .select<Package::Language::Monograph>();
  BAIL_IF(!package);
  Core::Option<Core::View::Bytes> symbol;
  for (const Package::Archive::Export& exported : archive.get_exports()) {
    if (exported.get_artifact_id() != artifact) {
      continue;
    }
    auto selected =
        resolve_context_route(*package, exported.get_semantic_route());
    if (selected && &selected->resolve() == &program) {
      BAIL_IF(symbol);
      symbol = exported.get_symbol_locator();
    }
  }
  return symbol;
}

auto Puffer::Application::run() const -> S32 {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::plain_sink);

  Core::View::Bytes complete_path = value(arguments, "complete"_view);
  Core::View::Bytes app_member = value(arguments, "app-member"_view);
  Core::View::Bytes artifact = value(arguments, "artifact"_view);
  Core::View::Bytes source_path = value(arguments, "source"_view);
  Core::View::Bytes abi_manifest_path = value(arguments, "abi-manifest"_view);
  if (complete_path.is_empty() || app_member.is_empty() ||
      source_path.is_empty() || abi_manifest_path.is_empty() ||
      artifact != "x86_64-sysv-linux"_view) {
    Core::Diagnostics::Log::error(
        "Puffer Application mode received an incomplete request."_view);
    return 2;
  }

  Memory::Allocator::Arena arena;
  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto render = toolchain.install<Render::Dialect>("Render"_view);
  if (!toolchain.install<Package::Dialect>("Package"_view) || !library ||
      !render || !toolchain.install<App::Dialect>("App"_view) ||
      !toolchain.install<Scene::Dialect>("Scene"_view, *library) ||
      !toolchain.install<Shader::Dialect>("Shader"_view, *library, *render)) {
    return 1;
  }
  Environment::Workspace workspace(toolchain);

  Memory::Dynamic::Vector<Memory::Dynamic::Bytes> dependency_bytes;
  Memory::Managed::Vector<Package::Archive::Archive> dependency_archives(arena);
  for (Core::View::Bytes dependency_path : values(arguments, "dep"_view)) {
    auto bytes = System::File::read(dependency_path);
    if (!bytes) {
      return 1;
    }

    dependency_bytes.emplace(static_cast<Memory::Dynamic::Bytes&&>(*bytes));
    auto archive =
        decode(arena, dependency_bytes[dependency_bytes.get_size() - 1]);
    if (!archive ||
        archive->get_profile() != Language::Persistence::Profile::Contract ||
        !workspace.restore_package(*archive, archive->get_identity())) {
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
  for (const Package::Archive::Archive& archive :
       dependency_archives.get_view()) {
    Count matches = 0;
    for (const Linker::Manifest& manifest : dependency_manifests.get_view()) {
      matches +=
          manifest.get_artifact() == artifact && archive.matches(manifest) ? 1
                                                                           : 0;
    }
    if (matches != 1) {
      Core::Diagnostics::Log::error(
          "Puffer Application could not match one dependency ABI Manifest."_view);
      return 1;
    }
  }

  auto complete_bytes = System::File::read(complete_path);
  if (!complete_bytes) {
    return 1;
  }

  auto root_archive = decode(arena, *complete_bytes);
  if (!root_archive ||
      root_archive->get_profile() != Language::Persistence::Profile::Complete ||
      !workspace.restore_package(*root_archive, root_archive->get_identity())) {
    return 1;
  }

  auto root_manifest_bytes = System::File::read(abi_manifest_path);
  if (!root_manifest_bytes) {
    return 1;
  }
  auto root_manifest = Linker::Manifest::read(arena, *root_manifest_bytes);
  Bool root_abi_matches = root_manifest.visit(
      [&](const Linker::Manifest& manifest) -> Bool {
        return manifest.get_artifact() == artifact &&
               root_archive->matches(manifest);
      },
      [](const Linker::Manifest::Error&) -> Bool { return False; });
  if (!root_abi_matches) {
    Core::Diagnostics::Log::error(
        "Puffer Application rejected a stale native ABI Manifest."_view);
    return 1;
  }

  const Ttx::Concept::Abstract& selected_root =
      workspace.resolve_context(root_archive->get_identity()).resolve();
  auto root = selected_root.select<Package::Language::Monograph>();
  Core::Option<const App::Language::Monograph&> app;
  if (root) {
    app = root->resolve_context(app_member)
              .resolve()
              .select<App::Language::Monograph>();
  }
  if (!app) {
    Core::Diagnostics::Log::error(
        "The selected Package member does not contain an App policy."_view);
    return 1;
  }

  Core::View::Bytes graphics_host_route =
      value(arguments, "graphics-host"_view);
  Core::View::Bytes graphics_shader_route =
      value(arguments, "graphics-shader"_view);
  Core::Option<const Ttx::Model::Type&> graphics_host;
  Memory::Managed::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>>
      graphics_types(arena);
  Core::View::Vector<Core::View::Bytes> graphics_descriptors =
      values(arguments, "graphics-descriptor"_view);
  if (!graphics_host_route.is_empty()) {
    auto selected = resolve_context_route(*root, graphics_host_route);
    graphics_host = selected ? selected->select<Ttx::Model::Type>()
                             : Core::Option<const Ttx::Model::Type&>();
    for (Core::View::Bytes route : values(arguments, "graphics-type"_view)) {
      auto type = resolve_context_route(*root, route);
      auto selected_type = type ? type->select<Ttx::Model::Type>()
                                : Core::Option<const Ttx::Model::Type&>();
      if (!selected_type) {
        Core::Diagnostics::Log::error(
            "Puffer Application could not resolve one configured graphics Type."_view);
        return 1;
      }
      graphics_types.insert(*selected_type);
    }
  }

  auto selected_shader =
      graphics_shader_route.is_empty()
          ? Core::Option<const Ttx::Concept::Abstract&>()
          : resolve_context_route(*root, graphics_shader_route);
  auto graphics_shader =
      selected_shader ? selected_shader->select<Shader::Language::Program>()
                      : Core::Option<const Shader::Language::Program&>();
  if (!graphics_shader_route.is_empty() && !graphics_shader) {
    Core::Diagnostics::Log::error(
        "Puffer Application could not resolve its configured Shader Program."_view);
  }
  Core::Option<Core::View::Bytes> graphics_shader_symbol;
  if (graphics_shader) {
    graphics_shader_symbol = find_program_symbol(
        workspace, *root_archive, *graphics_shader, artifact);
    for (const Package::Archive::Archive& dependency :
         dependency_archives.get_view()) {
      auto selected = find_program_symbol(
          workspace, dependency, *graphics_shader, artifact);
      if (selected) {
        if (graphics_shader_symbol) {
          return 1;
        }
        graphics_shader_symbol = *selected;
      }
    }
  }

  Core::Option<Memory::Dynamic::Bytes> generated;
  auto program = app->get_program();
  if (program) {
    Memory::Managed::Bytes route(arena, program->get_route());
    route.concat("::"_view);
    route.concat(program->get_callable_name());
    route.concat("[static]"_view);
    Core::Option<Core::View::Bytes> entry_symbol;
    for (const Package::Archive::Export& exported :
         root_archive->get_exports()) {
      if (exported.get_semantic_route() != route.get_view() ||
          exported.get_artifact_id() != artifact) {
        continue;
      }
      if (entry_symbol) {
        return 1;
      }
      entry_symbol = exported.get_symbol_locator();
    }
    if (!entry_symbol) {
      return 1;
    }
    Memory::Dynamic::Bytes source;
    Serialization::Stream::Textual<Memory::Dynamic::Bytes> output(source);
    output
        << "// # Tetrodotoxin\n"_view
        << "// Copyright (c) 2023-present Matt Kaes and contributors\n\n"_view
        << "extern \"C\" void "_view << *entry_symbol << "(void);\n\n"_view
        << "int main() {\n  "_view << *entry_symbol
        << "();\n  return 0;\n}\n"_view;
    generated = static_cast<Memory::Dynamic::Bytes&&>(source);
  } else {
    if (!graphics_host || graphics_types.is_empty() || !graphics_shader ||
        !graphics_shader_symbol ||
        graphics_descriptors.get_size() != graphics_types.get_size()) {
      Core::Diagnostics::Log::error(
          "Puffer Application has an incomplete graphics product selection."_view);
      return 1;
    }
    Terminal::Vulkan::Compiler vulkan_compiler;
    auto vulkan = vulkan_compiler.compile(
        arena, *graphics_shader, *graphics_shader_symbol);
    if (!vulkan) {
      Core::Diagnostics::Log::error(
          "Puffer Application could not derive its Vulkan product."_view);
      return 1;
    }
    generated = Terminal::Application::Generator::create(
        arena, *app, root_archive->get_identity(), artifact, *graphics_host,
        graphics_types.get_view(), graphics_descriptors, *vulkan);
  }
  if (!generated || !publish(source_path, *generated)) {
    Core::Diagnostics::Log::error(
        "Puffer Application could not publish its native entry."_view);
    return 1;
  }
  return 0;
}
