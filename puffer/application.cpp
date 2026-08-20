// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/application.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/llvm/program.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
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

auto Puffer::Application::run() const -> Signed_32 {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::console_sink);
  Core::Diagnostics::Log::set_disable_header(True);

  Core::View::Bytes complete_path = value(arguments, "complete"_view);
  Core::View::Bytes app_member = value(arguments, "app-member"_view);
  Core::View::Bytes artifact = value(arguments, "artifact"_view);
  Core::View::Bytes ir_path = value(arguments, "ir"_view);
  Core::View::Bytes object_path = value(arguments, "object"_view);
  if (complete_path.is_empty() || app_member.is_empty() || ir_path.is_empty() ||
      object_path.is_empty() || ir_path == object_path ||
      artifact != "x86_64-sysv-linux"_view) {
    Core::Diagnostics::Log::error(
        "Puffer Application mode received an incomplete request."_view);
    return 2;
  }

  Memory::Allocator::Arena arena;
  Environment::Toolchain toolchain;
  if (!toolchain.install<Package::Dialect>("Package"_view) ||
      !toolchain.install<Library::Dialect>("Library"_view) ||
      !toolchain.install<App::Dialect>("App"_view)) {
    return 1;
  }
  Environment::Workspace workspace(toolchain);

  Memory::Dynamic::Vector<Memory::Dynamic::Bytes> dependency_bytes;
  for (Core::View::Bytes dependency_path : values(arguments, "dep"_view)) {
    auto bytes = System::File::read(dependency_path);
    if (!bytes) {
      return 1;
    }

    dependency_bytes.emplace(static_cast<Memory::Dynamic::Bytes&&>(*bytes));
    auto archive =
        decode(arena, dependency_bytes[dependency_bytes.get_size() - 1]);
    if (!archive ||
        archive->get_profile() != Language::Persistence::Profile::Interface ||
        !workspace.restore_package(*archive, archive->get_identity())) {
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

  Memory::Managed::Bytes route(arena, app->get_program().get_route());
  route.concat("::"_view);
  route.concat(app->get_program().get_callable_name());
  route.concat("[static]"_view);
  Core::Option<Core::View::Bytes> entry_symbol;
  for (const Package::Archive::Export& exported : root_archive->get_exports()) {
    if (exported.get_semantic_route() != route.get_view() ||
        exported.get_artifact_id() != artifact) {
      continue;
    }

    if (entry_symbol) {
      Core::Diagnostics::Log::error(
          "The App entry has more than one native export."_view);
      return 1;
    }
    entry_symbol = exported.get_symbol_locator();
  }
  if (!entry_symbol) {
    Core::Diagnostics::Log::error(
        "The App entry has no matching native Package export."_view);
    return 1;
  }

  Ttx::Lexical::Errors errors;
  Library::Llvm::Program target(
      arena, errors, "<app-entry>"_view, {}, Library::Llvm::Target::X86_64SysV,
      Library::Llvm::Debug::Level::None, Library::Llvm::Unit());
  if (!target.initialize() ||
      !app->get_program().lower(target, *entry_symbol)) {
    return 1;
  }

  return target.compile().visit(
      [&](const Library::Llvm::Products& products) -> Signed_32 {
        return publish(ir_path, products.get_llvm_ir()) &&
                       publish(object_path, products.get_object())
                   ? 0
                   : 1;
      },
      [](const Library::Llvm::Failure&) -> Signed_32 { return 1; });
}
