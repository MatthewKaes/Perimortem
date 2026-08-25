// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/terminal/abi/compiler.hpp"
#include "tetrodotoxin/terminal/llvm/compiler.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness AppDialect = {
  .name = "Tetrodotoxin::App::Dialect"_view,
};

static auto install(Environment::Toolchain& toolchain) -> Bool {
  return toolchain.install<Package::Dialect>("Package"_view) &&
         toolchain.install<Library::Dialect>("Library"_view) &&
         toolchain.install<App::Dialect>("App"_view);
}

static auto interpret_app(
    Environment::Workspace& workspace,
    Errors& errors,
    View::Bytes library_source,
    View::Bytes app_source) -> Option<Tetrodotoxin::Language::Monograph&> {
  BAIL_IF(!workspace.interpret_source(
      errors, "Main"_view, "main.ttx"_view, library_source));
  return workspace.interpret_source(
      errors, "Application"_view, "app.ttx"_view, app_source);
}

static auto decode_archive(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes bytes) -> Option<Package::Archive::Archive> {
  return Package::Archive::Reader::read(arena, bytes)
      .visit(
          [](const Package::Archive::Archive& selected)
              -> Option<Package::Archive::Archive> { return selected; },
          [](const Package::Archive::Reader::Error&)
              -> Option<Package::Archive::Archive> { return {}; });
}

