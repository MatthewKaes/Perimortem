// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/return.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Flow::Return::interpret(Cursor& cursor, const Abstract& context)
    -> Core::Option<Return&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  Token operation = cursor.require(
      Code::Type::Return,
      "Library return statements require the `return` keyword."_view);
  BAIL_IF(!operation);

  Model::Pack* pack = nullptr;
  if (!cursor.matches(Code::Type::EndStatement)) {
    auto parsed = Model::Parser::Pack::parse(context, cursor);
    BAIL_IF(!parsed);
    pack = &*parsed;
  } else {
    pack = &Model::Pack::create_empty(domain);
  }

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library return statements require one terminating `;`."_view);
  BAIL_IF(!terminator);

  Return& result = domain.construct_from<Return>([&]() -> Return {
    return Return(
        Anchor::create(operation, Span(operation, terminator)), *pack);
  });
  return result;
}

auto Language::Flow::Return::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope,
    const Layout& results) -> Bool {
  if (linked) {
    return True;
  }

  Model::Pack& selected = pack.get();
  BAIL_IF(!selected.link(cursor, lexical_context, access_scope));
  // Return owns produced flow, not contextual identity traversal. Reject a
  // selected Type before result fitting asks it for a value Layout.
  if (&selected.resolve() != &selected) {
    cursor.create_expression_error(
        anchor, "Return expression did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }
  Bool fits = selected.fits(results);

  if (!fits) {
    auto report = cursor.create_report(anchor);
    report << "Return values do not fit the Function result Layout.\n"
              "Source produces: "_view;
    Language::Diagnostics::write_pack(report, selected);
    report << "\nFunction accepts: "_view;
    Language::Diagnostics::write_layout(report, results);
    report.get_hint()
        << "Return the exact ordered Types declared by the Function."_view;
    return False;
  }

  linked = True;
  return True;
}

auto Language::Flow::Return::finalize(Cursor& cursor) -> void {
  pack.get().finalize(cursor);
}

auto Language::Flow::Return::lower(Llvm::Builder& body) const -> Bool {
  Bool lowered = pack.get().lower(body);
  if (!lowered) {
    return False;
  }

  return body.return_values(*this, pack.get());
}
