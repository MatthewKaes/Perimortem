// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/branch.hpp"

#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/types/flag.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_condition_type(const Language::Model::Pack& condition)
    -> const Abstract& {
  const Layout& layout = condition.get_layout();
  auto first = layout.get_abstract(0);
  if (!first) {
    return Invalid::get_invalid();
  }

  auto pack = first->select<Language::Model::Pack>();
  if (pack) {
    return pack->get_type();
  }

  auto addressable = first->select<Addressable>();
  if (addressable) {
    return addressable->get_type();
  }

  auto type = first->select<Type>();
  return type ? static_cast<const Abstract&>(*type)
              : static_cast<const Abstract&>(Invalid::get_invalid());
}

auto Language::Flow::Branch::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Block& lexical_context,
    Callable& function,
    const Type& access_scope) -> Option<Branch&> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  Kind kind;
  if (transaction.matches(Code::Type::If)) {
    kind = Kind::If;
  } else if (transaction.matches(Code::Type::While)) {
    kind = Kind::While;
  } else {
    return {};
  }
  transaction.consume();

  auto condition = Model::Parser::Pack::parse(domain, source, transaction);
  BAIL_IF(!condition);

  auto body = Block::interpret(
      domain, source, transaction, lexical_context, function, access_scope);
  BAIL_IF(!body);

  Option<Reference<Block>> alternate;
  if (kind == Kind::If && transaction.matches(Code::Type::Else)) {
    transaction.consume();
    auto parsed = Block::interpret(
        domain, source, transaction, lexical_context, function, access_scope);
    BAIL_IF(!parsed);
    alternate = Reference<Block>(*parsed);
  }

  Token closing = transaction.peek(-1);
  Branch& result = domain.construct_from<Branch>([&]() -> Branch {
    return Branch(
        kind, *condition, *body, alternate,
        Anchor::create(opening, Span(opening, closing)));
  });
  cursor.join(transaction);
  return result;
}

auto Language::Flow::Branch::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    const Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }

  Model::Pack& retained_condition = condition.get();
  BAIL_IF(!retained_condition.link(source, lexical_context, access_scope));
  const Abstract& condition_type = select_condition_type(retained_condition);
  if (!condition_type.resolve().is<Ttx::Model::Types::Flag>()) {
    source.report(
        anchor, "Branch condition must produce a Flag as its first value."_view,
        "Keep any additional Pack values after one leading Flag value."_view);
    return False;
  }

  Bool failed = !body.get().link(source);
  alternate.visit(
      []() {},
      [&](Reference<Block>& selected) {
        failed |= !selected.get().link(source);
      });
  BAIL_IF(failed);

  linked = True;
  return True;
}

auto Language::Flow::Branch::finalize() -> void {
  condition.get().finalize();
  body.get().finalize();
  alternate.visit(
      []() {}, [](Reference<Block>& selected) { selected.get().finalize(); });
}

auto Language::Flow::Branch::reaches_next_statement() const -> Bool {
  if (kind == Kind::While || !alternate) {
    return True;
  }

  return body.get().reaches_next_statement() ||
         alternate->get().reaches_next_statement();
}
