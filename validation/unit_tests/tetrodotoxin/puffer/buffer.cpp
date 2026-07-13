// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

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

static auto documentation_matches(
    Ttx::Documentation left,
    Ttx::Documentation right) -> Bool {
  if (left.get_line_count() != right.get_line_count()) {
    return False;
  }

  for (Count i = 0; i < left.get_line_count(); i++) {
    if (left.line_at(i) != right.line_at(i)) {
      return False;
    }
  }

  return True;
}

static auto attributes_match(
    View::Vector<Ttx::Attribute> left,
    View::Vector<Ttx::Attribute> right) -> Bool {
  if (left.get_size() != right.get_size()) {
    return False;
  }

  for (Count i = 0; i < left.get_size(); i++) {
    if (left[i].get_key() != right[i].get_key() ||
        left[i].get_value() != right[i].get_value()) {
      return False;
    }
  }

  return True;
}

static auto type_refs_match(const Ttx::Type* left, const Ttx::Type* right)
    -> Bool {
  if (left == nullptr || right == nullptr) {
    return left == right;
  }

  return left->get_name() == right->get_name() &&
         left->describe().get_view() == right->describe().get_view();
}

static auto members_match(
    View::Vector<Ttx::Member> left,
    View::Vector<Ttx::Member> right) -> Bool {
  if (left.get_size() != right.get_size()) {
    return False;
  }

  for (Count i = 0; i < left.get_size(); i++) {
    if (left[i].get_name() != right[i].get_name() ||
        left[i].is_defaulted() != right[i].is_defaulted() ||
        !documentation_matches(
            left[i].get_documentation(), right[i].get_documentation()) ||
        !attributes_match(
            left[i].get_attributes(), right[i].get_attributes()) ||
        !type_refs_match(&left[i].get_type(), &right[i].get_type())) {
      return False;
    }
  }

  return True;
}

static auto functions_match(
    View::Vector<Ttx::Function> left,
    View::Vector<Ttx::Function> right) -> Bool {
  if (left.get_size() != right.get_size()) {
    return False;
  }

  for (Count i = 0; i < left.get_size(); i++) {
    if (left[i].get_name() != right[i].get_name() ||
        !documentation_matches(
            left[i].get_documentation(), right[i].get_documentation()) ||
        !left[i].get_parameters().equivalent_to(right[i].get_parameters()) ||
        !left[i].get_result().equivalent_to(right[i].get_result())) {
      return False;
    }
  }

  return True;
}

static auto type_trees_match(const Ttx::Type& left, const Ttx::Type& right)
    -> Bool {
  if (left.get_name() != right.get_name() ||
      left.is_alias() != right.is_alias() ||
      left.describe().get_view() != right.describe().get_view() ||
      !documentation_matches(
          left.get_documentation(), right.get_documentation()) ||
      !attributes_match(left.get_attributes(), right.get_attributes()) ||
      !type_refs_match(left.get_alias_parent(), right.get_alias_parent()) ||
      !members_match(left.get_members(), right.get_members()) ||
      !functions_match(left.get_functions(), right.get_functions())) {
    return False;
  }

  View::Vector<const Ttx::Type*> left_types = left.get_types();
  View::Vector<const Ttx::Type*> right_types = right.get_types();
  if (left_types.get_size() != right_types.get_size()) {
    return False;
  }

  for (Count i = 0; i < left_types.get_size(); i++) {
    if (left_types[i] == nullptr || right_types[i] == nullptr ||
        !type_trees_match(*left_types[i], *right_types[i])) {
      return False;
    }
  }

  return True;
}

static auto has_import(View::Vector<Dependency> imports, View::Bytes name)
    -> Bool {
  for (Count i = 0; i < imports.get_size(); i++) {
    if (imports[i].get_source_name() == name) {
      return True;
    }
  }

  return False;
}

