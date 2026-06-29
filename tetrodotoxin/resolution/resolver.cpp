// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/resolution/resolver.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

#include "tetrodotoxin/resolution/import_scope.hpp"
#include "tetrodotoxin/syntax/expression/literal.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

static auto is_absolute_path(View::Bytes path) -> Bool {
  return path.get_size() != 0 && path[0] == '/';
}

static auto parent_path(View::Bytes path) -> View::Bytes {
  for (Count i = path.get_size(); i > 0; i--) {
    if (path[i - 1] == '/') {
      return path.slice(0, i);
    }
  }

  return View::Bytes();
}

Resolution::Resolver::~Resolver() {
  for (Count i = 0; i < packages.get_size(); i++) {
    if (packages[i]) {
      packages[i]->~Record();
      Bibliotheca::remit(Data::cast<Bits_8>(packages[i]));
    }
  }
}

auto Resolution::Resolver::find(View::Bytes source_path) -> Record* {
  for (Count i = 0; i < packages.get_size(); i++) {
    if (packages[i] && packages[i]->get_path() == source_path) {
      return packages[i];
    }
  }

  return nullptr;
}

auto Resolution::Resolver::load(View::Bytes source_path) -> Record* {
  Record* record = find(source_path);
  if (!record) {
    record =
        new (Bibliotheca::check_out(sizeof(Record)).ptr) Record(source_path);
    packages.insert(record);
  }

  record->load_if_stale();
  return record;
}

auto Resolution::Resolver::find_imported(
    const Record& owner,
    const Syntax::Ast::Import& import) -> Record* {
  if (!import.is_file_source()) {
    return nullptr;
  }

  Dynamic::Bytes path = import_path(owner, import);
  return find(path.get_view());
}

auto Resolution::Resolver::find_type(
    View::Vector<Syntax::Type::Declaration*> types,
    View::Bytes name) const -> const Syntax::Type::Declaration* {
  for (Count i = 0; i < types.get_size(); i++) {
    if (types[i] && types[i]->get_definition().get_name() == name) {
      return types[i];
    }
  }

  return nullptr;
}

auto Resolution::Resolver::resolve_type(
    Record& record,
    const Syntax::Type::Ref& ref,
    const Syntax::Type::Declaration* current_type)
    -> const Syntax::Type::Declaration* {
  View::Bytes name = ref.get_name();
  if (name.is_empty()) {
    return nullptr;
  }

  if (current_type && name == current_type->get_definition().get_name()) {
    return current_type;
  }

  const Syntax::Type::Declaration* local =
      find_type(record.get_package().get_types(), name);
  if (local) {
    return local;
  }

  return resolve_import_type(record, ref);
}

auto Resolution::Resolver::resolve_import_type(
    Record& record,
    const Syntax::Type::Ref& ref) -> const Syntax::Type::Declaration* {
  View::Bytes import_name = ref.get_name();
  View::Bytes type_name = ref.get_second_name();
  if (import_name.is_empty() || type_name.is_empty()) {
    return nullptr;
  }

  View::Vector<Syntax::Ast::Import> imports =
      record.get_package().get_imports();
  for (Count i = 0; i < imports.get_size(); i++) {
    if (imports[i].get_local_name() != import_name) {
      continue;
    }

    Record* imported = find_imported(record, imports[i]);
    if (!imported) {
      return nullptr;
    }

    return find_type(imported->get_package().get_types(), type_name);
  }

  return nullptr;
}

auto Resolution::Resolver::query_type(
    Record& record,
    const Syntax::Type::Ref& ref,
    const Syntax::Type::Declaration* current_type) -> Resolution::Type {
  if (ref.is_empty()) {
    return Resolution::Type();
  }

  if (ref.is_size_literal()) {
    return Resolution::Type::size_literal(ref.get_name());
  }

  Ttx::StandardPackage package = standard_package_for_type(record, ref);
  if (package.exists()) {
    View::Bytes name = canonical_standard_type_name(record, ref);
    return Resolution::Type::standard(package, name, package.find_type(name));
  }

  if (const Syntax::Type::Declaration* declaration =
          resolve_type(record, ref, current_type)) {
    return Resolution::Type::source(declaration);
  }

  if (is_import_name(record, ref.get_name())) {
    return Resolution::Type::import_name(ref.get_name());
  }

  return Resolution::Type();
}

auto Resolution::Resolver::is_known_type(
    Record& record,
    const Syntax::Type::Ref& ref,
    const Syntax::Type::Declaration* current_type) -> Bool {
  Resolution::Type type = query_type(record, ref, current_type);
  if (!type.is_known()) {
    return False;
  }

  View::Vector<Syntax::Type::Ref> params = ref.get_params();
  for (Count i = 0; i < params.get_size(); i++) {
    if (!is_known_type(record, params[i], current_type)) {
      return False;
    }
  }

  return True;
}

