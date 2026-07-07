// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

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

  static constexpr Ttx::Type void_type =
      Ttx::Type("Void"_view, void_attributes.get_view());
  static constexpr Ttx::Type bool_type = Ttx::Type("Bool"_view);
  static constexpr Ttx::Type count_type = Ttx::Type("Count"_view);
  static constexpr Ttx::Type bits_8_type = Ttx::Type("Bits_8"_view);
  static constexpr Ttx::Type bits_16_type = Ttx::Type("Bits_16"_view);
  static constexpr Ttx::Type bits_32_type = Ttx::Type("Bits_32"_view);
  static constexpr Ttx::Type bits_64_type = Ttx::Type("Bits_64"_view);
  static constexpr Ttx::Type signed_8_type = Ttx::Type("Signed_8"_view);
  static constexpr Ttx::Type signed_16_type = Ttx::Type("Signed_16"_view);
  static constexpr Ttx::Type signed_32_type = Ttx::Type("Signed_32"_view);
  static constexpr Ttx::Type signed_64_type = Ttx::Type("Signed_64"_view);
  static constexpr Ttx::Type real_32_type = Ttx::Type("Real_32"_view);
  static constexpr Ttx::Type real_64_type = Ttx::Type("Real_64"_view);
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

  static constexpr Perimortem::Core::Static::Vector<Ttx::Type::Member, 4>
      vector_members = {{
        {"x"_view, real_32_type},
        {"y"_view, real_32_type},
        {"z"_view, real_32_type},
        {"w"_view, real_32_type},
      }};

  static constexpr Perimortem::Core::Static::Vector<Ttx::Type::Member, 2>
      size_2d_members = {{
        {"width"_view, bits_32_type},
        {"height"_view, bits_32_type},
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
  static constexpr Ttx::Type size2d_type = {
    "Size2D"_view,
    size_2d_members.get_view(),
  };

  static constexpr Perimortem::Core::Static::Vector types = {{
    &void_type,      &bool_type,       &count_type,     &bits_8_type,
    &bits_16_type,   &bits_32_type,    &bits_64_type,   &signed_8_type,
    &signed_16_type, &signed_32_type,  &signed_64_type, &real_32_type,
    &real_64_type,   &bytes_type,      &string_type,    &vec_type,
    &vec2d_type,     &vec3d_type,      &vec4d_type,     &size2d_type,
    &view_type,      &view_bytes_type, &access_type,    &list_type,
    &dict_type,      &action_type,
  }};
};

}  // namespace Tetrodotoxin::Standard
