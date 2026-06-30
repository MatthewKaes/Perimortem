// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/resolution/record.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"
#include "tetrodotoxin/ttx/standard.hpp"

namespace Tetrodotoxin::Resolution {

class ImportScope {
 public:
  static auto find_standard_package(const Syntax::Ast::Import& import)
      -> Ttx::StandardPackage;
  static auto find_standard_package(
      const Record& owner,
      Perimortem::Core::View::Bytes local_name) -> Ttx::StandardPackage;
  static auto standard_package_for_type(
      const Record& owner,
      const Syntax::Type::Ref& ref) -> Ttx::StandardPackage;
  static auto standard_type_name(
      const Record& owner,
      const Syntax::Type::Ref& ref) -> Perimortem::Core::View::Bytes;
  static auto standard_type_query(
      const Record& owner,
      const Syntax::Type::Ref& ref) -> Ttx::TypeQuery;
};

}  // namespace Tetrodotoxin::Resolution
