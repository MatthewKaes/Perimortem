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

#include "puffer/dependencies.hpp"
#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/linker/manifest.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
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

static auto resolve_source_route(
    const Environment::Workspace& workspace,
    const Package::Language::Monograph& package,
    Core::View::Bytes route) -> Core::Option<const Ttx::Concept::Abstract&> {
  Core::Option<const Ttx::Concept::Abstract&> selected =
      resolve_context_route(package, route);
  Count count = workspace.get_package_source_count(package);
  for (Count index = 0; index < count; index++) {
    auto source = workspace.get_package_source(package, index);
    auto candidate = source
                         ? resolve_context_route(source->get_monograph(), route)
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

    auto selected = resolve_source_route(
        workspace, *package, exported.get_semantic_route());
    if (selected && &selected->resolve() == &program) {
      BAIL_IF(symbol);
      symbol = exported.get_symbol_locator();
    }
  }

  return symbol;
}

static auto retain_scene(
    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Scene::Language::Monograph>>& scenes,
    const Scene::Language::Monograph& scene) -> void {
  for (const Ttx::Concept::Reference<const Scene::Language::Monograph>&
           retained : scenes.get_view()) {
    if (&retained.get() == &scene) {
      return;
    }
  }

  scenes.insert(scene);
}

static auto retain_program(
    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Shader::Language::Program>>& programs,
    const Shader::Language::Program& program) -> void {
  for (const Ttx::Concept::Reference<const Shader::Language::Program>&
           retained : programs.get_view()) {
    if (&retained.get() == &program) {
      return;
    }
  }

  programs.insert(program);
}

static auto collect_programs(
    const Library::Language::Model::Type& type,
    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Shader::Language::Program>>& programs,
    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Library::Language::Model::Type>>& visited)
    -> void {
  for (const Ttx::Concept::Reference<const Library::Language::Model::Type>&
           retained : visited.get_view()) {
    if (&retained.get() == &type) {
      return;
    }
  }

  visited.insert(type);

  auto object = type.select<Library::Language::Types::Object>();
  auto program = object ? object->get_definition()
                              .get_host()
                              .select<Shader::Language::Program>()
                        : Core::Option<const Shader::Language::Program&>();
  if (program && &program->get_instance() == &type) {
    retain_program(programs, *program);
  }

  auto composite = type.select<Library::Language::Types::Composite>();
  if (!composite) {
    return;
  }

  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& declaration :
       composite->get_addressables()) {
    auto field = declaration.get().select<Library::Language::Field>();
    if (field) {
      collect_programs(field->get_type(), programs, visited);
    }
  }
}