PERIMORTEM_UNIT_TEST(AppDialect, selects_echo_entry) {
  Environment::Toolchain toolchain;
  ASSERT(install(toolchain));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto memory_product = File::read(
      ".bin/bin/packages/ttx/Perimortem.Memory/1.0/contract.txa"_view);
  auto system_product = File::read(
      ".bin/bin/packages/ttx/Perimortem.System/1.0/contract.txa"_view);
  auto echo_product = File::read(
      ".bin/bin/apps/ttx/echo/Perimortem.Echo/1.0/complete.txa"_view);
  ASSERT(memory_product && system_product && echo_product);

  Perimortem::Memory::Allocator::Arena memory_product_arena;
  Perimortem::Memory::Allocator::Arena system_product_arena;
  Perimortem::Memory::Allocator::Arena echo_product_arena;
  auto memory_archive = decode_archive(memory_product_arena, *memory_product);
  auto system_archive = decode_archive(system_product_arena, *system_product);
  auto echo_archive = decode_archive(echo_product_arena, *echo_product);
  ASSERT(memory_archive && system_archive && echo_archive);

  auto memory_imported =
      workspace.restore_package(*memory_archive, "Memory"_view);
  ASSERT(memory_imported);
  auto system_imported =
      workspace.restore_package(*system_archive, "System"_view);
  ASSERT(system_imported);
  auto imported = workspace.restore_package(*echo_archive, "Echo"_view);

  ASSERT(imported && imported->is<Package::Language::Monograph>());
  auto& package = static_cast<Package::Language::Monograph&>(*imported);
  const auto& app = package.resolve_context("App"_view).resolve();
  ASSERT(app.is<App::Language::Monograph>());
  const auto& policy = static_cast<const App::Language::Monograph&>(app);
  auto program = policy.get_program();
  ASSERT(program);
  auto entry = program->get_entry();
  ASSERT(entry);
  EXPECT_TEXT(entry->get_name(), "run"_view);
  EXPECT(entry->get_parameters().is_empty());
  EXPECT(entry->get_results().is_empty());
  EXPECT(errors.is_empty());

  const auto& main = package.resolve_context("Main"_view).resolve();
  ASSERT(main.is<Tetrodotoxin::Library::Language::Monograph>());
  Library::Dialect library_archive_dialect;
  auto main_complete = library_archive_dialect.encode(
      main, Language::Persistence::Profile::Complete);
  auto main_interface = library_archive_dialect.encode(
      main, Language::Persistence::Profile::Contract);
  ASSERT(main_complete && main_interface);

  auto& memory_package =
      static_cast<Package::Language::Monograph&>(*memory_imported);
  const auto& memory = memory_package.resolve_context("Dynamic"_view).resolve();
  auto memory_interface = library_archive_dialect.encode(
      memory, Language::Persistence::Profile::Contract);
  ASSERT(memory_interface);
  EXPECT_EQ(
      Perimortem::Core::Algorithm::search(*memory_interface, "storage"_view),
      Count(-1));
  EXPECT(
      Perimortem::Core::Algorithm::search(
          *memory_interface, "get_capacity"_view) != Count(-1));
  Perimortem::Memory::Allocator::Arena memory_restore_arena;
  auto restored_memory = library_archive_dialect.restore(
      memory_restore_arena, *memory_interface,
      Language::Persistence::Profile::Contract,
      Ttx::Concept::Documentation::get_empty(), memory_package);
  ASSERT(restored_memory);
  ASSERT(restored_memory->link_restored());
  ASSERT(restored_memory->finalize_restored());
  const auto& restored_bytes =
      restored_memory->resolve_context("Bytes"_view).resolve();
  ASSERT(restored_bytes.is<Ttx::Model::Type>());
  EXPECT_EQ(
      static_cast<const Ttx::Model::Type&>(restored_bytes)
          .get_layout()
          .get_size(),
      Count(2));
  EXPECT(restored_bytes.resolve_access(restored_bytes, "storage"_view)
             .resolve()
             .is<Ttx::Concept::Invalid>());

  auto& system_package =
      static_cast<Package::Language::Monograph&>(*system_imported);
  const auto& system =
      system_package.resolve_context("Terminal"_view).resolve();
  auto system_interface = library_archive_dialect.encode(
      system, Language::Persistence::Profile::Contract);
  ASSERT(system_interface);
  Perimortem::Memory::Allocator::Arena system_restore_arena;
  auto restored_system = library_archive_dialect.restore(
      system_restore_arena, *system_interface,
      Language::Persistence::Profile::Contract,
      Ttx::Concept::Documentation::get_empty(), system_package);
  ASSERT(restored_system);
  ASSERT(restored_system->link_restored());
  ASSERT(restored_system->finalize_restored());

  Perimortem::Memory::Allocator::Arena main_restore_arena;
  auto restored_main = library_archive_dialect.restore(
      main_restore_arena, *main_complete,
      Language::Persistence::Profile::Complete,
      Ttx::Concept::Documentation::get_empty(), package);
  ASSERT(
      restored_main && restored_main->link_restored() &&
      restored_main->finalize_restored());
  const auto& restored_main_root =
      static_cast<const Tetrodotoxin::Library::Language::Monograph&>(
          *restored_main);
  const auto& restored_run =
      restored_main_root.resolve_context("run"_view).resolve();
  ASSERT(restored_run.is<Ttx::Model::Callable>());

  const auto& restored_memory_package =
      workspace.resolve_context("Memory"_view).resolve();
  const auto& restored_dynamic =
      restored_memory_package.resolve_context("Dynamic"_view).resolve();
  auto restored_dynamic_library =
      restored_dynamic.select<Library::Language::Monograph>();
  ASSERT(restored_dynamic_library);
  auto restored_bytes_type =
      restored_dynamic_library->resolve_context("Bytes"_view)
          .resolve()
          .select<Library::Language::Types::Structure>();
  ASSERT(restored_bytes_type);
  EXPECT(restored_bytes_type
             ->resolve_type_call(
                 *restored_bytes_type, "$construct"_view,
                 Library::Language::Model::Type::Access::Static)
             .resolve()
             .is<Ttx::Concept::Invalid>());

  static constexpr View::Bytes consumer_source =
      "// Contract construction consumer.\n"
      "dialect : Library;\n"
      "using Memory;\n"
      "public construct : func = [] -> [] {\n"
      "  state value : Dynamic::Bytes;\n"
      "}\n"_view;
  Errors consumer_errors;
  auto consumer = workspace.interpret_source(
      consumer_errors, "Consumer"_view, "consumer.ttx"_view, consumer_source);
  ASSERT(consumer && consumer->is<Library::Language::Monograph>());
  static constexpr View::Bytes construction_symbol =
      "TTX_FUNC_Perimortem_2eMemory__Dynamic__Bytes__construct_static"_view;
  Terminal::Abi::Unit::Binding construction_binding(
      *restored_bytes_type, construction_symbol);
  Static::Vector<Terminal::Abi::Unit::Binding, 1> bindings = {{
    construction_binding,
  }};
  Terminal::Abi::Unit consumer_unit(
      "Consumer"_view, "Main"_view, "x86_64-sysv-linux"_view,
      bindings.get_view());
  Perimortem::Memory::Allocator::Arena consumer_products;
  const auto& consumer_library =
      static_cast<const Library::Language::Monograph&>(*consumer);
  Terminal::Abi::Compiler interface_compiler;
  auto native_interface = interface_compiler.compile(
      consumer_products, consumer_library, consumer_unit, consumer_errors,
      "consumer.ttx"_view, consumer_source);
  ASSERT(native_interface);
  Llvm::Request consumer_request(
      consumer_library, consumer_errors, "consumer.ttx"_view, consumer_source,
      Llvm::Target::X86_64SysV, Llvm::Module::Debug::Level::None, consumer_unit,
      *native_interface);
  Llvm::Compiler consumer_compiler;
  auto compiled_consumer =
      consumer_compiler.compile(consumer_products, consumer_request);
  Option<Llvm::Products> selected_consumer;
  compiled_consumer.visit(
      [&](const Llvm::Products& products) { selected_consumer = products; },
      [](const Llvm::Failure&) {});
  ASSERT(selected_consumer);
  EXPECT(
      Perimortem::Core::Algorithm::search(
          selected_consumer->get_llvm_ir(), construction_symbol) != Count(-1));

  App::Dialect archive_dialect;
  auto complete =
      archive_dialect.encode(policy, Language::Persistence::Profile::Complete);
  auto complete_again =
      archive_dialect.encode(policy, Language::Persistence::Profile::Complete);
  auto contract =
      archive_dialect.encode(policy, Language::Persistence::Profile::Contract);
  ASSERT(complete && complete_again && contract);
  EXPECT(*complete == *complete_again);
  EXPECT(!(*complete == *contract));

  Perimortem::Memory::Dynamic::Bytes malformed(*complete);
  auto format = malformed.get_access()[4];
  ASSERT(format);
  *format = 2;
  Perimortem::Memory::Allocator::Arena malformed_arena;
  EXPECT_NOT(archive_dialect.restore(
      malformed_arena, malformed, Language::Persistence::Profile::Complete,
      Ttx::Concept::Documentation::get_empty(), package));

  Perimortem::Memory::Allocator::Arena disagreement_arena;
  EXPECT_NOT(archive_dialect.restore(
      disagreement_arena, *complete, Language::Persistence::Profile::Contract,
      Ttx::Concept::Documentation::get_empty(), package));

  Perimortem::Memory::Allocator::Arena complete_arena;
  auto restored_complete = archive_dialect.restore(
      complete_arena, *complete, Language::Persistence::Profile::Complete,
      Ttx::Concept::Documentation::get_empty(), package);
  ASSERT(
      restored_complete && restored_complete->link_restored() &&
      restored_complete->finalize_restored());
  const auto& complete_policy =
      static_cast<const App::Language::Monograph&>(*restored_complete);
  auto complete_program = complete_policy.get_program();
  ASSERT(complete_program && complete_program->get_entry());
  EXPECT_TEXT(complete_program->get_entry()->get_name(), "run"_view);

  Perimortem::Memory::Allocator::Arena contract_arena;
  auto restored_contract = archive_dialect.restore(
      contract_arena, *contract, Language::Persistence::Profile::Contract,
      Ttx::Concept::Documentation::get_empty(), package);
  ASSERT(
      restored_contract && restored_contract->link_restored() &&
      restored_contract->finalize_restored());
}

