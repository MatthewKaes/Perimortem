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
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/option.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryDefaults = {
  .name = "Tetrodotoxin::Library defaults"_view,
};

class ForeignUnsigned : public Model::Types::Unsigned {
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
  auto create_default(Allocator::Arena&) const
      -> Option<Model::Pack&> override {
    return {};
  }
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
      "public Safe : object { private state next : Option[Safe]; }\n"
      "public Mode : enum[Unsigned_8] { ready = 1; }"_view;
  auto imported = workspace.interpret_source(
      errors, "Defaults"_view, "defaults.ttx"_view, source);
  if (!imported || !imported->is<Monograph>()) {
    return {};
  }

  return static_cast<Monograph&>(*imported);
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, scalar_payloads_and_identity) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const Static::Vector<const Model::Type*, 4> unsigned_types = {{
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Unsigned_8"_view)),
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Unsigned_16"_view)),
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Unsigned_32"_view)),
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Unsigned_64"_view)),
  }};
  const Static::Vector<const Model::Type*, 4> signed_types = {{
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Signed_8"_view)),
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Signed_16"_view)),
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Signed_32"_view)),
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Signed_64"_view)),
  }};
  const Static::Vector<const Model::Type*, 2> real_types = {{
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Real_32"_view)),
    &static_cast<const Model::Type&>(
        monograph->resolve_context("Real_64"_view)),
  }};

  const auto& boolean_type =
      static_cast<const Model::Type&>(monograph->resolve_context("Bool"_view));
  auto boolean = boolean_type.create_default(domain);
  ASSERT(boolean && boolean->is<Constants::False>());
  EXPECT(&boolean->get_type() == &boolean_type);
  EXPECT_NOT(static_cast<const Constants::False&>(*boolean).get_value());

  for (Count i = 0; i < unsigned_types.get_size(); i++) {
    const Model::Type* type = unsigned_types.get_data()[i];
    auto created = type->create_default(domain);
    ASSERT(created && created->is<Constants::Unsigned>());
    const auto& value = static_cast<const Constants::Unsigned&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), Unsigned_64(0));
  }

  for (Count i = 0; i < signed_types.get_size(); i++) {
    const Model::Type* type = signed_types.get_data()[i];
    auto created = type->create_default(domain);
    ASSERT(created && created->is<Constants::Signed>());
    const auto& value = static_cast<const Constants::Signed&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), Signed_64(0));
  }

  for (Count i = 0; i < real_types.get_size(); i++) {
    const Model::Type* type = real_types.get_data()[i];
    auto created = type->create_default(domain);
    ASSERT(created && created->is<Constants::Real>());
    const auto& value = static_cast<const Constants::Real&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), Real_64(0));
  }
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, contiguous_and_optional_values) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& unsigned_8 = static_cast<const Model::Type&>(
      monograph->resolve_context("Unsigned_8"_view));
  Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(unsigned_8),
  }};
  const auto& view =
      static_cast<const Generic&>(monograph->resolve_context("View"_view));
  const Model::Type* type = nullptr;
  view.materialize(arguments.get_view())
      .visit(
          [&](const Model::Type& selected) { type = &selected; },
          [](const Generic::Failure&) {});
  ASSERT(type && type->is<Types::View>());

  auto created = type->create_default(domain);
  ASSERT(created && created->is<Constants::Bytes>());
  const auto& value = static_cast<const Constants::Bytes&>(*created);
  EXPECT(&value.get_type() == &*type);
  EXPECT(value.get_value().is_empty());

  const auto& access_formula =
      static_cast<const Generic&>(monograph->resolve_context("Access"_view));
  const Model::Type* access = nullptr;
  access_formula.materialize(arguments.get_view())
      .visit(
          [&](const Model::Type& selected) { access = &selected; },
          [](const Generic::Failure&) {});
  ASSERT(access);
  auto access_default = access->create_default(domain);
  ASSERT(access_default && access_default->is<Constants::Bytes>());
  const auto& access_value =
      static_cast<const Constants::Bytes&>(*access_default);
  EXPECT(&access_value.get_type() == &*access);
  EXPECT(access_value.get_value().is_empty());

  const auto& option_formula =
      static_cast<const Generic&>(monograph->resolve_context("Option"_view));
  const Model::Type* option = nullptr;
  option_formula.materialize(arguments.get_view())
      .visit(
          [&](const Model::Type& selected) { option = &selected; },
          [](const Generic::Failure&) {});
  ASSERT(option && option->is<Types::Option>());
  auto option_default = option->create_default(domain);
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

  auto structure_default =
      static_cast<const Model::Type&>(structure).create_default(domain);
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

  auto first_object =
      static_cast<const Model::Type&>(object).create_default(domain);
  auto second_object =
      static_cast<const Model::Type&>(object).create_default(domain);
  ASSERT(first_object && first_object->is<Expressions::Initializer>());
  ASSERT(second_object && second_object->is<Expressions::Initializer>());
  EXPECT(&*first_object != &*second_object);
  EXPECT(&first_object->get_type() == &object);
  EXPECT(&second_object->get_type() == &object);
  EXPECT_NOT(static_cast<const Model::Type&>(empty).create_default(domain));
  EXPECT_NOT(
      static_cast<const Model::Type&>(empty_object).create_default(domain));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, fixed_default_and_empty_rejection) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& unsigned_8 = static_cast<const Model::Type&>(
      monograph->resolve_context("Unsigned_8"_view));
  Types::Fixed fixed("Fixed[Unsigned_8,3]"_view, unsigned_8, 3);
  Types::Fixed empty("Fixed[Unsigned_8,0]"_view, unsigned_8, 0);

  auto fixed_default =
      static_cast<const Model::Type&>(fixed).create_default(domain);
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

  auto empty_default =
      static_cast<const Model::Type&>(empty).create_default(domain);
  EXPECT_NOT(empty_default);
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, enumeration_range_and_optional_default) {
  Allocator::Arena domain;
  Tetrodotoxin::Environment::Workspace workspace;
  Ttx::Lexical::Errors errors;
  ASSERT(workspace.install_dialect<Dialect>("Library"_view));
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  const Abstract& enumeration = source_type.resolve_context("Mode"_view);
  const Abstract& safe = source_type.resolve_context("Safe"_view);
  ASSERT(enumeration.is<Types::Enumeration>());
  ASSERT(safe.is<Types::Object>());

  auto enumeration_default =
      static_cast<const Model::Type&>(enumeration).create_default(domain);
  ASSERT(
      enumeration_default && enumeration_default->is<Constants::Enumeration>());
  EXPECT_EQ(
      static_cast<const Constants::Enumeration&>(*enumeration_default)
          .get_value(),
      Unsigned_64(0));

  const auto& unsigned_8 = static_cast<const Model::Type&>(
      monograph->resolve_context("Unsigned_8"_view));
  Types::Range range("Range[Unsigned_8]"_view, unsigned_8);
  auto range_default =
      static_cast<const Model::Type&>(range).create_default(domain);
  ASSERT(range_default && range_default->is<Constants::Range>());
  EXPECT(static_cast<const Constants::Range&>(*range_default).is_empty());

  auto safe_default =
      static_cast<const Model::Type&>(safe).create_default(domain);
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
  EXPECT_NOT(foreign_unsigned.create_default(domain));
  EXPECT(monograph->resolve_context("Descriptor"_view).is<Invalid>());
  EXPECT_NOT(
      static_cast<const Model::Type&>(monograph->get_source())
          .create_default(domain));
  EXPECT(errors.is_empty());
}
