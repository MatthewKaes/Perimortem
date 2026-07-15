// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/package/builder.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/archiver/terminal.hpp"
#include "tetrodotoxin/archiver/writer.hpp"
#include "tetrodotoxin/puffer/isa/boot/import.hpp"
#include "tetrodotoxin/puffer/resolution/source/record.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer;

auto Package::Builder::build(
    Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    View::Bytes package_name,
    Resolution::Source::Record& root,
    View::Vector<Resolution::Source::Record*> records,
    View::Vector<const Archiver::Package*> references,
    View::Vector<Archiver::Terminal> terminals,
    View::Vector<Tetrodotoxin::Abi::Linkage> linkages) -> View::Bytes {
  Dynamic::Map<View::Bytes, const Archiver::Package*> packages;
  for (Count i = 0; i < references.get_size(); i++) {
    if (references[i] == nullptr) {
      errors.insert(
          Ttx::Lexical::Source(root.get_source_path(), root.get_content()),
          "Package compilation received an empty dependency package."_view);
      return View::Bytes();
    }

    packages.insert(references[i]->get_manifest().get_name(), references[i]);
  }

  Managed::Vector<Archiver::Dependency> imports(arena);
  View::Vector<Puffer::Isa::Boot::Import> package_imports = root.get_imports();
  for (Count i = 0; i < package_imports.get_size(); i++) {
    if (!package_imports[i].is_package()) {
      continue;
    }

    const auto* entry = packages.find(package_imports[i].get_source_name());
    if (entry == nullptr) {
      errors.insert(
          Ttx::Lexical::Source(root.get_source_path(), root.get_content()),
          "Package import was not present in its resolved closure."_view);
      return View::Bytes();
    }

    Archiver::Dependency dependency(
        package_imports[i].get_local_name(),
        package_imports[i].get_source_name(),
        entry->value->get_manifest().get_version());
    imports.insert(dependency);
  }

  Managed::Vector<const Ttx::Type*> types(arena);
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i] != &root &&
        records[i]->get_dialect().get_name() != "Package"_view) {
      types.insert(&records[i]->get_type());
    }
  }

  if (package_name.is_empty()) {
    errors.insert(
        Ttx::Lexical::Source(root.get_source_path(), root.get_content()),
        "Package compilation requires a package name."_view);
    return View::Bytes();
  }

  return Archiver::Writer::write(
      arena, package_name, imports.get_view(), root.get_type(),
      types.get_view(), terminals, linkages, references);
}
