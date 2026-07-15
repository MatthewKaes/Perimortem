// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/compiler.hpp"

#include "perimortem/core/math.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "tetrodotoxin/abi/linkage.hpp"
#include "tetrodotoxin/abi/type.hpp"
#include "tetrodotoxin/archiver/terminal.hpp"
#include "tetrodotoxin/compiler/engine.hpp"
#include "tetrodotoxin/isa/lowering/context.hpp"
#include "tetrodotoxin/isa/lowering/input.hpp"
#include "tetrodotoxin/puffer/package/builder.hpp"
#include "tetrodotoxin/puffer/resolution/context.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/toolchain.hpp"
#include "tetrodotoxin/standard/types.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer;

// Owns the scratch lifetime for one call to Compiler.
//
// Resolution records, lowering products, linker state, and retained source
// views must survive until all three outputs have been copied into Output. None
// of that state survives the call, so the transaction stays in this
// implementation instead of becoming Compiler API. package_root is also the
// authoritative package fact. Storing a second mode would allow the requested
// operation and resolved source graph to disagree.
class Transaction {
 public:
  Transaction(
      Resolution::Context& context,
      View::Bytes unit_name,
      View::Bytes package_name = {});

  auto add_dependencies(View::Vector<View::Bytes> paths) -> Bool;
  auto add_library_sources(View::Vector<View::Bytes> paths) -> Bool;
  auto add_package_sources(View::Vector<View::Bytes> paths) -> Bool;
  auto build() -> Puffer::Compiler::Output;

 private:
  using Record = Resolution::Source::Record;
  struct Publication {
    View::Bytes path;
    View::Bytes source;
    const Ttx::Type* type;
  };

  auto add_dependency(View::Bytes path) -> Bool;
  auto add_library_source(View::Bytes path) -> Bool;
  auto add_package_source(View::Bytes path) -> Bool;
  auto add_record(Record& record) -> void;
  auto find_definition(const Ttx::Function& function) const
      -> const Tetrodotoxin::Isa::Base::Definition*;
  auto find_definition(const Ttx::Type& type) const
      -> const Tetrodotoxin::Isa::Base::Definition*;
  auto find_linkage(const Ttx::Function& function) const
      -> const Tetrodotoxin::Abi::Linkage*;
  auto publish_exports() -> Bool;
  auto collect_publications(
      View::Bytes path,
      View::Bytes source,
      const Ttx::Type& type,
      Managed::Vector<Publication>& publications) -> Bool;
  auto collect_dependency_publications(
      Managed::Vector<Publication>& publications) -> Bool;
  auto publish(
      const Publication& publication,
      View::Vector<Abi::Type> type_identities) -> Bool;
  auto publish_functions(
      View::Bytes path,
      View::Bytes source,
      View::Vector<Ttx::Function> functions,
      View::Vector<Abi::Type> type_identities) -> Bool;
  auto publish_function_tables(
      const Publication& publication,
      View::Vector<Abi::Type> type_identities) -> Bool;
  auto report(View::Bytes path, View::Bytes message) -> void;

  Allocator::Arena arena;
  Resolution::Context& context;
  Tetrodotoxin::Isa::Registry isa_registry;
  Resolution::Resolver resolver;
  Tetrodotoxin::Compiler::Program program;
  Tetrodotoxin::Compiler::Engine engine;
  Managed::Vector<Archiver::Terminal> terminal_products;
  Tetrodotoxin::Isa::Lowering::Context lowering;
  Dynamic::Vector<Record*> records;
  Dynamic::Bytes unit_name;
  Record* package_root = nullptr;
};

static auto is_package_root(View::Bytes path) -> Bool {
  return Path(path).get_file() == "package.ttx"_view;
}

static auto is_puffer_buffer(View::Bytes path) -> Bool {
  return Path(path).get_extension() == ".puffer"_view;
}

Transaction::Transaction(
    Resolution::Context& context,
    View::Bytes unit_name,
    View::Bytes package_name)
    : context(context),
      isa_registry(Toolchain::standard_registry()),
      resolver(isa_registry, unit_name, package_name),
      engine(context.get_error_sink(), Toolchain::standard_backend()),
      terminal_products(arena),
      lowering(
          arena,
          context.get_error_sink(),
          program,
          engine,
          terminal_products),
      unit_name(unit_name) {}

auto Transaction::report(View::Bytes path, View::Bytes message) -> void {
  context.get_error_sink().insert(
      Ttx::Lexical::Source(path, View::Bytes()), message);
}

