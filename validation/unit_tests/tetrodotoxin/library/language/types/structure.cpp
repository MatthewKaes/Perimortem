// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/return.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static auto find_return(const Language::Function& function)
    -> Option<const Language::Return&> {
  auto body = function.get_body();
  BAIL_IF(!body);
  for (const Reference<Abstract>& statement : body->get_statements()) {
    auto returned = statement.get().select<Language::Return>();
    if (returned) {
      return *returned;
    }
  }

  return {};
}

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  if (!workspace.install_dialect<Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "StructureTest"_view, "structure.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return !monograph && !errors.is_empty();
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph) {
    return False;
  }

  Bool linked = workspace.link(errors);
  return !linked && !errors.is_empty();
}

static auto rejects_finalize(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph || !workspace.link(errors)) {
    return False;
  }

  Bool finalized = workspace.finalize(errors);
  return !finalized && !errors.is_empty();
}

static Harness StructureTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Structure"_view,
};

PERIMORTEM_UNIT_TEST(StructureTests, nested_type_aliases) {
  static constexpr View::Bytes source =
      "// Nested Alias source.\n"
      "dialect : Library;\n"
      "// Hidden Type documentation.\n"
      "private Hidden : struct { private value : Bool; }\n"
      "public Packet : struct {\n"
      "  // Visible Alias documentation.\n"
      "  public Visible : alias = Hidden;\n"
      "  private Flag : alias = Bool;\n"
      "  public value : Visible;\n"
      "  private flag : Flag;\n"
      "}\n"
      "public Selected : alias = Packet::Visible;"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  auto types = source_type.get_types();
  ASSERT(types != types.end());
  const Abstract& hidden = (*types).get();
  const Abstract& packet_identity = monograph->resolve_context("Packet"_view);
  const Abstract& selected_identity =
      monograph->resolve_context("Selected"_view);
  ASSERT(hidden.is<Language::Types::Structure>());
  ASSERT(packet_identity.is<Language::Types::Structure>());
  ASSERT(selected_identity.is<Alias>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const Abstract& visible_identity = packet.resolve_context("Visible"_view);
  ASSERT(visible_identity.is<Alias>());
  const auto& visible = static_cast<const Alias&>(visible_identity);

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(&visible.resolve() == &hidden);
  EXPECT_EQ(visible.get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      visible.get_documentation().get_line(0),
      "Visible Alias documentation."_view);
  EXPECT_TEXT(
      visible.get_documentation().get_line(1),
      "Hidden Type documentation."_view);
  EXPECT(&packet.resolve_context("Flag"_view) == &Invalid::get_invalid());
  auto type_bindings = packet.get_types();
  ASSERT(type_bindings != type_bindings.end());
  ++type_bindings;
  ASSERT(type_bindings != type_bindings.end());
  ASSERT((*type_bindings).get().is<Alias>());
  EXPECT(&(*type_bindings).get().resolve() == &Dialect::get_bool());

  EXPECT(&selected_identity.resolve() == &hidden);
  auto fields = packet.get_addressables();
  ASSERT(fields != fields.end());
  const auto& hidden_field =
      static_cast<const Language::Field&>((*fields).get());
  ++fields;
  ASSERT(fields != fields.end());
  const auto& flag_field = static_cast<const Language::Field&>((*fields).get());
  EXPECT(&hidden_field.get_type() == &hidden);
  EXPECT(&flag_field.get_type() == &Dialect::get_bool());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, independent_access_axes) {
  static constexpr View::Bytes source =
      "// Structure access test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public ordinary_public : Bool;\n"
      "  private ordinary_private : Bool;\n"
      "  expose state state_exposed : Bool = false;\n"
      "  private state state_private : Bool = false;\n"
      "  public const const_public : Bool = false;\n"
      "  private const const_private : Bool = false;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& selected = monograph->resolve_context("Packet"_view);
  ASSERT(selected.is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(selected);
  auto fields = packet.get_addressables();

  auto field = fields.begin();
  ASSERT(field != fields.end());
  const auto& ordinary_public =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& ordinary_private =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& state_exposed =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& state_private =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& const_public =
      static_cast<const Language::Field&>((*field).get());
  ++field;
  ASSERT(field != fields.end());
  const auto& const_private =
      static_cast<const Language::Field&>((*field).get());
  EXPECT(
      ordinary_public.get_writability() == Language::Field::Writability::Full);
  EXPECT(
      ordinary_private.get_writability() == Language::Field::Writability::Full);
  EXPECT(
      state_exposed.get_writability() ==
      Language::Field::Writability::Internal);
  EXPECT(
      state_private.get_writability() ==
      Language::Field::Writability::Internal);
  EXPECT(const_public.get_writability() == Language::Field::Writability::Init);
  EXPECT(const_private.get_writability() == Language::Field::Writability::Init);

  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, declaration_reorder) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public First : struct { public next : Second; }\n"
    "public Second : struct { public value : Unsigned_64; }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Second : struct { public value : Unsigned_64; }\n"
    "public First : struct { public next : Second; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[i]);
    ASSERT(monograph);
    ASSERT(workspace.link(errors));
    ASSERT(workspace.finalize(errors));

    const Abstract& first = monograph->resolve_context("First"_view);
    const Abstract& second = monograph->resolve_context("Second"_view);
    ASSERT(first.is<Language::Types::Structure>());
    ASSERT(second.is<Language::Types::Structure>());
    const auto& first_structure =
        static_cast<const Language::Types::Structure&>(first);
    auto field = first_structure.get_layout().get_abstract(0);
    ASSERT(field);
    ASSERT(field->is<Addressable>());
    EXPECT(&static_cast<const Addressable&>(*field).get_type() == &second);
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, category_names_coexist) {
  static constexpr Static::Vector<View::Bytes, 3> accepted = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; public value : func = [] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { private storage : Bool; public value : func = [] -> [] {} public value : func = [self] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; public inspect : func = [self, .value : Bool] -> Bool { return value; } }"_view,
  }};
  for (Count i = 0; i < accepted.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, accepted[i]);
    ASSERT(monograph);
    ASSERT(workspace.link(errors));
    ASSERT(workspace.finalize(errors));
    EXPECT(errors.is_empty());
  }

  static constexpr View::Bytes duplicate_fields =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public value : Bool; private value : Bool; }"_view;
  EXPECT(rejects_interpretation(duplicate_fields));
}

