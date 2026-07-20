// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/uuid.hpp"

#include "perimortem/utility/table.hpp"

#include "ttx/type.hpp"

namespace Tetrodotoxin::Standard {

// Types is the Tetrodotoxin standard surface that every Dialect context can see.
//
// The standard surface is a condensed version of TTX Type objects that compiles
// to a `clang` friendly package like buffer. The table itself is not part of
// the TTX language model but it's a useful terminal format for Dialects to consume
// since it's mostly stable and ships with the standard toolchain. Concrete
// identities carry their ABI lowering; generic roots are dispatch identities
// whose concrete representations are produced by parameterization.
class Types {
 public:
  static constexpr auto get_types()
      -> Perimortem::Core::View::Vector<const Ttx::Type*> {
    return TypeTable::get_values();
  }

  static constexpr auto find_type(Perimortem::Core::View::Bytes name)
      -> const Ttx::Type& {
    return *TypeTable::find_or_default(name, &Ttx::Type::invalid());
  }

  static constexpr auto get_version() -> Perimortem::System::Uuid {
    constexpr Unsigned_64 major = 1;
    constexpr Unsigned_64 minor = 13;
    return Perimortem::System::Uuid(major, minor);
  }

 private:
  static constexpr TypeStorage unsigned_64_type = {
    "Unsigned_64"_view,
    Tetrodotoxin::Abi::Lowering::Unsigned_64,
  };
  static constexpr TypeStorage real_32_type = {
    "Real_32"_view,
    Tetrodotoxin::Abi::Lowering::Real_32,
  };
  static constexpr TypeStorage count_type = {
    "Count"_view,
    Tetrodotoxin::Abi::Lowering::Unsigned_64,
    unsigned_64_type.get_type(),
  };

  static constexpr Perimortem::Core::Static::Vector<Ttx::Member, 4>
      vector_members = {{
        Ttx::Member("x"_view, real_32_type.get_type()),
        Ttx::Member("y"_view, real_32_type.get_type()),
        Ttx::Member("z"_view, real_32_type.get_type()),
        Ttx::Member("w"_view, real_32_type.get_type()),
      }};

  static constexpr TypeStorage vec2d_type = {
    "Vec2D"_view,
    Tetrodotoxin::Abi::Lowering::Bytes,
    vector_members.slice(0, 2),
  };
  static constexpr TypeStorage vec3d_type = {
    "Vec3D"_view,
    Tetrodotoxin::Abi::Lowering::Bytes,
    vector_members.slice(0, 3),
  };
  static constexpr TypeStorage vec4d_type = {
    "Vec4D"_view,
    Tetrodotoxin::Abi::Lowering::Bytes,
    vector_members.slice(0, 4),
  };

  static constexpr TypeStorage type_storage[] = {
    {"Void"_view, Tetrodotoxin::Abi::Lowering::Void},
    {"Bool"_view, Tetrodotoxin::Abi::Lowering::Bool},
    {"Unsigned_8"_view, Tetrodotoxin::Abi::Lowering::Unsigned_8},
    {"Unsigned_16"_view, Tetrodotoxin::Abi::Lowering::Unsigned_16},
    {"Unsigned_32"_view, Tetrodotoxin::Abi::Lowering::Unsigned_32},
    {"Signed_8"_view, Tetrodotoxin::Abi::Lowering::Signed_8},
    {"Signed_16"_view, Tetrodotoxin::Abi::Lowering::Signed_16},
    {"Signed_32"_view, Tetrodotoxin::Abi::Lowering::Signed_32},
    {"Signed_64"_view, Tetrodotoxin::Abi::Lowering::Signed_64},
    {"Real_64"_view, Tetrodotoxin::Abi::Lowering::Real_64},
    {"Bytes"_view, Tetrodotoxin::Abi::Lowering::Bytes},
    {"String"_view, Tetrodotoxin::Abi::Lowering::Bytes},
    {"Action"_view, Tetrodotoxin::Abi::Lowering::Bytes},
    {"Type"_view, Tetrodotoxin::Abi::Lowering::Bytes},
  };

  static constexpr Ttx::Type parameterized_roots[] = {
    Ttx::Type("Vec"_view),  Ttx::Type("View"_view), Ttx::Type("Access"_view),
    Ttx::Type("List"_view), Ttx::Type("Dict"_view),
  };

  using TypeEntry = Perimortem::Utility::
      Pair<Perimortem::Core::View::Bytes, const Ttx::Type*>;
  static constexpr Perimortem::Core::Static::Vector<TypeEntry, 25> type_source =
      {{
        {"Void"_view, &type_storage[0].get_type()},
        {"Bool"_view, &type_storage[1].get_type()},
        {"Count"_view, &count_type.get_type()},
        {"Unsigned_8"_view, &type_storage[2].get_type()},
        {"Unsigned_16"_view, &type_storage[3].get_type()},
        {"Unsigned_32"_view, &type_storage[4].get_type()},
        {"Unsigned_64"_view, &unsigned_64_type.get_type()},
        {"Signed_8"_view, &type_storage[5].get_type()},
        {"Signed_16"_view, &type_storage[6].get_type()},
        {"Signed_32"_view, &type_storage[7].get_type()},
        {"Signed_64"_view, &type_storage[8].get_type()},
        {"Real_32"_view, &real_32_type.get_type()},
        {"Real_64"_view, &type_storage[9].get_type()},
        {"Bytes"_view, &type_storage[10].get_type()},
        {"String"_view, &type_storage[11].get_type()},
        {"Vec"_view, &parameterized_roots[0]},
        {"Vec2D"_view, &vec2d_type.get_type()},
        {"Vec3D"_view, &vec3d_type.get_type()},
        {"Vec4D"_view, &vec4d_type.get_type()},
        {"View"_view, &parameterized_roots[1]},
        {"Access"_view, &parameterized_roots[2]},
        {"List"_view, &parameterized_roots[3]},
        {"Dict"_view, &parameterized_roots[4]},
        {"Action"_view, &type_storage[12].get_type()},
        {"Type"_view, &type_storage[13].get_type()},
      }};

  using TypeTable = Perimortem::Utility::Table<const Ttx::Type*, type_source>;
};

}  // namespace Tetrodotoxin::Standard
