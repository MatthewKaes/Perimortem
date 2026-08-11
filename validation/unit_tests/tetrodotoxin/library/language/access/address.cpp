// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/address.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Validation;

static Harness AddressTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Address"_view,
};

class AddressReceiver : public Library::Language::Expression {
 public:
  AddressReceiver(View::Bytes name, const Type& type)
      : Expression(
            Anchor::create(Span(Token(
                0,
                1,
                1,
                Unsigned_8(name.get_size()),
                Code::Type::Addressable)))),
        name(name),
        type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Type& type;
  Ttx::Model::Layouts::Fluid inputs;
};

class LayoutAddressable : public Addressable {
 public:
  LayoutAddressable(View::Bytes name, const Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Type& override { return type; }

 private:
  View::Bytes name;
  const Type& type;
};

class LayoutType : public Type {
 public:
  LayoutType(View::Bytes name, Layouts::Named layout)
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Layout& override { return layout; }

 private:
  View::Bytes name;
  Layouts::Named layout;
};

static auto address_anchor() -> Anchor {
  Token receiver(0, 1, 1, 6, Code::Type::Addressable);
  Token member(7, 1, 8, 7, Code::Type::Addressable);
  return Anchor::create(member, Span(receiver, member));
}

static auto interpret(
    Environment::Workspace& workspace,
    Errors& errors,
    View::Bytes source) -> Option<Library::Language::Monograph&> {
  if (!workspace.install_dialect<Library::Dialect>("Library"_view)) {
    return {};
  }

  auto interpreted = workspace.interpret_source(
      errors, "AddressTest"_view, "address.ttx"_view, source);
  if (!interpreted || !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpreted);
}

PERIMORTEM_UNIT_TEST(AddressTests, structure_member_selection) {
  static constexpr View::Bytes source =
      "// Address selection test.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  public visible : Bool;\n"
      "  private secret : Bool;\n"
      "  public observe : func = [] -> [] {}\n"
      "}\n"
      "public Session : object { expose state progress : Bool = false; }\n"
      "private root : func = [] -> [] {}"_view;
  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const auto& source_type = monograph->get_source();
  const auto& packet = static_cast<const Library::Language::Types::Structure&>(
      source_type.resolve_context("Packet"_view));
  const auto& session = static_cast<const Library::Language::Types::Object&>(
      source_type.resolve_context("Session"_view));

  auto source_callables = source_type.get_callables();
  auto source_callable = source_callables.begin();
  ASSERT(source_callable != source_callables.end());
  const auto& root =
      static_cast<const Library::Language::Function&>((*source_callable).get());

  auto packet_fields = packet.get_addressables();
  auto packet_field = packet_fields.begin();
  ASSERT(packet_field != packet_fields.end());
  const auto& visible =
      static_cast<const Library::Language::Field&>((*packet_field).get());
  ++packet_field;
  ASSERT(packet_field != packet_fields.end());
  const auto& secret =
      static_cast<const Library::Language::Field&>((*packet_field).get());

  auto state_identity = session.get_layout().get_abstract(0);
  ASSERT(state_identity);
  const auto& state =
      static_cast<const Library::Language::Field&>(*state_identity);

  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  AddressReceiver packet_receiver("packet"_view, packet);
  AddressReceiver session_receiver("session"_view, session);
  AddressReceiver scalar_receiver("flag"_view, Library::Dialect::get_bool());
  auto& external = Library::Language::Access::Address::create_authored(
      domain, "visible"_view, packet_receiver, address_anchor());
  auto& rejected_callable = Library::Language::Access::Address::create_authored(
      domain, "observe"_view, packet_receiver, address_anchor());
  auto& missing = Library::Language::Access::Address::create_authored(
      domain, "missing"_view, packet_receiver, address_anchor());
  auto& object = Library::Language::Access::Address::create_authored(
      domain, "progress"_view, session_receiver, address_anchor());
  auto& scalar = Library::Language::Access::Address::create_authored(
      domain, "anything"_view, scalar_receiver, address_anchor());

  EXPECT(external.link(*monograph, root, materializations));
  EXPECT_NOT(rejected_callable.link(*monograph, root, materializations));
  EXPECT_NOT(missing.link(*monograph, root, materializations));
  EXPECT(object.link(*monograph, root, materializations));
  EXPECT_NOT(scalar.link(*monograph, root, materializations));
  ASSERT(external.get_addressable());
  ASSERT(object.get_addressable());
  EXPECT(&*external.get_addressable() == &visible);
  EXPECT(&*object.get_addressable() == &state);
  EXPECT(&external.get_type() == &Library::Dialect::get_bool());
  EXPECT_NOT(rejected_callable.get_addressable());
  EXPECT_NOT(missing.get_addressable());
  EXPECT_NOT(scalar.get_addressable());

  auto& synthetic = Library::Language::Access::Address::create_synthetic(
      domain, packet_receiver, visible);
  EXPECT_NOT(synthetic.get_anchor());
  EXPECT(synthetic.link(*monograph, root, materializations));
  EXPECT(&*synthetic.get_addressable() == &visible);
  LayoutAddressable unrelated("visible"_view, Library::Dialect::get_bool());
  auto& synthetic_unrelated =
      Library::Language::Access::Address::create_synthetic(
          domain, packet_receiver, unrelated);
  auto& synthetic_private =
      Library::Language::Access::Address::create_synthetic(
          domain, packet_receiver, secret);
  EXPECT_NOT(synthetic_unrelated.link(*monograph, root, materializations));
  EXPECT_NOT(synthetic_private.link(*monograph, root, materializations));
  LayoutAddressable value("value"_view, Library::Dialect::get_bool());
  const Static::Vector<Reference<const Abstract>, 1> values = {{value}};
  LayoutType container("Container"_view, Layouts::Named(values));
  AddressReceiver receiver("container"_view, container);
  auto& generic = Library::Language::Access::Address::create_authored(
      domain, "value"_view, receiver, address_anchor());

  EXPECT(generic.link(*monograph, root, materializations));
  ASSERT(generic.get_addressable());
  EXPECT(&*generic.get_addressable() == &value);
  EXPECT(&generic.get_type() == &Library::Dialect::get_bool());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AddressTests, descendant_private_authority) {
  static constexpr View::Bytes source =
      "// Descendant private Address access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  private secret : Bool;\n"
      "  public Inner : struct {\n"
      "    public read : func = [.outer : Outer] -> Bool {\n"
      "      return outer.secret;\n"
      "    }\n"
      "  }\n"
      "}"_view;
  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(AddressTests, sibling_private_denied) {
  static constexpr View::Bytes source =
      "// Sibling private Address access.\n"
      "dialect : Library;\n"
      "public Outer : struct {\n"
      "  public Target : struct { private secret : Bool; }\n"
      "  public Caller : struct {\n"
      "    public read : func = [.target : Outer::Target] -> Bool {\n"
      "      return target.secret;\n"
      "    }\n"
      "  }\n"
      "}"_view;
  Environment::Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  EXPECT_NOT(workspace.link(errors));
  EXPECT_NOT(errors.is_empty());
}