PERIMORTEM_UNIT_TEST(StructureTests, fields_require_address_access) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; public read : func = [] -> Bool { return value; } }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; public read : func = [self] -> Bool { return value; } }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_link(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, explicit_self_field_access) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private value : Bool;\n"
      "  public read : func = [self] -> Bool { return self.value; }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& selected = monograph->resolve_context("Packet"_view);
  ASSERT(selected.is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(selected);
  auto fields = packet.get_addressables();
  auto callables = packet.get_callables();
  ASSERT(fields != fields.end());
  const Abstract& field_identity = (*fields).get();
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& read = static_cast<const Language::Function&>((*callables).get());
  auto returned = find_return(read);
  ASSERT(returned && returned->get_expression());
  ASSERT(returned->get_expression()->is<Language::Access::Address>());
  const auto& address = static_cast<const Language::Access::Address&>(
      *returned->get_expression());
  EXPECT(&address.get_result() == &field_identity);
  ASSERT(address.get_receiver().is<Language::Identifier>());
  const auto& receiver =
      static_cast<const Language::Identifier&>(address.get_receiver());
  auto receiver_result = receiver.get_result().select<Addressable>();
  ASSERT(receiver_result);
  EXPECT_TEXT(receiver_result->get_name(), "self"_view);
  EXPECT(&receiver_result->get_type() == &packet);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 10> sources = {{
    "// Structure test.\ndialect : Library; public Packet struct {}"_view,
    "// Structure test.\ndialect : Library; public Packet : wrong {}"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public Value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Bool }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Core ::Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Core:: Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public state value : Bool = false; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { using Core; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, missing_type_rejected) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public missing : Missing; }"_view;
  EXPECT(rejects_link(source));
}

