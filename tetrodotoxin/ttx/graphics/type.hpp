// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/ttx/core/type.hpp"
#include "tetrodotoxin/ttx/facts.hpp"

namespace Tetrodotoxin::Ttx {

class Graphics {
 private:
  using LayoutMapping =
      Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Layout>;
  using TypeMapping =
      Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Type>;
  inline static constexpr Perimortem::Core::Static::Vector<Field, 4>
      color_fields = {
        Field{"r"_view, "Real_32"_view, Core::find_layout("Real_32"_view), 0},
        Field{"g"_view, "Real_32"_view, Core::find_layout("Real_32"_view), 4},
        Field{"b"_view, "Real_32"_view, Core::find_layout("Real_32"_view), 8},
        Field{"a"_view, "Real_32"_view, Core::find_layout("Real_32"_view), 12},
  };

  inline static constexpr Perimortem::Core::Static::Vector<LayoutMapping, 1>
      layouts = {{
        {"Color"_view,
         {Layout::Kind::Concrete, 16, 16, color_fields.get_view()}},
      }};

  using LayoutTable = Perimortem::Utility::Table<Layout, layouts>;

  inline static constexpr Perimortem::Core::Static::Vector<Field, 1>
      sample_args = {
        Field{
          Perimortem::Core::View::Bytes(), "Vec2D"_view,
          Core::find_layout("Vec2D"_view), 0},
  };

  inline static constexpr Layout sample_args_layout = {
    Layout::Kind::Fluid,
    8,
    8,
    sample_args.get_view(),
  };

  inline static constexpr Perimortem::Core::Static::Vector<Method, 1>
      sampler_2d_methods = {
        Method{
          "sample"_view,
          &sample_args_layout,
          "Color"_view,
          LayoutTable::find_or_null("Color"_view),
        },
  };

  inline static constexpr Layout sampler_2d_layout = {
    Layout::Kind::Concrete,
    0,
    1,
    Perimortem::Core::View::Vector<Field>(),
  };

  inline static constexpr Perimortem::Core::Static::Vector<TypeMapping, 2>
      types = {{
        {"Color"_view,
         {"Color"_view, Type::Class::Color,
          LayoutTable::find_or_null("Color"_view)}},
        {"Sampler_2D"_view,
         {"Sampler_2D"_view, Type::Class::Sampler2D, &sampler_2d_layout,
          sampler_2d_methods.get_view()}},
      }};

  using TypeTable = Perimortem::Utility::Table<Type, types>;

 public:
  static constexpr auto find_layout(Perimortem::Core::View::Bytes name)
      -> const Layout* {
    return LayoutTable::find_or_null(name);
  }

  static constexpr auto find_type(Perimortem::Core::View::Bytes name)
      -> const Type* {
    return TypeTable::find_or_null(name);
  }

  static constexpr auto is_type(Perimortem::Core::View::Bytes name) -> Bool {
    return find_type(name) != nullptr;
  }

  static constexpr auto type_class(Perimortem::Core::View::Bytes name)
      -> Type::Class {
    const Type* type = find_type(name);
    return type ? type->get_class() : Type::Class::Unknown;
  }

  static constexpr auto find_field(
      Perimortem::Core::View::Bytes type_name,
      Perimortem::Core::View::Bytes field_name) -> const Field* {
    const Type* type = find_type(type_name);
    return type ? type->find_field(field_name) : nullptr;
  }

  static constexpr auto find_method(
      Perimortem::Core::View::Bytes receiver_type,
      Perimortem::Core::View::Bytes name) -> const Method* {
    const Type* type = find_type(receiver_type);
    return type ? type->find_method(name) : nullptr;
  }

  static auto type_layout(const TypeQuery& type) -> Layout;
};

}  // namespace Tetrodotoxin::Ttx
