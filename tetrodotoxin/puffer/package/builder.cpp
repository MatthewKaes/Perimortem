// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/package/builder.hpp"

#include "tetrodotoxin/archiver/package.hpp"
#include "tetrodotoxin/archiver/writer.hpp"
#include "tetrodotoxin/puffer/isa/boot/import.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer;

auto Package::Builder::report(Record& root, View::Bytes message) -> void {
  errors.insert(
      Ttx::Lexical::Source(root.get_source_path(), root.get_content()),
      message);
}

auto Package::Builder::package_name(const Ttx::Type& type) -> View::Bytes {
  const Ttx::Member* member = type.find_member("package_name"_view);
  return member == nullptr ? View::Bytes() : member->get_type().get_name();
}

auto Package::Builder::build(
    Resolution::Resolver& resolver,
    Record& root,
    View::Vector<Record*> records,
    View::Vector<Archiver::Terminal> terminals) -> View::Bytes {
  packages.clear();
  Managed::Vector<Archiver::Reference> references(arena);
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i] == &root ||
        records[i]->get_dialect().get_name() != "Package"_view) {
      continue;
    }

    const auto* package = resolver.find_package(records[i]->get_source_path());
    if (package == nullptr) {
      report(root, "Resolved package record has no restored package."_view);
      return View::Bytes();
    }

    Archiver::Reference reference(*package);
    packages.insert(reference.get_source_name(), reference);
    references.insert(reference);
  }

  Managed::Vector<Archiver::Dependency> imports(arena);
  View::Vector<Puffer::Isa::Boot::Import> package_imports = root.get_imports();
  for (Count i = 0; i < package_imports.get_size(); i++) {
    if (!package_imports[i].is_package()) {
      continue;
    }

    const auto* entry = packages.find(package_imports[i].get_source_name());
    if (entry == nullptr) {
      report(
          root, "Package import was not present in its resolved closure."_view);
      return View::Bytes();
    }

    Archiver::Dependency dependency(
        package_imports[i].get_local_name(),
        package_imports[i].get_source_name(), entry->value.get_version());
    imports.insert(dependency);
  }

  Managed::Vector<const Ttx::Type*> types(arena);
  Managed::Vector<Tetrodotoxin::Compiler::Linkage> linkages(arena);
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i] != &root &&
        records[i]->get_dialect().get_name() != "Package"_view) {
      types.insert(&records[i]->get_type());

      auto record_linkages = records[i]->get_implementation().get_linkages();
      for (Count k = 0; k < record_linkages.get_size(); k++) {
        linkages.insert(record_linkages[k]);
      }
    }
  }

  View::Bytes name = package_name(root.get_type());
  if (name.is_empty()) {
    report(root, "Package is missing its compiler-provided name."_view);
    return View::Bytes();
  }

  Archiver::Package package(
      Archiver::Manifest(name, Tetrodotoxin::Archiver::Version(), imports),
      root.get_type(), types.get_view(), terminals, linkages.get_view());
  return Archiver::Writer::write(arena, package, references.get_view());
}
