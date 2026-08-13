// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/call.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness CallTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Call"_view,
};

static auto find_call(const Language::Function& function)
    -> Option<const Language::Access::Call&> {
  auto body = function.get_body();
  BAIL_IF(!body);
  for (const Reference<Abstract>& statement : body->get_statements()) {
    auto call = statement.get().select<Language::Access::Call>();
    if (call) {
      return *call;
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
      errors, "CallTest"_view, "call.ttx"_view, source);
  if (!interpreted || !interpreted->is<Language::Monograph>()) {
    return {};
  }

  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (!monograph || workspace.link(errors) || errors.is_empty()) {
    return False;
  }

  return &workspace.resolve_context("CallTest"_view) == &Invalid::get_invalid();
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  if (monograph || errors.is_empty()) {
    return False;
  }

  return &workspace.resolve_context("CallTest"_view) == &Invalid::get_invalid();
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  auto fields = composite.get_addressables();
  for (auto field = fields.begin(); field != fields.end(); ++field) {
    const Abstract& candidate = (*field).get();
    if (candidate.get_name() == name && candidate.is<Language::Field>()) {
      return static_cast<const Language::Field&>(candidate);
    }
  }

  return {};
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  auto callables = composite.get_callables();
  for (auto callable = callables.begin(); callable != callables.end();
       ++callable) {
    const Abstract& candidate = (*callable).get();
    if (candidate.get_name() == name && candidate.is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate);
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(CallTests, selection_fitting_and_signature_phase) {
  static constexpr View::Bytes source =
      "// Call selection test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private state positional := Packet -> choose(5, true,);\n"
      "  private named := Packet -> choose(.flag = true, .number = 5,);\n"
      "  public invoke : func = [self] -> Bool {\n"
      "    self -> choose(.flag = true,);\n"
      "    return true;\n"
      "  }\n"
      "  public choose : func = [\n"
      "    .number : Unsigned_64, .flag : Bool,\n"
      "  ] -> Unsigned_64 { return number; }\n"
      "  public choose : func = [self, .flag : Bool] -> Bool { return flag; }\n"
      "}\n"
      "public First : struct {\n"
      "  private seed : Later;\n"
      "  private copy := Factory -> identity(seed);\n"
      "}\n"
      "public Factory : struct {\n"
      "  public identity : func = [.value : Later] -> Later {\n"
      "    return value;\n"
      "  }\n"
      "}\n"
      "public Later : struct { public state value : Bool; }"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& packet_identity =
      monograph->get_source().resolve_context("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  auto positional = find_field(packet, "positional"_view);
  auto named = find_field(packet, "named"_view);
  auto invoke = find_function(packet, "invoke"_view);
  ASSERT(positional);
  ASSERT(named);
  ASSERT(invoke);
  ASSERT(positional->get_initializer());
  ASSERT(positional->get_initializer()->is<Language::Access::Call>());
  ASSERT(named->get_initializer());
  ASSERT(named->get_initializer()->is<Language::Access::Call>());
  auto self_call = find_call(*invoke);
  ASSERT(self_call);

  const auto& positional_call = static_cast<const Language::Access::Call&>(
      *positional->get_initializer());
  const auto& named_call =
      static_cast<const Language::Access::Call&>(*named->get_initializer());
  ASSERT(positional_call.get_callable());
  ASSERT(named_call.get_callable());
  ASSERT(self_call->get_callable());
  EXPECT_NOT(positional_call.get_callable()->is_type_bound());
  EXPECT(self_call->get_callable()->is_type_bound(packet));
  EXPECT(&positional->get_type() == &Dialect::get_unsigned_64());
  EXPECT(&named->get_type() == &Dialect::get_unsigned_64());
  EXPECT(&positional_call.get_type() == &Dialect::get_unsigned_64());
  EXPECT(&self_call->get_type() == &Dialect::get_bool());

  const Abstract& first_identity =
      monograph->get_source().resolve_context("First"_view);
  const Abstract& later_identity =
      monograph->get_source().resolve_context("Later"_view);
  ASSERT(first_identity.is<Language::Types::Structure>());
  ASSERT(later_identity.is<Language::Types::Structure>());
  auto copy = find_field(
      static_cast<const Language::Types::Structure&>(first_identity),
      "copy"_view);
  ASSERT(copy);
  EXPECT(&copy->get_type() == &later_identity);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(CallTests, invalid_invocation_is_transactional) {
  static constexpr Static::Vector<View::Bytes, 2> invalid_calls = {{
    "// Call mismatch test.\ndialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [.value : Unsigned_64] -> Bool { return true; }\n"
    "}\n"
    "private invalid := Target -> use(false);"_view,
    "// Call arity test.\ndialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [.value : Unsigned_64] -> Bool { return true; }\n"
    "}\n"
    "private invalid := Target -> use();"_view,
  }};

  for (Count i = 0; i < invalid_calls.get_size(); i++) {
    EXPECT(rejects_link(invalid_calls[i]));
  }
}

PERIMORTEM_UNIT_TEST(CallTests, duplicate_role_is_rejected_at_registration) {
  static constexpr Static::Vector<View::Bytes, 2> sources = {{
    "// Static Callable registration collision.\n"
    "dialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [] -> Bool { return true; }\n"
    "  private use : func = [.value : Unsigned_64] -> Unsigned_64 {\n"
    "    return value;\n"
    "  }\n"
    "}"_view,
    "// Self Callable registration collision.\n"
    "dialect : Library;\n"
    "public Target : struct {\n"
    "  public use : func = [self] -> Bool { return true; }\n"
    "  private use : func = [self, .value : Unsigned_64] -> Unsigned_64 {\n"
    "    return value;\n"
    "  }\n"
    "}"_view,
  }};

  for (Count i = 0; i < sources.get_size(); i++) {
    EXPECT(rejects_interpretation(sources[i]));
  }
}

PERIMORTEM_UNIT_TEST(CallTests, argument_pack_requires_parentheses) {
  EXPECT(rejects_interpretation(
      "// Missing Call argument Pack.\n"
      "dialect : Library;\n"
      "public Target : struct {\n"
      "  public use : func = [] -> Bool { return true; }\n"
      "}\n"
      "private invalid := Target -> use;"_view));
}

PERIMORTEM_UNIT_TEST(CallTests, definition_host_grants_private_authority) {
  static constexpr View::Bytes accepted =
      "// Hosted private Call test.\n"
      "dialect : Library;\n"
      "public Vault : struct {\n"
      "  private secret : func = [] -> Bool { return true; }\n"
      "  public open : func = [] -> Bool { return true; }\n"
      "  public Nested : struct {\n"
      "    private observed := Vault -> secret();\n"
      "  }\n"
      "}\n"
      "public VaultAlias : alias = Vault;\n"
      "private public_alias := VaultAlias -> open();"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, accepted);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& vault_identity =
      monograph->get_source().resolve_context("Vault"_view);
  ASSERT(vault_identity.is<Language::Types::Structure>());
  const auto& vault =
      static_cast<const Language::Types::Structure&>(vault_identity);
  const Abstract& nested_identity = vault.resolve_context("Nested"_view);
  ASSERT(nested_identity.is<Language::Types::Structure>());
  const auto& nested =
      static_cast<const Language::Types::Structure&>(nested_identity);
  auto observed = find_field(nested, "observed"_view);
  auto public_alias = find_field(monograph->get_source(), "public_alias"_view);
  ASSERT(observed);
  ASSERT(public_alias);
  EXPECT(&observed->get_type() == &Dialect::get_bool());
  EXPECT(&public_alias->get_type() == &Dialect::get_bool());
  EXPECT(errors.is_empty());

  EXPECT(rejects_link(
      "// Alias private Call test.\ndialect : Library;\n"
      "public Vault : struct {\n"
      "  private secret : func = [] -> Bool { return true; }\n"
      "}\n"
      "public VaultAlias : alias = Vault;\n"
      "private denied := VaultAlias -> secret();"_view));
}

PERIMORTEM_UNIT_TEST(CallTests, result_layout_and_addressable_access) {
  static constexpr View::Bytes source =
      "// Call result flow test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public state value : Unsigned_64; public state flag : Bool;\n"
      "  public self_none : func = [self] -> [] {}\n"
      "  public self_one : func = [self] -> Bool { return true; }\n"
      "}\n"
      "public Results : struct {\n"
      "  public none : func = [] -> [] {}\n"
      "  public one : func = [] -> Bool { return true; }\n"
      "  public many : func = [.packet : Packet] -> [Unsigned_64, Bool] {\n"
      "    return packet.[value, flag];\n"
      "  }\n"
      "  public identity : func = [.packet : Packet] -> Packet {\n"
      "    return packet;\n"
      "  }\n"
      "}\n"
      "private seed : Packet;\n"
      "private selected := seed.value;\n"
      "private observe : func = [] -> Void {\n"
      "  Results -> none();\n"
      "  Results -> one();\n"
      "  Results -> many(seed);\n"
      "  seed -> self_none();\n"
      "  seed -> self_one();\n"
      "}"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  auto observe = find_function(monograph->get_source(), "observe"_view);
  ASSERT(observe);
  auto body = observe->get_body();
  ASSERT(body);
  auto statements = body->get_statements();
  ASSERT_EQ(statements.get_size(), Count(5));
  for (Count i = 0; i < statements.get_size(); i++) {
    ASSERT(statements.get_data()[i].get().is<Language::Access::Call>());
  }

  const auto& none = static_cast<const Language::Access::Call&>(
      statements.get_data()[0].get());
  const auto& one = static_cast<const Language::Access::Call&>(
      statements.get_data()[1].get());
  const auto& many = static_cast<const Language::Access::Call&>(
      statements.get_data()[2].get());
  const auto& self_none = static_cast<const Language::Access::Call&>(
      statements.get_data()[3].get());
  const auto& self_one = static_cast<const Language::Access::Call&>(
      statements.get_data()[4].get());
  EXPECT(none.get_layout().is_empty());
  EXPECT(&none.get_type() == &Invalid::get_invalid());
  EXPECT_EQ(one.get_layout().get_size(), Count(1));
  EXPECT(&one.get_type() == &Dialect::get_bool());
  EXPECT_EQ(many.get_layout().get_size(), Count(2));
  EXPECT(&many.get_type() == &Invalid::get_invalid());
  EXPECT(self_none.get_layout().is_empty());
  EXPECT_EQ(self_one.get_layout().get_size(), Count(1));
  ASSERT(self_none.get_callable());
  ASSERT(self_one.get_callable());
  EXPECT(self_none.get_callable()->is_type_bound());
  EXPECT(self_one.get_callable()->is_type_bound());

  ASSERT(many.get_callable());
  const Layout& many_results = many.get_callable()->get_results();
  EXPECT(many.get_layout().fits(many_results));
  for (Count index = 0; index < many.get_layout().get_size(); index++) {
    EXPECT(
        many.get_layout()
            .get_fitted(many_results, index)
            .visit(
                [&](const Abstract& fitted) { return Bool(&fitted == &many); },
                [](Layout::Errors) { return False; }));
  }

  auto selected = find_field(monograph->get_source(), "selected"_view);
  ASSERT(selected);
  ASSERT(selected->get_initializer());
  ASSERT(selected->get_initializer()->is<Language::Access::Address>());
  const auto& address = static_cast<const Language::Access::Address&>(
      *selected->get_initializer());
  EXPECT(address.get_receiver().get_result().is<Ttx::Model::Addressable>());
  EXPECT(&selected->get_type() == &Dialect::get_unsigned_64());
  EXPECT(errors.is_empty());

  static constexpr View::Bytes invalid_source =
      "// Computed result Address test.\n"
      "dialect : Library;\n"
      "public Packet : struct { public value : Unsigned_64; }\n"
      "public Results : struct {\n"
      "  public identity : func = [.packet : Packet] -> Packet {\n"
      "    return packet;\n"
      "  }\n"
      "}\n"
      "private seed : Packet;\n"
      "private invalid := Results -> identity(seed).value;"_view;
  Workspace invalid_workspace;
  Errors invalid_errors;
  auto invalid_monograph =
      interpret(invalid_workspace, invalid_errors, invalid_source);
  ASSERT(invalid_monograph);
  auto invalid = find_field(invalid_monograph->get_source(), "invalid"_view);
  ASSERT(invalid);
  ASSERT(invalid->get_initializer());
  ASSERT(invalid->get_initializer()->is<Language::Access::Address>());
  const auto& invalid_address = static_cast<const Language::Access::Address&>(
      *invalid->get_initializer());
  EXPECT(invalid_address.get_receiver().is<Language::Access::Call>());
  EXPECT(!invalid_workspace.link(invalid_errors));
  EXPECT(!invalid_errors.is_empty());
}
