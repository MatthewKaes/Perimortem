// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/option.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
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
      "// Default value types.\n"
      "dialect : Library;\n"
      "public Empty : struct {}\n"
      "public EmptyObject : object {}\n"
      "public Inner : struct {\n"
      "  private state number : Unsigned_64;\n"
      "  private state enabled : Bool;\n"
      "}\n"
      "public Packet : struct {\n"
      "  private state inner : Inner;\n"
      "  private state count : Unsigned_64 = 9;\n"
      "}\n"
      "public Session : object { private state count : Unsigned_64; }\n"
      "public Cycle : object { private state next : Cycle; }\n"
      "public Safe : object { private state next : Option[Safe]; }\n"
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

PERIMORTEM_UNIT_TEST(LibraryDefaults, contiguous_and_optional_values) {
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

  auto access = materializations.materialize(
      Generics::Access::get_formula(), arguments.get_view());
  ASSERT(access);
  auto access_default = Dialect::create_default(domain, *access);
  ASSERT(access_default && access_default->is<Constants::Bytes>());
  const auto& access_value =
      static_cast<const Constants::Bytes&>(*access_default);
  EXPECT(&access_value.get_type() == &*access);
  EXPECT(access_value.get_value().is_empty());

  auto option = materializations.materialize(
      Generics::Option::get_formula(), arguments.get_view());
  ASSERT(option && option->is<Types::Option>());
  auto option_default = Dialect::create_default(domain, *option);
  ASSERT(option_default && option_default->is<Constants::Option>());
  const auto& option_value =
      static_cast<const Constants::Option&>(*option_default);
  EXPECT(option_value.get_kind() == Types::Option::Kind::Absent);
  EXPECT_NOT(option_value.get_payload());
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, aggregate_order_and_fresh_identity) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  const Abstract& inner = source_type.resolve_context("Inner"_view);
  const Abstract& empty = source_type.resolve_context("Empty"_view);
  const Abstract& empty_object =
      source_type.resolve_context("EmptyObject"_view);
  const Abstract& structure = source_type.resolve_context("Packet"_view);
  const Abstract& object = source_type.resolve_context("Session"_view);
  ASSERT(inner.is<Types::Structure>());
  ASSERT(empty.is<Types::Structure>());
  ASSERT(empty_object.is<Types::Object>());
  ASSERT(structure.is<Types::Structure>());
  ASSERT(object.is<Types::Object>());

  auto structure_default = Dialect::create_default(
      domain, static_cast<const Ttx::Model::Type&>(structure));
  ASSERT(
      structure_default && structure_default->is<Expressions::Initializer>());
  const auto& structure_value =
      static_cast<const Expressions::Initializer&>(*structure_default);
  ASSERT(structure_value.get_completed_values());
  const Layout& structure_values =
      structure_value.get_completed_values()->get_layout();
  ASSERT_EQ(structure_values.get_size(), Count(2));
  auto nested = structure_values.get_abstract(0);
  auto count = structure_values.get_abstract(1);
  ASSERT(nested && nested->is<Expressions::Initializer>());
  ASSERT(count && count->is<Constants::Unsigned>());
  const auto& nested_value =
      static_cast<const Expressions::Initializer&>(*nested);
  EXPECT(&nested_value.get_type() == &inner);
  ASSERT(nested_value.get_completed_values());
  EXPECT_EQ(
      nested_value.get_completed_values()->get_layout().get_size(), Count(2));
  EXPECT_EQ(
      static_cast<const Constants::Unsigned&>(*count).get_value(),
      Unsigned_64(9));

  auto first_object = Dialect::create_default(
      domain, static_cast<const Ttx::Model::Type&>(object));
  auto second_object = Dialect::create_default(
      domain, static_cast<const Ttx::Model::Type&>(object));
  ASSERT(first_object && first_object->is<Expressions::Initializer>());
  ASSERT(second_object && second_object->is<Expressions::Initializer>());
  EXPECT(&*first_object != &*second_object);
  EXPECT(&first_object->get_type() == &object);
  EXPECT(&second_object->get_type() == &object);
  EXPECT_NOT(
      Dialect::create_default(
          domain, static_cast<const Ttx::Model::Type&>(empty)));
  EXPECT_NOT(
      Dialect::create_default(
          domain, static_cast<const Ttx::Model::Type&>(empty_object)));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, fixed_default_and_empty_rejection) {
  Allocator::Arena domain;
  Types::Fixed fixed("Fixed[Unsigned_8,3]"_view, Dialect::get_unsigned_8(), 3);
  Types::Fixed empty("Fixed[Unsigned_8,0]"_view, Dialect::get_unsigned_8(), 0);

  auto fixed_default = Dialect::create_default(domain, fixed);
  ASSERT(fixed_default && fixed_default->is<Expressions::Initializer>());
  const auto& fixed_value =
      static_cast<const Expressions::Initializer&>(*fixed_default);
  ASSERT(fixed_value.get_completed_values());
  const Layout& values = fixed_value.get_completed_values()->get_layout();
  ASSERT_EQ(values.get_size(), Count(3));
  for (Count index = 0; index < values.get_size(); index++) {
    auto entry = values.get_abstract(index);
    ASSERT(entry && entry->is<Constants::Unsigned>());
    EXPECT_EQ(
        static_cast<const Constants::Unsigned&>(*entry).get_value(),
        Unsigned_64(0));
  }

  auto empty_default = Dialect::create_default(domain, empty);
  EXPECT_NOT(empty_default);
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, enumeration_range_and_cycles) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  const Abstract& enumeration = source_type.resolve_context("Mode"_view);
  const Abstract& cycle = source_type.resolve_context("Cycle"_view);
  const Abstract& safe = source_type.resolve_context("Safe"_view);
  ASSERT(enumeration.is<Types::Enumeration>());
  ASSERT(cycle.is<Types::Object>());
  ASSERT(safe.is<Types::Object>());

  auto enumeration_default = Dialect::create_default(
      domain, static_cast<const Ttx::Model::Type&>(enumeration));
  ASSERT(
      enumeration_default && enumeration_default->is<Constants::Enumeration>());
  EXPECT_EQ(
      static_cast<const Constants::Enumeration&>(*enumeration_default)
          .get_value(),
      Unsigned_64(0));

  Types::Range range("Range[Unsigned_8]"_view, Dialect::get_unsigned_8());
  auto range_default = Dialect::create_default(domain, range);
  ASSERT(range_default && range_default->is<Constants::Range>());
  EXPECT(static_cast<const Constants::Range&>(*range_default).is_empty());

  EXPECT_NOT(
      Dialect::create_default(
          domain, static_cast<const Ttx::Model::Type&>(cycle)));
  auto safe_default = Dialect::create_default(
      domain, static_cast<const Ttx::Model::Type&>(safe));
  ASSERT(safe_default && safe_default->is<Expressions::Initializer>());
  const auto& safe_value =
      static_cast<const Expressions::Initializer&>(*safe_default);
  ASSERT(safe_value.get_completed_values());
  auto next = safe_value.get_completed_values()->get_layout().get_abstract(0);
  ASSERT(next && next->is<Constants::Option>());
  EXPECT(
      static_cast<const Constants::Option&>(*next).get_kind() ==
      Types::Option::Kind::Absent);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, unsupported_domains_are_absent) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);

  ForeignUnsigned foreign_unsigned;
  EXPECT_NOT(Dialect::create_default(domain, foreign_unsigned));
  EXPECT_NOT(Dialect::create_default(domain, Dialect::get_descriptor()));
  EXPECT_NOT(Dialect::create_default(domain, monograph->get_source()));
  EXPECT(errors.is_empty());
}