PERIMORTEM_UNIT_TEST(StructureTests, empty_types_are_static_only) {
  static constexpr Static::Vector<View::Bytes, 5> rejected = {{
    "// Void Addressable.\ndialect : Library; private invalid : Void;"_view,
    "// Empty Fixed Addressable.\ndialect : Library; private invalid := 0x[];"_view,
    "// Empty Composite Addressable.\ndialect : Library; private Empty : struct {} private invalid : Empty;"_view,
    "// Empty Self receiver.\ndialect : Library; public Empty : struct { public invalid : func = [self] -> [] {} }"_view,
    "// Empty named parameter.\ndialect : Library; private Empty : struct {} private invalid : func = [.value : Empty] -> [] {}"_view,
  }};
  EXPECT(rejects_link(rejected[0]));
  EXPECT(rejects_link(rejected[1]));
  EXPECT(rejects_link(rejected[2]));
  EXPECT(rejects_link(rejected[3]));
  EXPECT(rejects_link(rejected[4]));

  static constexpr View::Bytes source =
      "// Empty Types retain only Static bindings.\n"
      "dialect : Library;\n"
      "public Empty : struct { public create : func = [] -> [] {} }\n"
      "private void_result : func = [] -> Void {}\n"
      "private empty_result : func = [] -> [] {}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& empty_identity = monograph->resolve_context("Empty"_view);
  ASSERT(empty_identity.is<Language::Types::Structure>());
  const auto& empty =
      static_cast<const Language::Types::Structure&>(empty_identity);
  EXPECT(empty.get_layout().is_empty());
  auto empty_callables = empty.get_callables();
  ASSERT(empty_callables != empty_callables.end());
  EXPECT_TEXT((*empty_callables).get().get_name(), "create"_view);
  ++empty_callables;
  EXPECT(empty_callables == empty.get_callables().end());

  auto callables = monograph->get_source().get_callables();
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& void_result =
      static_cast<const Language::Function&>((*callables).get());
  ++callables;
  ASSERT(callables != monograph->get_source().get_callables().end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& empty_result =
      static_cast<const Language::Function&>((*callables).get());
  ++callables;
  EXPECT(callables == monograph->get_source().get_callables().end());
  EXPECT(void_result.get_results().is_empty());
  EXPECT(empty_result.get_results().is_empty());
  EXPECT(void_result.get_results().fits(empty_result.get_results()));
  EXPECT(empty_result.get_results().fits(void_result.get_results()));

  const auto& scalar = Dialect::get_unsigned_8();
  ASSERT_EQ(scalar.get_layout().get_size(), Count(1));
  EXPECT(&*scalar.get_layout().get_abstract(0) == &scalar);
  EXPECT_NOT(void_result.get_results().fits(scalar.get_layout()));
  EXPECT_NOT(scalar.get_layout().fits(void_result.get_results()));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, public_field_exposure_rejected) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "private Hidden : struct { private value : Bool; }\n"
      "public Packet : struct { public hidden : Hidden; }"_view;
  EXPECT(rejects_finalize(source));
}

