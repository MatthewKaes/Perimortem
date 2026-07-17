// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/archiver/format.hpp"
#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/archiver/reader.hpp"
#include "tetrodotoxin/archiver/type/reference.hpp"
#include "tetrodotoxin/archiver/writer.hpp"
#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/puffer/package/builder.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin::Archiver;
using namespace Tetrodotoxin::Puffer;
using namespace Validation;

static Harness PufferBuffer = {
  .name = "Puffer::Buffer"_view,
};

static auto build_archive_buffer(
    Resolution::Resolver& resolver,
    Resolution::Source::Record& package,
    View::Bytes package_name,
    Dynamic::Bytes& output) -> Bool {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors(arena);
  Dynamic::Set<Resolution::Source::Record*> record_set;
  Dynamic::Vector<Resolution::Source::Record*> records;
  Managed::Vector<const Tetrodotoxin::Archiver::Package*> references(arena);
  Managed::Vector<Tetrodotoxin::Abi::Linkage> linkages(arena);
  resolver.visit_reachable(
      package, [&](Resolution::Source::Record& record) -> void {
        Bool inserted = record_set.insert(&record);
        if (inserted) {
          records.insert(&record);
          View::Vector<Tetrodotoxin::Abi::Linkage> source_linkages =
              record.get_implementation().get_linkages();
          for (Count i = 0; i < source_linkages.get_size(); i++) {
            linkages.insert(source_linkages[i]);
          }
        }
      });
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i] == &package ||
        records[i]->get_dialect().get_name() != "Package"_view) {
      continue;
    }

    const auto* restored = resolver.find_package(records[i]->get_source_path());
    if (restored == nullptr) {
      return False;
    }

    references.insert(restored);
  }

  output = Tetrodotoxin::Puffer::Package::Builder::build(
      arena, errors, package_name, package, records, references.get_view(),
      View::Vector<Terminal>(), linkages.get_view());
  return !errors.has_errors() && !output.is_empty();
}

static auto read_buffer_manifest(Allocator::Arena& arena, View::Bytes buffer)
    -> const Manifest* {
  Tetrodotoxin::Archiver::Reader reader(buffer);
  return reader.read_manifest(arena);
}

static auto read_buffer_package(
    Allocator::Arena& arena,
    View::Bytes buffer,
    View::Vector<const Tetrodotoxin::Archiver::Package*> references =
        View::Vector<const Tetrodotoxin::Archiver::Package*>())
    -> const Tetrodotoxin::Archiver::Package* {
  Tetrodotoxin::Archiver::Reader reader(buffer);
  const Manifest* manifest = reader.read_manifest(arena);
  if (manifest == nullptr) {
    return nullptr;
  }

  return reader.read_package(arena, *manifest, references);
}

static auto write_test_package(
    Allocator::Arena& arena,
    View::Bytes name,
    Ttx::Type& root,
    View::Vector<Terminal> terminals = View::Vector<Terminal>())
    -> View::Bytes {
  return Tetrodotoxin::Archiver::Writer::write(
      arena, name, View::Vector<Dependency>(), root,
      View::Vector<const Ttx::Type*>(), terminals,
      View::Vector<Tetrodotoxin::Abi::Linkage>(),
      View::Vector<const Tetrodotoxin::Archiver::Package*>());
}

static auto load_core_package(
    Resolution::Resolver& resolver,
    View::Bytes value_type = "Unsigned_32"_view)
    -> Resolution::Source::Record* {
  Resolution::Resolver::Context types_context;
  Dynamic::Bytes types_source;
  types_source.concat("dialect : Library;\npublic Value : alias = "_view);
  types_source.concat(value_type);
  types_source.concat(";\n"_view);
  Resolution::Source::Record* types = resolver.load_source(
      types_context, "user/core/types.ttx"_view, types_source);
  if (types == nullptr || types_context.has_errors()) {
    return nullptr;
  }

  Resolution::Resolver::Context package_context;
  Resolution::Source::Record* package = resolver.load_source(
      package_context, "user/core/package.ttx"_view,
      "dialect : Package;\n"
      "import Types : Library = \"types.ttx\";\n"
      "expose Value : alias = Types::Value;\n"_view);
  if (package == nullptr || package_context.has_errors()) {
    return nullptr;
  }

  return package;
}

