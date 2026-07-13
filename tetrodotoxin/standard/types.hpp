// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "tetrodotoxin/archiver/version.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Standard {

// Types is the Tetrodotoxin standard surface that every ISA context can see.
//
// These are TTX Type objects, but the table itself is not part of the TTX
// language model. It carries standard-library identity and lowering metadata
// such as `cpp` and `abi`, so Tetrodotoxin owns it instead of TTX.
class Types {
 public:
  static constexpr auto get_types()
      -> Perimortem::Core::View::Vector<const Ttx::Type*> {
    return types.get_view();
  }

  static auto get_version() -> Tetrodotoxin::Archiver::Version;

  static constexpr auto find_type(Perimortem::Core::View::Bytes name)
      -> const Ttx::Type* {
    for (Count i = 0; i < types.get_size(); i++) {
      if (types[i]->get_name() == name) {
        return types[i];
      }
    }

    return nullptr;
  }

  static constexpr auto is_type(Perimortem::Core::View::Bytes name) -> Bool {
    return find_type(name) != nullptr;
  }

 private:
  static constexpr auto hash_bytes(
      Bits_64 hash,
      Perimortem::Core::View::Bytes bytes) -> Bits_64 {
    return Perimortem::Core::Hash(hash).Rehash(
        Perimortem::Core::Hash(bytes).get_value());
  }

  static constexpr auto hash_member(Bits_64 hash, const Ttx::Member& member)
      -> Bits_64 {
    hash = hash_bytes(hash, member.get_name());
    return hash_bytes(hash, member.get_type().get_name());
  }

  static constexpr auto hash_layout(Bits_64 hash, Ttx::Layout layout)
      -> Bits_64 {
    Perimortem::Core::View::Vector<Ttx::Member> members = layout.get_members();
    for (Count i = 0; i < members.get_size(); i++) {
      hash = hash_member(hash, members[i]);
    }

    return hash;
  }

  static constexpr auto hash_function(
      Bits_64 hash,
      const Ttx::Function& function) -> Bits_64 {
    hash = hash_bytes(hash, function.get_name());
    hash = hash_layout(hash, function.get_parameters());
    return hash_layout(hash, function.get_result());
  }

  static constexpr auto hash_type(Bits_64 hash, const Ttx::Type& type)
      -> Bits_64 {
    hash = hash_bytes(hash, type.get_name());
    if (type.get_alias_parent() != nullptr) {
      hash = hash_bytes(hash, type.get_alias_parent()->get_name());
    }

    Perimortem::Core::View::Vector<Ttx::Attribute> attributes =
        type.get_attributes();
    for (Count i = 0; i < attributes.get_size(); i++) {
      hash = hash_bytes(hash, attributes[i].get_key());
      hash = hash_bytes(hash, attributes[i].get_value());
    }

    Perimortem::Core::View::Vector<Ttx::Member> members = type.get_members();
    for (Count i = 0; i < members.get_size(); i++) {
      hash = hash_member(hash, members[i]);
    }

    Perimortem::Core::View::Vector<const Ttx::Type*> nested = type.get_types();
    for (Count i = 0; i < nested.get_size(); i++) {
      hash =
          nested[i] == nullptr ? hash : hash_bytes(hash, nested[i]->get_name());
    }

    Perimortem::Core::View::Vector<Ttx::Function> functions =
        type.get_functions();
    for (Count i = 0; i < functions.get_size(); i++) {
      hash = hash_function(hash, functions[i]);
    }

    return hash;
  }

  static constexpr auto compute_version() -> Tetrodotoxin::Archiver::Version {
    Bits_64 high =
        Perimortem::Core::Hash("Tetrodotoxin.Standard.Types"_view).get_value();
    for (Count i = 0; i < types.get_size(); i++) {
      high = hash_type(high, *types[i]);
    }

    Bits_64 low =
        Perimortem::Core::Hash("Tetrodotoxin.Standard.Types.Version"_view)
            .Rehash(high);
    return Tetrodotoxin::Archiver::Version(high, low);
  }