PERIMORTEM_UNIT_TEST(StructureTests, public_callable_exposure_rejected) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct {}\n"
    "public Packet : struct { public reveal : func = [Hidden] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct {}\n"
    "public Packet : struct { public reveal : func = [] -> Hidden {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public reveal : func = [Hidden] -> [] {}\n"
    "private Hidden : struct {}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_finalize(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, private_exposure_retained_locally) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "private Hidden : struct { private value : Bool; }\n"
      "public Packet : struct {\n"
      "  private hidden : Hidden;\n"
      "  private reveal : func = [.value : Hidden] -> Hidden { return value; "
      "}\n"
      "}\n"
      "private root : func = [.value : Hidden] -> Hidden { return value; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  const auto& source_type = monograph->get_source();
  auto types = source_type.get_types();
  auto source_callables = source_type.get_callables();
  ASSERT(types != types.end());
  ASSERT((*types).get().is<Language::Types::Structure>());
  const auto& hidden =
      static_cast<const Language::Types::Structure&>((*types).get());
  const Abstract& packet_identity = monograph->resolve_context("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  ASSERT(source_callables != source_callables.end());
  ASSERT((*source_callables).get().is<Language::Function>());
  const auto& root =
      static_cast<const Language::Function&>((*source_callables).get());
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  EXPECT(&monograph->resolve_context("root"_view) == &Invalid::get_invalid());
  EXPECT(&root.resolve_context("Hidden"_view) == &hidden);
  EXPECT(&packet.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("reveal"_view) == &Invalid::get_invalid());
  auto fields = packet.get_addressables();
  auto callables = packet.get_callables();
  ASSERT(fields != fields.end());
  ASSERT(callables != callables.end());
  ASSERT((*callables).get().is<Language::Function>());
  const auto& reveal =
      static_cast<const Language::Function&>((*callables).get());
  EXPECT(&reveal.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&reveal.resolve_context("Hidden"_view) == &hidden);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, initializer_fitting) {
  static constexpr View::Bytes source =
      "// Structure initializer test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private exact : Bool = false;\n"
      "  private copy : Bool = exact;\n"
      "  private narrow : Unsigned_8 = 255;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const Abstract& selected = monograph->resolve_context("Packet"_view);
  ASSERT(selected.is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(selected);
  auto authored_fields = packet.get_addressables();
  ASSERT(authored_fields != authored_fields.end());
  const auto& exact =
      static_cast<const Language::Field&>((*authored_fields).get());
  ++authored_fields;
  ASSERT(authored_fields != authored_fields.end());
  const auto& copy =
      static_cast<const Language::Field&>((*authored_fields).get());
  ++authored_fields;
  ASSERT(authored_fields != authored_fields.end());
  const auto& narrow =
      static_cast<const Language::Field&>((*authored_fields).get());
  auto exact_initializer = exact.get_initializer();
  auto copy_initializer = copy.get_initializer();
  auto narrow_initializer = narrow.get_initializer();
  ASSERT(exact_initializer);
  ASSERT(copy_initializer);
  ASSERT(narrow_initializer);

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  ASSERT(exact.get_initializer());
  ASSERT(copy.get_initializer());
  ASSERT(narrow.get_initializer());
  EXPECT(&*exact.get_initializer() == &*exact_initializer);
  EXPECT(&*copy.get_initializer() == &*copy_initializer);
  EXPECT(&*narrow.get_initializer() == &*narrow_initializer);
  ASSERT(copy_initializer->is<Language::Identifier>());
  const auto& identifier =
      static_cast<const Language::Identifier&>(*copy_initializer);
  EXPECT(&identifier.get_result() == &exact);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, inferred_source_and_nested_fields) {
  static constexpr View::Bytes source =
      "// Field inference test.\n"
      "dialect : Library;\n"
      "private root : Bool = false;\n"
      "private root_copy := root;\n"
      "public Packet : struct {\n"
      "  private scalar := 7;\n"
      "  private scalar_copy := scalar;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  const auto& source_type = monograph->get_source();
  auto types = source_type.get_types();
  auto type = types.begin();
  ASSERT(type != types.end());
  ASSERT((*type).get().is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>((*type).get());
  ++type;
  EXPECT(type == types.end());

  ASSERT(workspace.link(errors));
  auto source_fields = source_type.get_addressables();
  auto packet_fields = packet.get_addressables();
  auto source_field = source_fields.begin();
  ASSERT(source_field != source_fields.end());
  const auto& root = static_cast<const Language::Field&>((*source_field).get());
  ++source_field;
  ASSERT(source_field != source_fields.end());
  const auto& root_copy =
      static_cast<const Language::Field&>((*source_field).get());
  ++source_field;
  EXPECT(source_field == source_fields.end());
  auto packet_field = packet_fields.begin();
  ASSERT(packet_field != packet_fields.end());
  const auto& scalar =
      static_cast<const Language::Field&>((*packet_field).get());
  ++packet_field;
  ASSERT(packet_field != packet_fields.end());
  const auto& scalar_copy =
      static_cast<const Language::Field&>((*packet_field).get());
  ++packet_field;
  EXPECT(packet_field == packet_fields.end());
  EXPECT(&root_copy.get_type() == &root.get_type());
  EXPECT(&root.get_type() == &Dialect::get_bool());
  EXPECT(&scalar_copy.get_type() == &scalar.get_type());
  EXPECT(&scalar.get_type() == &Dialect::get_unsigned_64());
  ASSERT(root_copy.get_initializer());
  ASSERT(root_copy.get_initializer()->is<Language::Identifier>());
  const auto& root_identifier =
      static_cast<const Language::Identifier&>(*root_copy.get_initializer());
  EXPECT(&root_identifier.get_result() == &root);

  const Language::Field* source_identity = &root_copy;
  const Language::Field* nested_identity = &scalar_copy;
  ASSERT(monograph->link());
  ASSERT(monograph->link());
  auto retained_source_fields = source_type.get_addressables();
  auto retained_source_field = retained_source_fields.begin();
  ASSERT(retained_source_field != retained_source_fields.end());
  ++retained_source_field;
  ASSERT(retained_source_field != retained_source_fields.end());
  EXPECT(&(*retained_source_field).get() == source_identity);
  ++retained_source_field;
  EXPECT(retained_source_field == retained_source_fields.end());
  auto retained_packet_fields = packet.get_addressables();
  auto retained_packet_field = retained_packet_fields.begin();
  ASSERT(retained_packet_field != retained_packet_fields.end());
  ++retained_packet_field;
  ASSERT(retained_packet_field != retained_packet_fields.end());
  EXPECT(&(*retained_packet_field).get() == nested_identity);
  ++retained_packet_field;
  EXPECT(retained_packet_field == retained_packet_fields.end());
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, inference_failure_rolls_back) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Field inference test.\ndialect : Library; private value := missing;"_view,
    "// Field inference test.\ndialect : Library; private value := true + false;"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[i]);
    ASSERT(monograph);
    const auto& source_type = monograph->get_source();
    EXPECT_NOT(workspace.link(errors));
    auto fields = source_type.get_addressables();
    auto field = fields.begin();
    ASSERT(field != fields.end());
    const auto* identity = &static_cast<const Language::Field&>((*field).get());
    ++field;
    EXPECT(field == fields.end());
    EXPECT(&identity->resolve() == &Invalid::get_invalid());
    EXPECT_NOT(workspace.link(errors));
    auto retained_fields = source_type.get_addressables();
    auto retained_field = retained_fields.begin();
    ASSERT(retained_field != retained_fields.end());
    EXPECT(&(*retained_field).get() == identity);
    ++retained_field;
    EXPECT(retained_field == retained_fields.end());
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, inferred_public_type_reachability) {
  static constexpr View::Bytes source =
      "// Field inference test.\n"
      "dialect : Library;\n"
      "private Hidden : struct { private value : Bool; }\n"
      "private seed : Hidden;\n"
      "public revealed := seed;"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  const auto& source_type = monograph->get_source();
  auto fields = source_type.get_addressables();
  auto field = fields.begin();
  ASSERT(field != fields.end());
  ++field;
  ASSERT(field != fields.end());
  ++field;
  EXPECT(field == fields.end());
  EXPECT_NOT(workspace.finalize(errors));
  auto diagnostics = monograph->get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_message(),
      "Externally readable Field publishes an unreachable Type."_view);
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, initializer_mismatch_rejected) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Structure initializer test.\ndialect : Library; public Packet : struct { private value : Unsigned_8 = false; }"_view,
    "// Structure initializer test.\ndialect : Library; public Packet : struct { private value : Unsigned_8 = 256; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[i]);
    ASSERT(monograph);
    const auto& source_type = monograph->get_source();
    auto types = source_type.get_types();
    auto type = types.begin();
    ASSERT(type != types.end());
    ASSERT((*type).get().is<Language::Types::Structure>());
    const auto& packet =
        static_cast<const Language::Types::Structure&>((*type).get());
    ++type;
    EXPECT(type == types.end());
    auto authored_fields = packet.get_addressables();
    auto authored_field_selection = authored_fields.begin();
    ASSERT(authored_field_selection != authored_fields.end());
    const auto& authored_field =
        static_cast<const Language::Field&>((*authored_field_selection).get());
    ++authored_field_selection;
    EXPECT(authored_field_selection == authored_fields.end());
    auto authored_initializer = authored_field.get_initializer();
    ASSERT(authored_initializer);

    EXPECT_NOT(workspace.link(errors));
    auto fields = packet.get_addressables();
    auto field_selection = fields.begin();
    ASSERT(field_selection != fields.end());
    const auto& retained_field =
        static_cast<const Language::Field&>((*field_selection).get());
    ++field_selection;
    EXPECT(field_selection == fields.end());
    ASSERT(retained_field.get_initializer());
    EXPECT(&*retained_field.get_initializer() == &*authored_initializer);
    auto diagnostics = monograph->get_diagnostics();
    ASSERT_EQ(diagnostics.get_size(), Count(1));
    ASSERT(diagnostics.get_data()[0].get_anchor());
    EXPECT(
        diagnostics.get_data()[0].get_anchor()->get_span().caculate_text(
            sources[i]) == (i == 0 ? "false"_view : "256"_view));
    EXPECT_TEXT(
        diagnostics.get_data()[0].get_message(),
        "Field initializer does not fit the declared Field Type's semantic "
        "domain."_view);
    EXPECT_TEXT(
        diagnostics.get_data()[0].get_hint(),
        "Supply one value accepted by the declared Field Type."_view);
    EXPECT_NOT(errors.is_empty());
  }
}
