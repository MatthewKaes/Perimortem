// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static_assert(!__is_constructible(
    Language::Types::Structure,
    const Language::Types::Structure&));
static_assert(!__is_constructible(
    Language::Types::Structure,
    Language::Types::Structure&&));

class EmptyContext : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "EmptyContext"_view;
  }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

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

PERIMORTEM_UNIT_TEST(StructureTests, stable_authored_graph) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Later : struct { public value : Unsigned_64; }\n"
      "private Hidden : struct { public flag : Bool; }\n"
      "public Packet : struct {\n"
      "  // Later field.\n"
      "  public later : Later;\n"
      "  public const initialized : Bool = false;\n"
      "  private hidden : Hidden;\n"
      "  public func inspect[] -> Packet {}\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(3));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
  ASSERT(bindings.get_data()[1].get().is<Language::Types::Structure>());
  ASSERT(bindings.get_data()[2].get().is<Language::Types::Structure>());
  const auto& later = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[0].get());
  const auto& hidden = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[1].get());
  const auto& packet = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[2].get());
  const auto& source_structure =
      static_cast<const Language::Types::Structure&>(monograph->get_source());
  auto exposed_types = source_structure.get_external_static_bindings();
  ASSERT_EQ(exposed_types.get_size(), Count(2));
  EXPECT(&exposed_types.get_data()[0].get() == &later);
  EXPECT(&exposed_types.get_data()[1].get() == &packet);
  EXPECT(&monograph->resolve_context("source"_view) == &source_structure);
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  const Abstract& reserved = monograph->resolve_context("Packet"_view);
  EXPECT(&reserved == &packet);
  EXPECT(&packet.resolve() == &Invalid::get_invalid());
  ASSERT(packet.get_anchor());
  EXPECT_TEXT(
      packet.get_anchor()->get_span().caculate_text(source),
      "public Packet : struct {\n"
      "  // Later field.\n"
      "  public later : Later;\n"
      "  public const initialized : Bool = false;\n"
      "  private hidden : Hidden;\n"
      "  public func inspect[] -> Packet {}\n"
      "}"_view);
  EXPECT_TEXT(
      packet.get_anchor()->get_token().caculate_text(source), "struct"_view);
  ASSERT(packet.get_name_anchor());
  EXPECT_TEXT(
      packet.get_name_anchor()->get_span().caculate_text(source),
      "Packet"_view);
  auto authored_fields = packet.get_field_sources();
  ASSERT_EQ(authored_fields.get_size(), Count(3));
  const Language::Field::Source& authored_later = authored_fields.get_data()[0];
  EXPECT(authored_later.get_exposure() == Language::Field::Exposure::Public);
  EXPECT(
      authored_later.get_writability() == Language::Field::Writability::Full);
  EXPECT_TEXT(authored_later.get_name(), "later"_view);
  ASSERT(authored_later.get_type_access());
  EXPECT_TEXT(authored_later.get_type_access()->get_route(), "Later"_view);
  EXPECT_NOT(authored_later.get_documentation().is_empty());
  EXPECT_TEXT(
      authored_later.get_anchor().get_span().caculate_text(source),
      "public later : Later;"_view);
  ASSERT(authored_later.get_type_anchor());
  EXPECT_TEXT(
      authored_later.get_type_anchor()->get_span().caculate_text(source),
      "Later"_view);

  ASSERT(workspace.link(errors));
  EXPECT(&packet.resolve() == &packet);
  ASSERT(workspace.finalize(errors));
  EXPECT(packet.is_finalized());

  auto fields = packet.get_fields();
  auto public_fields = packet.get_public_fields();
  auto callables = packet.get_callables();
  auto public_callables = packet.get_public_callables();
  ASSERT_EQ(fields.get_size(), Count(3));
  ASSERT_EQ(public_fields.get_size(), Count(2));
  ASSERT_EQ(callables.get_size(), Count(1));
  ASSERT_EQ(public_callables.get_size(), Count(1));
  EXPECT_TEXT(fields.get_data()[0].get().get_name(), "later"_view);
  EXPECT_TEXT(fields.get_data()[1].get().get_name(), "initialized"_view);
  EXPECT_TEXT(fields.get_data()[2].get().get_name(), "hidden"_view);
  EXPECT(&fields.get_data()[0].get().get_type() == &later);
  EXPECT(&fields.get_data()[2].get().get_type() == &hidden);
  EXPECT(
      fields.get_data()[1].get().get_writability() ==
      Language::Field::Writability::Init);
  EXPECT(&public_fields.get_data()[0].get() == &fields.get_data()[0].get());
  EXPECT(&public_fields.get_data()[1].get() == &fields.get_data()[1].get());
  EXPECT(&packet.resolve_context("later"_view) == &fields.get_data()[0].get());
  EXPECT(
      &packet.resolve_context("initialized"_view) ==
      &fields.get_data()[1].get());
  EXPECT(&packet.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("inspect"_view) == &Invalid::get_invalid());
  auto callable_bindings = packet.get_callable_bindings();
  ASSERT_EQ(callable_bindings.get_size(), Count(1));
  EXPECT(
      &callable_bindings.get_data()[0].get() == &callables.get_data()[0].get());
  ASSERT(callables.get_data()[0].get().is<Language::Function>());
  const auto& inspect =
      static_cast<const Language::Function&>(callables.get_data()[0].get());
  EXPECT(&inspect.get_source() == &*monograph);
  EXPECT(&inspect.get_host() == &packet);
  EXPECT(&inspect.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("Later"_view) == &Invalid::get_invalid());

  const Layouts::Named& layout = packet.get_layout();
  ASSERT_EQ(layout.get_size(), Count(3));
  ASSERT(layout.get_abstract(0));
  ASSERT(layout.get_abstract(1));
  ASSERT(layout.get_abstract(2));
  EXPECT(&*layout.get_abstract(0) == &fields.get_data()[0].get());
  EXPECT(&*layout.get_abstract(1) == &fields.get_data()[1].get());
  EXPECT(&*layout.get_abstract(2) == &fields.get_data()[2].get());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, nested_type_aliases) {
  static constexpr View::Bytes source =
      "// Nested Alias source.\n"
      "dialect : Library;\n"
      "// Hidden Type documentation.\n"
      "private Hidden : struct {}\n"
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
  auto authored = monograph->get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(3));
  const Abstract& hidden = authored.get_data()[0].get();
  const Abstract& packet_identity = authored.get_data()[1].get();
  const Abstract& selected_identity = authored.get_data()[2].get();
  ASSERT(hidden.is<Language::Types::Structure>());
  ASSERT(packet_identity.is<Language::Types::Structure>());
  ASSERT(selected_identity.is<Alias>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  const Abstract& visible_identity = packet.resolve_context("Visible"_view);
  ASSERT(visible_identity.is<Alias>());
  const auto& visible = static_cast<const Alias&>(visible_identity);
  EXPECT(&visible.get_target() == &hidden);
  EXPECT_EQ(visible.get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      visible.get_documentation().get_line(0),
      "Visible Alias documentation."_view);
  EXPECT_TEXT(
      visible.get_documentation().get_line(1),
      "Hidden Type documentation."_view);
  EXPECT(&packet.resolve_context("Flag"_view) == &Invalid::get_invalid());
  auto type_bindings = packet.get_static_bindings();
  ASSERT_EQ(type_bindings.get_size(), Count(2));
  EXPECT(&type_bindings.get_data()[0].get() == &visible_identity);
  ASSERT(type_bindings.get_data()[1].get().is<Alias>());
  EXPECT(&type_bindings.get_data()[1].get().resolve() == &Dialect::get_bool());

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(&selected_identity.resolve() == &hidden);
  auto fields = packet.get_fields();
  ASSERT_EQ(fields.get_size(), Count(2));
  EXPECT(&fields.get_data()[0].get().get_type() == &hidden);
  EXPECT(&fields.get_data()[1].get().get_type() == &Dialect::get_bool());
  EXPECT_EQ(packet.get_layout().get_size(), Count(2));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, nested_declaration_lifecycle) {
  static constexpr View::Bytes source =
      "// Nested declaration source.\n"
      "dialect : Library;\n"
      "public Container : struct {\n"
      "  public State : enum[Unsigned_8] { ready = 1; }\n"
      "  public Child : struct {\n"
      "    public Copy : alias = State;\n"
      "    public value : Copy;\n"
      "  }\n"
      "  public child : Child;\n"
      "}\n"
      "public Entity : object {\n"
      "  public StateCopy : alias = Container::State;\n"
      "  public value : StateCopy;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& container_identity =
      monograph->resolve_context("Container"_view);
  ASSERT(container_identity.is<Language::Types::Structure>());
  const auto& container =
      static_cast<const Language::Types::Structure&>(container_identity);
  const Abstract& state = container.resolve_context("State"_view);
  const Abstract& child_identity = container.resolve_context("Child"_view);
  ASSERT(state.is<Language::Types::Enumeration>());
  ASSERT(child_identity.is<Language::Types::Structure>());
  const auto& child =
      static_cast<const Language::Types::Structure&>(child_identity);
  const Abstract& copy = child.resolve_context("Copy"_view);
  ASSERT(copy.is<Alias>());
  EXPECT(&copy.resolve() == &state);

  auto child_fields = child.get_fields();
  ASSERT_EQ(child_fields.get_size(), Count(1));
  EXPECT(&child_fields.get_data()[0].get().get_type() == &state);
  EXPECT_EQ(child.get_layout().get_size(), Count(1));
  auto container_fields = container.get_fields();
  ASSERT_EQ(container_fields.get_size(), Count(1));
  EXPECT(&container_fields.get_data()[0].get().get_type() == &child);
  EXPECT_EQ(container.get_layout().get_size(), Count(1));

  const Abstract& entity_identity = monograph->resolve_context("Entity"_view);
  ASSERT(entity_identity.is<Language::Types::Object>());
  const auto& entity =
      static_cast<const Language::Types::Object&>(entity_identity);
  const Abstract& state_copy = entity.resolve_context("StateCopy"_view);
  ASSERT(state_copy.is<Alias>());
  EXPECT(&state_copy.resolve() == &state);
  auto entity_fields = entity.get_fields();
  ASSERT_EQ(entity_fields.get_size(), Count(1));
  EXPECT(&entity_fields.get_data()[0].get().get_type() == &state);
  EXPECT_EQ(entity.get_layout().get_size(), Count(1));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    StructureTests,
    recursive_types_settle_before_reversed_sibling_fields) {
  static constexpr View::Bytes source =
      "// Reversed sibling source.\n"
      "dialect : Library;\n"
      "public Consumer : struct { public selected : Provider::State; }\n"
      "public Provider : struct {\n"
      "  public State : enum[Unsigned_8] { ready = 1; }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(monograph->get_source().is<Language::Types::Source>());
  auto& source_root =
      static_cast<Language::Types::Source&>(monograph->get_source());
  const Layouts::Named& source_layout = source_root.get_layout();
  EXPECT(source_layout.is_empty());

  const Abstract& consumer_identity =
      monograph->resolve_context("Consumer"_view);
  const Abstract& provider_identity =
      monograph->resolve_context("Provider"_view);
  ASSERT(consumer_identity.is<Language::Types::Structure>());
  ASSERT(provider_identity.is<Language::Types::Structure>());
  const auto& consumer =
      static_cast<const Language::Types::Structure&>(consumer_identity);
  const auto& provider =
      static_cast<const Language::Types::Structure&>(provider_identity);
  const Abstract& state = provider.resolve_context("State"_view);
  ASSERT(state.is<Language::Types::Enumeration>());
  EXPECT(&state.resolve() == &Invalid::get_invalid());
  EXPECT(consumer.get_fields().is_empty());

  // This is the barrier Monograph repeats across its whole import closure. The
  // later sibling's nested Enumeration becomes complete while the earlier
  // sibling still has no published Field identity.
  ASSERT(source_root.link_types());
  EXPECT(&state.resolve() == &state);
  EXPECT(consumer.get_fields().is_empty());
  EXPECT_NOT(source_root.is_linked());

  ASSERT(source_root.link_fields());
  auto fields = consumer.get_fields();
  ASSERT_EQ(fields.get_size(), Count(1));
  const Language::Field& exact_field = fields.get_data()[0].get();
  EXPECT(&exact_field.get_type() == &state);
  EXPECT(&source_root.get_layout() == &source_layout);
  EXPECT(source_root.get_layout().is_empty());

  ASSERT(monograph->link());
  auto repeated_fields = consumer.get_fields();
  ASSERT_EQ(repeated_fields.get_size(), Count(1));
  EXPECT(&repeated_fields.get_data()[0].get() == &exact_field);
  ASSERT(monograph->finalize());
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

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[0].get());
  auto fields = packet.get_fields();
  auto public_fields = packet.get_public_fields();
  ASSERT_EQ(fields.get_size(), Count(6));
  ASSERT_EQ(public_fields.get_size(), Count(3));

  const Language::Field& ordinary_public = fields.get_data()[0].get();
  const Language::Field& ordinary_private = fields.get_data()[1].get();
  const Language::Field& state_exposed = fields.get_data()[2].get();
  const Language::Field& state_private = fields.get_data()[3].get();
  const Language::Field& const_public = fields.get_data()[4].get();
  const Language::Field& const_private = fields.get_data()[5].get();
  EXPECT(ordinary_public.get_exposure() == Language::Field::Exposure::Public);
  EXPECT(
      ordinary_public.get_writability() == Language::Field::Writability::Full);
  EXPECT(ordinary_private.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(
      ordinary_private.get_writability() == Language::Field::Writability::Full);
  EXPECT(state_exposed.get_exposure() == Language::Field::Exposure::Exposed);
  EXPECT(
      state_exposed.get_writability() ==
      Language::Field::Writability::Internal);
  EXPECT(state_private.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(
      state_private.get_writability() ==
      Language::Field::Writability::Internal);
  EXPECT(const_public.get_exposure() == Language::Field::Exposure::Public);
  EXPECT(const_public.get_writability() == Language::Field::Writability::Init);
  EXPECT(const_private.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(const_private.get_writability() == Language::Field::Writability::Init);

  EXPECT(&public_fields.get_data()[0].get() == &ordinary_public);
  EXPECT(&public_fields.get_data()[1].get() == &state_exposed);
  EXPECT(&public_fields.get_data()[2].get() == &const_public);
  EXPECT(&packet.resolve_context("ordinary_public"_view) == &ordinary_public);
  EXPECT(
      &packet.resolve_context("ordinary_private"_view) ==
      &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("state_exposed"_view) == &state_exposed);
  EXPECT(
      &packet.resolve_context("state_private"_view) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("const_public"_view) == &const_public);
  EXPECT(
      &packet.resolve_context("const_private"_view) == &Invalid::get_invalid());
  EmptyContext external;
  EXPECT(packet.is_readable(ordinary_public, external));
  EXPECT_NOT(packet.is_readable(ordinary_private, external));
  EXPECT(packet.is_readable(state_exposed, external));
  EXPECT_NOT(packet.is_readable(state_private, external));
  EXPECT(packet.is_readable(const_public, external));
  EXPECT_NOT(packet.is_readable(const_private, external));
  EXPECT(packet.is_readable(ordinary_private, ordinary_private));
  EXPECT(packet.is_readable(state_private, state_private));
  EXPECT(packet.is_readable(const_private, const_private));
  for (Count i = 0; i < fields.get_size(); i++) {
    EXPECT(&fields.get_data()[i].get().get_host() == &packet);
  }
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
    const Abstract& field = first.resolve_context("next"_view);
    ASSERT(field.is<Addressable>());
    EXPECT(&static_cast<const Addressable&>(field).get_type() == &second);
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, category_names_coexist) {
  static constexpr Static::Vector<View::Bytes, 3> accepted = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; public func value[] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public func value[] -> [] {} public func value[self] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; public func inspect[self, .value : Bool] -> Bool { return value; } }"_view,
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
    "public Packet : struct { public value : Bool; public func read[] -> Bool { return value; } }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public value : Bool; public func read[self] -> Bool { return value; } }"_view,
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
      "  public func read[self] -> Bool { return self.value; }\n"
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
  auto fields = packet.get_fields();
  auto callables = packet.get_callables();
  ASSERT_EQ(fields.get_size(), Count(1));
  ASSERT_EQ(callables.get_size(), Count(1));
  ASSERT(callables.get_data()[0].get().is<Language::Function>());
  const auto& read =
      static_cast<const Language::Function&>(callables.get_data()[0].get());
  auto returned = read.get_return_expression();
  ASSERT(returned);
  ASSERT(returned->is<Language::Access::Address>());
  const auto& address =
      static_cast<const Language::Access::Address&>(*returned);
  ASSERT(address.get_addressable());
  EXPECT(&*address.get_addressable() == &fields.get_data()[0].get());
  ASSERT(address.get_receiver().is<Language::Identifier>());
  const auto& receiver =
      static_cast<const Language::Identifier&>(address.get_receiver());
  ASSERT(receiver.get_addressable());
  EXPECT_TEXT(receiver.get_addressable()->get_name(), "self"_view);
  EXPECT(&receiver.get_addressable()->get_type() == &packet);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 11> sources = {{
    "// Structure test.\ndialect : Library; public Packet struct {}"_view,
    "// Structure test.\ndialect : Library; public Packet : wrong {}"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public Value : Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value Bool; }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public value : Bool }"_view,
    "// Structure test.\ndialect : Library; public Packet : struct { public const value : Bool; }"_view,
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

PERIMORTEM_UNIT_TEST(StructureTests, public_field_exposure_rejected) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "private Hidden : struct {}\n"
      "public Packet : struct { public hidden : Hidden; }"_view;
  EXPECT(rejects_finalize(source));
}

PERIMORTEM_UNIT_TEST(StructureTests, public_callable_exposure_rejected) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct {}\n"
    "public Packet : struct { public func reveal[Hidden] -> [] {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "private Hidden : struct {}\n"
    "public Packet : struct { public func reveal[] -> Hidden {} }"_view,
    "// Structure test.\n"
    "dialect : Library;\n"
    "public func reveal[Hidden] -> [] {}\n"
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
      "private Hidden : struct {}\n"
      "public Packet : struct {\n"
      "  private hidden : Hidden;\n"
      "  private func reveal[Hidden] -> Hidden {}\n"
      "}\n"
      "private func root[Hidden] -> Hidden {}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(3));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
  ASSERT(bindings.get_data()[1].get().is<Language::Types::Structure>());
  ASSERT(bindings.get_data()[2].get().is<Language::Function>());
  const auto& hidden = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[0].get());
  const auto& packet = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[1].get());
  const auto& root =
      static_cast<const Language::Function&>(bindings.get_data()[2].get());
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  EXPECT(&monograph->resolve_context("root"_view) == &Invalid::get_invalid());
  EXPECT(&root.get_host() == &monograph->get_source());
  EXPECT(&root.resolve_context("Hidden"_view) == &hidden);
  EXPECT(&packet.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("reveal"_view) == &Invalid::get_invalid());
  auto fields = packet.get_fields();
  auto callables = packet.get_callables();
  ASSERT_EQ(fields.get_size(), Count(1));
  ASSERT_EQ(callables.get_size(), Count(1));
  ASSERT(callables.get_data()[0].get().is<Language::Function>());
  const auto& reveal =
      static_cast<const Language::Function&>(callables.get_data()[0].get());
  EXPECT(&reveal.get_host() == &packet);
  EXPECT(&reveal.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&reveal.resolve_context("Hidden"_view) == &hidden);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, authenticated_host_access) {
  static constexpr View::Bytes source =
      "// Structure authentication test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private const seed : Bool = false;\n"
      "  private const copy : Bool = seed;\n"
      "  private func inspect[] -> [] {}\n"
      "}\n"
      "public Other : struct { private value : Bool; }\n"
      "private func root[] -> [] {}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(3));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
  ASSERT(bindings.get_data()[1].get().is<Language::Types::Structure>());
  ASSERT(bindings.get_data()[2].get().is<Language::Function>());
  const auto& packet = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[0].get());
  const auto& other = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[1].get());
  const auto& root =
      static_cast<const Language::Function&>(bindings.get_data()[2].get());
  auto& source_structure =
      static_cast<Language::Types::Source&>(monograph->get_source());
  auto packet_callables = packet.get_callables();
  ASSERT_EQ(packet_callables.get_size(), Count(1));
  ASSERT(packet_callables.get_data()[0].get().is<Language::Function>());
  const auto& inspect = static_cast<const Language::Function&>(
      packet_callables.get_data()[0].get());

  Count source_bindings = source_structure.get_static_bindings().get_size();
  EXPECT_NOT(source_structure.bind_static(
      const_cast<Language::Function&>(inspect), Language::Visibility::Private));
  EmptyContext extra;
  EXPECT_EQ(source_structure.get_static_bindings().get_size(), source_bindings);

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  auto packet_fields = packet.get_fields();
  auto other_fields = other.get_fields();
  ASSERT_EQ(packet_fields.get_size(), Count(2));
  ASSERT_EQ(other_fields.get_size(), Count(1));
  const Language::Field& seed = packet_fields.get_data()[0].get();
  const Language::Field& copy = packet_fields.get_data()[1].get();
  const Language::Field& foreign = other_fields.get_data()[0].get();
  auto initializer = copy.get_initializer();
  ASSERT(initializer);
  ASSERT(initializer->is<Language::Identifier>());
  const auto& identifier =
      static_cast<const Language::Identifier&>(*initializer);
  auto addressable = identifier.get_addressable();
  ASSERT(addressable);
  EXPECT(&*addressable == &seed);

  EXPECT(&packet.resolve_context("seed"_view) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("seed"_view, seed) == &seed);
  EXPECT(
      &packet.resolve_context("seed"_view, foreign) == &Invalid::get_invalid());
  EXPECT(
      &packet.resolve_context("seed"_view, inspect) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("seed"_view, root) == &Invalid::get_invalid());
  EXPECT(&packet.resolve_context("seed"_view) == &Invalid::get_invalid());
  EXPECT(
      &source_structure.resolve_context("root"_view, *monograph) ==
      &Invalid::get_invalid());
  EXPECT(packet.is_readable(seed, inspect));
  EXPECT_NOT(packet.is_readable(seed, root));
  EXPECT_NOT(packet.is_readable(foreign, inspect));
  auto packet_bindings = packet.get_callable_bindings(inspect);
  auto external_packet_bindings = packet.get_callable_bindings(root);
  auto source_bindings_view =
      source_structure.get_callable_bindings(*monograph);
  ASSERT_EQ(packet_bindings.get_size(), Count(1));
  EXPECT(packet_bindings.get_data()[0].get().get_name() == "inspect"_view);
  EXPECT(external_packet_bindings.is_empty());
  ASSERT_EQ(source_bindings_view.get_size(), Count(1));
  EXPECT(&source_bindings_view.get_data()[0].get() == &root);

  EXPECT_NOT(source_structure.can_bind_static(extra));
  EXPECT_NOT(
      source_structure.bind_static(extra, Language::Visibility::Private));
  EXPECT_EQ(source_structure.get_static_bindings().get_size(), source_bindings);
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
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[0].get());
  auto authored_fields = packet.get_field_sources();
  ASSERT_EQ(authored_fields.get_size(), Count(3));
  auto exact_initializer = authored_fields.get_data()[0].get_initializer();
  auto copy_initializer = authored_fields.get_data()[1].get_initializer();
  auto narrow_initializer = authored_fields.get_data()[2].get_initializer();
  ASSERT(exact_initializer);
  ASSERT(copy_initializer);
  ASSERT(narrow_initializer);

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto fields = packet.get_fields();
  ASSERT_EQ(fields.get_size(), Count(3));
  ASSERT(fields.get_data()[0].get().get_initializer());
  ASSERT(fields.get_data()[1].get().get_initializer());
  ASSERT(fields.get_data()[2].get().get_initializer());
  EXPECT(&*fields.get_data()[0].get().get_initializer() == &*exact_initializer);
  EXPECT(&*fields.get_data()[1].get().get_initializer() == &*copy_initializer);
  EXPECT(
      &*fields.get_data()[2].get().get_initializer() == &*narrow_initializer);
  ASSERT(copy_initializer->is<Language::Identifier>());
  const auto& identifier =
      static_cast<const Language::Identifier&>(*copy_initializer);
  ASSERT(identifier.get_addressable());
  EXPECT(&*identifier.get_addressable() == &fields.get_data()[0].get());
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
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
  const auto& packet = static_cast<const Language::Types::Structure&>(
      bindings.get_data()[0].get());
  const auto& source_structure =
      static_cast<const Language::Types::Source&>(monograph->get_source());

  ASSERT(workspace.link(errors));
  auto source_fields = source_structure.get_fields();
  auto packet_fields = packet.get_fields();
  ASSERT_EQ(source_fields.get_size(), Count(2));
  ASSERT_EQ(packet_fields.get_size(), Count(2));
  const Language::Field& root = source_fields.get_data()[0].get();
  const Language::Field& root_copy = source_fields.get_data()[1].get();
  const Language::Field& scalar = packet_fields.get_data()[0].get();
  const Language::Field& scalar_copy = packet_fields.get_data()[1].get();
  EXPECT(&root_copy.get_type() == &root.get_type());
  EXPECT(&root.get_type() == &Dialect::get_bool());
  EXPECT(&scalar_copy.get_type() == &scalar.get_type());
  EXPECT(&scalar.get_type() == &Dialect::get_unsigned_64());
  EXPECT_NOT(root_copy.get_type_access());
  EXPECT_NOT(scalar.get_type_access());
  ASSERT(root_copy.get_initializer());
  ASSERT(root_copy.get_initializer()->is<Language::Identifier>());
  const auto& root_identifier =
      static_cast<const Language::Identifier&>(*root_copy.get_initializer());
  ASSERT(root_identifier.get_addressable());
  EXPECT(&*root_identifier.get_addressable() == &root);

  const Language::Field* source_identity = &root_copy;
  const Language::Field* nested_identity = &scalar_copy;
  ASSERT(monograph->link());
  ASSERT(monograph->link());
  EXPECT(&source_structure.get_fields().get_data()[1].get() == source_identity);
  EXPECT(&packet.get_fields().get_data()[1].get() == nested_identity);
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
    const auto& source_structure =
        static_cast<const Language::Types::Source&>(monograph->get_source());
    EXPECT_NOT(workspace.link(errors));
    EXPECT(source_structure.get_fields().is_empty());
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(StructureTests, inferred_public_type_reachability) {
  static constexpr View::Bytes source =
      "// Field inference test.\n"
      "dialect : Library;\n"
      "private Hidden : struct {}\n"
      "private seed : Hidden;\n"
      "public revealed := seed;"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  const auto& source_structure =
      static_cast<const Language::Types::Source&>(monograph->get_source());
  ASSERT_EQ(source_structure.get_fields().get_size(), Count(2));
  EXPECT_NOT(workspace.finalize(errors));
  auto diagnostics = monograph->get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_message(),
      "Externally readable Structure Field publishes an unreachable Type."_view);
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, inferred_new_rejected) {
  static constexpr View::Bytes source = "private value := new;"_view;
  Allocator::Arena arena;
  EmptyContext context;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "inferred-new.ttx"_view);
  Cursor cursor(tokenizer, errors);
  Token opening = cursor.current();
  auto field = Language::Field::interpret(
      arena, materializations, cursor, Documentation::get_empty(), context);

  EXPECT_NOT(field);
  EXPECT_EQ(cursor.current().get_offset(), opening.get_offset());
  EXPECT(cursor.current().get_code() == opening.get_code());
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
    auto bindings = monograph->get_authored_bindings();
    ASSERT_EQ(bindings.get_size(), Count(1));
    ASSERT(bindings.get_data()[0].get().is<Language::Types::Structure>());
    const auto& packet = static_cast<const Language::Types::Structure&>(
        bindings.get_data()[0].get());
    auto authored_fields = packet.get_field_sources();
    ASSERT_EQ(authored_fields.get_size(), Count(1));
    auto authored_initializer = authored_fields.get_data()[0].get_initializer();
    ASSERT(authored_initializer);

    EXPECT_NOT(workspace.link(errors));
    auto fields = packet.get_fields();
    ASSERT_EQ(fields.get_size(), Count(1));
    const Language::Field& field = fields.get_data()[0].get();
    ASSERT(field.get_initializer());
    EXPECT(&*field.get_initializer() == &*authored_initializer);
    EXPECT_NOT(field.is_linked());
    EXPECT_NOT(packet.is_finalized());
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