static auto build_archive_buffer(
    Resolution::Resolver& resolver,
    Resolution::Source::Record& package,
    Dynamic::Bytes& output) -> Bool {
  Allocator::Arena arena;
  Ttx::Lexical::Errors errors(arena);
  Dynamic::Set<Resolution::Source::Record*> record_set;
  Dynamic::Vector<Resolution::Source::Record*> records;
  resolver.visit_reachable(
      package, [&](Resolution::Source::Record& record) -> void {
        if (record_set.insert(&record)) {
          records.insert(&record);
        }
      });

  Tetrodotoxin::Puffer::Package::Builder builder(arena, errors);
  output = builder.build(resolver, package, records, View::Vector<Terminal>());
  return !errors.has_errors() && !output.is_empty();
}

static auto read_buffer_manifest(Allocator::Arena& arena, View::Bytes buffer)
    -> Manifest {
  Tetrodotoxin::Archiver::Reader reader(buffer);
  return reader.read_manifest(arena);
}

static auto read_buffer_package(
    Allocator::Arena& arena,
    View::Bytes buffer,
    View::Vector<Reference> references = View::Vector<Reference>())
    -> const Tetrodotoxin::Archiver::Package* {
  Tetrodotoxin::Archiver::Reader reader(buffer);
  Manifest manifest = reader.read_manifest(arena);
  return reader.read_package(arena, manifest, references);
}

static auto write_test_package(
    Allocator::Arena& arena,
    View::Bytes name,
    Ttx::Type& root,
    View::Vector<Terminal> terminals = View::Vector<Terminal>())
    -> View::Bytes {
  Tetrodotoxin::Archiver::Package package(
      Manifest(name, Version(), View::Vector<Dependency>()), root,
      View::Vector<const Ttx::Type*>(), terminals);
  return Tetrodotoxin::Archiver::Writer::write(
      arena, package, View::Vector<Reference>());
}

static auto load_core_package(
    Resolution::Resolver& resolver,
    View::Bytes value_type = "Bits_32"_view) -> Resolution::Source::Record* {
  Resolution::Resolver::Context types_context;
  Dynamic::Bytes types_source;
  types_source.concat("dialect : Library;\npublic Value : alias = "_view);
  types_source.concat(value_type);
  types_source.concat(";\n"_view);
  if (resolver.load_source(
          types_context, "user/core/types.ttx"_view, types_source) == nullptr ||
      types_context.has_errors()) {
    return nullptr;
  }

  Resolution::Resolver::Context package_context;
  resolver.set_package_name("User.Core"_view);
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
    View::Bytes value_type = "Bits_32"_view) -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Source::Record* package = load_core_package(resolver, value_type);
  return package != nullptr && build_archive_buffer(resolver, *package, output);
}

static auto build_foreign_buffer(Dynamic::Bytes& output) -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context library_context;
  if (resolver.load_source(
          library_context, "user/host/console.ttx"_view,
          "dialect : Library;\n"
          "public Console : foreign {\n"
          "  expose func print[.data : View[Bytes]] -> [];\n"
          "}\n"_view) == nullptr ||
      library_context.has_errors()) {
    return False;
  }

  Resolution::Resolver::Context package_context;
  resolver.set_package_name("User.Host"_view);
  Resolution::Source::Record* package = resolver.load_source(
      package_context, "user/host/package.ttx"_view,
      "dialect : Package;\n"
      "import Api : Library = \"console.ttx\";\n"
      "expose Console : alias = Api::Console;\n"_view);
  return package != nullptr && !package_context.has_errors() &&
         build_archive_buffer(resolver, *package, output);
}

static auto build_linkage_buffer(Dynamic::Bytes& output) -> Bool {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context library_context;
  if (resolver.load_source(
          library_context, "user/callable/api.ttx"_view,
          "dialect : Library;\n"
          "public func echo[.data : View[Bytes]] -> [] {\n"
          "  return;\n"
          "}\n"_view) == nullptr ||
      library_context.has_errors()) {
    return False;
  }

  Resolution::Resolver::Context package_context;
  resolver.set_package_name("User.Callable"_view);
  Resolution::Source::Record* package = resolver.load_source(
      package_context, "user/callable/package.ttx"_view,
      "dialect : Package;\n"
      "import Api : Library = \"api.ttx\";\n"
      "expose Api : alias = Api;\n"_view);
  return package != nullptr && !package_context.has_errors() &&
         build_archive_buffer(resolver, *package, output);
}

