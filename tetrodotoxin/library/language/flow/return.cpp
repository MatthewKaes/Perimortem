// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/return.hpp"

#include "tetrodotoxin/library/language/model/parser/pack.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Flow::Return::interpret(
    Memory::Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor) -> Core::Option<Return&> {
  auto transaction = cursor.branch();
  Token operation = transaction.require(
      Code::Type::Return,
      "Library return statements require the `return` keyword."_view);
  BAIL_IF(!operation);

  Model::Pack* pack = nullptr;
  if (!transaction.matches(Code::Type::EndStatement)) {
    auto parsed = Model::Parser::Pack::parse(domain, source, transaction);
    BAIL_IF(!parsed);
    pack = &*parsed;
  } else {
    pack = &Model::Pack::create_empty(domain);
  }

  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library return statements require one terminating `;`."_view);
  BAIL_IF(!terminator);

  Return& result = domain.construct_from<Return>([&]() -> Return {
    return Return(
        Anchor::create(operation, Span(operation, terminator)), *pack);
  });
  cursor.join(transaction);
  return result;
}

auto Language::Flow::Return::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    const Type& access_scope,
    const Layout& results) -> Bool {
  if (linked) {
    return True;
  }

  Model::Pack& selected = pack.get();
  BAIL_IF(!selected.link(source, lexical_context, access_scope));
  Bool fits = selected.fits(results);

  if (!fits) {
    source.report(
        anchor,
        "Return value Layout does not fit the Function result Layout."_view,
        "Return the complete ordered values required by the Function "
        "signature."_view);
    return False;
  }

  linked = True;
  return True;
}

auto Language::Flow::Return::finalize() -> void {
  pack.get().finalize();
}