static auto build_core_buffer(
    Dynamic::Bytes& output,
    View::Bytes value_type = "Unsigned_32"_view) -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "User.Core"_view);
  Resolution::Source::Record* package = load_core_package(resolver, value_type);
  if (package == nullptr) {
    return False;
  }

  return build_archive_buffer(resolver, *package, "User.Core"_view, output);
}

static auto build_foreign_buffer(Dynamic::Bytes& output) -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "User.Host"_view);
  Resolution::Resolver::Context library_context;
  Resolution::Source::Record* library = resolver.load_source(
      library_context, "user/host/console.ttx"_view,
      "dialect : Library;\n"
      "public Console : foreign {\n"
      "  expose func print[.data : View[Bytes]] -> [];\n"
      "}\n"_view);
  if (library == nullptr || library_context.has_errors()) {
    return False;
  }

  Resolution::Resolver::Context package_context;
  Resolution::Source::Record* package = resolver.load_source(
      package_context, "user/host/package.ttx"_view,
      "dialect : Package;\n"
      "import Api : Library = \"console.ttx\";\n"
      "expose Console : alias = Api::Console;\n"_view);
  if (package == nullptr || package_context.has_errors()) {
    return False;
  }

  return build_archive_buffer(resolver, *package, "User.Host"_view, output);
}

static auto build_linkage_buffer(Dynamic::Bytes& output) -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "User.Callable"_view);
  Resolution::Resolver::Context library_context;
  Resolution::Source::Record* library = resolver.load_source(
      library_context, "user/callable/api.ttx"_view,
      "dialect : Library;\n"
      "@cpp(Count)\n"
      "@abi(integer)\n"
      "public Counter : struct {\n"
      "  public func identity[.value : Counter] -> Counter {\n"
      "    return value;\n"
      "  }\n"
      "  public func identity[self] -> Counter {\n"
      "    return self;\n"
      "  }\n"
      "}\n"
      "public func echo[.data : View[Bytes]] -> [] {\n"
      "  return;\n"
      "}\n"_view);
  if (library == nullptr || library_context.has_errors()) {
    return False;
  }

  Resolution::Resolver::Context package_context;
  Resolution::Source::Record* package = resolver.load_source(
      package_context, "user/callable/package.ttx"_view,
      "dialect : Package;\n"
      "import Api : Library = \"api.ttx\";\n"
      "expose Api : alias = Api;\n"_view);
  if (package == nullptr || package_context.has_errors()) {
    return False;
  }

  return build_archive_buffer(resolver, *package, "User.Callable"_view, output);
}

static auto register_core_buffer(
    Resolution::Resolver& resolver,
    View::Bytes core_buffer) -> Bool {
  Resolution::Resolver::Context buffer_context;
  Bool registered = resolver.register_package_buffer(
      buffer_context, "user_core.puffer"_view, core_buffer);
  return registered && !buffer_context.has_errors();
}

static auto load_ui_package(Resolution::Resolver& resolver)
    -> Resolution::Source::Record* {
  Resolution::Resolver::Context package_context;
  Resolution::Source::Record* package = resolver.load_source(
      package_context, "user/ui/package.ttx"_view,
      "dialect : Package;\n"
      "import Core : Package = User.Core;\n"
      "expose Core : alias = Core;\n"_view);
  if (package == nullptr || package_context.has_errors()) {
    return nullptr;
  }

  return package;
}

static auto build_ui_buffer(View::Bytes core_buffer, Dynamic::Bytes& output)
    -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "User.Ui"_view);
  Bool core_registered = register_core_buffer(resolver, core_buffer);
  if (!core_registered) {
    return False;
  }

  Resolution::Source::Record* package = load_ui_package(resolver);
  if (package == nullptr) {
    return False;
  }

  return build_archive_buffer(resolver, *package, "User.Ui"_view, output);
}

static auto register_ui_buffer(
    Resolution::Resolver& resolver,
    View::Bytes ui_buffer) -> Bool {
  Resolution::Resolver::Context buffer_context;
  Bool registered = resolver.register_package_buffer(
      buffer_context, "user_ui.puffer"_view, ui_buffer);
  return registered && !buffer_context.has_errors();
}

static auto load_app_package(Resolution::Resolver& resolver)
    -> Resolution::Source::Record* {
  Resolution::Resolver::Context package_context;
  Resolution::Source::Record* package = resolver.load_source(
      package_context, "user/app/package.ttx"_view,
      "dialect : Package;\n"
      "import Ui : Package = User.Ui;\n"
      "expose Ui : alias = Ui;\n"_view);
  if (package == nullptr || package_context.has_errors()) {
    return nullptr;
  }

  return package;
}

