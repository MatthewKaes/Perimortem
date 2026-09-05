// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/workspace.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

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
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/option.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/model/initialization.hpp"
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
#include "ttx/lexical/errors.hpp"
#include "ttx/concept/unknown.hpp"

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
  auto initialize_default(Allocator::Arena&) const
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
      "  private state number : U64;\n"
      "  private state enabled : Bool;\n"
      "}\n"
      "public Packet : struct {\n"
      "  private state inner : Inner;\n"
      "  private state count : U64 = 9;\n"
      "}\n"
      "public Session : object { private state count : U64; }\n"
      "public Safe : object { private state next : Option[Safe]; }\n"
      "public Mode : enum[U8] { ready = 1; }"_view;
  auto imported = workspace.interpret_source(
      errors, "Defaults"_view, "defaults.ttx"_view, source);
  if (!imported || !imported->is<Monograph>()) {
    return {};
  }

  return static_cast<Monograph&>(*imported);
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, unfinished_body_retains_field_domain) {
  static constexpr View::Bytes source =
      "// Progressive Callable completion.\n"
      "dialect : Library;\n"
      "public Item : struct {\n"
      "  private state marker : U8;\n"
      "  public size : func = [self] -> U64 : return 0;\n"
      "}\n"
      "public item : Item;\n"
      "public explore : func = [] -> [] {\n"
      "  item ->"_view;
  auto toolchain = create_library_toolchain();
  Tetrodotoxin::Environment::Workspace workspace(*toolchain);
  Ttx::Lexical::Errors errors;
  EXPECT_NOT(workspace.interpret_source(
      errors, "Progressive"_view, "progressive.ttx"_view, source));
  auto retained = workspace.get_monograph("progressive.ttx"_view);
  ASSERT(retained && retained->is<Monograph>());
  const Abstract& item = retained->resolve_concept("item"_view);
  ASSERT(item.is<Field>());
  const Abstract& domain = static_cast<const Field&>(item).get_type();
  EXPECT_TEXT(domain.get_name(), "Item"_view);
  EXPECT(ttx_abstract_same(Ttx::resolve(item.get_handle()), item.get_handle()));
  const auto direct_observed = Ttx::resolve_domain(item.get_handle());
  ASSERT(direct_observed.state == Ttx::Observation::Resolved);
  const auto associations = workspace.get_associations("progressive.ttx"_view);
  ASSERT(associations);
  const Count receiver = Algorithm::search(source, "item ->"_view);
  ASSERT(receiver != Count(-1));
  const auto expression = associations->find_at(receiver);
  ASSERT(expression);
  EXPECT_TEXT(expression->get_name(), "item"_view);
  ASSERT(expression->is<Field>());
  const auto observed = Ttx::resolve_domain(expression->get_handle());
  ASSERT(observed.state == Ttx::Observation::Resolved);
  const ttx_borrowed_bytes observed_name =
      observed.domain.operations->name(observed.domain);
  EXPECT_TEXT(View::Bytes(observed_name.data, observed_name.size), "Item"_view);
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, scalar_defaults) {
  Allocator::Arena domain;
  auto workspace_toolchain = create_library_toolchain();
  Tetrodotoxin::Environment::Workspace workspace(*workspace_toolchain);
  Ttx::Lexical::Errors errors;
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const Static::Vector<const Model::Type*, 4> unsigned_types = {{
    &static_cast<const Model::Type&>(monograph->resolve_concept("U8"_view)),
    &static_cast<const Model::Type&>(monograph->resolve_concept("U16"_view)),
    &static_cast<const Model::Type&>(monograph->resolve_concept("U32"_view)),
    &static_cast<const Model::Type&>(monograph->resolve_concept("U64"_view)),
  }};
  const Static::Vector<const Model::Type*, 4> signed_types = {{
    &static_cast<const Model::Type&>(monograph->resolve_concept("S8"_view)),
    &static_cast<const Model::Type&>(monograph->resolve_concept("S16"_view)),
    &static_cast<const Model::Type&>(monograph->resolve_concept("S32"_view)),
    &static_cast<const Model::Type&>(monograph->resolve_concept("S64"_view)),
  }};
  const Static::Vector<const Model::Type*, 2> real_types = {{
    &static_cast<const Model::Type&>(monograph->resolve_concept("R32"_view)),
    &static_cast<const Model::Type&>(monograph->resolve_concept("R64"_view)),
  }};

  const auto& boolean_type =
      static_cast<const Model::Type&>(monograph->resolve_concept("Bool"_view));
  auto boolean = Model::initialize_default(boolean_type, domain);
  ASSERT(boolean && boolean->is_identity<Constants::False>());
  EXPECT(&boolean->get_type() == &boolean_type);
  EXPECT_NOT(static_cast<const Constants::False&>(*boolean).get_value());

  for (Count i = 0; i < unsigned_types.get_size(); i++) {
    const Model::Type* type = unsigned_types.get_data()[i];
    auto created = Model::initialize_default(*type, domain);
    ASSERT(created && created->is_identity<Constants::Unsigned>());
    const auto& value = static_cast<const Constants::Unsigned&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), U64(0));
  }

  for (Count i = 0; i < signed_types.get_size(); i++) {
    const Model::Type* type = signed_types.get_data()[i];
    auto created = Model::initialize_default(*type, domain);
    ASSERT(created && created->is_identity<Constants::Signed>());
    const auto& value = static_cast<const Constants::Signed&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), S64(0));
  }

  for (Count i = 0; i < real_types.get_size(); i++) {
    const Model::Type* type = real_types.get_data()[i];
    auto created = Model::initialize_default(*type, domain);
    ASSERT(created && created->is_identity<Constants::Real>());
    const auto& value = static_cast<const Constants::Real&>(*created);
    EXPECT(&value.get_type() == type);
    EXPECT_EQ(value.get_value(), R64(0));
  }
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, carrier_defaults) {
  Allocator::Arena domain;
  auto workspace_toolchain = create_library_toolchain();
  Tetrodotoxin::Environment::Workspace workspace(*workspace_toolchain);
  Ttx::Lexical::Errors errors;
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& u8 =
      static_cast<const Model::Type&>(monograph->resolve_concept("U8"_view));
  Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(u8),
  }};
  const auto& view =
      static_cast<const Generic&>(monograph->resolve_concept("View"_view));
  const Model::Type* type = nullptr;
  view.materialize(arguments.get_view())
      .visit(
          [&](const Model::Type& selected) { type = &selected; },
          [](const Generic::Failure&) {});
  ASSERT(type && type->is<Types::View>());

  auto created = Model::initialize_default(*type, domain);
  ASSERT(created && created->is_identity<Constants::Bytes>());
  const auto& value = static_cast<const Constants::Bytes&>(*created);
  EXPECT(&value.get_type() == &*type);
  EXPECT(value.get_value().is_empty());

  const auto& access_formula =
      static_cast<const Generic&>(monograph->resolve_concept("Access"_view));
  const Model::Type* access = nullptr;
  access_formula.materialize(arguments.get_view())
      .visit(
          [&](const Model::Type& selected) { access = &selected; },
          [](const Generic::Failure&) {});
  ASSERT(access);
  auto access_default = Model::initialize_default(*access, domain);
  ASSERT(access_default && access_default->is_identity<Constants::Bytes>());
  const auto& access_value =
      static_cast<const Constants::Bytes&>(*access_default);
  EXPECT(&access_value.get_type() == &*access);
  EXPECT(access_value.get_value().is_empty());

  const auto& option_formula =
      static_cast<const Generic&>(monograph->resolve_concept("Option"_view));
  const Model::Type* option = nullptr;
  option_formula.materialize(arguments.get_view())
      .visit(
          [&](const Model::Type& selected) { option = &selected; },
          [](const Generic::Failure&) {});
  ASSERT(option && option->is<Types::Option>());
  auto option_default = Model::initialize_default(*option, domain);
  ASSERT(option_default && option_default->is_identity<Constants::Option>());
  const auto& option_value =
      static_cast<const Constants::Option&>(*option_default);
  EXPECT(option_value.get_kind() == Types::Option::Kind::Absent);
  EXPECT_NOT(option_value.get_payload());
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, aggregate_defaults) {
  Allocator::Arena domain;
  auto workspace_toolchain = create_library_toolchain();
  Tetrodotoxin::Environment::Workspace workspace(*workspace_toolchain);
  Ttx::Lexical::Errors errors;
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  const Abstract& inner = source_type.resolve_concept("Inner"_view);
  const Abstract& empty = source_type.resolve_concept("Empty"_view);
  const Abstract& empty_object =
      source_type.resolve_concept("EmptyObject"_view);
  const Abstract& structure = source_type.resolve_concept("Packet"_view);
  const Abstract& object = source_type.resolve_concept("Session"_view);
  ASSERT(inner.is<Types::Structure>());
  ASSERT(empty.is<Types::Structure>());
  ASSERT(empty_object.is<Types::Object>());
  ASSERT(structure.is<Types::Structure>());
  ASSERT(object.is<Types::Object>());

  auto structure_default = Model::initialize_default(
      static_cast<const Model::Type&>(structure), domain);
  ASSERT(
      structure_default &&
      structure_default->is_identity<Expressions::Initializer>());
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
      static_cast<const Constants::Unsigned&>(*count).get_value(), U64(9));

  auto first_object = Model::initialize_default(
      static_cast<const Model::Type&>(object), domain);
  auto second_object = Model::initialize_default(
      static_cast<const Model::Type&>(object), domain);
  ASSERT(first_object && first_object->is_identity<Expressions::Initializer>());
  ASSERT(
      second_object && second_object->is_identity<Expressions::Initializer>());
  EXPECT(&*first_object != &*second_object);
  EXPECT(&first_object->get_type() == &object);
  EXPECT(&second_object->get_type() == &object);
  EXPECT_NOT(
      Model::initialize_default(
          static_cast<const Model::Type&>(empty), domain));
  EXPECT_NOT(
      Model::initialize_default(
          static_cast<const Model::Type&>(empty_object), domain));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, fixed_defaults) {
  Allocator::Arena domain;
  auto workspace_toolchain = create_library_toolchain();
  Tetrodotoxin::Environment::Workspace workspace(*workspace_toolchain);
  Ttx::Lexical::Errors errors;
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& u8 =
      static_cast<const Model::Type&>(monograph->resolve_concept("U8"_view));
  Types::Fixed fixed("Fixed[U8,3]"_view, u8, 3);
  Types::Fixed empty("Fixed[U8,0]"_view, u8, 0);

  auto fixed_default =
      Model::initialize_default(static_cast<const Model::Type&>(fixed), domain);
  ASSERT(
      fixed_default && fixed_default->is_identity<Expressions::Initializer>());
  const auto& fixed_value =
      static_cast<const Expressions::Initializer&>(*fixed_default);
  ASSERT(fixed_value.get_completed_values());
  const Layout& values = fixed_value.get_completed_values()->get_layout();
  ASSERT_EQ(values.get_size(), Count(3));
  for (Count index = 0; index < values.get_size(); index++) {
    auto entry = values.get_abstract(index);
    ASSERT(entry && entry->is<Constants::Unsigned>());
    EXPECT_EQ(
        static_cast<const Constants::Unsigned&>(*entry).get_value(), U64(0));
  }

  auto empty_default =
      Model::initialize_default(static_cast<const Model::Type&>(empty), domain);
  EXPECT_NOT(empty_default);
}

