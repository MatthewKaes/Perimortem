// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryDefaults = {
  .name = "Tetrodotoxin::Library defaults"_view,
};

class ForeignUnsigned : public Ttx::Model::Types::Unsigned {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "ForeignUnsigned"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  constexpr auto get_width() const -> Count override { return 8; }
  constexpr auto get_size() const -> Count override { return 1; }
  constexpr auto get_alignment() const -> Count override { return 1; }
};

static auto import_types(
    Tetrodotoxin::Environment::Workspace& workspace,
    Ttx::Lexical::Errors& errors) -> Option<Monograph&> {
  static constexpr View::Bytes source =
      "// Default rejection types.\n"
      "dialect : Library;\n"
      "public Packet : struct {}\n"
      "public Session : object {}\n"
      "public Mode : enum[Unsigned_8] { ready = 1; }"_view;
  auto imported = workspace.interpret_source(
      errors, "Defaults"_view, "defaults.ttx"_view, source);
  if (!imported || !imported->is<Monograph>() || !workspace.link(errors) ||
      !workspace.finalize(errors)) {
    return {};
  }

  return static_cast<Monograph&>(*imported);
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, scalar_payloads_and_identity) {
  Allocator::Arena domain;
  const Static::Vector<const Ttx::Model::Type*, 4> unsigned_types = {{
    &Dialect::get_unsigned_8(),
    &Dialect::get_unsigned_16(),
    &Dialect::get_unsigned_32(),
    &Dialect::get_unsigned_64(),
  }};
  const Static::Vector<const Ttx::Model::Type*, 4> signed_types = {{
    &Dialect::get_signed_8(),
    &Dialect::get_signed_16(),
    &Dialect::get_signed_32(),
    &Dialect::get_signed_64(),
  }};
  const Static::Vector<const Ttx::Model::Type*, 2> real_types = {{
    &Dialect::get_real_32(),
    &Dialect::get_real_64(),
  }};

  auto boolean = Dialect::create_default(domain, Dialect::get_bool());
  ASSERT(boolean && boolean->is<Constants::False>());
  EXPECT(&boolean->get_type() == &Dialect::get_bool());
  EXPECT_NOT(static_cast<const Constants::False&>(*boolean).get_value());

  for (Count i = 0; i < unsigned_types.get_size(); i++) {
    const Ttx::Model::Type* type = unsigned_types.get_data()[i];
    auto created = Dialect::create_default(domain, *type);
    ASSERT(created && created->is<Constants::Unsigned>());
    const auto& value = static_cast<const Constants::Unsigned&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), Unsigned_64(0));
  }

  for (Count i = 0; i < signed_types.get_size(); i++) {
    const Ttx::Model::Type* type = signed_types.get_data()[i];
    auto created = Dialect::create_default(domain, *type);
    ASSERT(created && created->is<Constants::Signed>());
    const auto& value = static_cast<const Constants::Signed&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), Signed_64(0));
  }

  for (Count i = 0; i < real_types.get_size(); i++) {
    const Ttx::Model::Type* type = real_types.get_data()[i];
    auto created = Dialect::create_default(domain, *type);
    ASSERT(created && created->is<Constants::Real>());
    const auto& value = static_cast<const Constants::Real&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), Real_64(0));
  }
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, materialized_view_is_empty) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(Dialect::get_unsigned_8()),
  }};
  auto type = materializations.materialize(
      Generics::View::get_formula(), arguments.get_view());
  ASSERT(type && type->is<Types::View>());

  auto created = Dialect::create_default(domain, *type);
  ASSERT(created && created->is<Constants::Bytes>());
  const auto& value = static_cast<const Constants::Bytes&>(*created);
  EXPECT(&value.get_type() == &*type);
  EXPECT(value.get_value().is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, unsupported_domains_are_absent) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  const Abstract& structure = source_type.resolve_context("Packet"_view);
  const Abstract& object = source_type.resolve_context("Session"_view);
  const Abstract& enumeration = source_type.resolve_context("Mode"_view);
  ASSERT(structure.is<Types::Structure>());
  ASSERT(object.is<Types::Object>());
  ASSERT(enumeration.is<Types::Enumeration>());

  ForeignUnsigned foreign_unsigned;
  Types::Range range("Range[Unsigned_8]"_view, Dialect::get_unsigned_8());
  Types::Fixed fixed("Fixed[Unsigned_8,1]"_view, Dialect::get_unsigned_8(), 1);
  Materializations materializations(domain);
  Static::Vector<Generic::Argument, 1> access_arguments = {{
    Generic::Argument(Dialect::get_unsigned_8()),
  }};
  auto access = materializations.materialize(
      Generics::Access::get_formula(), access_arguments.get_view());
  ASSERT(access);

  EXPECT_NOT(Dialect::create_default(domain, Dialect::get_void()));
  EXPECT_NOT(Dialect::create_default(domain, range));
  EXPECT_NOT(Dialect::create_default(domain, fixed));
  EXPECT_NOT(Dialect::create_default(domain, *access));
  EXPECT_NOT(
      Dialect::create_default(
          domain, static_cast<const Ttx::Model::Type&>(structure)));
  EXPECT_NOT(
      Dialect::create_default(
          domain, static_cast<const Ttx::Model::Type&>(object)));
  EXPECT_NOT(
      Dialect::create_default(
          domain, static_cast<const Ttx::Model::Type&>(enumeration)));
  EXPECT_NOT(Dialect::create_default(domain, foreign_unsigned));
  EXPECT(errors.is_empty());
}