static auto build_app_buffer(
    View::Bytes core_buffer,
    View::Bytes ui_buffer,
    Dynamic::Bytes& output) -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry, {}, "User.App"_view);
  Bool core_registered = register_core_buffer(resolver, core_buffer);
  if (!core_registered) {
    return False;
  }

  Bool ui_registered = register_ui_buffer(resolver, ui_buffer);
  if (!ui_registered) {
    return False;
  }

  Resolution::Source::Record* package = load_app_package(resolver);
  if (package == nullptr) {
    return False;
  }

  return build_archive_buffer(resolver, *package, "User.App"_view, output);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, transitive_chain) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();

  Dynamic::Bytes core_buffer;
  Dynamic::Bytes ui_buffer;
  Dynamic::Bytes app_buffer;
  ASSERT(build_core_buffer(core_buffer));
  ASSERT(build_ui_buffer(core_buffer, ui_buffer));
  ASSERT(build_app_buffer(core_buffer, ui_buffer, app_buffer));

  Resolution::Resolver consumer(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_app.puffer"_view, app_buffer));
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_ui.puffer"_view, ui_buffer));
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_core.puffer"_view, core_buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;
  const Resolution::Source::Record* root = consumer.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import App : Package = User.App;\n"
      "private Value : alias = App::Ui::Core::Value;\n"_view);
  ASSERT(root != nullptr);
  EXPECT_NOT(root_source_context.has_errors());

  const Resolution::Source::Record* app = consumer.resolve("User.App"_view);
  const Resolution::Source::Record* ui = consumer.resolve("User.Ui"_view);
  const Resolution::Source::Record* core = consumer.resolve("User.Core"_view);
  ASSERT(app != nullptr);
  ASSERT(ui != nullptr);
  ASSERT(core != nullptr);

  const Ttx::Type* app_ui = app->get_type().find_type("Ui"_view);
  ASSERT(app_ui != nullptr);
  const Ttx::Type* ui_core = app_ui->find_type("Core"_view);
  ASSERT(ui_core != nullptr);
  const Ttx::Type* core_value = core->get_type().find_type("Value"_view);
  ASSERT(core_value != nullptr);
  EXPECT(ui_core->find_type("Value"_view) == core_value);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, standard_pkg) {
  auto math_buffer = File::read(
      ".bin/bin/tetrodotoxin/standard/Perimortem.Math/"
      "binary_archive.puffer"_view);
  auto graphics_buffer = File::read(
      ".bin/bin/tetrodotoxin/standard/Perimortem.Graphics/"
      "binary_archive.puffer"_view);
  ASSERT(!math_buffer.is_empty());
  ASSERT(!graphics_buffer.is_empty());

  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(resolver.register_package_buffer(
      dependency_context, "math/binary_archive.puffer"_view, math_buffer));
  ASSERT(resolver.register_package_buffer(
      dependency_context, "graphics/binary_archive.puffer"_view,
      graphics_buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;
  const Resolution::Source::Record* root = resolver.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Graphics : Package = Perimortem.Graphics;\n"
      "private Size : alias = Graphics::Size2D;\n"
      "private Shader : alias = Graphics::Shaders::Default2D;\n"_view);
  ASSERT(root != nullptr);
  EXPECT_NOT(root_source_context.has_errors());

  const Resolution::Source::Record* graphics =
      resolver.resolve("Perimortem.Graphics"_view);
  const Resolution::Source::Record* math =
      resolver.resolve("Perimortem.Math"_view);
  ASSERT(graphics != nullptr);
  ASSERT(math != nullptr);
  const auto* graphics_package =
      resolver.find_package("Perimortem.Graphics"_view);
  ASSERT(graphics_package != nullptr);
  EXPECT_TEXT(graphics->get_source_path(), "Perimortem.Graphics"_view);
  View::Vector<Dependency> graphics_imports =
      graphics_package->get_manifest().get_imports();
  ASSERT_EQ(graphics_imports.get_size(), Count(1));
  EXPECT_TEXT(graphics_imports[0].get_source_name(), "Perimortem.Math"_view);

  const Ttx::Type* graphics_type = &graphics->get_type();
  ASSERT(graphics_type->find_type("Color"_view) != nullptr);
  ASSERT(graphics_type->find_type("Render2D"_view) != nullptr);
  const Ttx::Type* image = graphics_type->find_type("Image"_view);
  ASSERT(image != nullptr);
  const Ttx::Function* decode = image->find_type_function("decode"_view);
  ASSERT(decode != nullptr);
  const auto* decode_linkage =
      graphics->get_implementation().find_linkage(*decode);
  ASSERT(decode_linkage != nullptr);
  ASSERT(decode_linkage->get_symbol().get_size() >= Count(5));
  EXPECT_TEXT(decode_linkage->get_symbol().slice(0, 4), "ttx_"_view);
  EXPECT(image->find_addressable_function("get_size_pixels"_view) != nullptr);
  EXPECT(image->find_addressable_function("sample"_view) != nullptr);
  const Ttx::Type* size = graphics_type->find_type("Size2D"_view);
  ASSERT(size != nullptr);
  EXPECT_TEXT(
      size->describe().get_view(),
      "Perimortem.Graphics::Size2D alias of "
      "Perimortem.Math::Geometry::Size2D"_view);

  const Ttx::Type* shaders = graphics_type->find_type("Shaders"_view);
  ASSERT(shaders != nullptr);
  ASSERT(shaders->find_type("Default2D"_view) != nullptr);

  View::Vector<Terminal> terminals = graphics_package->get_terminals();
  ASSERT_EQ(terminals.get_size(), Count(2));
  EXPECT_TEXT(terminals[0].get_group(), "linker"_view);
  EXPECT_TEXT(terminals[0].get_path(), "x86_64.a"_view);
  EXPECT_NOT(terminals[0].get_content().is_empty());
  EXPECT_TEXT(terminals[1].get_group(), "header"_view);
  EXPECT_TEXT(terminals[1].get_path(), "cpp_abi.hpp"_view);
  EXPECT_NOT(terminals[1].get_content().is_empty());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, foreign_facts) {
  Dynamic::Bytes buffer;
  ASSERT(build_foreign_buffer(buffer));

  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(resolver.register_package_buffer(
      dependency_context, "user_host.puffer"_view, buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context source_context;
  const Resolution::Source::Record* source = resolver.load_source(
      source_context, "unit/main.ttx"_view,
      "dialect : Library;\n"
      "import Host : Package = User.Host;\n"
      "public func main[] -> [] {\n"
      "  Host::Console -> print(\"Hello\");\n"
      "}\n"_view);
  ASSERT(source != nullptr);
  EXPECT_NOT(source_context.has_errors());

  const Ttx::Function* main =
      source->get_type().find_type_function("main"_view);
  ASSERT(main != nullptr);
  const auto* body = source->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*main);
  ASSERT(body != nullptr);
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Call>(), Count(1));
  EXPECT_TEXT(
      body->find<Tetrodotoxin::Compiler::Execution::Call>()->get_symbol(),
      "print"_view);

  const Resolution::Source::Record* host = resolver.resolve("User.Host"_view);
  ASSERT(host != nullptr);
  const Ttx::Type* console = host->get_type().find_type("Console"_view);
  ASSERT(console != nullptr);
  const Ttx::Function* print = console->find_type_function("print"_view);
  ASSERT(print != nullptr);
  const auto* linkage = host->get_implementation().find_linkage(*print);
  ASSERT(linkage != nullptr);
  EXPECT_TEXT(linkage->get_symbol(), "print"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, linkage_facts) {
  Dynamic::Bytes buffer;
  ASSERT(build_linkage_buffer(buffer));

  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(resolver.register_package_buffer(
      dependency_context, "user_callable.puffer"_view, buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context source_context;
  const Resolution::Source::Record* source = resolver.load_source(
      source_context, "unit/main.ttx"_view,
      "dialect : Library;\n"
      "import Callable : Package = User.Callable;\n"
      "public func main[] -> [] {\n"
      "  Callable::Api -> echo(\"Hello\");\n"
      "}\n"_view);
  ASSERT(source != nullptr);
  EXPECT_NOT(source_context.has_errors());

  const Ttx::Function* main =
      source->get_type().find_type_function("main"_view);
  ASSERT(main != nullptr);
  const auto* body = source->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*main);
  ASSERT(body != nullptr);
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Call>(), Count(1));
  View::Bytes call_symbol =
      body->find<Tetrodotoxin::Compiler::Execution::Call>()->get_symbol();
  ASSERT(call_symbol.get_size() >= Count(6));
  EXPECT_TEXT(call_symbol.slice(0, 13), "ttx_internal_"_view);

  const Resolution::Source::Record* restored =
      resolver.resolve("User.Callable"_view);
  ASSERT(restored != nullptr);
  const Ttx::Type* api = restored->get_type().find_type("Api"_view);
  ASSERT(api != nullptr);
  const Ttx::Function* echo = api->find_type_function("echo"_view);
  ASSERT(echo != nullptr);
  const auto* linkage = restored->get_implementation().find_linkage(*echo);
  ASSERT(linkage != nullptr);
  EXPECT_TEXT(linkage->get_symbol(), call_symbol);

  const Ttx::Type* counter = api->find_type("Counter"_view);
  ASSERT(counter != nullptr);
  const Ttx::Function* type_identity =
      counter->find_type_function("identity"_view);
  const Ttx::Function* addressable_identity =
      counter->find_addressable_function("identity"_view);
  ASSERT(type_identity != nullptr);
  ASSERT(addressable_identity != nullptr);
  const auto* type_linkage =
      restored->get_implementation().find_linkage(*type_identity);
  const auto* addressable_linkage =
      restored->get_implementation().find_linkage(*addressable_identity);
  ASSERT(type_linkage != nullptr);
  ASSERT(addressable_linkage != nullptr);
  EXPECT(type_linkage->get_symbol() != addressable_linkage->get_symbol());
  ASSERT(addressable_linkage->get_symbol().get_size() >= Count(6));
  EXPECT_TEXT(
      addressable_linkage->get_symbol().slice(0, 13), "ttx_internal_"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, bad_header) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context context;

  EXPECT_NOT(resolver.register_package_buffer(
      context, "bad.puffer"_view, "not a puffer buffer"_view));
  ASSERT(context.has_errors());
  EXPECT_TEXT(
      context.get_errors()[0].get_message(),
      "Puffer Buffer is not a package snapshot."_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, dup_same) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Dynamic::Bytes core_buffer;
  ASSERT(build_core_buffer(core_buffer));

  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context context;
  ASSERT(resolver.register_package_buffer(
      context, "user_core.puffer"_view, core_buffer));
  ASSERT(resolver.register_package_buffer(
      context, "user_core_copy.puffer"_view, core_buffer));
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, dup_conflict) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Dynamic::Bytes expected_core_buffer;
  Dynamic::Bytes actual_core_buffer;
  ASSERT(build_core_buffer(expected_core_buffer));
  ASSERT(build_core_buffer(actual_core_buffer, "Unsigned_64"_view));

  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context valid_context;
  ASSERT(resolver.register_package_buffer(
      valid_context, "user_core.puffer"_view, expected_core_buffer));
  EXPECT_NOT(valid_context.has_errors());

  Resolution::Resolver::Context conflict_context;
  EXPECT_NOT(resolver.register_package_buffer(
      conflict_context, "user_core_conflict.puffer"_view, actual_core_buffer));
  ASSERT(conflict_context.has_errors());
  EXPECT_TEXT(
      conflict_context.get_errors()[0].get_message(),
      "Puffer Buffer package conflicts with an already registered "
      "version."_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, missing_transitive) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Dynamic::Bytes core_buffer;
  Dynamic::Bytes ui_buffer;
  Dynamic::Bytes app_buffer;
  ASSERT(build_core_buffer(core_buffer));
  ASSERT(build_ui_buffer(core_buffer, ui_buffer));
  ASSERT(build_app_buffer(core_buffer, ui_buffer, app_buffer));

  Resolution::Resolver consumer(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_app.puffer"_view, app_buffer));
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_ui.puffer"_view, ui_buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;
  EXPECT_NOT(consumer.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import App : Package = User.App;\n"_view));
  ASSERT(root_source_context.has_errors());
  EXPECT_TEXT(
      root_source_context.get_errors()[0].get_message(),
      "Puffer Buffer dependency is not registered: User.Core"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, version_mismatch) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Dynamic::Bytes expected_core_buffer;
  Dynamic::Bytes actual_core_buffer;
  Dynamic::Bytes ui_buffer;
  ASSERT(build_core_buffer(expected_core_buffer));
  ASSERT(build_ui_buffer(expected_core_buffer, ui_buffer));
  ASSERT(build_core_buffer(actual_core_buffer, "Unsigned_64"_view));

  Allocator::Arena archive_arena;
  const Manifest* expected_archive =
      read_buffer_manifest(archive_arena, expected_core_buffer);
  const Manifest* actual_archive =
      read_buffer_manifest(archive_arena, actual_core_buffer);
  ASSERT(expected_archive != nullptr);
  ASSERT(actual_archive != nullptr);
  EXPECT(expected_archive->get_version() != actual_archive->get_version());

  Resolution::Resolver consumer(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_ui.puffer"_view, ui_buffer));
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_core.puffer"_view, actual_core_buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;
  EXPECT_NOT(consumer.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Ui : Package = User.Ui;\n"_view));
  ASSERT(root_source_context.has_errors());
  EXPECT_TEXT(
      root_source_context.get_errors()[0].get_message(),
      "Puffer Buffer dependency version mismatch: User.Core"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, archive_identity) {
  Dynamic::Bytes first_core;
  Dynamic::Bytes second_core;
  Dynamic::Bytes ui_buffer;
  ASSERT(build_core_buffer(first_core));
  ASSERT(build_core_buffer(second_core));
  ASSERT(build_ui_buffer(first_core, ui_buffer));
  EXPECT(first_core == second_core);

  Allocator::Arena arena;
  const Tetrodotoxin::Archiver::Package* core =
      read_buffer_package(arena, first_core);
  ASSERT(core != nullptr);
  EXPECT_TEXT(core->get_manifest().get_name(), "User.Core"_view);
  EXPECT(core->get_manifest().get_version().is_set());
  EXPECT_TEXT(core->get_type().get_name(), "Package"_view);

  const Ttx::Type* value = core->get_type().find_type("Value"_view);
  ASSERT(value != nullptr);
  EXPECT(value->is_alias());
  EXPECT_TEXT(
      value->describe().get_view(),
      "User.Core::Value alias of Unsigned_32"_view);
  View::Vector<const Ttx::Type*> types = core->get_types();
  ASSERT(types.get_size() >= Count(2));
  EXPECT(types[0] == &core->get_type());
  Bool found_value = False;
  for (Count i = 0; i < types.get_size(); i++) {
    found_value = found_value || types[i] == value;
  }

  EXPECT(found_value);

  const Manifest* ui = read_buffer_manifest(arena, ui_buffer);
  ASSERT(ui != nullptr);
  View::Vector<Dependency> imports = ui->get_imports();
  ASSERT_EQ(imports.get_size(), Count(1));
  EXPECT_TEXT(imports[0].get_local_name(), "Core"_view);
  EXPECT_TEXT(imports[0].get_source_name(), "User.Core"_view);
  EXPECT(imports[0].get_version() == core->get_manifest().get_version());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, reference_identity) {
  constexpr Uuid a_version(1, 1);
  constexpr Uuid b_version(2, 2);
  constexpr Uuid c_version(3, 3);
  Ttx::Type a_type("A"_view);
  Ttx::Type b_type("B"_view);
  Ttx::Type c_type("C"_view);
  Static::Vector<const Ttx::Type*, 1> a_types = {{&a_type}};
  Static::Vector<const Ttx::Type*, 1> b_types = {{&b_type}};
  Static::Vector<const Ttx::Type*, 1> c_types = {{&c_type}};
  Static::Vector<Dependency, 1> a_imports = {{
    Dependency("C"_view, "User.C"_view, c_version),
  }};
  Tetrodotoxin::Archiver::Package a_package(
      Manifest("User.A"_view, a_version, a_imports), a_type, a_types,
      View::Vector<Terminal>());
  Tetrodotoxin::Archiver::Package b_package(
      Manifest("User.B"_view, b_version, View::Vector<Dependency>()), b_type,
      b_types, View::Vector<Terminal>());
  Tetrodotoxin::Archiver::Package c_package(
      Manifest("User.C"_view, c_version, View::Vector<Dependency>()), c_type,
      c_types, View::Vector<Terminal>());

  Static::Vector<Ttx::Member, 3> members = {{
    Ttx::Member("a"_view, a_type),
    Ttx::Member("b"_view, b_type),
    Ttx::Member("c"_view, c_type),
  }};
  Ttx::Type root("Root"_view, members);
  Static::Vector<Dependency, 2> imports = {{
    Dependency("A"_view, "User.A"_view, a_version),
    Dependency("B"_view, "User.B"_view, b_version),
  }};
  Static::Vector<const Tetrodotoxin::Archiver::Package*, 3> write_references = {
    {
      &a_package,
      &c_package,
      &b_package,
    }};

  Allocator::Arena writer_arena;
  View::Bytes output = Tetrodotoxin::Archiver::Writer::write(
      writer_arena, "User.Root"_view, imports, root,
      View::Vector<const Ttx::Type*>(), View::Vector<Terminal>(),
      View::Vector<Tetrodotoxin::Abi::Linkage>(), write_references);
  ASSERT(!output.is_empty());

  Static::Vector<const Tetrodotoxin::Archiver::Package*, 3> read_references = {{
    &b_package,
    &a_package,
    &c_package,
  }};
  Allocator::Arena reader_arena;
  Tetrodotoxin::Archiver::Reader reader(output);
  const Manifest* manifest = reader.read_manifest(reader_arena);
  ASSERT(manifest != nullptr);
  const Tetrodotoxin::Archiver::Package* restored =
      reader.read_package(reader_arena, *manifest, read_references);
  ASSERT(restored != nullptr);
  View::Vector<Ttx::Member> restored_members =
      restored->get_type().get_members();
  ASSERT_EQ(restored_members.get_size(), Count(3));
  EXPECT(&restored_members[0].get_type() == &a_type);
  EXPECT(&restored_members[1].get_type() == &b_type);
  EXPECT(&restored_members[2].get_type() == &c_type);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, archive_stress) {
  Allocator::Arena writer_arena;
  Managed::Vector<Ttx::Type::Reference> nested(writer_arena);
  nested.reset(256);
  for (Count i = 0; i < 256; i++) {
    const Ttx::Type& type = writer_arena.construct<Ttx::Type>("Item"_view);
    nested.insert(Ttx::Type::Reference(type));
  }

  Ttx::Type root("Root"_view, View::Vector<Ttx::Member>(), nested.get_view());
  View::Bytes output =
      write_test_package(writer_arena, "User.Stress"_view, root);
  ASSERT(!output.is_empty());

  Tetrodotoxin::Archiver::Reader reader(output);
  Allocator::Arena restore_arena;
  for (Count i = 0; i < 64; i++) {
    restore_arena.reset();
    const Manifest* manifest = reader.read_manifest(restore_arena);
    ASSERT(manifest != nullptr);

    const Tetrodotoxin::Archiver::Package* package = reader.read_package(
        restore_arena, *manifest,
        View::Vector<const Tetrodotoxin::Archiver::Package*>());
    ASSERT(package != nullptr);
    ASSERT_EQ(package->get_types().get_size(), Count(257));
    EXPECT(package->get_types()[0] == &package->get_type());
  }
}

PERIMORTEM_UNIT_TEST(PufferBuffer, table_reads) {
  Allocator::Arena writer_arena;
  Ttx::Type root("Root"_view);
  View::Bytes output =
      write_test_package(writer_arena, "User.Continuation"_view, root);
  ASSERT(!output.is_empty());

  Allocator::Arena reader_arena;
  Tetrodotoxin::Archiver::Reader reader(output);
  const Manifest* first = reader.read_manifest(reader_arena);
  ASSERT(first != nullptr);

  const Tetrodotoxin::Archiver::Package* package = reader.read_package(
      reader_arena, *first,
      View::Vector<const Tetrodotoxin::Archiver::Package*>());
  ASSERT(package != nullptr);
  EXPECT_TEXT(package->get_manifest().get_name(), "User.Continuation"_view);

  const Manifest* second = reader.read_manifest(reader_arena);
  ASSERT(second != nullptr);
  EXPECT_TEXT(second->get_name(), first->get_name());

  const Tetrodotoxin::Archiver::Package* repeated = reader.read_package(
      reader_arena, *second,
      View::Vector<const Tetrodotoxin::Archiver::Package*>());
  ASSERT(repeated != nullptr);
  EXPECT_TEXT(repeated->get_manifest().get_name(), first->get_name());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, table_bounds) {
  Allocator::Arena writer_arena;
  Ttx::Type root("Root"_view);
  Dynamic::Bytes output =
      write_test_package(writer_arena, "User.Bounds"_view, root);
  ASSERT(!output.is_empty());

  Unsigned_64 invalid_offset =
      Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Little>(
          Unsigned_64(output.get_size()));
  Data::copy(
      output.get_access().get_data() + Format::slot(Format::Table::Manifest),
      invalid_offset);

  Allocator::Arena reader_arena;
  const Manifest* manifest =
      Tetrodotoxin::Archiver::Reader(output).read_manifest(reader_arena);
  EXPECT(manifest == nullptr);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, invalid_alias_reference) {
  Allocator::Arena writer_arena;
  Ttx::Type target("Target"_view);
  Ttx::Type alias = Ttx::Type::alias("Alias"_view, target);
  Dynamic::Bytes output =
      write_test_package(writer_arena, "User.InvalidAlias"_view, alias);
  ASSERT(!output.is_empty());

  Perimortem::Core::Reader::Binary<Data::ByteOrder::Little> encoded(output);
  encoded.set_location(Format::slot(Format::Table::Package));
  Count package_offset = Count(encoded.read_unsigned_64());
  encoded.set_location(package_offset);
  EXPECT_EQ(encoded.read_unsigned_8(), Unsigned_8(0));
  EXPECT_EQ(encoded.read_unsigned_8(), Unsigned_8(2));
  Count name_size = encoded.read_unsigned_8();
  encoded.set_location(encoded.get_location() + name_size);
  EXPECT_EQ(encoded.read_unsigned_8(), Unsigned_8(0));
  EXPECT_EQ(encoded.read_unsigned_8(), Unsigned_8(0));
  EXPECT_EQ(
      encoded.read_unsigned_8(),
      Unsigned_8(Tetrodotoxin::Archiver::Type::Reference::Kind::Local));

  Count alias_id_location = encoded.get_location();
  output.get_access().get_data()[alias_id_location] = Unsigned_8(0x7f);

  Allocator::Arena reader_arena;
  Tetrodotoxin::Archiver::Reader reader(output);
  const Manifest* manifest = reader.read_manifest(reader_arena);
  ASSERT(manifest != nullptr);
  EXPECT(
      reader.read_package(
          reader_arena, *manifest,
          View::Vector<const Tetrodotoxin::Archiver::Package*>()) == nullptr);

  output.get_access().get_data()[alias_id_location] = Unsigned_8(1);
  Unsigned_64 truncated_package =
      Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Little>(
          Unsigned_64(alias_id_location));
  Data::copy(
      output.get_access().get_data() + Format::slot(Format::Table::Linkages),
      truncated_package);

  Allocator::Arena truncated_arena;
  Tetrodotoxin::Archiver::Reader truncated_reader(output);
  const Manifest* truncated_manifest =
      truncated_reader.read_manifest(truncated_arena);
  ASSERT(truncated_manifest != nullptr);
  EXPECT(
      truncated_reader.read_package(
          truncated_arena, *truncated_manifest,
          View::Vector<const Tetrodotoxin::Archiver::Package*>()) == nullptr);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, manifest_identity) {
  Allocator::Arena writer_arena;
  Ttx::Type first_root("First"_view);
  Ttx::Type second_root("Second"_view);
  View::Bytes first =
      write_test_package(writer_arena, "User.First"_view, first_root);
  View::Bytes second =
      write_test_package(writer_arena, "User.Second"_view, second_root);
  ASSERT(!first.is_empty());
  ASSERT(!second.is_empty());

  Allocator::Arena reader_arena;
  Tetrodotoxin::Archiver::Reader first_reader(first);
  const Manifest* second_manifest =
      Tetrodotoxin::Archiver::Reader(second).read_manifest(reader_arena);
  ASSERT(second_manifest != nullptr);

  EXPECT(
      first_reader.read_package(
          reader_arena, *second_manifest,
          View::Vector<const Tetrodotoxin::Archiver::Package*>()) == nullptr);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, terminal_bytes) {
  Allocator::Arena writer_arena;
  Ttx::Type root("Root"_view);
  Static::Vector<Terminal, 1> terminals = {{
    Terminal("linker"_view, "x86_64.a"_view, "archive bytes"_view),
  }};
  View::Bytes output =
      write_test_package(writer_arena, "User.Terminal"_view, root, terminals);

  ASSERT(!output.is_empty());
  Allocator::Arena archive_arena;
  const Tetrodotoxin::Archiver::Package* archive =
      read_buffer_package(archive_arena, output);
  ASSERT(archive != nullptr);
  EXPECT_TEXT(
      archive->find_terminal("linker"_view, "x86_64.a"_view),
      "archive bytes"_view);
}