PERIMORTEM_UNIT_TEST(AppDialect, unresolved_entry) {
  static constexpr View::Bytes source =
      "// Broken App.\n"
      "dialect : App;\n"
      "runtime = Terminal {}\n"
      "lifecycle = Program { start Missing -> run, }\n"_view;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<App::Dialect>("App"_view));
  Environment::Workspace workspace(toolchain);
  Errors errors;
  auto interpreted = workspace.interpret_source(
      errors, "BrokenApp"_view, "broken-app.ttx"_view, source);

  EXPECT_NOT(interpreted);
  EXPECT_EQ(errors.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(AppDialect, exact_policy) {
  static constexpr View::Bytes missing_runtime =
      "// App.\n"
      "dialect : App;\n"
      "lifecycle = Program { start Main -> run, }\n"_view;
  static constexpr View::Bytes duplicate_program =
      "// App.\n"
      "dialect : App;\n"
      "runtime = Terminal {}\n"
      "lifecycle = Program { start Main -> run, }\n"
      "lifecycle = Program { start Main -> run, }\n"_view;
  static constexpr View::Bytes library =
      "// Library.\n"
      "dialect : Library;\n"
      "public run : func = [] -> [] : return;\n"_view;

  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Library::Dialect>("Library"_view));
  ASSERT(toolchain.install<App::Dialect>("App"_view));
  Environment::Workspace missing_workspace(toolchain);
  Errors missing_errors;
  EXPECT_NOT(interpret_app(
      missing_workspace, missing_errors, library, missing_runtime));
  EXPECT_NOT(missing_errors.is_empty());

  Environment::Workspace duplicate_workspace(toolchain);
  Errors duplicate_errors;
  EXPECT_NOT(interpret_app(
      duplicate_workspace, duplicate_errors, library, duplicate_program));
  EXPECT_NOT(duplicate_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AppDialect, invalid_entries) {
  static constexpr View::Bytes app =
      "// App.\n"
      "dialect : App;\n"
      "runtime = Terminal {}\n"
      "lifecycle = Program { start Main::Worker -> run, }\n"_view;
  static constexpr View::Bytes self_entry =
      "// Library.\n"
      "dialect : Library;\n"
      "public Worker : struct {\n"
      "  public run : func = [self] -> [] : return;\n"
      "}\n"_view;
  static constexpr View::Bytes parameter_entry =
      "// Library.\n"
      "dialect : Library;\n"
      "public Worker : struct {\n"
      "  public run : func = [.value : U64] -> [] : return;\n"
      "}\n"_view;
  static constexpr View::Bytes result_entry =
      "// Library.\n"
      "dialect : Library;\n"
      "public Worker : struct {\n"
      "  public run : func = [] -> U64 : return 0;\n"
      "}\n"_view;

  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Library::Dialect>("Library"_view));
  ASSERT(toolchain.install<App::Dialect>("App"_view));
  Environment::Workspace self_workspace(toolchain);
  Errors self_errors;
  EXPECT_NOT(interpret_app(self_workspace, self_errors, self_entry, app));
  EXPECT_NOT(self_errors.is_empty());

  Environment::Workspace parameter_workspace(toolchain);
  Errors parameter_errors;
  EXPECT_NOT(interpret_app(
      parameter_workspace, parameter_errors, parameter_entry, app));
  EXPECT_NOT(parameter_errors.is_empty());

  Environment::Workspace result_workspace(toolchain);
  Errors result_errors;
  EXPECT_NOT(interpret_app(result_workspace, result_errors, result_entry, app));
  EXPECT_NOT(result_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AppDialect, startup_profiles) {
  static constexpr View::Bytes library_source =
      "// Library.\n"
      "dialect : Library;\n"
      "public run : func = [] -> [] : return;"_view;
  static constexpr View::Bytes headless_source =
      "// Headless App.\n"
      "dialect : App;\n"
      "runtime = Headless;\n"
      "lifecycle = Program { start Main -> run, }"_view;
  Environment::Toolchain direct_toolchain;
  ASSERT(direct_toolchain.install<Library::Dialect>("Library"_view));
  ASSERT(direct_toolchain.install<App::Dialect>("App"_view));
  Environment::Workspace direct_workspace(direct_toolchain);
  Errors direct_errors;
  auto headless = interpret_app(
      direct_workspace, direct_errors, library_source, headless_source);
  ASSERT(headless && headless->is<App::Language::Monograph>());
  const auto& headless_policy =
      static_cast<const App::Language::Monograph&>(*headless);
  EXPECT(
      headless_policy.get_runtime().get_profile() ==
      App::Language::Runtime::Profile::Headless);
  EXPECT_NOT(headless_policy.get_runtime().get_windowed());
  EXPECT(direct_errors.is_empty());

  Environment::Toolchain package_toolchain;
  ASSERT(install(package_toolchain));
  Environment::Workspace package_workspace(package_toolchain);
  Errors package_errors;
  auto imported = package_workspace.import_package(
      package_errors, "validation/data/ttx/app_profiles"_view,
      "ProfilePackage"_view, "package.ttx"_view, "Validation.AppProfiles"_view,
      Version(1, 0));
  ASSERT(imported && imported->is<Package::Language::Monograph>());
  auto& package = static_cast<Package::Language::Monograph&>(*imported);
  const auto& app = package.resolve_context("Application"_view).resolve();
  ASSERT(app.is<App::Language::Monograph>());
  const auto& policy = static_cast<const App::Language::Monograph&>(app);
  const auto& runtime = policy.get_runtime();
  EXPECT(runtime.get_profile() == App::Language::Runtime::Profile::Windowed);
  auto windowed = runtime.get_windowed();
  ASSERT(windowed);
  ASSERT(windowed->get_title());
  EXPECT_TEXT(*windowed->get_title(), "Profile Test"_view);
  ASSERT(windowed->get_icon_route());
  EXPECT_TEXT(*windowed->get_icon_route(), "$[resources/icon.bin]"_view);
  ASSERT(windowed->get_icon());
  EXPECT_TEXT(windowed->get_icon()->get_value(), "icon\n"_view);
  ASSERT(windowed->get_width());
  ASSERT(windowed->get_height());
  ASSERT(windowed->get_resizable());
  EXPECT_EQ(*windowed->get_width(), U32(800));
  EXPECT_EQ(*windowed->get_height(), U32(600));
  EXPECT(*windowed->get_resizable());
  EXPECT(package_errors.is_empty());

  App::Dialect archive_dialect;
  auto archive =
      archive_dialect.encode(policy, Language::Persistence::Profile::Complete);
  ASSERT(archive);
  Perimortem::Memory::Allocator::Arena restored_arena;
  auto restored = archive_dialect.restore(
      restored_arena, *archive, Language::Persistence::Profile::Complete,
      Ttx::Concept::Documentation::get_empty(), package);
  ASSERT(restored && restored->is<App::Language::Monograph>());
  const auto& restored_policy =
      static_cast<const App::Language::Monograph&>(*restored);
  auto restored_window = restored_policy.get_runtime().get_windowed();
  ASSERT(restored_window && restored_window->get_icon());
  EXPECT_TEXT(restored_window->get_icon()->get_value(), "icon\n"_view);
}

PERIMORTEM_UNIT_TEST(AppDialect, startup_rejections) {
  static constexpr View::Bytes library_source =
      "// Library.\n"
      "dialect : Library;\n"
      "public run : func = [] -> [] : return;"_view;
  static constexpr View::Bytes invalid_source =
      "// Invalid App.\n"
      "dialect : App;\n"
      "runtime = Terminal { .width = 800, }\n"
      "lifecycle = Program { start Main -> run, }"_view;
  static constexpr View::Bytes unterminated_title =
      "// Invalid App.\n"
      "dialect : App;\n"
      "runtime = Windowed { .title = \"unfinished\n"
      "lifecycle = Program { start Main -> run, }"_view;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Library::Dialect>("Library"_view));
  ASSERT(toolchain.install<App::Dialect>("App"_view));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  EXPECT_NOT(interpret_app(workspace, errors, library_source, invalid_source));
  auto retained = workspace.get_monograph("app.ttx"_view);
  ASSERT(retained && retained->is<App::Language::Monograph>());
  EXPECT_NOT(errors.is_empty());

  Environment::Workspace malformed_workspace(toolchain);
  Errors malformed_errors;
  EXPECT_NOT(interpret_app(
      malformed_workspace, malformed_errors, library_source,
      unterminated_title));
  EXPECT_NOT(malformed_errors.is_empty());
}
