// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/object.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
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

static_assert(
    __is_base_of(Language::Types::Structure, Language::Types::Object));
static_assert(__is_same(
    decltype(&Language::Types::Object::get_fields),
    decltype(&Language::Types::Composite::get_fields)));
static_assert(__is_same(
    decltype(&Language::Types::Object::get_callables),
    decltype(&Language::Types::Composite::get_callables)));
static_assert(__is_same(
    decltype(&Language::Types::Object::get_layout),
    decltype(&Language::Types::Composite::get_layout)));
static_assert(__is_same(
    decltype(&Language::Types::Object::link_fields),
    decltype(&Language::Types::Composite::link_fields)));
static_assert(__is_same(
    decltype(&Language::Types::Object::finalize),
    decltype(&Language::Types::Composite::finalize)));
static_assert(!__is_constructible(
    Language::Types::Object,
    const Language::Types::Object&));
static_assert(
    !__is_constructible(Language::Types::Object, Language::Types::Object&&));

class EmptyContext : public Type {
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
      errors, "ObjectTest"_view, "object.ttx"_view, source);
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

static Harness ObjectTests = {
  .name = "Tetrodotoxin::Library::Language::Types::Object"_view,
};

PERIMORTEM_UNIT_TEST(ObjectTests, stable_authored_graph) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Later : object { private state ready : Bool = false; }\n"
      "private Hidden : object {}\n"
      "// Session documentation.\n"
      "public Session : object {\n"
      "  // Progress documentation.\n"
      "  expose state progress : Unsigned_64 = 0;\n"
      "  private state token : Unsigned_64 = progress;\n"
      "  public advance : func = [] -> Unsigned_64 { return 0; }\n"
      "  private token_value : func = [] -> Unsigned_64 { return 0; }\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(3));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Object>());
  ASSERT(bindings.get_data()[1].get().is<Language::Types::Object>());
  ASSERT(bindings.get_data()[2].get().is<Language::Types::Object>());
  const auto& later =
      static_cast<const Language::Types::Object&>(bindings.get_data()[0].get());
  const auto& hidden =
      static_cast<const Language::Types::Object&>(bindings.get_data()[1].get());
  const auto& session =
      static_cast<const Language::Types::Object&>(bindings.get_data()[2].get());
  const auto& structure =
      static_cast<const Language::Types::Structure&>(session);
  EXPECT(&structure == &session);
  EXPECT(session.is<Language::Types::Composite>());
  EXPECT(session.is<Language::Types::Structure>());
  const auto& source_type =
      static_cast<const Language::Types::Source&>(monograph->get_source());
  auto exposed_objects = source_type.get_external_static_bindings();
  ASSERT_EQ(exposed_objects.get_size(), Count(2));
  EXPECT(&exposed_objects.get_data()[0].get() == &later);
  EXPECT(&exposed_objects.get_data()[1].get() == &session);
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  EXPECT(&hidden != &session);
  const Abstract& reserved = monograph->resolve_context("Session"_view);
  EXPECT(&reserved == &session);
  EXPECT(&session.resolve() == &Invalid::get_invalid());
  EXPECT_NOT(session.get_documentation().is_empty());
  ASSERT(session.get_anchor());
  EXPECT_TEXT(
      session.get_anchor()->get_token().caculate_text(source), "object"_view);
  EXPECT_TEXT(
      session.get_definition().get_name_anchor().get_span().caculate_text(
          source),
      "Session"_view);

  auto authored_fields = session.get_fields();
  ASSERT_EQ(authored_fields.get_size(), Count(2));
  const Language::Field& progress = authored_fields.get_data()[0].get();
  const Language::Field& token = authored_fields.get_data()[1].get();
  EXPECT(progress.get_exposure() == Language::Field::Exposure::Exposed);
  EXPECT(progress.get_writability() == Language::Field::Writability::Internal);
  EXPECT_NOT(progress.get_documentation().is_empty());
  EXPECT_TEXT(progress.get_name(), "progress"_view);
  ASSERT(progress.get_type_access());
  EXPECT_TEXT(progress.get_type_access()->get_route(), "Unsigned_64"_view);
  EXPECT(token.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(token.get_writability() == Language::Field::Writability::Internal);
  ASSERT(progress.get_initializer());
  ASSERT(token.get_initializer());
  EXPECT_TEXT(
      progress.get_initializer()->get_anchor()->get_span().caculate_text(
          source),
      "0"_view);
  EXPECT_TEXT(
      token.get_initializer()->get_anchor()->get_span().caculate_text(source),
      "progress"_view);
  EXPECT(token.get_initializer()->is<Language::Identifier>());

  ASSERT(workspace.link(errors));
  EXPECT(&session.resolve() == &session);
  ASSERT(workspace.finalize(errors));
  EXPECT(session.is_finalized());

  auto fields = session.get_fields();
  auto public_fields = session.get_public_fields();
  auto callables = session.get_callables();
  auto public_callables = session.get_public_callables();
  ASSERT_EQ(fields.get_size(), Count(2));
  ASSERT_EQ(public_fields.get_size(), Count(1));
  ASSERT_EQ(callables.get_size(), Count(2));
  ASSERT_EQ(public_callables.get_size(), Count(1));
  const Language::Field& progress_field = fields.get_data()[0].get();
  EXPECT(&progress == &progress_field);
  EXPECT(&token == &fields.get_data()[1].get());
  EXPECT(&public_fields.get_data()[0].get() == &progress_field);
  EXPECT(&progress_field.get_host() == &session);
  EXPECT(&fields.get_data()[1].get().get_host() == &session);
  EXPECT(
      progress_field.get_writability() ==
      Language::Field::Writability::Internal);
  EXPECT(&session.resolve_context("progress"_view) == &progress_field);
  EXPECT(&session.resolve_context("token"_view) == &Invalid::get_invalid());
  EXPECT(
      &session.resolve_context("token_value"_view) == &Invalid::get_invalid());
  EXPECT(
      &public_callables.get_data()[0].get() == &callables.get_data()[0].get());
  ASSERT(callables.get_data()[0].get().is<Language::Function>());
  ASSERT(callables.get_data()[1].get().is<Language::Function>());
  const auto& advance =
      static_cast<const Language::Function&>(callables.get_data()[0].get());
  const auto& token_value =
      static_cast<const Language::Function&>(callables.get_data()[1].get());
  EXPECT(&advance.get_host() == &session);
  EXPECT(&token_value.get_host() == &session);
  EXPECT(&token_value.resolve_context("token"_view) == &Invalid::get_invalid());

  const Layouts::Named& layout = session.get_layout();
  ASSERT_EQ(layout.get_size(), Count(2));
  ASSERT(layout.get_abstract(0));
  ASSERT(layout.get_abstract(1));
  EXPECT(&*layout.get_abstract(0) == &progress_field);
  EXPECT(&*layout.get_abstract(1) == &fields.get_data()[1].get());

  const auto& token_identifier =
      static_cast<const Language::Identifier&>(*token.get_initializer());
  ASSERT(token_identifier.get_addressable());
  EXPECT(&*token_identifier.get_addressable() == &progress_field);
  EXPECT_NOT(progress.get_initializer()->get_folded());
  EXPECT_NOT(token.get_initializer()->get_folded());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, exposure_and_writability) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object {\n"
      "  public open : Bool;\n"
      "  private closed : Bool;\n"
      "  expose state observed : Bool = false;\n"
      "  private state hidden_state : Bool = false;\n"
      "  public const fixed : Bool = false;\n"
      "  private const hidden_const : Bool = false;\n"
      "  public inspect : func = [] -> [] {}\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& selected = monograph->resolve_context("Session"_view);
  ASSERT(selected.is<Language::Types::Object>());
  const auto& object = static_cast<const Language::Types::Object&>(selected);
  auto fields = object.get_fields();
  auto public_fields = object.get_public_fields();
  ASSERT_EQ(fields.get_size(), Count(6));
  ASSERT_EQ(public_fields.get_size(), Count(3));

  // All six policy combinations stay on real Fields. Exposure controls which
  // identities enter external lookup without changing their write policy.
  const Language::Field& open = fields.get_data()[0].get();
  const Language::Field& closed = fields.get_data()[1].get();
  const Language::Field& observed = fields.get_data()[2].get();
  const Language::Field& hidden_state = fields.get_data()[3].get();
  const Language::Field& fixed = fields.get_data()[4].get();
  const Language::Field& hidden_const = fields.get_data()[5].get();
  EXPECT(open.get_exposure() == Language::Field::Exposure::Public);
  EXPECT(closed.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(observed.get_exposure() == Language::Field::Exposure::Exposed);
  EXPECT(hidden_state.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(fixed.get_exposure() == Language::Field::Exposure::Public);
  EXPECT(hidden_const.get_exposure() == Language::Field::Exposure::Private);
  EXPECT(open.get_writability() == Language::Field::Writability::Full);
  EXPECT(closed.get_writability() == Language::Field::Writability::Full);
  EXPECT(observed.get_writability() == Language::Field::Writability::Internal);
  EXPECT(
      hidden_state.get_writability() == Language::Field::Writability::Internal);
  EXPECT(fixed.get_writability() == Language::Field::Writability::Init);
  EXPECT(hidden_const.get_writability() == Language::Field::Writability::Init);
  // Address comparison catches a copied member even if it kept the same Type
  // and spelling. Both views must return the Field retained by Composite.
  EXPECT(&public_fields.get_data()[0].get() == &open);
  EXPECT(&public_fields.get_data()[1].get() == &observed);
  EXPECT(&public_fields.get_data()[2].get() == &fixed);
  EXPECT(&object.resolve_context("open"_view) == &open);
  EXPECT(&object.resolve_context("closed"_view) == &Invalid::get_invalid());
  EXPECT(&object.resolve_context("observed"_view) == &observed);
  EXPECT(
      &object.resolve_context("hidden_state"_view) == &Invalid::get_invalid());
  EXPECT(&object.resolve_context("fixed"_view) == &fixed);
  EXPECT(
      &object.resolve_context("hidden_const"_view) == &Invalid::get_invalid());

  // The exact host authenticates private Address access without supplying an
  // implicit receiver or giving Object another inventory.
  auto callables = object.get_callables();
  ASSERT_EQ(callables.get_size(), Count(1));
  ASSERT(callables.get_data()[0].get().is<Language::Function>());
  const auto& inspect =
      static_cast<const Language::Function&>(callables.get_data()[0].get());
  EXPECT(&inspect.get_host() == &object);
  EXPECT(&inspect.resolve_context("closed"_view) == &Invalid::get_invalid());
  EXPECT(
      &inspect.resolve_context("hidden_state"_view) == &Invalid::get_invalid());
  EXPECT(
      &inspect.resolve_context("hidden_const"_view) == &Invalid::get_invalid());
  EXPECT(object.is_readable(closed, inspect));
  EXPECT(object.is_readable(hidden_state, inspect));
  EXPECT(object.is_readable(hidden_const, inspect));
  for (Count i = 0; i < fields.get_size(); i++) {
    EXPECT(&fields.get_data()[i].get().get_host() == &object);
  }
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, declaration_reorder) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Object test.\n"
    "dialect : Library;\n"
    "public Packet : struct { public session : Session; }\n"
    "public Session : object { private packet : Packet; }\n"
    "public open : func = [Session] -> Packet {}"_view,
    "// Object test.\n"
    "dialect : Library;\n"
    "public open : func = [Session] -> Packet {}\n"
    "public Session : object { private packet : Packet; }\n"
    "public Packet : struct { public session : Session; }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    Workspace workspace;
    Errors errors;
    auto monograph = interpret(workspace, errors, sources[i]);
    ASSERT(monograph);
    ASSERT(workspace.link(errors));
    ASSERT(workspace.finalize(errors));

    const Abstract& packet = monograph->resolve_context("Packet"_view);
    const Abstract& session = monograph->resolve_context("Session"_view);
    ASSERT(packet.is<Language::Types::Structure>());
    ASSERT(session.is<Language::Types::Object>());
    const Abstract& session_field = packet.resolve_context("session"_view);
    ASSERT(session_field.is<Addressable>());
    const auto& object = static_cast<const Language::Types::Object&>(session);
    auto object_fields = object.get_fields();
    ASSERT_EQ(object_fields.get_size(), Count(1));
    const Addressable& packet_field = object_fields.get_data()[0].get();
    EXPECT(&session.resolve_context("packet"_view) == &Invalid::get_invalid());
    EXPECT(
        &static_cast<const Addressable&>(session_field).get_type() == &session);
    EXPECT(&packet_field.get_type() == &packet);
    EXPECT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, exact_collision_domain) {
  static constexpr Static::Vector<View::Bytes, 3> accepted = {{
    "// Object test.\ndialect : Library; public Session : object { public value : func = [] -> [] {} private state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { private state value : Bool = false; public value : func = [] -> [] {} }"_view,
    "// Object test.\ndialect : Library; public Session : object { public value : func = [] -> [] {} private value : func = [] -> [] {} }"_view,
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

  static constexpr Static::Vector<View::Bytes, 5> rejected = {{
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false; private state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : object {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : struct {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : enum[Unsigned_8] {}"_view,
    "// Object test.\ndialect : Library; public Same : object {} private Same : func = [] -> [] {}"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_interpretation(rejected[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, malformed_grammar) {
  static constexpr Static::Vector<View::Bytes, 10> sources = {{
    "// Object test.\ndialect : Library; public Session object {}"_view,
    "// Object test.\ndialect : Library; public Session : managed {}"_view,
    "// Object test.\ndialect : Library; public Session : object { state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { public state value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { public const value : Bool; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state Value : Bool = false; }"_view,
    "// Object test.\ndialect : Library; public Session : object { expose state value : Bool = false;"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, private_exposure_rejected) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
    "// Object test.\ndialect : Library; private Hidden : object {} public reveal : func = [Hidden] -> [] {}"_view,
    "// Object test.\ndialect : Library; private Hidden : object {} public Holder : struct { public hidden : Hidden; }"_view,
    "// Object test.\ndialect : Library; private Hidden : object {} public Holder : object { public hidden : Hidden; }"_view,
    "// Object test.\ndialect : Library; private Hidden : object {} public Holder : object { public reveal : func = [] -> Hidden {} }"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_finalize(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(ObjectTests, private_surface_retained_locally) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "private Hidden : object {}\n"
      "public Holder : object {\n"
      "  private hidden : Hidden;\n"
      "  private reveal : func = [Hidden] -> Hidden {}\n"
      "}\n"
      "private root : func = [Hidden] -> Hidden {}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(3));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Object>());
  ASSERT(bindings.get_data()[1].get().is<Language::Types::Object>());
  ASSERT(bindings.get_data()[2].get().is<Language::Function>());
  const Abstract& hidden = bindings.get_data()[0].get();
  const Abstract& holder = monograph->resolve_context("Holder"_view);
  EXPECT(hidden.is<Language::Types::Object>());
  ASSERT(holder.is<Language::Types::Object>());
  EXPECT(&monograph->resolve_context("Hidden"_view) == &Invalid::get_invalid());
  EXPECT(&holder.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&holder.resolve_context("reveal"_view) == &Invalid::get_invalid());
  EXPECT(&monograph->resolve_context("root"_view) == &Invalid::get_invalid());
  const auto& holder_object =
      static_cast<const Language::Types::Object&>(holder);
  auto fields = holder_object.get_fields();
  auto callables = holder_object.get_callables();
  ASSERT_EQ(fields.get_size(), Count(1));
  ASSERT_EQ(callables.get_size(), Count(1));
  ASSERT(callables.get_data()[0].get().is<Language::Function>());
  const auto& reveal =
      static_cast<const Language::Function&>(callables.get_data()[0].get());
  const auto& root =
      static_cast<const Language::Function&>(bindings.get_data()[2].get());
  const auto& source_type =
      static_cast<const Language::Types::Source&>(monograph->get_source());
  EXPECT(source_type.is<Language::Types::Source>());
  EXPECT(source_type.get_fields().is_empty());
  EXPECT_EQ(source_type.get_layout().get_size(), Count(0));
  EXPECT(&reveal.get_host() == &holder_object);
  EXPECT(&reveal.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&reveal.resolve_context("Hidden"_view) == &hidden);
  EXPECT(&root.get_source() == &*monograph);
  EXPECT(&root.get_host() == &source_type);
  EXPECT(
      &source_type.resolve_context("root"_view, *monograph) ==
      &Invalid::get_invalid());
  auto source_callables = source_type.get_callable_bindings(*monograph);
  ASSERT_EQ(source_callables.get_size(), Count(1));
  EXPECT(&source_callables.get_data()[0].get() == &root);
  EXPECT(&root.resolve_context("Hidden"_view) == &hidden);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, inherited_initializer_mismatch) {
  static constexpr View::Bytes source =
      "// Object initializer test.\n"
      "dialect : Library;\n"
      "public Session : object { private state value : Unsigned_8 = false; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Object>());
  const auto& object =
      static_cast<const Language::Types::Object&>(bindings.get_data()[0].get());
  auto authored_fields = object.get_fields();
  ASSERT_EQ(authored_fields.get_size(), Count(1));
  auto authored_initializer =
      authored_fields.get_data()[0].get().get_initializer();
  ASSERT(authored_initializer);

  EXPECT_NOT(workspace.link(errors));
  auto fields = object.get_fields();
  ASSERT_EQ(fields.get_size(), Count(1));
  const Language::Field& field = fields.get_data()[0].get();
  ASSERT(field.get_initializer());
  EXPECT(&*field.get_initializer() == &*authored_initializer);
  EXPECT_NOT(field.is_linked());
  EXPECT_NOT(object.is_finalized());
  auto diagnostics = monograph->get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_anchor()->get_span().caculate_text(source),
      "false"_view);
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, link_failure_keeps_publication_empty) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object { expose state value : Bool = missing; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Object>());
  const auto& object =
      static_cast<const Language::Types::Object&>(bindings.get_data()[0].get());
  EXPECT_NOT(workspace.link(errors));
  EXPECT(object.is_linked());
  ASSERT_EQ(object.get_public_fields().get_size(), Count(1));
  EXPECT(object.get_public_callables().is_empty());
  EXPECT(
      &workspace.resolve_context("ObjectTest"_view) == &Invalid::get_invalid());
  EXPECT_NOT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, missing_type_rejected) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object { private state value : Missing = false; }"_view;
  EXPECT(rejects_link(source));
}

PERIMORTEM_UNIT_TEST(ObjectTests, repeat_lifecycle) {
  static constexpr View::Bytes source =
      "// Object test.\n"
      "dialect : Library;\n"
      "public Session : object { expose state value : Bool = false; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(monograph->link());
  ASSERT(monograph->link());
  ASSERT(monograph->finalize());
  ASSERT(monograph->finalize());
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(1));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Object>());
  const auto& object =
      static_cast<const Language::Types::Object&>(bindings.get_data()[0].get());
  ASSERT_EQ(object.get_public_fields().get_size(), Count(1));
  EXPECT(monograph->get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, lifecycle_order_rejected) {
  static constexpr View::Bytes source =
      "public Session : object { expose state value : Bool = false; }"_view;
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
  Tokenizer tokenizer(arena, source, "object-stage.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto definition = Tetrodotoxin::Language::Definition::parse(cursor);
  ASSERT(definition);
  auto object = Language::Types::Object::interpret(
      arena, cursor, *definition, monograph, materializations, source_scope);
  ASSERT(object);

  EXPECT_NOT(object->link_initializers());
  EXPECT_NOT(object->link_callable_signatures());
  EXPECT_NOT(object->link_callable_bodies());
  EXPECT_NOT(object->finalize());
  EXPECT_EQ(monograph.get_diagnostics().get_size(), Count(4));
  EXPECT_NOT(object->is_linked());
  EXPECT_NOT(object->is_finalized());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, inferred_object_identity) {
  static constexpr View::Bytes source =
      "// Object inference test.\n"
      "dialect : Library;\n"
      "public Child : object {}\n"
      "public Holder : object {\n"
      "  private child : Child;\n"
      "  private copy := child;\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  auto bindings = monograph->get_authored_bindings();
  ASSERT_EQ(bindings.get_size(), Count(2));
  ASSERT(bindings.get_data()[0].get().is<Language::Types::Object>());
  ASSERT(bindings.get_data()[1].get().is<Language::Types::Object>());
  const auto& child =
      static_cast<const Language::Types::Object&>(bindings.get_data()[0].get());
  const auto& holder =
      static_cast<const Language::Types::Object&>(bindings.get_data()[1].get());

  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  auto fields = holder.get_fields();
  ASSERT_EQ(fields.get_size(), Count(2));
  const Language::Field& child_field = fields.get_data()[0].get();
  const Language::Field& copy_field = fields.get_data()[1].get();
  EXPECT(&child_field.get_type() == &child);
  EXPECT(&copy_field.get_type() == &child);
  EXPECT_NOT(copy_field.get_type_access());
  ASSERT(copy_field.get_initializer());
  ASSERT(copy_field.get_initializer()->is<Language::Identifier>());
  const auto& identifier =
      static_cast<const Language::Identifier&>(*copy_field.get_initializer());
  ASSERT(identifier.get_addressable());
  EXPECT(&*identifier.get_addressable() == &child_field);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, state_cursor_atomicity) {
  static constexpr View::Bytes malformed =
      "expose state value : Bool = false"_view;
  static constexpr View::Bytes complete =
      "private state value : Bool = false;"_view;
  EmptyContext context;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);

  Errors malformed_errors;
  Tokenizer malformed_tokenizer(arena, malformed, "malformed-state.ttx"_view);
  Cursor malformed_cursor(malformed_tokenizer, malformed_errors);
  Token opening = malformed_cursor.current();
  auto malformed_transaction = malformed_cursor.branch();
  auto malformed_definition =
      Tetrodotoxin::Language::Definition::parse(malformed_transaction);
  ASSERT(malformed_definition);
  auto rejected = Language::Field::interpret(
      arena, materializations, malformed_transaction, *malformed_definition,
      context);
  EXPECT_NOT(rejected);
  EXPECT_EQ(malformed_cursor.current().get_offset(), opening.get_offset());
  EXPECT_NOT(malformed_errors.is_empty());

  Errors complete_errors;
  Tokenizer complete_tokenizer(arena, complete, "complete-state.ttx"_view);
  Cursor complete_cursor(complete_tokenizer, complete_errors);
  auto complete_transaction = complete_cursor.branch();
  auto complete_definition =
      Tetrodotoxin::Language::Definition::parse(complete_transaction);
  ASSERT(complete_definition);
  auto field = Language::Field::interpret(
      arena, materializations, complete_transaction, *complete_definition,
      context);
  ASSERT(field);
  complete_cursor.join(complete_transaction);
  EXPECT_TEXT(field->get_name(), "value"_view);
  EXPECT(field->get_writability() == Language::Field::Writability::Internal);
  ASSERT(field->get_initializer());
  EXPECT(complete_cursor.matches(Code::Type::Terminal));
  EXPECT(complete_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ObjectTests, cursor_atomicity) {
  static constexpr View::Bytes malformed =
      "public Session : object { expose state value : Bool = false }"_view;
  static constexpr View::Bytes complete =
      "public Session : object { expose state value : Bool = false; }"_view;
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
  Tokenizer malformed_tokenizer(arena, malformed, "malformed-object.ttx"_view);
  Cursor malformed_cursor(malformed_tokenizer, malformed_errors);
  Token opening = malformed_cursor.current();
  auto malformed_transaction = malformed_cursor.branch();
  auto malformed_definition =
      Tetrodotoxin::Language::Definition::parse(malformed_transaction);
  ASSERT(malformed_definition);
  auto rejected = Language::Types::Object::interpret(
      arena, malformed_transaction, *malformed_definition, monograph,
      materializations, source_scope);
  EXPECT_NOT(rejected);
  EXPECT_EQ(malformed_cursor.current().get_offset(), opening.get_offset());
  EXPECT(malformed_cursor.current().get_code() == opening.get_code());
  EXPECT_NOT(malformed_errors.is_empty());

  Errors complete_errors;
  Tokenizer complete_tokenizer(arena, complete, "complete-object.ttx"_view);
  Cursor complete_cursor(complete_tokenizer, complete_errors);
  auto complete_transaction = complete_cursor.branch();
  auto complete_definition =
      Tetrodotoxin::Language::Definition::parse(complete_transaction);
  ASSERT(complete_definition);
  auto parsed = Language::Types::Object::interpret(
      arena, complete_transaction, *complete_definition, monograph,
      materializations, source_scope);
  ASSERT(parsed);
  complete_cursor.join(complete_transaction);
  EXPECT(complete_cursor.matches(Code::Type::Terminal));
  EXPECT(complete_errors.is_empty());
}
