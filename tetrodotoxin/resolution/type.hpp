// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/syntax/type/declaration.hpp"
#include "tetrodotoxin/ttx/standard.hpp"

namespace Tetrodotoxin::Resolution {

class Type {
 public:
  enum class Kind : Bits_8 {
    Unknown,
    SizeLiteral,
    ImportName,
    Source,
    Standard,
  };

  constexpr Type() = default;

  static constexpr auto size_literal(Perimortem::Core::View::Bytes name)
      -> Type {
    Type type;
    type.kind = Kind::SizeLiteral;
    type.name = name;
    return type;
  }

  static constexpr auto import_name(Perimortem::Core::View::Bytes name)
      -> Type {
    Type type;
    type.kind = Kind::ImportName;
    type.name = name;
    return type;
  }

  static constexpr auto source(const Syntax::Type::Declaration* declaration)
      -> Type {
    Type type;
    type.kind = declaration ? Kind::Source : Kind::Unknown;
    type.declaration = declaration;
    return type;
  }

  static constexpr auto standard(
      Ttx::StandardPackage package,
      Perimortem::Core::View::Bytes name,
      const Ttx::Type* standard_type) -> Type {
    Type type;
    type.kind = standard_type ? Kind::Standard : Kind::Unknown;
    type.package = package;
    type.name = name;
    type.standard_type = standard_type;
    return type;
  }

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_declaration() const -> const Syntax::Type::Declaration* {
    return declaration;
  }
  constexpr auto get_standard_package() const -> Ttx::StandardPackage {
    return package;
  }
  constexpr auto get_standard_type() const -> const Ttx::Type* {
    return standard_type;
  }

  constexpr auto is_known() const -> Bool { return kind != Kind::Unknown; }
  constexpr auto is_size_literal() const -> Bool {
    return kind == Kind::SizeLiteral;
  }
  constexpr auto is_import_name() const -> Bool {
    return kind == Kind::ImportName;
  }
  constexpr auto is_source() const -> Bool { return kind == Kind::Source; }
  constexpr auto is_standard() const -> Bool { return kind == Kind::Standard; }

 private:
  Kind kind = Kind::Unknown;
  Perimortem::Core::View::Bytes name;
  const Syntax::Type::Declaration* declaration = nullptr;
  Ttx::StandardPackage package;
  const Ttx::Type* standard_type = nullptr;
};

}  // namespace Tetrodotoxin::Resolution
