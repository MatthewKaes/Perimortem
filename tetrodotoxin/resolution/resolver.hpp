// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/resolution/import_scope.hpp"
#include "tetrodotoxin/resolution/record.hpp"
#include "tetrodotoxin/resolution/type.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"
#include "tetrodotoxin/ttx/standard.hpp"

namespace Tetrodotoxin::Resolution {

class Resolver {
 public:
  using State = Resolution::State;

  Resolver() = default;
  ~Resolver();

  auto load(Perimortem::Core::View::Bytes source_path) -> Record*;
  auto resolve(Perimortem::Core::View::Bytes source_path) -> Bool;
  auto resolve(Record& record) -> Bool;

  auto find(Perimortem::Core::View::Bytes source_path) -> Record*;
  auto find_imported(const Record& owner, const Syntax::Ast::Import& import)
      -> Record*;
  auto resolve_type(
      Record& record,
      const Syntax::Type::Ref& ref,
      const Syntax::Type::Declaration* current_type = nullptr)
      -> const Syntax::Type::Declaration*;
  auto resolve_import_type(Record& record, const Syntax::Type::Ref& ref)
      -> const Syntax::Type::Declaration*;
  auto query_type(
      Record& record,
      const Syntax::Type::Ref& ref,
      const Syntax::Type::Declaration* current_type = nullptr) -> Type;
  auto is_known_type(
      Record& record,
      const Syntax::Type::Ref& ref,
      const Syntax::Type::Declaration* current_type = nullptr) -> Bool;
  auto is_import_name(const Record& record, Perimortem::Core::View::Bytes name)
      const -> Bool;
  auto standard_package_for_type(
      const Record& owner,
      const Syntax::Type::Ref& ref) const -> Ttx::StandardPackage;
  auto canonical_standard_type_name(
      const Record& owner,
      const Syntax::Type::Ref& ref) const -> Perimortem::Core::View::Bytes;
  auto find_standard_type(const Record& owner, const Syntax::Type::Ref& ref)
      const -> const Ttx::Type*;
  auto find_standard_field(
      const Record& owner,
      const Syntax::Type::Ref& ref,
      Perimortem::Core::View::Bytes field) const -> const Ttx::Field*;
  auto find_standard_method(
      const Record& owner,
      const Syntax::Type::Ref& ref,
      Perimortem::Core::View::Bytes method) const -> const Ttx::Method*;
  auto find_standard_package(const Syntax::Ast::Import& import) const
      -> Ttx::StandardPackage;
  auto find_standard_package(
      const Record& owner,
      Perimortem::Core::View::Bytes local_name) const -> Ttx::StandardPackage;
  auto get_packages() const -> Perimortem::Core::View::Vector<Record*> {
    return packages.get_view();
  }
  auto get_error_count() const -> Count { return error_count; }

 private:
  auto resolve_record(Record& record) -> Bool;
  auto resolve_import(Record& owner, const Syntax::Ast::Import& import) -> Bool;
  auto import_path(const Record& owner, const Syntax::Ast::Import& import)
      -> Perimortem::Memory::Dynamic::Bytes;
  auto find_type(
      Perimortem::Core::View::Vector<Syntax::Type::Declaration*> types,
      Perimortem::Core::View::Bytes name) const
      -> const Syntax::Type::Declaration*;

  Perimortem::Memory::Dynamic::Vector<Record*> packages;
  Count error_count = 0;
};

}  // namespace Tetrodotoxin::Resolution