static auto register_core_buffer(
    Resolution::Resolver& resolver,
    View::Bytes core_buffer) -> Bool {
  Resolution::Resolver::Context buffer_context;
  return resolver.register_package_buffer(
             buffer_context, "user_core.puffer"_view, core_buffer) &&
         !buffer_context.has_errors();
}

static auto load_ui_package(Resolution::Resolver& resolver)
    -> Resolution::Source::Record* {
  Resolution::Resolver::Context package_context;
  resolver.set_package_name("User.Ui"_view);
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
  Resolution::Resolver resolver(isa_registry);
  if (!register_core_buffer(resolver, core_buffer)) {
    return False;
  }

  Resolution::Source::Record* package = load_ui_package(resolver);
  return package != nullptr && build_archive_buffer(resolver, *package, output);
}

static auto register_ui_buffer(
    Resolution::Resolver& resolver,
    View::Bytes ui_buffer) -> Bool {
  Resolution::Resolver::Context buffer_context;
  return resolver.register_package_buffer(
             buffer_context, "user_ui.puffer"_view, ui_buffer) &&
         !buffer_context.has_errors();
}

static auto load_app_package(Resolution::Resolver& resolver)
    -> Resolution::Source::Record* {
  Resolution::Resolver::Context package_context;
  resolver.set_package_name("User.App"_view);
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
  Resolution::Resolver resolver(isa_registry);
  if (!register_core_buffer(resolver, core_buffer) ||
      !register_ui_buffer(resolver, ui_buffer)) {
    return False;
  }

  Resolution::Source::Record* package = load_app_package(resolver);
  return package != nullptr && build_archive_buffer(resolver, *package, output);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, package_chain) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();

  Dynamic::Bytes core_buffer;
  Dynamic::Bytes ui_buffer;
  ASSERT(build_core_buffer(core_buffer));
  ASSERT(build_ui_buffer(core_buffer, ui_buffer));

  Resolution::Resolver consumer(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_ui.puffer"_view, ui_buffer));
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_core.puffer"_view, core_buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;
  const Resolution::Source::Record* root = consumer.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Ui : Package = User.Ui;\n"
      "import Core : Package = User.Core;\n"
      "private Value : alias = Ui::Core::Value;\n"_view);
  ASSERT(root != nullptr);
  EXPECT_NOT(root_source_context.has_errors());

  const Resolution::Source::Record* ui = consumer.resolve("User.Ui"_view);
  const Resolution::Source::Record* core = consumer.resolve("User.Core"_view);
  ASSERT(ui != nullptr);
  ASSERT(core != nullptr);

  const Ttx::Type* ui_core = ui->get_type().find_type("Core"_view);
  const Ttx::Type* core_value = core->get_type().find_type("Value"_view);
  ASSERT(ui_core != nullptr);
  ASSERT(core_value != nullptr);
  EXPECT(ui_core->find_type("Value"_view) == core_value);
  EXPECT_TEXT(
      ui_core->describe().get_view(), "User.Ui::Core alias of User.Core"_view);
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