  static constexpr Perimortem::Core::Static::Vector<Ttx::Attribute, 2>
      void_attributes = {{
        {"cpp"_view, "void"_view},
        {"abi"_view, "void"_view},
      }};
  static constexpr Perimortem::Core::Static::Vector<Ttx::Attribute, 2>
      view_bytes_attributes = {{
        {"cpp"_view, "Perimortem::Core::View::Bytes"_view},
        {"abi"_view, "view_bytes"_view},
      }};
  static constexpr Perimortem::Core::Static::Vector<Ttx::Attribute, 2>
      count_attributes = {{
        {"cpp"_view, "Count"_view},
        {"abi"_view, "integer"_view},
      }};
  static constexpr Perimortem::Core::Static::Vector<Ttx::Attribute, 2>
      bits_64_attributes = {{
        {"cpp"_view, "Bits_64"_view},
        {"abi"_view, "integer"_view},
      }};
  static constexpr Perimortem::Core::Static::Vector<Ttx::Attribute, 2>
      signed_64_attributes = {{
        {"cpp"_view, "Signed_64"_view},
        {"abi"_view, "integer"_view},
      }};
  static constexpr Perimortem::Core::Static::Vector<Ttx::Attribute, 2>
      real_64_attributes = {{
        {"cpp"_view, "Real_64"_view},
        {"abi"_view, "real"_view},
      }};
  static constexpr Perimortem::Core::Static::Vector<Ttx::Attribute, 2>
      bool_attributes = {{
        {"cpp"_view, "Bool"_view},
        {"abi"_view, "integer"_view},
      }};
  static constexpr Ttx::Type void_type =
      Ttx::Type("Void"_view, void_attributes.get_view());
  static constexpr Ttx::Type bool_type =
      Ttx::Type("Bool"_view, bool_attributes.get_view());
  static constexpr Ttx::Type bits_8_type = Ttx::Type("Bits_8"_view);
  static constexpr Ttx::Type bits_16_type = Ttx::Type("Bits_16"_view);
  static constexpr Ttx::Type bits_32_type = Ttx::Type("Bits_32"_view);
  static constexpr Ttx::Type bits_64_type =
      Ttx::Type("Bits_64"_view, bits_64_attributes.get_view());
  static constexpr Ttx::Type count_type = Ttx::Type::alias(
      "Count"_view,
      bits_64_type,
      {},
      count_attributes.get_view());
  static constexpr Ttx::Type signed_8_type = Ttx::Type("Signed_8"_view);
  static constexpr Ttx::Type signed_16_type = Ttx::Type("Signed_16"_view);
  static constexpr Ttx::Type signed_32_type = Ttx::Type("Signed_32"_view);
  static constexpr Ttx::Type signed_64_type =
      Ttx::Type("Signed_64"_view, signed_64_attributes.get_view());
  static constexpr Ttx::Type real_32_type = Ttx::Type("Real_32"_view);
  static constexpr Ttx::Type real_64_type =
      Ttx::Type("Real_64"_view, real_64_attributes.get_view());
  static constexpr Ttx::Type bytes_type = Ttx::Type("Bytes"_view);
  static constexpr Ttx::Type string_type = Ttx::Type("String"_view);
  static constexpr Ttx::Type vec_type = Ttx::Type("Vec"_view);
  static constexpr Ttx::Type view_type = Ttx::Type("View"_view);
  static constexpr Ttx::Type view_bytes_type =
      Ttx::Type("View[Bytes]"_view, view_bytes_attributes.get_view());
  static constexpr Ttx::Type access_type = Ttx::Type("Access"_view);
  static constexpr Ttx::Type list_type = Ttx::Type("List"_view);
  static constexpr Ttx::Type dict_type = Ttx::Type("Dict"_view);
  static constexpr Ttx::Type action_type = Ttx::Type("Action"_view);

  static constexpr Perimortem::Core::Static::Vector<Ttx::Member, 4>
      vector_members = {{
        Ttx::Member("x"_view, real_32_type),
        Ttx::Member("y"_view, real_32_type),
        Ttx::Member("z"_view, real_32_type),
        Ttx::Member("w"_view, real_32_type),
      }};

  static constexpr Ttx::Type vec2d_type = {
    "Vec2D"_view,
    vector_members.slice(0, 2),
  };
  static constexpr Ttx::Type vec3d_type = {
    "Vec3D"_view,
    vector_members.slice(0, 3),
  };
  static constexpr Ttx::Type vec4d_type = {
    "Vec4D"_view,
    vector_members.slice(0, 4),
  };
  static constexpr Perimortem::Core::Static::Vector types = {{
    &void_type,       &bool_type,      &count_type,     &bits_8_type,
    &bits_16_type,    &bits_32_type,   &bits_64_type,   &signed_8_type,
    &signed_16_type,  &signed_32_type, &signed_64_type, &real_32_type,
    &real_64_type,    &bytes_type,     &string_type,    &vec_type,
    &vec2d_type,      &vec3d_type,     &vec4d_type,     &view_type,
    &view_bytes_type, &access_type,    &list_type,      &dict_type,
    &action_type,
  }};
};

inline auto Types::get_version() -> Tetrodotoxin::Archiver::Version {
  // Keep the version content-derived without recomputing the built-in type
  // tree on every package archive read/write.
  static constexpr auto version = compute_version();
  return version;
}

}  // namespace Tetrodotoxin::Standard
