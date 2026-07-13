// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/compiler.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "tetrodotoxin/puffer/package/builder.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer;

Puffer::Compiler::Compiler(Mode mode, View::Bytes package_name)
    : errors(arena),
      isa_registry(Toolchain::standard_registry()),
      resolver(isa_registry),
      engine(errors, Toolchain::standard_backend()),
      terminal_products(arena),
      lowering(arena, errors, engine, terminal_products),
      mode(mode) {
  if (mode == Mode::Package) {
    resolver.set_package_name(package_name);
  }
}

auto Puffer::Compiler::is_package_root(View::Bytes path) -> Bool {
  return Path(path).get_file() == "package.ttx"_view;
}

auto Puffer::Compiler::is_puffer_buffer(View::Bytes path) -> Bool {
  return Path(path).get_extension() == ".puffer"_view;
}

auto Puffer::Compiler::report(View::Bytes path, View::Bytes message) -> Bool {
  errors.insert(Ttx::Lexical::Source(path, View::Bytes()), message);
  return False;
}

auto Puffer::Compiler::add_dependency(View::Bytes path) -> Bool {
  if (!is_puffer_buffer(path)) {
    return report(path, "Dependency is not a Puffer Buffer."_view);
  }

  Dynamic::Bytes content = File::read(path);
  if (content.is_empty()) {
    return report(path, "Puffer Buffer dependency could not be read."_view);
  }

  return resolver.register_package_buffer(resolution, path, content);
}

auto Puffer::Compiler::add_record(Record& record) -> void {
  if (!records.contains(&record)) {
    records.insert(&record);
  }
}

auto Puffer::Compiler::add_source(View::Bytes path) -> Bool {
  if (mode == Mode::Package && !is_package_root(path)) {
    return True;
  }

  if (mode == Mode::Package && package_root != nullptr) {
    return report(path, "Package compilation has more than one root."_view);
  }

  Record* record = resolver.load_source(resolution, path);
  if (record == nullptr) {
    return False;
  }

  if (mode == Mode::Library) {
    add_record(*record);
    return True;
  }

  package_root = record;
  resolver.visit_reachable(
      *record, [this](Record& reachable) -> void { add_record(reachable); });
  return True;
}

auto Puffer::Compiler::build(
    View::Bytes object_name,
    Dynamic::Bytes& archive,
    Dynamic::Bytes& header,
    Dynamic::Bytes& puffer_buffer) -> Bool {
  if (records.get_size() == 0) {
    return report(
        View::Bytes(),
        mode == Mode::Package
            ? "Package compilation needs a package.ttx root."_view
            : "Library compilation has no source roots."_view);
  }

  for (Count i = 0; i < records.get_size(); i++) {
    const Tetrodotoxin::Isa::Dialect& dialect = records[i]->get_dialect();
    if (!dialect.can_lower() ||
        (mode == Mode::Package && !dialect.can_lower_package())) {
      continue;
    }

    const Tetrodotoxin::Isa::Lowering::Input input = {
      .source = Ttx::Lexical::Source(
          records[i]->get_source_path(), records[i]->get_content()),
      .module = records[i]->get_module(),
      .type = records[i]->get_type(),
      .implementation = records[i]->get_implementation(),
    };
    if (!dialect.get_lowerer()(lowering, input)) {
      return False;
    }
  }

  archive = engine.build_archive(object_name);
  if (archive.is_empty()) {
    return False;
  }

  header = engine.build_header();
  lowering.publish("linker"_view, "root.a"_view, archive);
  lowering.publish("header"_view, "root.hpp"_view, header);
  if (mode == Mode::Library) {
    return True;
  }

  Package::Builder builder(arena, errors);
  puffer_buffer = builder.build(
      resolver, *package_root, records, terminal_products.get_view());
  if (puffer_buffer.is_empty()) {
    return report(
        package_root->get_source_path(),
        "Package could not be written as a Puffer Buffer."_view);
  }

  return True;
}