PERIMORTEM_UNIT_TEST(PufferBuffer, import_views) {
  Dynamic::Bytes core_buffer;
  Dynamic::Bytes ui_buffer;
  Dynamic::Bytes app_buffer;
  ASSERT(build_core_buffer(core_buffer));
  ASSERT(build_ui_buffer(core_buffer, ui_buffer));
  ASSERT(build_app_buffer(core_buffer, ui_buffer, app_buffer));

  Allocator::Arena arena;
  Manifest archive = read_buffer_manifest(arena, app_buffer);
  ASSERT(archive.is_valid());

  View::Vector<Dependency> imports = archive.get_imports();
  ASSERT_EQ(imports.get_size(), Count(1));
  EXPECT_TEXT(imports[0].get_local_name(), "Ui"_view);
  EXPECT_TEXT(imports[0].get_source_name(), "User.Ui"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, standard_pkg) {
  auto math_buffer = File::read(
      ".bin/bin/tetrodotoxin/standard/Perimortem.Math/perimortem_math.puffer"_view);
  auto graphics_buffer = File::read(
      ".bin/bin/tetrodotoxin/standard/Perimortem.Graphics/perimortem_graphics.puffer"_view);
  ASSERT(!math_buffer.is_empty());
  ASSERT(!graphics_buffer.is_empty());

  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(resolver.register_package_buffer(
      dependency_context, "perimortem_math.puffer"_view, math_buffer));
  ASSERT(resolver.register_package_buffer(
      dependency_context, "perimortem_graphics.puffer"_view, graphics_buffer));
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
  EXPECT(has_import(
      graphics_package->get_manifest().get_imports(), "Perimortem.Math"_view));

  const Ttx::Type* graphics_type = &graphics->get_type();
  ASSERT(graphics_type->find_type("Color"_view) != nullptr);
  ASSERT(graphics_type->find_type("Render2D"_view) != nullptr);
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
  EXPECT_TEXT(terminals[0].get_path(), "root.a"_view);
  EXPECT_NOT(terminals[0].get_content().is_empty());
  EXPECT_TEXT(terminals[1].get_group(), "header"_view);
  EXPECT_TEXT(terminals[1].get_path(), "root.hpp"_view);
  EXPECT_NOT(terminals[1].get_content().is_empty());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, round_trip) {
  static constexpr View::Bytes round_trip_path =
      ".bin/bin/validation/puffer_round_trip.puffer"_view;

  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Resolution::Resolver resolver(isa_registry);
  Resolution::Source::Record* package = load_core_package(resolver);
  ASSERT(package != nullptr);

  Dynamic::Bytes buffer;
  ASSERT(build_archive_buffer(resolver, *package, buffer));
  ASSERT(File::write(buffer, round_trip_path));

  auto input = File::read(round_trip_path);
  ASSERT(!input.is_empty());
  Allocator::Arena archive_arena;
  const Tetrodotoxin::Archiver::Package* archive =
      read_buffer_package(archive_arena, input);
  ASSERT(archive != nullptr);

  EXPECT(type_trees_match(package->get_type(), archive->get_type()));
  EXPECT_TEXT(archive->get_manifest().get_name(), "User.Core"_view);
  File::remove(round_trip_path);
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
      "  Host::Console->print(\"Hello\");\n"
      "}\n"_view);
  ASSERT(source != nullptr);
  EXPECT_NOT(source_context.has_errors());

  const Ttx::Function* main = source->get_type().find_function("main"_view);
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
  const Ttx::Function* print = console->find_function("print"_view);
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
      "  Callable::Api->echo(\"Hello\");\n"
      "}\n"_view);
  ASSERT(source != nullptr);
  EXPECT_NOT(source_context.has_errors());

  const Ttx::Function* main = source->get_type().find_function("main"_view);
  ASSERT(main != nullptr);
  const auto* body = source->get_implementation()
                         .find<Tetrodotoxin::Compiler::Execution::Body>(*main);
  ASSERT(body != nullptr);
  ASSERT_EQ(body->count<Tetrodotoxin::Compiler::Execution::Call>(), Count(1));
  EXPECT_TEXT(
      body->find<Tetrodotoxin::Compiler::Execution::Call>()->get_symbol(),
      "TTX_api_echo"_view);

  const Resolution::Source::Record* restored =
      resolver.resolve("User.Callable"_view);
  ASSERT(restored != nullptr);
  const Ttx::Type* api = restored->get_type().find_type("Api"_view);
  ASSERT(api != nullptr);
  const Ttx::Function* echo = api->find_function("echo"_view);
  ASSERT(echo != nullptr);
  const auto* linkage = restored->get_implementation().find_linkage(*echo);
  ASSERT(linkage != nullptr);
  EXPECT_TEXT(linkage->get_symbol(), "TTX_api_echo"_view);
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
  ASSERT(build_core_buffer(actual_core_buffer, "Bits_64"_view));

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

