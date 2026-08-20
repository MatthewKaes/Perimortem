// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/semantic_workspace.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/version.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Puffer;
using namespace Tetrodotoxin;

static constexpr Version package_version(1, 0);

Lsp::SemanticWorkspace::SemanticWorkspace(
    Mode mode,
    Perimortem::Memory::Dynamic::Record<Package::Snapshots> snapshots,
    View::Bytes source_name,
    View::Bytes source,
    View::Bytes package_root,
    View::Bytes packages_root)
    : workspace(snapshots) {
  auto package = workspace.install_dialect<Package::Dialect>("Package"_view);
  auto library = workspace.install_dialect<Library::Dialect>("Library"_view);
  if (!package || !library) {
    return;
  }

  if (!workspace.install_dialect<Scene::Dialect>("Scene"_view, *library)) {
    return;
  }

  if (mode == Mode::Standalone) {
    workspace.interpret_source(
        errors, "puffer.document"_view, source_name, source);
    return;
  }

  Perimortem::Memory::Dynamic::Bytes memory_root(packages_root);
  Perimortem::Memory::Dynamic::Bytes system_root(packages_root);
  if (!packages_root.is_empty()) {
    memory_root.concat("/Perimortem.Memory"_view);
    system_root.concat("/Perimortem.System"_view);
  }

  // Standard Packages enter the Workspace in dependency order. A standard
  // Package under analysis remains the final target rather than being imported
  // once as a dependency and again as the edited root.
  Bool memory_package = package_root == memory_root.get_view();
  Bool system_package = package_root == system_root.get_view();
  if (!packages_root.is_empty() && !memory_package) {
    workspace.import_package(
        errors, memory_root.get_view(), "Perimortem.Memory"_view,
        "package.ttx"_view, "Perimortem.Memory"_view, package_version);
  }

  if (!packages_root.is_empty() && !memory_package && !system_package) {
    workspace.import_package(
        errors, system_root.get_view(), "Perimortem.System"_view,
        "package.ttx"_view, "Perimortem.System"_view, package_version);
  }

  workspace.import_package(
      errors, package_root, "puffer.package"_view, "package.ttx"_view,
      package_root, package_version);
}

auto Lsp::SemanticWorkspace::find(View::Bytes source_name, Count byte_offset)
    const -> Option<const Ttx::Concept::Abstract&> {
  auto associations = workspace.get_associations(source_name);
  if (!associations) {
    return {};
  }

  return associations->find_at(byte_offset);
}