auto Transaction::add_dependency(View::Bytes path) -> Bool {
  if (!is_puffer_buffer(path)) {
    report(path, "Dependency is not a Puffer Buffer."_view);
    return False;
  }

  Dynamic::Bytes content = File::read(path);
  if (content.is_empty()) {
    report(path, "Puffer Buffer dependency could not be read."_view);
    return False;
  }

  Bool registered = resolver.register_package_buffer(context, path, content);
  return registered;
}

auto Transaction::add_dependencies(View::Vector<View::Bytes> paths) -> Bool {
  for (Count i = 0; i < paths.get_size(); i++) {
    Bool added = add_dependency(paths[i]);
    if (!added) {
      return False;
    }
  }

  return True;
}

auto Transaction::add_record(Record& record) -> void {
  if (!records.contains(&record)) {
    records.insert(&record);
  }
}

auto Transaction::find_definition(const Ttx::Function& function) const
    -> const Tetrodotoxin::Isa::Base::Definition* {
  for (Count i = 0; i < records.get_size(); i++) {
    const Tetrodotoxin::Isa::Base::Definition* definition =
        records[i]->get_implementation().find_definition(function);
    if (definition != nullptr) {
      return definition;
    }
  }

  return nullptr;
}

auto Transaction::find_definition(const Ttx::Type& type) const
    -> const Tetrodotoxin::Isa::Base::Definition* {
  for (Count i = 0; i < records.get_size(); i++) {
    const Tetrodotoxin::Isa::Base::Definition* definition =
        records[i]->get_implementation().find(type);
    if (definition != nullptr) {
      return definition;
    }
  }

  return nullptr;
}

auto Transaction::find_linkage(const Ttx::Function& function) const
    -> const Tetrodotoxin::Abi::Linkage* {
  for (Count i = 0; i < records.get_size(); i++) {
    const Tetrodotoxin::Abi::Linkage* linkage =
        records[i]->get_implementation().find_linkage(function);
    if (linkage != nullptr) {
      return linkage;
    }
  }

  return nullptr;
}

auto Transaction::add_library_source(View::Bytes path) -> Bool {
  Record* record = resolver.load_source(context, path);
  if (record == nullptr) {
    return False;
  }

  add_record(*record);
  return True;
}

auto Transaction::add_library_sources(View::Vector<View::Bytes> paths) -> Bool {
  for (Count i = 0; i < paths.get_size(); i++) {
    Bool added = add_library_source(paths[i]);
    if (!added) {
      return False;
    }
  }

  return True;
}

auto Transaction::add_package_source(View::Bytes path) -> Bool {
  if (package_root != nullptr) {
    report(path, "Package compilation has more than one root."_view);
    return False;
  }

  Record* record = resolver.load_source(context, path);
  if (record == nullptr) {
    return False;
  }

  package_root = record;
  resolver.visit_reachable(
      *record, [this](Record& reachable) -> void { add_record(reachable); });
  return True;
}

auto Transaction::add_package_sources(View::Vector<View::Bytes> paths) -> Bool {
  for (Count i = 0; i < paths.get_size(); i++) {
    if (!is_package_root(paths[i])) {
      continue;
    }

    Bool added = add_package_source(paths[i]);
    if (!added) {
      return False;
    }
  }

  if (package_root == nullptr) {
    report(View::Bytes(), "Package compilation needs a package.ttx root."_view);
    return False;
  }

  return True;
}

auto Transaction::publish_functions(
    View::Bytes path,
    View::Bytes source,
    View::Vector<Ttx::Function> functions,
    View::Vector<Abi::Type> type_identities) -> Bool {
  for (Count i = 0; i < functions.get_size(); i++) {
    const Tetrodotoxin::Isa::Base::Definition* definition =
        find_definition(functions[i]);
    Ttx::Lexical::Class::Type modifier =
        definition == nullptr ? Ttx::Lexical::Class::Type::Unknown
                              : definition->get_modifier();
    // Public functions are supplied by the TTX unit. Expose functions are
    // supplied by their external owner. Both are callable ABI facts once the
    // containing type is published, while Private remains implementation-only.
    Bool is_exported = modifier == Ttx::Lexical::Class::Type::Public ||
                       modifier == Ttx::Lexical::Class::Type::Expose;
    if (definition != nullptr && !is_exported) {
      continue;
    }

    const Tetrodotoxin::Abi::Linkage* linkage = find_linkage(functions[i]);
    if (linkage == nullptr) {
      continue;
    }

    const Abi::Export* existing = program.find_export(functions[i]);
    const Abi::Export* export_ =
        existing == nullptr ? Abi::Export::create(
                                  arena, path, functions[i],
                                  linkage->get_symbol(), type_identities)
                            : Abi::Export::project(arena, path, *existing);
    if (export_ == nullptr) {
      report(
          source,
          "Public function signature uses a type without a stable ABI "
          "identity."_view);
      return False;
    }

    Bool exposed = program.expose(*export_);
    if (!exposed) {
      report(
          source,
          "Public ABI path or symbol collides with another export."_view);
      return False;
    }
  }

  return True;
}