PERIMORTEM_UNIT_TEST(LibraryDefaults, domain_defaults) {
  Allocator::Arena domain;
  auto workspace_toolchain = create_library_toolchain();
  Tetrodotoxin::Environment::Workspace workspace(*workspace_toolchain);
  Ttx::Lexical::Errors errors;
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  const Abstract& enumeration = source_type.resolve_concept("Mode"_view);
  const Abstract& safe = source_type.resolve_concept("Safe"_view);
  ASSERT(enumeration.is<Types::Enumeration>());
  ASSERT(safe.is<Types::Object>());

  auto enumeration_default = Model::initialize_default(
      static_cast<const Model::Type&>(enumeration), domain);
  ASSERT(
      enumeration_default &&
      enumeration_default->is_identity<Constants::Enumeration>());
  EXPECT_EQ(
      static_cast<const Constants::Enumeration&>(*enumeration_default)
          .get_value(),
      U64(0));

  const auto& u8 =
      static_cast<const Model::Type&>(monograph->resolve_concept("U8"_view));
  Types::Range range("Range[U8]"_view, u8);
  auto range_default =
      Model::initialize_default(static_cast<const Model::Type&>(range), domain);
  ASSERT(range_default && range_default->is_identity<Constants::Range>());
  EXPECT(static_cast<const Constants::Range&>(*range_default).is_empty());

  auto safe_default =
      Model::initialize_default(static_cast<const Model::Type&>(safe), domain);
  ASSERT(safe_default && safe_default->is_identity<Expressions::Initializer>());
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

PERIMORTEM_UNIT_TEST(LibraryDefaults, absent_defaults) {
  Allocator::Arena domain;
  auto workspace_toolchain = create_library_toolchain();
  Tetrodotoxin::Environment::Workspace workspace(*workspace_toolchain);
  Ttx::Lexical::Errors errors;
  auto monograph = import_types(workspace, errors);
  ASSERT(monograph);

  ForeignUnsigned foreign_unsigned;
  EXPECT_NOT(Model::initialize_default(foreign_unsigned, domain));
  EXPECT(monograph->resolve_concept("Descriptor"_view).is<Unknown>());
  EXPECT_NOT(
      Model::initialize_default(
          static_cast<const Model::Type&>(monograph->get_source()), domain));
  EXPECT(errors.is_empty());
}