PERIMORTEM_UNIT_TEST(StructureTests, repeat_lifecycle) {
  static constexpr View::Bytes source =
      "// Structure test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public value : Unsigned_64; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(monograph->link());
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  ASSERT(monograph->finalize());
  EXPECT(monograph->get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, lifecycle_order_rejected) {
  static constexpr View::Bytes source =
      "public Packet : struct { public value : Bool; }"_view;
  EmptyContext context;
  Allocator::Arena arena;
  Dialect dialect;
  Language::Materializations materializations(arena);
  auto& monograph = Language::Monograph::create_authored(
      arena, Documentation::get_empty(), dialect, context, materializations);
  ASSERT(monograph.get_source().is<Language::Types::Source>());
  const auto& source_scope =
      static_cast<const Language::Types::Source&>(monograph.get_source());
  Errors errors;
  Tokenizer tokenizer(arena, source, "structure-stage.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto structure = Language::Types::Structure::interpret(
      arena, cursor, Documentation::get_empty(), monograph, materializations,
      source_scope);
  ASSERT(structure);

  EmptyContext extra;
  EXPECT_NOT(structure->can_bind_member(extra));
  EXPECT_NOT(structure->bind_member(extra, Language::Visibility::Private));
  EXPECT(structure->get_static_bindings().is_empty());
  EXPECT_NOT(structure->link_callable_signatures());
  EXPECT_NOT(structure->link_callable_bodies());
  EXPECT_NOT(structure->finalize());
  EXPECT_EQ(monograph.get_diagnostics().get_size(), Count(3));
  EXPECT_NOT(structure->is_linked());
  EXPECT_NOT(structure->is_finalized());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, field_cursor_atomicity) {
  static constexpr View::Bytes malformed = "public value : Bool"_view;
  static constexpr View::Bytes complete = "private value : Core::Bool;"_view;
  Allocator::Arena arena;
  EmptyContext context;
  Language::Materializations materializations(arena);
  Errors malformed_errors;
  Tokenizer malformed_tokenizer(arena, malformed, "malformed-field.ttx"_view);
  Cursor malformed_cursor(malformed_tokenizer, malformed_errors);
  Token opening = malformed_cursor.current();
  auto rejected = Language::Field::interpret(
      arena, materializations, malformed_cursor, Documentation::get_empty(),
      context);
  EXPECT_NOT(rejected);
  EXPECT_EQ(malformed_cursor.current().get_offset(), opening.get_offset());
  EXPECT_NOT(malformed_errors.is_empty());

  Errors complete_errors;
  Tokenizer complete_tokenizer(arena, complete, "complete-field.ttx"_view);
  Cursor complete_cursor(complete_tokenizer, complete_errors);
  auto field = Language::Field::interpret(
      arena, materializations, complete_cursor, Documentation::get_empty(),
      context);
  ASSERT(field);
  EXPECT_TEXT(field->get_name(), "value"_view);
  ASSERT(field->get_type_access());
  EXPECT_TEXT(field->get_type_access()->get_route(), "Core::Bool"_view);
  EXPECT_TEXT(field->get_anchor().get_span().caculate_text(complete), complete);
  ASSERT(field->get_type_anchor());
  EXPECT_TEXT(
      field->get_type_anchor()->get_span().caculate_text(complete),
      "Core::Bool"_view);
  EXPECT(field->get_exposure() == Language::Field::Exposure::Private);
  EXPECT(field->get_writability() == Language::Field::Writability::Full);
  EXPECT(complete_cursor.matches(Code::Type::Terminal));
  EXPECT(complete_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(StructureTests, cursor_atomicity) {
  static constexpr View::Bytes malformed =
      "public Packet : struct { public value : Bool }"_view;
  static constexpr View::Bytes complete =
      "public Packet : struct { public value : Bool; }"_view;
  EmptyContext context;
  Allocator::Arena arena;
  Dialect dialect;
  Language::Materializations materializations(arena);
  auto& monograph = Language::Monograph::create_authored(
      arena, Documentation::get_empty(), dialect, context, materializations);
  ASSERT(monograph.get_source().is<Language::Types::Source>());
  const auto& source_scope =
      static_cast<const Language::Types::Source&>(monograph.get_source());

  Errors malformed_errors;
  Tokenizer malformed_tokenizer(
      arena, malformed, "malformed-structure.ttx"_view);
  Cursor malformed_cursor(malformed_tokenizer, malformed_errors);
  Token opening = malformed_cursor.current();
  auto rejected = Language::Types::Structure::interpret(
      arena, malformed_cursor, Documentation::get_empty(), monograph,
      materializations, source_scope);
  EXPECT_NOT(rejected);
  EXPECT_EQ(malformed_cursor.current().get_offset(), opening.get_offset());
  EXPECT(malformed_cursor.current().get_code() == opening.get_code());
  EXPECT_NOT(malformed_errors.is_empty());

  Errors complete_errors;
  Tokenizer complete_tokenizer(arena, complete, "complete-structure.ttx"_view);
  Cursor complete_cursor(complete_tokenizer, complete_errors);
  auto parsed = Language::Types::Structure::interpret(
      arena, complete_cursor, Documentation::get_empty(), monograph,
      materializations, source_scope);
  ASSERT(parsed);
  EXPECT(complete_cursor.matches(Code::Type::Terminal));
  EXPECT(complete_errors.is_empty());
}