auto Transaction::collect_publications(
    View::Bytes path,
    View::Bytes source,
    const Ttx::Type& type,
    Managed::Vector<Publication>& publications) -> Bool {
  const Ttx::Type& interface = type.canonical();
  if (interface.is_invalid()) {
    report(source, "Public type alias cannot be canonicalized."_view);
    return False;
  }

  Managed::Bytes stored_path(arena, path);
  Managed::Bytes stored_source(arena, source);
  publications.insert({
    stored_path.get_view(),
    stored_source.get_view(),
    &type,
  });

  View::Vector<Ttx::Type::Reference> nested = interface.get_types();
  for (Count i = 0; i < nested.get_size(); i++) {
    const Ttx::Type& nested_type = nested[i].get_type();

    // A package group owns only the aliases explicitly written inside that
    // group, so its local nested table is already a public surface. An alias
    // projects another source type. Its canonical nested table can also hold
    // private Library declarations or Render and Shader facts, which must not
    // become host exports merely because the parent received a public alias.
    if (type.is_alias()) {
      const Tetrodotoxin::Isa::Base::Definition* definition =
          find_definition(nested_type);

      if (definition == nullptr ||
          definition->get_modifier() != Ttx::Lexical::Class::Type::Public) {
        continue;
      }
    }

    Managed::Bytes nested_path(arena, path);
    nested_path.append('.');
    nested_path.concat(nested_type.get_name());
    Bool collected =
        collect_publications(nested_path, source, nested_type, publications);
    if (!collected) {
      return False;
    }
  }

  return True;
}

auto Transaction::collect_dependency_publications(
    Managed::Vector<Publication>& publications) -> Bool {
  for (Count i = 0; i < records.get_size(); i++) {
    View::Vector<Puffer::Isa::Boot::Import> imports = records[i]->get_imports();
    for (Count k = 0; k < imports.get_size(); k++) {
      if (!imports[k].is_package()) {
        continue;
      }

      const Archiver::Package* package =
          resolver.find_package(imports[k].get_source_name());
      if (package == nullptr) {
        report(
            records[i]->get_source_path(),
            "Imported package has no restored Puffer Buffer."_view);
        return False;
      }

      View::Bytes package_name = package->get_manifest().get_name();
      View::Vector<Ttx::Type::Reference> types =
          package->get_type().get_types();
      for (Count n = 0; n < types.get_size(); n++) {
        const Ttx::Type& type = types[n].get_type();

        Managed::Bytes path(arena, package_name);
        path.append('.');
        path.concat(type.get_name());
        Bool collected = collect_publications(
            path, records[i]->get_source_path(), type, publications);
        if (!collected) {
          return False;
        }
      }
    }
  }

  return True;
}

auto Transaction::publish(
    const Publication& publication,
    View::Vector<Abi::Type> type_identities) -> Bool {
  Abi::Type type =
      Abi::Type::create(arena, publication.path, *publication.type);
  Bool exposed = program.expose(type);
  if (!exposed) {
    report(
        publication.source,
        "Public type path collides with another ABI identity."_view);
    return False;
  }

  return publish_function_tables(publication, type_identities);
}

auto Transaction::publish_function_tables(
    const Publication& publication,
    View::Vector<Abi::Type> type_identities) -> Bool {
  const Ttx::Type& interface = publication.type->canonical();
  Managed::Bytes type_path(arena, publication.path);
  type_path.concat(".Type"_view);
  Managed::Bytes addressable_path(arena, publication.path);
  addressable_path.concat(".Addressable"_view);
  Bool published_type = publish_functions(
      type_path, publication.source, interface.get_type_functions(),
      type_identities);
  if (!published_type) {
    return False;
  }

  return publish_functions(
      addressable_path, publication.source,
      interface.get_addressable_functions(), type_identities);
}

