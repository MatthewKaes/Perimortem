// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/ttx/facts.hpp"

namespace Tetrodotoxin::Ttx {

class Core {
 private:
  using LayoutMapping =
      Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Layout>;

  inline static constexpr Perimortem::Core::Static::Vector<LayoutMapping, 13>
      primitive_layouts = {{
        {"Void"_view, {Layout::Kind::Concrete, 0, 1}},
        {"Bool"_view, {Layout::Kind::Concrete, 1, 1}},
        {"Count"_view, {Layout::Kind::Concrete, 8, 8}},
        {"Bits_8"_view, {Layout::Kind::Concrete, 1, 1}},
        {"Bits_16"_view, {Layout::Kind::Concrete, 2, 2}},
        {"Bits_32"_view, {Layout::Kind::Concrete, 4, 4}},
        {"Bits_64"_view, {Layout::Kind::Concrete, 8, 8}},
        {"Signed_8"_view, {Layout::Kind::Concrete, 1, 1}},
        {"Signed_16"_view, {Layout::Kind::Concrete, 2, 2}},
        {"Signed_32"_view, {Layout::Kind::Concrete, 4, 4}},
        {"Signed_64"_view, {Layout::Kind::Concrete, 8, 8}},
        {"Real_32"_view, {Layout::Kind::Concrete, 4, 4}},
        {"Real_64"_view, {Layout::Kind::Concrete, 8, 8}},
      }};

  using PrimitiveLayoutTable =
      Perimortem::Utility::Table<Layout, primitive_layouts>;

  using TypeMapping =
      Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Type>;

  inline static constexpr Perimortem::Core::Static::Vector<TypeMapping, 13>
      primitive_types = {{
        {"Void"_view,
         {"Void"_view, Type::Class::Void,
          PrimitiveLayoutTable::find_or_null("Void"_view)}},
        {"Bool"_view,
         {"Bool"_view, Type::Class::Bool,
          PrimitiveLayoutTable::find_or_null("Bool"_view)}},
        {"Count"_view,
         {"Count"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Count"_view)}},
        {"Bits_8"_view,
         {"Bits_8"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Bits_8"_view)}},
        {"Bits_16"_view,
         {"Bits_16"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Bits_16"_view)}},
        {"Bits_32"_view,
         {"Bits_32"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Bits_32"_view)}},
        {"Bits_64"_view,
         {"Bits_64"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Bits_64"_view)}},
        {"Signed_8"_view,
         {"Signed_8"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Signed_8"_view)}},
        {"Signed_16"_view,
         {"Signed_16"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Signed_16"_view)}},
        {"Signed_32"_view,
         {"Signed_32"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Signed_32"_view)}},
        {"Signed_64"_view,
         {"Signed_64"_view, Type::Class::Integer,
          PrimitiveLayoutTable::find_or_null("Signed_64"_view)}},
        {"Real_32"_view,
         {"Real_32"_view, Type::Class::Real,
          PrimitiveLayoutTable::find_or_null("Real_32"_view)}},
        {"Real_64"_view,
         {"Real_64"_view, Type::Class::Real,
          PrimitiveLayoutTable::find_or_null("Real_64"_view)}},
      }};

  using PrimitiveTypeTable = Perimortem::Utility::Table<Type, primitive_types>;

  inline static constexpr Perimortem::Core::Static::Vector<Field, 4>
      vectorized_fields = {
        Field{
          "x"_view, "Real_32"_view,
          PrimitiveLayoutTable::find_or_null("Real_32"_view), 0},
        Field{
          "y"_view, "Real_32"_view,
          PrimitiveLayoutTable::find_or_null("Real_32"_view), 4},
        Field{
          "z"_view, "Real_32"_view,
          PrimitiveLayoutTable::find_or_null("Real_32"_view), 8},
        Field{
          "w"_view, "Real_32"_view,
          PrimitiveLayoutTable::find_or_null("Real_32"_view), 12},
  };

  inline static constexpr Perimortem::Core::Static::Vector<LayoutMapping, 3>
      vector_layouts = {{
        {"Vec2D"_view,
         {Layout::Kind::Concrete, 8, 8, vectorized_fields.slice(0, 2)}},
        {"Vec3D"_view,
         {Layout::Kind::Concrete, 12, 16, vectorized_fields.slice(0, 3)}},
        {"Vec4D"_view,
         {Layout::Kind::Concrete, 16, 16, vectorized_fields.slice(0, 4)}},
      }};

  using VectorLayoutTable = Perimortem::Utility::Table<Layout, vector_layouts>;

  inline static constexpr Perimortem::Core::Static::Vector<TypeMapping, 11>
      types = {{
        {"Bytes"_view, {"Bytes"_view, Type::Class::Bytes}},
        {"String"_view, {"String"_view, Type::Class::String}},
        {"Vec"_view, {"Vec"_view, Type::Class::Vector}},
        {"Vec2D"_view,
         {"Vec2D"_view, Type::Class::Vector,
          VectorLayoutTable::find_or_null("Vec2D"_view)}},
        {"Vec3D"_view,
         {"Vec3D"_view, Type::Class::Vector,
          VectorLayoutTable::find_or_null("Vec3D"_view)}},
        {"Vec4D"_view,
         {"Vec4D"_view, Type::Class::Vector,
          VectorLayoutTable::find_or_null("Vec4D"_view)}},
        {"View"_view, {"View"_view, Type::Class::View}},
        {"Access"_view, {"Access"_view, Type::Class::Access}},
        {"List"_view, {"List"_view, Type::Class::List}},
        {"Dict"_view, {"Dict"_view, Type::Class::Dict}},
        {"Action"_view, {"Action"_view, Type::Class::Action}},
      }};

  using TypeTable = Perimortem::Utility::Table<Type, types>;

 public:
  static constexpr auto find_layout(Perimortem::Core::View::Bytes name)
      -> const Layout* {
    if (const Layout* layout = PrimitiveLayoutTable::find_or_null(name)) {
      return layout;
    }

    return VectorLayoutTable::find_or_null(name);
  }

  static constexpr auto find_type(Perimortem::Core::View::Bytes name)
      -> const Type* {
    if (const Type* primitive = PrimitiveTypeTable::find_or_null(name)) {
      return primitive;
    }

    return TypeTable::find_or_null(name);
  }

  static constexpr auto find_field(
      Perimortem::Core::View::Bytes type_name,
      Perimortem::Core::View::Bytes field_name) -> const Field* {
    const Type* type = find_type(type_name);
    return type ? type->find_field(field_name) : nullptr;
  }

  static constexpr auto is_type(Perimortem::Core::View::Bytes name) -> Bool {
    return find_type(name) != nullptr;
  }

  static constexpr auto type_class(Perimortem::Core::View::Bytes name)
      -> Type::Class {
    const Type* type = find_type(name);
    return type ? type->get_class() : Type::Class::Unknown;
  }

  static constexpr auto is_numeric(Perimortem::Core::View::Bytes name) -> Bool {
    const Type* type = find_type(name);
    return type && type->is_numeric();
  }

  static constexpr auto is_real(Perimortem::Core::View::Bytes name) -> Bool {
    const Type* type = find_type(name);
    return type && type->is_real();
  }

  static constexpr auto type_slot_count(Perimortem::Core::View::Bytes name)
      -> Count {
    const Type* type = find_type(name);
    return type ? type->get_field_count() : 0;
  }

  static auto type_layout(const TypeQuery& type) -> Layout;
};

}  // namespace Tetrodotoxin::Ttx