PERIMORTEM_UNIT_TEST(PufferBuffer, missing_dep) {
  Tetrodotoxin::Isa::Registry isa_registry =
      Tetrodotoxin::Puffer::Toolchain::standard_registry();
  Dynamic::Bytes core_buffer;
  Dynamic::Bytes ui_buffer;
  ASSERT(build_core_buffer(core_buffer));
  ASSERT(build_ui_buffer(core_buffer, ui_buffer));

  Resolution::Resolver consumer(isa_registry);
  Resolution::Resolver::Context dependency_context;
  ASSERT(consumer.register_package_buffer(
      dependency_context, "user_ui.puffer"_view, ui_buffer));
  EXPECT_NOT(dependency_context.has_errors());

  Resolution::Resolver::Context root_source_context;
  EXPECT_NOT(consumer.load_source(
      root_source_context, "unit/root.ttx"_view,
      "dialect : Library;\n"
      "import Ui : Package = User.Ui;\n"_view));
  ASSERT(root_source_context.has_errors());
  EXPECT_TEXT(
      root_source_context.get_errors()[0].get_message(),
      "Puffer Buffer dependency is not registered: User.Core"_view);
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
  ASSERT(build_core_buffer(actual_core_buffer, "Bits_64"_view));

  Allocator::Arena archive_arena;
  Manifest expected_archive =
      read_buffer_manifest(archive_arena, expected_core_buffer);
  Manifest actual_archive =
      read_buffer_manifest(archive_arena, actual_core_buffer);
  ASSERT(expected_archive.get_version().is_set());
  ASSERT(actual_archive.get_version().is_set());
  EXPECT(expected_archive.get_version() != actual_archive.get_version());

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

PERIMORTEM_UNIT_TEST(PufferBuffer, pure_version) {
  Dynamic::Bytes first_buffer;
  Dynamic::Bytes second_buffer;
  ASSERT(build_core_buffer(first_buffer));
  ASSERT(build_core_buffer(second_buffer));

  Allocator::Arena archive_arena;
  Manifest first_archive = read_buffer_manifest(archive_arena, first_buffer);
  Manifest second_archive = read_buffer_manifest(archive_arena, second_buffer);

  EXPECT(first_buffer == second_buffer);
  EXPECT(first_archive.get_version() == second_archive.get_version());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, core_name) {
  Dynamic::Bytes core_buffer;
  ASSERT(build_core_buffer(core_buffer));

  Allocator::Arena archive_arena;
  Manifest archive = read_buffer_manifest(archive_arena, core_buffer);

  EXPECT_TEXT(archive.get_name(), "User.Core"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, package_version) {
  Dynamic::Bytes core_buffer;
  ASSERT(build_core_buffer(core_buffer));

  Allocator::Arena archive_arena;
  const Tetrodotoxin::Archiver::Package* archive =
      read_buffer_package(archive_arena, core_buffer);
  ASSERT(archive != nullptr);
  Version version = archive->get_manifest().get_version();

  EXPECT(version.is_set());
  EXPECT(archive->get_manifest().get_version() == version);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, type_table) {
  Dynamic::Bytes core_buffer;
  ASSERT(build_core_buffer(core_buffer));

  Allocator::Arena archive_arena;
  const Tetrodotoxin::Archiver::Package* archive =
      read_buffer_package(archive_arena, core_buffer);
  ASSERT(archive != nullptr);
  const Ttx::Type* restored = &archive->get_type();
  View::Vector<const Ttx::Type*> types = archive->get_types();

  ASSERT(restored != nullptr);
  ASSERT(types.get_size() >= Count(2));
  EXPECT(types[0] == restored);
  const Ttx::Type* value = restored->find_type("Value"_view);
  Bool found_value = False;
  for (Count i = 0; i < types.get_size(); i++) {
    found_value = found_value || types[i] == value;
  }

  EXPECT(found_value);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, reference_identity) {
  constexpr Version a_version(1, 1);
  constexpr Version b_version(2, 2);
  constexpr Version c_version(3, 3);
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
  Tetrodotoxin::Archiver::Package source(
      Manifest("User.Root"_view, Version(), imports), root,
      View::Vector<const Ttx::Type*>(), View::Vector<Terminal>());
  Static::Vector<Reference, 3> write_references = {{
    Reference(a_package),
    Reference(c_package),
    Reference(b_package),
  }};

  Allocator::Arena writer_arena;
  View::Bytes output = Tetrodotoxin::Archiver::Writer::write(
      writer_arena, source, write_references);
  ASSERT(!output.is_empty());

  Static::Vector<Reference, 3> read_references = {{
    Reference(b_package),
    Reference(a_package),
    Reference(c_package),
  }};
  Allocator::Arena reader_arena;
  Tetrodotoxin::Archiver::Reader reader(output);
  Manifest manifest = reader.read_manifest(reader_arena);
  const Tetrodotoxin::Archiver::Package* restored =
      reader.read_package(reader_arena, manifest, read_references);
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
  Managed::Vector<const Ttx::Type*> nested(writer_arena);
  nested.reset(256);
  for (Count i = 0; i < 256; i++) {
    nested.insert(&writer_arena.construct<Ttx::Type>("Item"_view));
  }

  Ttx::Type root("Root"_view, View::Vector<Ttx::Member>(), nested.get_view());
  View::Bytes output =
      write_test_package(writer_arena, "User.Stress"_view, root);
  ASSERT(!output.is_empty());

  Tetrodotoxin::Archiver::Reader reader(output);
  Allocator::Arena restore_arena;
  for (Count i = 0; i < 64; i++) {
    restore_arena.reset();
    Manifest manifest = reader.read_manifest(restore_arena);
    ASSERT(manifest.is_valid());

    const Tetrodotoxin::Archiver::Package* package =
        reader.read_package(restore_arena, manifest, View::Vector<Reference>());
    ASSERT(package != nullptr);
    ASSERT_EQ(package->get_types().get_size(), Count(257));
    EXPECT(package->get_types()[0] == &package->get_type());
  }
}

PERIMORTEM_UNIT_TEST(PufferBuffer, ui_imports) {
  Dynamic::Bytes core_buffer;
  Dynamic::Bytes ui_buffer;
  ASSERT(build_core_buffer(core_buffer));
  ASSERT(build_ui_buffer(core_buffer, ui_buffer));

  Allocator::Arena archive_arena;
  Manifest archive = read_buffer_manifest(archive_arena, ui_buffer);
  View::Vector<Dependency> imports = archive.get_imports();

  ASSERT(archive.is_valid());
  ASSERT_EQ(imports.get_size(), Count(1));
  EXPECT_TEXT(imports[0].get_local_name(), "Core"_view);
  EXPECT_TEXT(imports[0].get_source_name(), "User.Core"_view);
  EXPECT(imports[0].get_version().is_set());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, restore_alias) {
  Dynamic::Bytes core_buffer;
  ASSERT(build_core_buffer(core_buffer));

  Allocator::Arena archive_arena;
  const Tetrodotoxin::Archiver::Package* archive =
      read_buffer_package(archive_arena, core_buffer);
  ASSERT(archive != nullptr);
  const Ttx::Type* restored = &archive->get_type();

  ASSERT(restored != nullptr);
  const Ttx::Type* value = restored->find_type("Value"_view);
  ASSERT(value != nullptr);
  EXPECT(value->is_alias());
  EXPECT_TEXT(
      value->describe().get_view(), "User.Core::Value alias of Bits_32"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, standard_imports) {
  auto graphics_buffer = File::read(
      ".bin/bin/tetrodotoxin/standard/Perimortem.Graphics/perimortem_graphics.puffer"_view);
  ASSERT(!graphics_buffer.is_empty());

  Allocator::Arena archive_arena;
  Manifest archive = read_buffer_manifest(archive_arena, graphics_buffer);
  View::Vector<Dependency> imports = archive.get_imports();

  ASSERT(archive.is_valid());
  EXPECT(has_import(imports, "Perimortem.Math"_view));
}

PERIMORTEM_UNIT_TEST(PufferBuffer, package_header) {
  Allocator::Arena writer_arena;
  Ttx::Type root("Root"_view);
  View::Bytes output =
      write_test_package(writer_arena, "User.Header"_view, root);

  ASSERT(!output.is_empty());
  Allocator::Arena archive_arena;
  Manifest archive = read_buffer_manifest(archive_arena, output);
  EXPECT_TEXT(archive.get_name(), "User.Header"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, table_reads) {
  Allocator::Arena writer_arena;
  Ttx::Type root("Root"_view);
  View::Bytes output =
      write_test_package(writer_arena, "User.Continuation"_view, root);
  ASSERT(!output.is_empty());

  Allocator::Arena reader_arena;
  Tetrodotoxin::Archiver::Reader reader(output);
  Manifest first = reader.read_manifest(reader_arena);
  ASSERT(first.is_valid());

  const Tetrodotoxin::Archiver::Package* package =
      reader.read_package(reader_arena, first, View::Vector<Reference>());
  ASSERT(package != nullptr);
  EXPECT_TEXT(package->get_manifest().get_name(), "User.Continuation"_view);

  Manifest second = reader.read_manifest(reader_arena);
  ASSERT(second.is_valid());
  EXPECT_TEXT(second.get_name(), first.get_name());

  const Tetrodotoxin::Archiver::Package* repeated =
      reader.read_package(reader_arena, second, View::Vector<Reference>());
  ASSERT(repeated != nullptr);
  EXPECT_TEXT(repeated->get_manifest().get_name(), first.get_name());
}

PERIMORTEM_UNIT_TEST(PufferBuffer, table_bounds) {
  Allocator::Arena writer_arena;
  Ttx::Type root("Root"_view);
  Dynamic::Bytes output =
      write_test_package(writer_arena, "User.Bounds"_view, root);
  ASSERT(!output.is_empty());

  Bits_64 invalid_offset =
      Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Little>(
          Bits_64(output.get_size()));
  Data::copy(
      output.get_access().get_data() + Format::slot(Format::Table::Manifest),
      invalid_offset);

  Allocator::Arena reader_arena;
  Manifest manifest =
      Tetrodotoxin::Archiver::Reader(output).read_manifest(reader_arena);
  EXPECT_NOT(manifest.is_valid());
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
  Manifest second_manifest =
      Tetrodotoxin::Archiver::Reader(second).read_manifest(reader_arena);
  ASSERT(second_manifest.is_valid());

  EXPECT(
      first_reader.read_package(
          reader_arena, second_manifest, View::Vector<Reference>()) == nullptr);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, terminal_bytes) {
  Allocator::Arena writer_arena;
  Ttx::Type root("Root"_view);
  Static::Vector<Terminal, 1> terminals = {{
    Terminal("linker"_view, "root.a"_view, "archive bytes"_view),
  }};
  View::Bytes output =
      write_test_package(writer_arena, "User.Terminal"_view, root, terminals);

  ASSERT(!output.is_empty());
  Allocator::Arena archive_arena;
  const Tetrodotoxin::Archiver::Package* archive =
      read_buffer_package(archive_arena, output);
  ASSERT(archive != nullptr);
  EXPECT_TEXT(
      archive->find_terminal("linker"_view, "root.a"_view),
      "archive bytes"_view);
}

PERIMORTEM_UNIT_TEST(PufferBuffer, package_bad) {
  Allocator::Arena archive_arena;
  Manifest archive =
      read_buffer_manifest(archive_arena, "not a puffer buffer"_view);

  EXPECT(archive.get_name().is_empty());
}