auto Transaction::publish_exports() -> Bool {
  if (unit_name.is_empty()) {
    report(View::Bytes(), "Compilation requires a stable ABI unit name."_view);
    return False;
  }

  Managed::Vector<Publication> dependencies(arena);
  Managed::Vector<Publication> publications(arena);
  Managed::Vector<Publication> containers(arena);
  Bool collected_dependencies = collect_dependency_publications(dependencies);
  if (!collected_dependencies) {
    return False;
  }

  if (package_root != nullptr) {
    View::Vector<Ttx::Type::Reference> types =
        package_root->get_type().get_types();
    for (Count i = 0; i < types.get_size(); i++) {
      const Ttx::Type& type = types[i].get_type();

      Managed::Bytes path(arena, unit_name);
      path.append('.');
      path.concat(type.get_name());
      Bool collected = collect_publications(
          path, package_root->get_source_path(), type, publications);
      if (!collected) {
        return False;
      }
    }
  } else {
    for (Count i = 0; i < records.get_size(); i++) {
      const Ttx::Type& root = records[i]->get_type();
      Bool dialect_container =
          root.get_name() == records[i]->get_dialect().get_name();
      Managed::Bytes path(arena, unit_name);
      if (!dialect_container) {
        path.append('.');
        path.concat(root.get_name());
      }

      if (dialect_container) {
        Managed::Bytes stored_path(arena, path);
        Managed::Bytes stored_source(arena, records[i]->get_source_path());
        containers.insert({
          stored_path.get_view(),
          stored_source.get_view(),
          &root,
        });

        View::Vector<Ttx::Type::Reference> nested =
            root.canonical().get_types();
        for (Count k = 0; k < nested.get_size(); k++) {
          const Ttx::Type& nested_type = nested[k].get_type();

          const Tetrodotoxin::Isa::Base::Definition* definition =
              records[i]->get_implementation().find(nested_type);
          if (definition == nullptr ||
              definition->get_modifier() != Ttx::Lexical::Class::Type::Public) {
            continue;
          }

          Managed::Bytes nested_path(arena, path);
          nested_path.append('.');
          nested_path.concat(nested_type.get_name());
          Bool collected = collect_publications(
              nested_path, records[i]->get_source_path(), nested_type,
              publications);
          if (!collected) {
            return False;
          }
        }
      } else {
        Bool collected = collect_publications(
            path, records[i]->get_source_path(), root, publications);
        if (!collected) {
          return False;
        }
      }
    }
  }

  Dynamic::Map<const Ttx::Type*, View::Bytes> identity_paths;
  Dynamic::Vector<const Ttx::Type*> identity_types;
  auto path_precedes = [](View::Bytes left, View::Bytes right) {
    Count size = Math::min(left.get_size(), right.get_size());
    for (Count i = 0; i < size; i++) {
      if (left[i] != right[i]) {
        return left[i] < right[i];
      }
    }

    return left.get_size() < right.get_size();
  };
  auto retain_identity = [&](View::Bytes path, const Ttx::Type& type) {
    const Ttx::Type& canonical = type.canonical();
    auto* existing = identity_paths.find(&canonical);
    if (existing == nullptr) {
      Managed::Bytes stored_path(arena, path);
      identity_paths.insert(&canonical, stored_path.get_view());
      identity_types.insert(&canonical);
      return;
    }

    if (path_precedes(path, existing->value)) {
      Managed::Bytes stored_path(arena, path);
      existing->value = stored_path.get_view();
    }
  };
  View::Vector<const Ttx::Type*> standard = Standard::Types::get_types();
  for (Count i = 0; i < standard.get_size(); i++) {
    Managed::Bytes path(arena, "Tetrodotoxin.Standard."_view);
    path.concat(standard[i]->get_name());
    retain_identity(path, *standard[i]);
  }

  for (Count i = 0; i < dependencies.get_size(); i++) {
    retain_identity(dependencies[i].path, *dependencies[i].type);
  }

  for (Count i = 0; i < publications.get_size(); i++) {
    retain_identity(publications[i].path, *publications[i].type);
  }

  Managed::Vector<Abi::Type> type_identities(arena);
  for (Count i = 0; i < identity_types.get_size(); i++) {
    const auto* entry = identity_paths.find(identity_types[i]);
    if (entry == nullptr) {
      return False;
    }

    type_identities.insert(
        Abi::Type::create(arena, entry->value, *identity_types[i]));
  }

  for (Count i = 0; i < publications.get_size(); i++) {
    Bool published = publish(publications[i], type_identities.get_view());
    if (!published) {
      return False;
    }
  }

  for (Count i = 0; i < containers.get_size(); i++) {
    Bool published =
        publish_function_tables(containers[i], type_identities.get_view());
    if (!published) {
      return False;
    }
  }

  return True;
}

