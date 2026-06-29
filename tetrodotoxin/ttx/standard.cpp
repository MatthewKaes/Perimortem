// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/ttx/standard.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/ttx/core/type.hpp"
#include "tetrodotoxin/ttx/graphics/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Ttx;

static constexpr StandardPackage invalid_package = {};

using StandardPackageMapping = Pair<View::Bytes, StandardPackage>;
static constexpr Static::Vector<StandardPackageMapping, 4> standard_packages = {
  {
    {
      "Core"_view,
      {
        "Core"_view,
        True,
        False,
        Tetrodotoxin::Ttx::Core::is_type,
        Tetrodotoxin::Ttx::Core::find_type,
        Tetrodotoxin::Ttx::Core::type_layout,
      },
    },
    {
      "Graphics"_view,
      {
        "Graphics"_view,
        True,
        False,
        Tetrodotoxin::Ttx::Graphics::is_type,
        Tetrodotoxin::Ttx::Graphics::find_type,
        Tetrodotoxin::Ttx::Graphics::type_layout,
      },
    },
    {
      "Log"_view,
      {
        "Log"_view,
        True,
        False,
        nullptr,
        nullptr,
        nullptr,
      },
    },
    {
      "Math"_view,
      {
        "Math"_view,
        True,
        False,
        nullptr,
        nullptr,
        nullptr,
      },
    },
  }};

using StandardPackageTable = Table<StandardPackage, standard_packages>;

auto StandardPackage::is_type(View::Bytes type_name) const -> Bool {
  return exists() && is_type_fn && is_type_fn(type_name);
}

auto StandardPackage::find_type(View::Bytes type_name) const -> const Type* {
  return exists() && find_type_fn ? find_type_fn(type_name) : nullptr;
}

auto StandardPackage::type_class(View::Bytes type_name) const -> Type::Class {
  const Type* type = find_type(type_name);
  return type ? type->get_class() : Type::Class::Unknown;
}

auto StandardPackage::find_field(View::Bytes type_name, View::Bytes field_name)
    const -> const Field* {
  const Type* type = find_type(type_name);
  return type ? type->find_field(field_name) : nullptr;
}

auto StandardPackage::find_method(
    View::Bytes receiver_type,
    View::Bytes method_name) const -> const Method* {
  const Type* type = find_type(receiver_type);
  return type ? type->find_method(method_name) : nullptr;
}

auto StandardPackage::type_layout(const Syntax::Type::Ref& type) const
    -> Layout {
  return exists() && type_layout_fn ? type_layout_fn(type) : Layout();
}

auto Tetrodotoxin::Ttx::implicit_core_package() -> StandardPackage {
  return find_standard_package("Core"_view).as_implicit();
}

auto Tetrodotoxin::Ttx::find_standard_package(View::Bytes name)
    -> StandardPackage {
  return StandardPackageTable::find_or_default(name, invalid_package);
}

auto Tetrodotoxin::Ttx::find_standard_package(View::Vector<View::Bytes> path)
    -> StandardPackage {
  if (path.get_size() != 2 || path[0] != "TTX"_view) {
    return invalid_package;
  }

  return find_standard_package(path[1]);
}

auto Tetrodotoxin::Ttx::is_standard_type(View::Bytes type_name) -> Bool {
  return Tetrodotoxin::Ttx::Core::is_type(type_name) ||
         Tetrodotoxin::Ttx::Graphics::is_type(type_name);
}
