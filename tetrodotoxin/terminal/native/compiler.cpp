// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/native/compiler.hpp"

#include <cstddef>
#include <cstdlib>
#include <ftw.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/linker/elf/object.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/terminal/abi/compiler.hpp"
#include "tetrodotoxin/terminal/abi/projection.hpp"
#include "tetrodotoxin/terminal/abi/resource_product.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/application/generator.hpp"
#include "tetrodotoxin/terminal/graphics/compiler.hpp"
#include "tetrodotoxin/terminal/llvm/compiler.hpp"
#include "tetrodotoxin/terminal/spirv/request.hpp"
#include "tetrodotoxin/terminal/vulkan/compiler.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

struct NativePackage {
  const Package::Language::Monograph* package;
};

struct NativeMember {
  const Package::Language::Monograph* package;
  Core::View::Bytes name;
  Core::View::Bytes logical_route;
  const Tetrodotoxin::Language::Monograph* monograph;
  const Library::Language::Monograph* library;
  const Shader::Language::Monograph* shader;
  std::vector<const Library::Language::Model::Callable*> excluded;
  std::vector<Terminal::Abi::Projection> projections;
  Memory::Dynamic::Bytes embedded_object;
};

struct NativeObject {
  Memory::Dynamic::Bytes bytes;
};

class TemporaryDirectory {
 public:
  TemporaryDirectory() {
    char pattern[] = "/tmp/tetrodotoxin-native-XXXXXX";
    char* selected = mkdtemp(pattern);
    if (selected != nullptr) {
      path = selected;
    }
  }

  ~TemporaryDirectory() {
    if (!path.empty()) {
      nftw(path.c_str(), remove_entry, 32, FTW_DEPTH | FTW_PHYS);
    }
  }

  auto valid() const -> bool { return !path.empty(); }
  auto get_path() const -> const std::string& { return path; }

 private:
  static auto
      remove_entry(const char* path, const struct stat*, S32, struct FTW*)
          -> S32 {
    return remove(path);
  }

  std::string path;
};

template <typename Value>
static auto vector_view(const std::vector<Value>& values)
    -> Core::View::Vector<Value> {
  return values.empty()
             ? Core::View::Vector<Value>()
             : Core::View::Vector<Value>(values.data(), values.size());
}