auto Transaction::build() -> Puffer::Compiler::Output {
  Puffer::Compiler::Output output;
  if (records.get_size() == 0) {
    report(View::Bytes(), "Library compilation has no source roots."_view);
    return output;
  }

  for (Count i = 0; i < records.get_size(); i++) {
    const Tetrodotoxin::Isa::Dialect& dialect = records[i]->get_dialect();
    if (!dialect.can_lower() ||
        (package_root != nullptr && !dialect.can_lower_package())) {
      continue;
    }

    const Tetrodotoxin::Isa::Lowering::Input input = {
      .source = Ttx::Lexical::Source(
          records[i]->get_source_path(), records[i]->get_content()),
      .module = records[i]->get_module(),
      .type = records[i]->get_type(),
      .implementation = records[i]->get_implementation(),
    };
    Bool lowered = dialect.get_lowerer()(lowering, input);
    if (!lowered) {
      if (!context.has_errors()) {
        report(
            records[i]->get_source_path(),
            "Dialect lowering failed without a source diagnostic."_view);
      }

      return output;
    }
  }

  Bool published_exports = publish_exports();
  if (!published_exports) {
    return output;
  }

  output.native_archive = engine.build_archive(program, "x86_64.o"_view);
  if (output.native_archive.is_empty()) {
    if (!context.has_errors()) {
      report(View::Bytes(), "The x86-64 archive could not be produced."_view);
    }

    return output;
  }

  output.cpp_header = engine.build_header(program);
  lowering.publish("linker"_view, "x86_64.a"_view, output.native_archive);
  lowering.publish("header"_view, "cpp_abi.hpp"_view, output.cpp_header);
  if (package_root == nullptr) {
    return output;
  }

  Managed::Vector<const Archiver::Package*> package_references(arena);
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i] == package_root ||
        records[i]->get_dialect().get_name() != "Package"_view) {
      continue;
    }

    const Archiver::Package* package =
        resolver.find_package(records[i]->get_source_path());
    if (package == nullptr) {
      report(
          records[i]->get_source_path(),
          "Resolved package record has no restored package."_view);
      return output;
    }

    package_references.insert(package);
  }

  Managed::Vector<Tetrodotoxin::Abi::Linkage> package_linkages(arena);
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i] == package_root ||
        records[i]->get_dialect().get_name() == "Package"_view) {
      continue;
    }

    View::Vector<Tetrodotoxin::Abi::Linkage> record_linkages =
        records[i]->get_implementation().get_linkages();
    for (Count k = 0; k < record_linkages.get_size(); k++) {
      if (program.find_export(record_linkages[k].get_function()) == nullptr) {
        continue;
      }

      View::Bytes symbol =
          program.resolve_symbol(record_linkages[k].get_symbol());
      package_linkages.insert(
          symbol == record_linkages[k].get_symbol()
              ? record_linkages[k]
              : Tetrodotoxin::Abi::Linkage(
                    record_linkages[k].get_owner(),
                    record_linkages[k].get_function(), symbol));
    }
  }

  output.package_buffer = Package::Builder::build(
      arena, context.get_error_sink(), unit_name, *package_root, records,
      package_references.get_view(), terminal_products.get_view(),
      package_linkages.get_view());
  if (output.package_buffer.is_empty() && !context.has_errors()) {
    report(
        package_root->get_source_path(),
        "Package could not be written as a Puffer Buffer."_view);
  }

  return output;
}

auto Puffer::Compiler::build_library(
    Resolution::Context& context,
    View::Bytes unit_name,
    View::Vector<View::Bytes> dependencies,
    View::Vector<View::Bytes> sources) -> Output {
  Transaction transaction(context, unit_name);
  Bool added_dependencies = transaction.add_dependencies(dependencies);
  if (!added_dependencies) {
    return Output();
  }

  Bool added_sources = transaction.add_library_sources(sources);
  if (!added_sources) {
    return Output();
  }

  return transaction.build();
}

auto Puffer::Compiler::build_package(
    Resolution::Context& context,
    View::Bytes unit_name,
    View::Vector<View::Bytes> dependencies,
    View::Vector<View::Bytes> sources) -> Output {
  Transaction transaction(context, unit_name, unit_name);
  Bool added_dependencies = transaction.add_dependencies(dependencies);
  if (!added_dependencies) {
    return Output();
  }

  Bool added_sources = transaction.add_package_sources(sources);
  if (!added_sources) {
    return Output();
  }

  return transaction.build();
}