static auto find_program_symbol(
    const Environment::Workspace& workspace,
    const Package::Archive::Archive& root,
    Core::View::Vector<Package::Archive::Archive> dependencies,
    const Shader::Language::Program& program,
    Core::View::Bytes artifact) -> Core::Option<Core::View::Bytes> {
  auto symbol = find_program_symbol(workspace, root, program, artifact);
  for (const Package::Archive::Archive& dependency : dependencies) {
    auto selected =
        find_program_symbol(workspace, dependency, program, artifact);
    BAIL_IF(selected && symbol);
    if (selected) {
      symbol = *selected;
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
  if (!library || !render ||
      !toolchain.install<Package::Dialect>("Package"_view, *library) ||
      !toolchain.install<App::Dialect>("App"_view) ||
      !toolchain.install<Scene::Dialect>("Scene"_view, *library) ||
      !toolchain.install<Shader::Dialect>("Shader"_view, *library, *render)) {
    return 1;
  }

  Memory::Dynamic::Vector<Memory::Dynamic::Bytes> dependency_bytes;
  Dependencies selected_dependencies(arena, repository);
  for (Core::View::Bytes dependency_path : values(arguments, "dep"_view)) {
    auto bytes = System::File::read(dependency_path);
    if (!bytes) {
      return 1;
    }

    dependency_bytes.emplace(static_cast<Memory::Dynamic::Bytes&&>(*bytes));
    auto archive =
        decode(arena, dependency_bytes[dependency_bytes.get_size() - 1]);
    if (!archive || !selected_dependencies.retain(*archive)) {
      return 1;
    }
  }

  auto complete_bytes = System::File::read(complete_path);
  if (!complete_bytes) {
    return 1;
  }

  auto root_archive = decode(arena, *complete_bytes);
  if (!root_archive ||
      root_archive->get_profile() != Language::Persistence::Profile::Complete) {
    return 1;
  }

  for (const Package::Archive::GraphImport& import :
       root_archive->get_imports()) {
    if (import.get_kind() == Language::Import::Kind::Package &&
        !selected_dependencies.acquire(
            import.get_target(), import.get_version())) {
      return 1;
    }
  }

  for (const Package::Language::Dependency& dependency :
       root_archive->get_dependencies()) {
    if (!selected_dependencies.acquire(
            dependency.get_package_name(), dependency.get_version())) {
      return 1;
    }
  }

  auto dependency_archives = selected_dependencies.get_archives();
  Environment::Workspace workspace(toolchain);
  for (const Package::Archive::Archive& archive : dependency_archives) {
    if (!workspace.restore_package(archive, archive.get_identity())) {
      return 1;
    }
  }

  if (!workspace.restore_package(*root_archive, root_archive->get_identity())) {
    return 1;
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

  for (const Package::Archive::Archive& archive : dependency_archives) {
    Count matches = 0;
    for (const Linker::Manifest& manifest : dependency_manifests.get_view()) {
      matches +=
          manifest.get_artifact() == artifact && archive.matches(manifest) ? 1
                                                                           : 0;
    }

    if (matches == 0) {
      repository
          .select_manifest(
              archive.get_identity(), archive.get_version(), artifact)
          .visit(
              [&](const Linker::Manifest& manifest) {
                dependency_manifests.insert(manifest);
                matches = 1;
              },
              [](Package::Repository::Repository::Error) {});
    }

    if (matches != 1) {
      Core::Diagnostics::Log::error(
          "Puffer Application could not match one dependency ABI Manifest."_view);
      return 1;
    }
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

  Core::View::Bytes graphics_placement_route =
      value(arguments, "graphics-placement"_view);
  Core::Option<const Ttx::Model::Type&> graphics_placement;
  Memory::Managed::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>>
      graphics_types(arena);
  Core::View::Vector<Core::View::Bytes> graphics_placements =
      values(arguments, "graphics-placement-provider"_view);
  Core::View::Vector<Core::View::Bytes> graphics_children =
      values(arguments, "graphics-children-provider"_view);
  Core::View::Vector<Core::View::Bytes> graphics_drawables =
      values(arguments, "graphics-drawable-provider"_view);
  if (!graphics_placement_route.is_empty()) {
    auto selected =
        resolve_source_route(workspace, *root, graphics_placement_route);
    graphics_placement = selected ? selected->select<Ttx::Model::Type>()
                                  : Core::Option<const Ttx::Model::Type&>();
    for (Core::View::Bytes route : values(arguments, "graphics-type"_view)) {
      auto type = resolve_source_route(workspace, *root, route);
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

  Core::Option<Memory::Dynamic::Bytes> generated;
  auto program = app->get_program();
  if (program) {
    Memory::Managed::Bytes route(arena, app_member);
    route.concat("::"_view);
    route.concat(program->get_route());
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
    if (!graphics_placement || graphics_types.is_empty() ||
        graphics_placements.get_size() != graphics_types.get_size() ||
        graphics_children.get_size() != graphics_types.get_size() ||
        graphics_drawables.get_size() != graphics_types.get_size()) {
      Core::Diagnostics::Log::error(
          "Puffer Application has an incomplete graphics product selection."_view);
      return 1;
    }

    auto scene_policy = app->get_scene();
    auto initial_scene =
        scene_policy ? scene_policy->get_initial_scene()
                     : Core::Option<const Scene::Language::Monograph&>();
    if (!scene_policy || !initial_scene) {
      return 1;
    }

    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Scene::Language::Monograph>>
        scenes(arena);
    retain_scene(scenes, *initial_scene);
    for (const Ttx::Concept::Reference<App::Language::Transition>& retained :
         scene_policy->get_transitions()) {
      auto source = retained.get().get_source_scene();
      auto destination = retained.get().get_destination_scene();
      if (!source) {
        return 1;
      }

      retain_scene(scenes, *source);
      if (destination) {
        retain_scene(scenes, *destination);
      }
    }

    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Shader::Language::Program>>
        programs(arena);
    Memory::Managed::Vector<
        Ttx::Concept::Reference<const Library::Language::Model::Type>>
        visited(arena);
    Memory::Managed::Vector<Terminal::Application::Generator::MemberBinding>
        scene_members(arena);
    for (const Ttx::Concept::Reference<const Scene::Language::Monograph>&
             scene : scenes.get_view()) {
      collect_programs(scene.get().get_instance(), programs, visited);
    }

    Count source_count = workspace.get_package_source_count(*root);
    for (Count index = 0; index < source_count; index++) {
      auto source = workspace.get_package_source(*root, index);
      auto scene =
          source ? source->get_monograph().select<Scene::Language::Monograph>()
                 : Core::Option<const Scene::Language::Monograph&>();
      if (source && scene) {
        scene_members.insert(
            Terminal::Application::Generator::MemberBinding(
                *scene, source->get_name()));
      }
    }

    if (programs.is_empty()) {
      return 1;
    }

    Memory::Managed::Vector<Terminal::Vulkan::Products> vulkan(arena);
    Terminal::Vulkan::Compiler vulkan_compiler;
    for (const Ttx::Concept::Reference<const Shader::Language::Program>&
             selected : programs.get_view()) {
      auto symbol = find_program_symbol(
          workspace, *root_archive, dependency_archives, selected.get(),
          artifact);
      auto product =
          symbol ? vulkan_compiler.describe(arena, selected.get(), *symbol)
                 : Core::Option<Terminal::Vulkan::Products>();
      if (!product) {
        Core::Diagnostics::Log::error(
            "Puffer Application could not derive one reachable Vulkan Program."_view);
        return 1;
      }

      vulkan.insert(*product);
    }

    generated = Terminal::Application::Generator::create(
        arena, *app, root_archive->get_identity(), artifact,
        scene_members.get_view(), *graphics_placement,
        graphics_types.get_view(), graphics_placements, graphics_children,
        graphics_drawables, vulkan.get_view());
  }

  if (!generated || !publish(source_path, *generated)) {
    Core::Diagnostics::Log::error(
        "Puffer Application could not publish its native entry."_view);
    return 1;
  }

  return 0;
}