static auto as_string(Core::View::Bytes value) -> std::string {
  return std::string(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto append_path(const std::string& root, const std::string& name)
    -> std::string {
  return root + "/" + name;
}

static auto write_file(const std::string& path, Core::View::Bytes contents)
    -> bool {
  return bool(System::File::write(
      contents, Core::View::Bytes(
                    reinterpret_cast<const U8*>(path.data()), path.size())));
}

static auto run_process(const std::vector<std::string>& arguments) -> bool {
  if (arguments.empty()) {
    return false;
  }
  std::vector<char*> values;
  values.reserve(arguments.size() + 1);
  for (const std::string& argument : arguments) {
    values.push_back(const_cast<char*>(argument.c_str()));
  }
  values.push_back(nullptr);

  pid_t child = fork();
  if (child == 0) {
    execv(values[0], values.data());
    _exit(127);
  }
  if (child < 0) {
    return false;
  }
  S32 status = 0;
  while (waitpid(child, &status, 0) < 0) {
    if (errno != EINTR) {
      return false;
    }
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static auto select_library(
    const Tetrodotoxin::Language::Monograph& monograph,
    const Library::Dialect& library)
    -> Core::Option<const Library::Language::Monograph&> {
  auto direct = monograph.select<Library::Language::Monograph>();
  if (direct) {
    return *direct;
  }
  auto layer = monograph.get_layer(library);
  return layer ? layer->select<Library::Language::Monograph>()
               : Core::Option<const Library::Language::Monograph&>();
}

static auto append_package(
    const Environment::Workspace& workspace,
    const Package::Language::Monograph& package,
    std::vector<NativePackage>& packages) -> bool {
  for (const NativePackage& retained : packages) {
    if (retained.package == &package) {
      return true;
    }
  }
  packages.push_back({.package = &package});

  auto inspect = [&](const Tetrodotoxin::Language::Monograph& monograph) {
    for (const Tetrodotoxin::Language::Import* import :
         monograph.get_imports()) {
      if (import->get_kind() != Tetrodotoxin::Language::Import::Kind::Package) {
        continue;
      }
      auto acquired = import->get_acquired();
      auto dependency =
          acquired ? acquired->select<Package::Language::Monograph>()
                   : Core::Option<const Package::Language::Monograph&>();
      if (!dependency || !append_package(workspace, *dependency, packages)) {
        return false;
      }
    }
    return true;
  };

  if (!inspect(package)) {
    return false;
  }
  for (Count index = 0; index < workspace.get_package_source_count(package);
       ++index) {
    auto source = workspace.get_package_source(package, index);
    if (!source || !inspect(source->get_monograph())) {
      return false;
    }
  }
  return true;
}

static auto retain_type(
    Memory::Allocator::Arena& arena,
    std::vector<Terminal::Abi::Unit::TypeBinding>& bindings,
    Core::View::Bytes package,
    Core::View::Bytes member,
    Core::View::Bytes route,
    const Ttx::Concept::Abstract& candidate) -> bool {
  auto type = candidate.resolve().select<Library::Language::Model::Type>();
  if (!type) {
    return true;
  }
  for (const Terminal::Abi::Unit::TypeBinding& retained : bindings) {
    if (&retained.get_semantic() == &*type) {
      return true;
    }
    if (retained.get_package() == package && retained.get_route() == route) {
      return false;
    }
  }
  const Core::View::Bytes retained_route = arena.proxy(route);
  bindings.emplace_back(*type, package, member, retained_route);

  auto composite = type->select<Library::Language::Types::Composite>();
  if (!composite) {
    return true;
  }
  for (const Ttx::Concept::Abstract* nested : composite->get_types()) {
    Memory::Managed::Bytes nested_route(arena, retained_route);
    nested_route.concat("::"_view);
    nested_route.concat(nested->get_name());
    if (!retain_type(
            arena, bindings, package, member, nested_route.get_view(),
            *nested)) {
      return false;
    }
  }
  return true;
}

static auto retain_library_types(
    Memory::Allocator::Arena& arena,
    std::vector<Terminal::Abi::Unit::TypeBinding>& bindings,
    Core::View::Bytes package,
    Core::View::Bytes member,
    const Library::Language::Monograph& library) -> bool {
  for (const Ttx::Concept::Abstract* type : library.get_source().get_types()) {
    Memory::Managed::Bytes route(arena, member);
    route.concat("::"_view);
    route.concat(type->get_name());
    if (!retain_type(
            arena, bindings, package, member, route.get_view(), *type)) {
      return false;
    }
  }
  return true;
}

static auto retain_binding(
    std::vector<Terminal::Abi::Unit::Binding>& bindings,
    const Ttx::Concept::Abstract& semantic,
    Core::View::Bytes symbol) -> bool {
  for (const Terminal::Abi::Unit::Binding& retained : bindings) {
    if (&retained.get_semantic() == &semantic) {
      return bool(retained.get_symbol() == symbol);
    }
    if (retained.get_symbol() == symbol) {
      return false;
    }
  }
  bindings.emplace_back(semantic, symbol);
  return true;
}

static auto source_input(
    const Environment::Workspace& workspace,
    const Tetrodotoxin::Language::Monograph& monograph)
    -> Environment::Workspace::AuthoredLocation {
  return workspace.find_source_input(monograph).visit(
      []() {
        return Environment::Workspace::AuthoredLocation(
            {}, {}, {}, Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()));
      },
      [](const Environment::Workspace::AuthoredLocation& selected) {
        return selected;
      });
}

static auto create_unit(
    const NativeMember& member,
    Core::View::Bytes artifact,
    const std::vector<Terminal::Abi::Unit::Binding>& external,
    const std::vector<Terminal::Abi::Unit::TypeBinding>& types)
    -> Terminal::Abi::Unit {
  return Terminal::Abi::Unit(
      member.package->get_name(), member.name, artifact, vector_view(external),
      vector_view(types));
}

static auto find_package(
    const std::vector<NativePackage>& packages,
    Core::View::Bytes name)
    -> Core::Option<const Package::Language::Monograph&> {
  for (const NativePackage& package : packages) {
    if (package.package->get_name() == name) {
      return *package.package;
    }
  }
  return {};
}

static auto find_domain(
    const Package::Language::Monograph& package,
    Core::View::Bytes route) -> Core::Option<const Ttx::Model::Domain&> {
  const Ttx::Concept::Abstract& selected = package.resolve_concept(route);
  auto direct = selected.resolve().select<Ttx::Model::Domain>();
  return direct ? direct : selected.select<Ttx::Model::Domain>();
}

static auto application_source(
    Memory::Allocator::Arena& arena,
    const App::Language::Monograph& app,
    Core::View::Bytes package,
    const std::vector<NativeMember>& members,
    const std::vector<Terminal::Abi::Unit::Binding>& external,
    const std::vector<NativePackage>& packages,
    Core::View::Bytes artifact,
    Core::View::Vector<Terminal::Vulkan::Products> vulkan)
    -> Core::Option<Memory::Dynamic::Bytes> {
  auto program = app.get_program();
  if (program) {
    auto entry = program->get_entry();
    if (!entry) {
      return {};
    }
    Core::Option<Core::View::Bytes> symbol;
    for (const Terminal::Abi::Unit::Binding& binding : external) {
      if (&binding.get_semantic() == &*entry) {
        if (symbol) {
          return {};
        }
        symbol = binding.get_symbol();
      }
    }
    if (!symbol) {
      return {};
    }
    Memory::Dynamic::Bytes source;
    Serialization::Stream::Textual<Memory::Dynamic::Bytes> output(source);
    output
        << "// # Tetrodotoxin\n"_view
        << "// Copyright (c) 2023-present Matt Kaes and contributors\n\n"_view
        << "extern \"C\" void "_view << *symbol
        << "(void);\n\nint main() {\n  "_view << *symbol
        << "();\n  return 0;\n}\n"_view;
    return source;
  }

  auto graphics_package = find_package(packages, "Perimortem.Graphics"_view);
  auto placement = graphics_package
                       ? find_domain(*graphics_package, "DrawableUI"_view)
                       : Core::Option<const Ttx::Model::Domain&>();
  auto sprite = graphics_package ? find_domain(*graphics_package, "Sprite"_view)
                                 : Core::Option<const Ttx::Model::Domain&>();
  if (!placement || !sprite) {
    return {};
  }

  std::vector<Terminal::Application::Generator::MemberBinding> scenes;
  for (const NativeMember& member : members) {
    auto scene = member.monograph->select<Scene::Language::Monograph>();
    if (scene) {
      scenes.emplace_back(*scene, member.name);
    }
  }
  const Ttx::Model::Domain* graphics_types[] = {&*sprite};
  const Core::View::Bytes placements[] = {
    "tetrodotoxin_graphics_sprite_placement_2d"_view,
  };
  const Core::View::Bytes children[] = {Core::View::Bytes()};
  const Core::View::Bytes drawables[] = {
    "tetrodotoxin_graphics_sprite_drawable_2d"_view,
  };
  return Terminal::Application::Generator::create(
      arena, app, package, artifact, vector_view(scenes), *placement,
      graphics_types, placements, children, drawables, vulkan);
}

static auto link_application(
    const Terminal::Native::Toolchain& toolchain,
    Core::View::Bytes source,
    const std::vector<NativeObject>& objects)
    -> Core::Option<Memory::Dynamic::Bytes> {
  TemporaryDirectory temporary;
  if (!temporary.valid()) {
    return {};
  }
  const std::string source_path =
      append_path(temporary.get_path(), "application.cpp");
  const std::string executable_path =
      append_path(temporary.get_path(), "Application");
  if (!write_file(source_path, source)) {
    return {};
  }

  std::vector<std::string> object_paths;
  object_paths.reserve(objects.size());
  for (size_t index = 0; index < objects.size(); ++index) {
    const std::string path = append_path(
        temporary.get_path(), "object-" + std::to_string(index) + ".o");
    if (!write_file(path, objects[index].bytes.get_view())) {
      return {};
    }
    object_paths.push_back(path);
  }

  std::vector<std::string> arguments;
  arguments.push_back(as_string(toolchain.get_compiler()));
  arguments.push_back("-std=c++26");
  arguments.push_back("-O0");
  arguments.push_back("-g");
  for (Core::View::Bytes root : toolchain.get_include_roots()) {
    arguments.push_back("-I" + as_string(root));
  }
  arguments.push_back(source_path);
  arguments.insert(arguments.end(), object_paths.begin(), object_paths.end());
  arguments.push_back("-Wl,--start-group");
  for (Core::View::Bytes archive : toolchain.get_archives()) {
    arguments.push_back(as_string(archive));
  }
  arguments.push_back("-Wl,--end-group");
  for (Core::View::Bytes option : toolchain.get_link_options()) {
    arguments.push_back(as_string(option));
  }
  arguments.push_back("-o");
  arguments.push_back(executable_path);
  if (!run_process(arguments)) {
    return {};
  }
  auto bytes = System::File::read(
      Core::View::Bytes(
          reinterpret_cast<const U8*>(executable_path.data()),
          executable_path.size()));
  return bytes ? Core::Option<Memory::Dynamic::Bytes>(
                     static_cast<Memory::Dynamic::Bytes&&>(*bytes))
               : Core::Option<Memory::Dynamic::Bytes>();
}

auto Terminal::Native::Compiler::compile(
    Memory::Allocator::Arena& arena,
    const Environment::Workspace& workspace,
    const Package::Language::Monograph& package,
    const Ttx::Concept::Abstract& product,
    const Library::Dialect& library,
    const Toolchain& toolchain,
    Ttx::Lexical::Errors& errors) const
    -> Utility::Result<Memory::Dynamic::Bytes, Failure> {
  auto app = product.resolve().select<App::Language::Monograph>();
  if (!app || toolchain.get_compiler().is_empty()) {
    return Failure::InvalidProduct;
  }

  std::vector<NativePackage> packages;
  if (!append_package(workspace, package, packages)) {
    return Failure::IncompleteGraph;
  }
  std::vector<NativeMember> members;
  std::vector<Terminal::Abi::Unit::TypeBinding> type_bindings;
  for (const NativePackage& retained : packages) {
    const Package::Language::Monograph& selected = *retained.package;
    for (Count index = 0; index < workspace.get_package_source_count(selected);
         ++index) {
      auto source = workspace.get_package_source(selected, index);
      if (!source) {
        return Failure::IncompleteGraph;
      }
      auto library_layer = select_library(source->get_monograph(), library);
      if (library_layer && !retain_library_types(
                               arena, type_bindings, selected.get_name(),
                               source->get_name(), *library_layer)) {
        return Failure::InterfaceFailed;
      }
      auto shader =
          source->get_monograph().select<Shader::Language::Monograph>();
      members.push_back({
        .package = &selected,
        .name = source->get_name(),
        .logical_route = source->get_logical_route(),
        .monograph = &source->get_monograph(),
        .library = library_layer ? &*library_layer : nullptr,
        .shader = shader ? &*shader : nullptr,
        .excluded = {},
        .projections = {},
        .embedded_object = {},
      });
    }
  }

  static constexpr Core::View::Bytes artifact = "x86_64-sysv-linux"_view;
  std::vector<Terminal::Abi::Unit::Binding> external;
  std::vector<NativeObject> objects;
  std::vector<Terminal::Vulkan::Products> vulkan;

  for (const NativePackage& retained : packages) {
    auto resources = Terminal::Abi::ResourceProduct::compile(
        arena, retained.package->get_resources(), retained.package->get_name(),
        artifact);
    if (!resources) {
      return Failure::InterfaceFailed;
    }
    for (const Terminal::Abi::Unit::Binding& binding :
         resources->get_bindings()) {
      if (!retain_binding(
              external, binding.get_semantic(), binding.get_symbol())) {
        return Failure::InterfaceFailed;
      }
    }
    if (!resources->get_object().is_empty()) {
      objects.push_back(
          {.bytes = Memory::Dynamic::Bytes(resources->get_object())});
    }
  }

  for (NativeMember& member : members) {
    if (member.shader == nullptr) {
      continue;
    }
    Terminal::Abi::Unit unit =
        create_unit(member, artifact, external, type_bindings);
    Linker::Elf::Object object;
    for (const Shader::Language::Program* program :
         member.shader->get_programs()) {
      for (const Ttx::Concept::Abstract* candidate : program->get_callables()) {
        auto callable = candidate->select<Library::Language::Model::Callable>();
        if (!callable) {
          return Failure::GpuFailed;
        }
        member.excluded.push_back(&*callable);
      }
      const auto input = source_input(workspace, *member.monograph);
      Terminal::Spirv::Request request(
          *member.shader, *program, errors, input.get_diagnostic_path(),
          input.get_source_text(), Terminal::Spirv::Target::Vulkan1_0);
      Core::Option<Terminal::Spirv::Products> module;
      Terminal::Spirv::Compiler()
          .compile(arena, request)
          .visit(
              [&](const Terminal::Spirv::Products& selected) {
                module = selected;
              },
              [](Terminal::Spirv::Failure) {});
      if (!module) {
        return Failure::GpuFailed;
      }
      Terminal::Abi::Symbol symbol(
          arena, *program, Terminal::Abi::Symbol::Kind::ReadOnly, unit);
      Memory::Managed::Bytes end_symbol(arena, symbol.get_view());
      end_symbol.concat("_end"_view);
      if (!object.add_read_only(
              symbol.get_view(), end_symbol.get_view(), module->get_module())) {
        return Failure::GpuFailed;
      }
      auto projection = Terminal::Abi::Projection::create(
          arena, *program, symbol.get_view(), unit);
      auto description = Terminal::Vulkan::Compiler().describe(
          arena, *program, symbol.get_view());
      if (!projection || !description ||
          !retain_binding(external, *program, symbol.get_view())) {
        return Failure::GpuFailed;
      }
      member.projections.push_back(*projection);
      vulkan.push_back(*description);
    }
    auto embedded = object.build();
    if (!embedded) {
      return Failure::GpuFailed;
    }
    member.embedded_object = static_cast<Memory::Dynamic::Bytes&&>(*embedded);
    if (!member.embedded_object.is_empty()) {
      objects.push_back(
          {.bytes = Memory::Dynamic::Bytes(member.embedded_object.get_view())});
    }
  }

  for (const NativeMember& member : members) {
    if (member.library == nullptr) {
      continue;
    }
    const auto input = source_input(workspace, *member.monograph);
    Terminal::Abi::Unit unit =
        create_unit(member, artifact, external, type_bindings);
    auto interface = Terminal::Abi::Compiler().compile(
        arena, *member.library, unit, errors, input.get_diagnostic_path(),
        input.get_source_text(), {}, vector_view(member.excluded),
        vector_view(member.projections));
    if (!interface) {
      return Failure::InterfaceFailed;
    }
    for (const Terminal::Abi::Publication& publication :
         interface->get_publications()) {
      if (!retain_binding(
              external, publication.get_semantic(), publication.get_symbol())) {
        return Failure::InterfaceFailed;
      }
    }
  }

  auto graphics_package = find_package(packages, "Perimortem.Graphics"_view);
  auto graphics_requirement =
      graphics_package ? find_domain(*graphics_package, "DrawableUI"_view)
                       : Core::Option<const Ttx::Model::Domain&>();
  auto graphics_sprite = graphics_package
                             ? find_domain(*graphics_package, "Sprite"_view)
                             : Core::Option<const Ttx::Model::Domain&>();
  for (const NativeMember& member : members) {
    if (member.library == nullptr) {
      continue;
    }
    const auto input = source_input(workspace, *member.monograph);
    Terminal::Abi::Unit unit =
        create_unit(member, artifact, external, type_bindings);
    auto interface = Terminal::Abi::Compiler().compile(
        arena, *member.library, unit, errors, input.get_diagnostic_path(),
        input.get_source_text(), {}, vector_view(member.excluded),
        vector_view(member.projections));
    if (!interface) {
      return Failure::InterfaceFailed;
    }

    Core::Option<Terminal::Graphics::Products> graphics;
    auto scene = member.monograph->select<Scene::Language::Monograph>();
    if (scene) {
      if (!graphics_requirement || !graphics_sprite) {
        return Failure::GpuFailed;
      }
      const Ttx::Model::Domain* configured[] = {&*graphics_sprite};
      graphics = Terminal::Graphics::Compiler().compile(
          arena, *scene, *graphics_requirement, configured);
      if (!graphics) {
        return Failure::GpuFailed;
      }
    }

    Terminal::Llvm::Request request(
        *member.library, errors, input.get_diagnostic_path(),
        input.get_source_text(), Terminal::Llvm::Target::X86_64SysV,
        Terminal::Llvm::Module::Debug::Level::Full, unit, *interface,
        graphics ? Core::Option<const Terminal::Graphics::Products&>(*graphics)
                 : Core::Option<const Terminal::Graphics::Products&>(),
        vector_view(member.excluded));
    Core::Option<Terminal::Llvm::Products> compiled;
    Terminal::Llvm::Compiler()
        .compile(arena, request)
        .visit(
            [&](const Terminal::Llvm::Products& selected) {
              compiled = selected;
            },
            [](Terminal::Llvm::Failure) {});
    if (!compiled) {
      return Failure::CpuFailed;
    }
    objects.push_back(
        {.bytes = Memory::Dynamic::Bytes(compiled->get_object())});
  }

  auto source = application_source(
      arena, *app, package.get_name(), members, external, packages, artifact,
      vector_view(vulkan));
  if (!source) {
    return Failure::ApplicationFailed;
  }
  auto executable = link_application(toolchain, *source, objects);
  return executable ? Utility::Result<Memory::Dynamic::Bytes, Failure>(
                          static_cast<Memory::Dynamic::Bytes&&>(*executable))
                    : Utility::Result<Memory::Dynamic::Bytes, Failure>(
                          Failure::LinkerFailed);
}
