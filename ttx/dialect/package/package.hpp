// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"
#include "ttx/dialect/package/export.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Ttx::Dialect::Package {

// The package dialect body owns package metadata and its public type surface.
//
// Source parses the common source envelope and leaves the cursor at the package
// body. Package then parses package-owned directives such as @package_name and
// export syntax. Exported package facts are represented as real Ttx::Type
// objects once their targets can be resolved. Package deliberately does not
// invent a second alias or namespace IR beside Ttx::Type.
class Package {
 public:
  Package() = default;
  Package(
      Ttx::Documentation documentation,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Export> exports =
          Perimortem::Core::View::Vector<Export>(),
      Perimortem::Core::View::Vector<const Ttx::Type*> types =
          Perimortem::Core::View::Vector<const Ttx::Type*>())
      : documentation(documentation),
        name(name),
        exports(exports),
        types(types) {}

  static auto parse(Lexical::Cursor& cursor) -> void*;

  static constexpr auto get_name() -> Perimortem::Core::View::Bytes {
    return "Package"_view;
  }

  constexpr auto get_documentation() const -> Ttx::Documentation {
    return documentation;
  }
  constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_exports() const -> Perimortem::Core::View::Vector<Export> {
    return exports;
  }
  constexpr auto get_types() const
      -> Perimortem::Core::View::Vector<const Ttx::Type*> {
    return types;
  }
  constexpr auto is_valid() const -> Bool { return !name.is_empty(); }
  auto find_type(Perimortem::Core::View::Bytes name) const -> const Ttx::Type*;

 private:
  Ttx::Documentation documentation;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Vector<Export> exports;
  Perimortem::Core::View::Vector<const Ttx::Type*> types;
};

}  // namespace Ttx::Dialect::Package