auto Resolution::Resolver::is_import_name(
    const Record& record,
    View::Bytes name) const -> Bool {
  View::Vector<Syntax::Ast::Import> imports =
      record.get_package().get_imports();
  for (Count i = 0; i < imports.get_size(); i++) {
    if (imports[i].get_local_name() == name) {
      return True;
    }
  }

  return False;
}

auto Resolution::Resolver::standard_package_for_type(
    const Record& owner,
    const Syntax::Type::Ref& ref) const -> Ttx::StandardPackage {
  return ImportScope::standard_package_for_type(owner, ref);
}

auto Resolution::Resolver::canonical_standard_type_name(
    const Record& owner,
    const Syntax::Type::Ref& ref) const -> View::Bytes {
  Ttx::StandardPackage package = standard_package_for_type(owner, ref);
  if (!package.exists()) {
    return ref.get_name();
  }

  View::Vector<View::Bytes> path = ref.get_name_path();
  return package.is_implicit() || path.get_size() < 2 ? ref.get_name()
                                                      : path[1];
}

auto Resolution::Resolver::find_standard_type(
    const Record& owner,
    const Syntax::Type::Ref& ref) const -> const Ttx::Type* {
  Ttx::StandardPackage package = standard_package_for_type(owner, ref);
  return package.find_type(canonical_standard_type_name(owner, ref));
}

auto Resolution::Resolver::find_standard_field(
    const Record& owner,
    const Syntax::Type::Ref& ref,
    View::Bytes field) const -> const Ttx::Field* {
  Ttx::StandardPackage package = standard_package_for_type(owner, ref);
  return package.find_field(canonical_standard_type_name(owner, ref), field);
}

auto Resolution::Resolver::find_standard_method(
    const Record& owner,
    const Syntax::Type::Ref& ref,
    View::Bytes method) const -> const Ttx::Method* {
  Ttx::StandardPackage package = standard_package_for_type(owner, ref);
  return package.find_method(canonical_standard_type_name(owner, ref), method);
}

auto Resolution::Resolver::find_standard_package(
    const Syntax::Ast::Import& import) const -> Ttx::StandardPackage {
  return ImportScope::find_standard_package(import);
}

auto Resolution::Resolver::find_standard_package(
    const Record& owner,
    View::Bytes local_name) const -> Ttx::StandardPackage {
  return ImportScope::find_standard_package(owner, local_name);
}

auto Resolution::Resolver::resolve(View::Bytes source_path) -> Bool {
  Record* record = load(source_path);
  if (!record) {
    error_count++;
    return False;
  }

  return resolve(*record);
}

auto Resolution::Resolver::resolve(Record& record) -> Bool {
  error_count = 0;

  for (Count i = 0; i < packages.get_size(); i++) {
    if (packages[i]) {
      packages[i]->invalidate_resolution();
    }
  }

  return resolve_record(record);
}

auto Resolution::Resolver::import_path(
    const Record& owner,
    const Syntax::Ast::Import& import) -> Dynamic::Bytes {
  if (!import.is_file_source()) {
    return Dynamic::Bytes();
  }

  View::Bytes path =
      Syntax::Expression::Literal::inner_text(import.get_file_path());
  if (is_absolute_path(path)) {
    return Dynamic::Bytes(path);
  }

  Dynamic::Bytes result;
  result.concat(parent_path(owner.get_path()));
  result.concat(path);
  return result;
}

auto Resolution::Resolver::resolve_import(
    Record& owner,
    const Syntax::Ast::Import& import) -> Bool {
  if (import.is_package_source()) {
    if (find_standard_package(import).exists()) {
      return True;
    }

    owner.fail();
    error_count++;
    return False;
  }

  if (!import.is_file_source()) {
    owner.fail();
    error_count++;
    return False;
  }

  Dynamic::Bytes path = import_path(owner, import);
  Record* imported = load(path.get_view());
  if (!imported || imported->get_state() == State::Failed) {
    owner.fail();
    error_count++;
    return False;
  }

  if (imported->get_package().get_kind() != import.get_class()) {
    owner.fail();
    error_count++;
    return False;
  }

  if (!resolve_record(*imported)) {
    owner.fail();
    return False;
  }

  return True;
}

auto Resolution::Resolver::resolve_record(Record& record) -> Bool {
  if (!record.load_if_stale()) {
    error_count++;
    return False;
  }

  switch (record.get_state()) {
  case State::Resolved:
    return True;

  case State::Resolving:
    record.fail();
    error_count++;
    return False;

  case State::Parsed:
    break;

  case State::Unloaded:
  case State::Failed:
    error_count++;
    return False;
  }

  record.begin_resolution();

  View::Vector<Syntax::Ast::Import> imports =
      record.get_package().get_imports();
  for (Count i = 0; i < imports.get_size(); i++) {
    if (!resolve_import(record, imports[i])) {
      return False;
    }
  }

  record.finish_resolution();
  return True;
}
