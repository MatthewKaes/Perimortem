// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/resolution/import_scope.hpp"

#include "perimortem/core/reader/textual.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

static auto parse_count(View::Bytes text, Count* out) -> Bool {
  if (!out || text.is_empty()) {
    return False;
  }

  Reader::Textual reader(text);
  Bits_64 result = reader.read_unsigned();
  if (!reader.is_valid() || !reader.is_empty()) {
    return False;
  }

  *out = Count(result);
  return True;
}

auto Resolution::ImportScope::standard_type_name(
    const Record& owner,
    const Syntax::Type::Ref& ref) -> View::Bytes {
  Ttx::StandardPackage package =
      Resolution::ImportScope::standard_package_for_type(owner, ref);
  if (!package.exists()) {
    return ref.get_name();
  }

  View::Vector<View::Bytes> path = ref.get_name_path();
  return package.is_implicit() || path.get_size() < 2 ? ref.get_name()
                                                      : path[1];
}

auto Resolution::ImportScope::find_standard_package(
    const Syntax::Ast::Import& import) -> Ttx::StandardPackage {
  if (!import.is_package_source()) {
    return Ttx::StandardPackage();
  }

  return Ttx::find_standard_package(import.get_package_path());
}

auto Resolution::ImportScope::find_standard_package(
    const Record& owner,
    View::Bytes local_name) -> Ttx::StandardPackage {
  View::Vector<Syntax::Ast::Import> imports = owner.get_package().get_imports();
  for (Count i = 0; i < imports.get_size(); i++) {
    if (imports[i].get_local_name() == local_name) {
      return find_standard_package(imports[i]);
    }
  }

  return Ttx::StandardPackage();
}

auto Resolution::ImportScope::standard_package_for_type(
    const Record& owner,
    const Syntax::Type::Ref& ref) -> Ttx::StandardPackage {
  if (ref.is_empty() || ref.is_size_literal()) {
    return Ttx::StandardPackage();
  }

  View::Vector<View::Bytes> path = ref.get_name_path();
  View::Bytes name = path[0];

  Ttx::StandardPackage core = Ttx::implicit_core_package();
  if (core.is_type(name)) {
    return core;
  }

  if (path.get_size() < 2) {
    return Ttx::StandardPackage();
  }

  if (path[0] == "Core"_view) {
    Ttx::StandardPackage explicit_core =
        Ttx::find_standard_package("Core"_view);
    return explicit_core.is_type(path[1]) ? explicit_core
                                          : Ttx::StandardPackage();
  }

  Ttx::StandardPackage package = find_standard_package(owner, path[0]);
  return package.is_type(path[1]) ? package : Ttx::StandardPackage();
}

auto Resolution::ImportScope::standard_type_query(
    const Record& owner,
    const Syntax::Type::Ref& ref) -> Ttx::TypeQuery {
  Ttx::TypeQuery result(standard_type_name(owner, ref));

  View::Vector<Syntax::Type::Ref> params = ref.get_params();
  for (Count i = 0; i < params.get_size(); i++) {
    if (params[i].is_size_literal()) {
      Count value = 0;
      if (parse_count(params[i].get_name(), &value)) {
        result.add_param(
            Ttx::TypeArgument::size_literal(params[i].get_name(), value));
      }
      continue;
    }

    Ttx::StandardPackage package = standard_package_for_type(owner, params[i]);
    Ttx::TypeQuery param_query = standard_type_query(owner, params[i]);
    result.add_param(
        Ttx::TypeArgument::type(
            param_query.get_name(), package.type_layout(param_query)));
  }

  return result;
}
