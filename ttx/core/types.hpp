// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/type.hpp"

namespace Ttx::Core {

class Types {
 public:
  static constexpr auto get_types()
      -> Perimortem::Core::View::Vector<const Type*> {
    return types.get_view();
  }

  static constexpr auto find_type(Perimortem::Core::View::Bytes name)
      -> const Type* {
    for (Count type_index = 0; type_index < types.get_size(); type_index++) {
      if (types[type_index]->get_name() == name) {
        return types[type_index];
      }
    }

    return nullptr;
  }

  static constexpr auto is_type(Perimortem::Core::View::Bytes name) -> Bool {
    return find_type(name) != nullptr;
  }

 private:
  static constexpr Type void_type = Type("Void"_view);
  static constexpr Type bool_type = Type("Bool"_view);
  static constexpr Type count_type = Type("Count"_view);
  static constexpr Type bits_8_type = Type("Bits_8"_view);
  static constexpr Type bits_16_type = Type("Bits_16"_view);
  static constexpr Type bits_32_type = Type("Bits_32"_view);
  static constexpr Type bits_64_type = Type("Bits_64"_view);
  static constexpr Type signed_8_type = Type("Signed_8"_view);
  static constexpr Type signed_16_type = Type("Signed_16"_view);
  static constexpr Type signed_32_type = Type("Signed_32"_view);
  static constexpr Type signed_64_type = Type("Signed_64"_view);
  static constexpr Type real_32_type = Type("Real_32"_view);
  static constexpr Type real_64_type = Type("Real_64"_view);
  static constexpr Type bytes_type = Type("Bytes"_view);
  static constexpr Type string_type = Type("String"_view);
  static constexpr Type vec_type = Type("Vec"_view);
  static constexpr Type view_type = Type("View"_view);
  static constexpr Type access_type = Type("Access"_view);
  static constexpr Type list_type = Type("List"_view);
  static constexpr Type dict_type = Type("Dict"_view);
  static constexpr Type action_type = Type("Action"_view);

  static constexpr Perimortem::Core::Static::Vector<Type::Member, 4>
      vector_members = {{
        {"x"_view, real_32_type},
        {"y"_view, real_32_type},
        {"z"_view, real_32_type},
        {"w"_view, real_32_type},
      }};

  static constexpr Perimortem::Core::Static::Vector<Type::Member, 2>
      size_2d_members = {{
        {"width"_view, real_32_type},
        {"height"_view, real_32_type},
      }};

  static constexpr Type vec2d_type = {
    "Vec2D"_view,
    vector_members.slice(0, 2),
  };
  static constexpr Type vec3d_type = {
    "Vec3D"_view,
    vector_members.slice(0, 3),
  };
  static constexpr Type vec4d_type = {
    "Vec4D"_view,
    vector_members.slice(0, 4),
  };
  static constexpr Type size2d_type = {
    "Size2D"_view,
    size_2d_members.get_view(),
  };

  static constexpr Perimortem::Core::Static::Vector types = {{
    &void_type,      &bool_type,      &count_type,     &bits_8_type,
    &bits_16_type,   &bits_32_type,   &bits_64_type,   &signed_8_type,
    &signed_16_type, &signed_32_type, &signed_64_type, &real_32_type,
    &real_64_type,   &bytes_type,     &string_type,    &vec_type,
    &vec2d_type,     &vec3d_type,     &vec4d_type,     &size2d_type,
    &view_type,      &access_type,    &list_type,      &dict_type,
    &action_type,
  }};
};

}  // namespace Ttx::Core
