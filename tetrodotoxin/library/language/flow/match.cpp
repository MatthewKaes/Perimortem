// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/match.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/types/flag.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

class Payload final : public Addressable {
 public:
  TTX_CONTRACT(Payload, Addressable, 0x43e392a908fd46ed, 0xaaf47238d64c7711);

  constexpr Payload(View::Bytes name) : name(name) {}

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  auto bind(const Type& selected) -> Bool {
    if (type && &type->get() != &selected) {
      return False;
    }
    type = Reference<const Type>(selected);
    return True;
  }

  auto resolve() const -> const Abstract& override {
    return type ? static_cast<const Abstract&>(*this)
                : static_cast<const Abstract&>(Invalid::get_invalid());
  }

  auto get_type() const -> const Type& override { return type->get(); }

 private:
  View::Bytes name;
  Option<Reference<const Type>> type;
};

class PatternContext final : public Abstract {
 public:
  TTX_CONTRACT(
      PatternContext,
      Abstract,
      0x9a56fc513b014505,
      0x875d09107259fb90);

  constexpr PatternContext(const Abstract& parent, Payload& payload)
      : parent(parent), payload(payload) {}

  TTX_NAME("Option pattern"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == payload.get_name() &&
        &payload.resolve() != &Invalid::get_invalid()) {
      return payload;
    }

    return parent.resolve_context(route);
  }

 private:
  const Abstract& parent;
  Payload& payload;
};

auto Language::Flow::Match::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Block& lexical_context,
    Callable& function,
    const Type& access_scope) -> Option<Match&> {
  auto transaction = cursor.branch();
  if (!transaction.matches(Code::Type::Match)) {
    return {};
  }

  Token opening = transaction.consume();
  Token input_opening = transaction.current();
  auto input_pack = Parser::Expression::parse(domain, source, transaction);
  BAIL_IF(!input_pack);
  auto input = input_pack->select<Expression>();
  if (!input) {
    transaction.create_expression_error(
        Span(input_opening, transaction.peek(-1)),
        "Library match input must be one scalar Expression."_view,
        "Use one unlabelled value instead of empty, named, or composed Pack "
        "flow."_view);
    return {};
  }

  Token scope_opening = transaction.require(
      Code::Type::ScopeStart,
      "Library match cases require a body beginning with `{`."_view);
  BAIL_IF(!scope_opening);

  Match& result = domain.construct_from<Match>([&]() -> Match {
    return Match(
        domain, *input, Anchor::create(opening, Span(opening, scope_opening)));
  });
  Option<Reference<const Abstract>> enclosing_loop;
  auto inherited = lexical_context.get_enclosing_loop();
  if (inherited) {
    enclosing_loop = Reference<const Abstract>(*inherited);
  }

  Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  while (!transaction.matches(Code::Type::ScopeEnd)) {
    Token case_token = transaction.require(
        Code::Type::Case,
        "Library match bodies contain only authored `case` forms."_view);
    BAIL_IF(!case_token);

    if (transaction.matches(Code::Type::Discard)) {
      if (result.default_body) {
        transaction.create_token_error(
            "A Library match may contain at most one `_` case."_view);
        return {};
      }
      transaction.consume();
      BAIL_IF(!transaction.require(
          Code::Type::Define,
          "Library match cases require `:` before their Block."_view));

      auto body = Block::interpret(
          domain, source, transaction, lexical_context, function, access_scope,
          enclosing_loop);
      BAIL_IF(!body);
      result.default_body = Reference<Block>(*body);

      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
      if (!transaction.matches(Code::Type::ScopeEnd)) {
        transaction.create_token_error(
            "The `_` Library match case must be final."_view);
        return {};
      }
      continue;
    }

    if (transaction.matches(Code::Type::Addressable) &&
        transaction.peek(1).get_code().get_type() == Code::Type::Define) {
      // The payload context borrows its parent and exposes one private binding
      // only after Option linking proves this case is elimination. Until then
      // the same token remains a normal Identifier Constant candidate.
      Token value_token = transaction.consume();
      View::Bytes value_name = domain.proxy(
          value_token.caculate_text(transaction.get_source_text()));
      auto& binding = domain.construct<Payload>(value_name);
      auto& context =
          domain.construct<PatternContext>(lexical_context, binding);
      auto& expression = Expressions::Identifier::create_authored(
          domain, value_token, transaction.get_source_text(),
          Anchor::create(Span(value_token)));

      BAIL_IF(!transaction.require(
          Code::Type::Define,
          "Library match cases require `:` before their Block."_view));
      auto body = Block::interpret(
          domain, source, transaction, context, function, access_scope,
          enclosing_loop);
      BAIL_IF(!body);
      result.cases.insert(
          Case{
            .kind = Match::CaseKind::Value,
            .expression = Reference<Expression>(expression),
            .body = Reference<Block>(*body),
            .payload = Reference<Addressable>(binding),
            .anchor = Anchor::create(Span(value_token)),
            .constant = {},
          });
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
      continue;
    }

    Token expression_opening = transaction.current();
    auto case_pack = Parser::Expression::parse(domain, source, transaction);
    BAIL_IF(!case_pack);
    auto expression = case_pack->select<Expression>();
    if (!expression) {
      transaction.create_expression_error(
          Span(expression_opening, transaction.peek(-1)),
          "Library match case must be one scalar Expression."_view,
          "Use one unlabelled value that can fold to a Constant."_view);
      return {};
    }

    BAIL_IF(!transaction.require(
        Code::Type::Define,
        "Library match cases require `:` before their Block."_view));
    auto body = Block::interpret(
        domain, source, transaction, lexical_context, function, access_scope,
        enclosing_loop);
    BAIL_IF(!body);
    result.cases.insert(
        Case{
          .kind = Match::CaseKind::Constant,
          .expression = Reference<Expression>(*expression),
          .body = Reference<Block>(*body),
          .payload = {},
          .anchor = expression->get_anchor().visit(
              [&]() { return Anchor::create(Span(expression_opening)); },
              [](Anchor selected) { return selected; }),
          .constant = {},
        });
    Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  }

  Token closing = transaction.consume();
  result.anchor = Anchor::create(opening, Span(opening, closing));
  cursor.join(transaction);
  return result;
}

auto Language::Flow::Match::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    const Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }

  Expression& retained_input = input.get();
  BAIL_IF(!retained_input.link(source, lexical_context, access_scope));
  const Abstract& resolved_input_type = retained_input.get_type().resolve();
  auto input_type = resolved_input_type.select<Type>();
  if (!input_type || retained_input.get_layout().get_size() != 1) {
    source.report(
        retained_input.get_anchor(),
        "Library match input did not produce one scalar value."_view,
        "Use one Expression with one exact completed Type."_view);
    return False;
  }

  Bool failed = False;
  auto option_type = input_type->select<Language::Types::Option>();
  if (option_type) {
    // Option elimination owns exactly one branch local payload and one absent
    // branch. The input Type never becomes an empty Layout in either case.
    if (cases.get_size() != 1 || !default_body) {
      source.report(
          anchor, "Option match requires one value case and one `_` case."_view,
          "Bind the present value first and keep `_` as the final absent branch."_view);
      failed = True;
    }

    for (Count index = 0; index < cases.get_size(); index++) {
      Case& entry = cases[index];
      if (entry.kind != CaseKind::Value || !entry.payload) {
        source.report(
            entry.anchor,
            "Option match value case requires one local name."_view,
            "Use `case value:` to bind the exact payload."_view);
        failed = True;
        continue;
      }

      auto binding = entry.payload->get().select<Payload>();
      if (!binding || !binding->bind(option_type->get_element_type())) {
        source.report(
            entry.anchor,
            "Option match payload selected a different Type."_view,
            "Repeat linking with the same completed Option Type."_view);
        failed = True;
        continue;
      }

      failed |= !entry.body.get().link(source);
    }

    default_body.visit(
        []() {},
        [&](Reference<Block>& selected) {
          failed |= !selected.get().link(source);
        });
    BAIL_IF(failed);

    complete_coverage = True;
    linked = True;
    return True;
  }

  // Case Expressions settle before bodies so every Constant and duplicate is
  // known before Match publishes any control coverage. The authored Expression
  // remains the fold owner while Match retains only the resulting exact value.
  for (Count index = 0; index < cases.get_size(); index++) {
    Case& entry = cases[index];
    entry.kind = CaseKind::Constant;

    Expression& expression = entry.expression->get();
    Bool case_failed = !expression.link(source, lexical_context, access_scope);
    if (!case_failed) {
      const Abstract& case_type = expression.get_type().resolve();
      if (!case_type.is<Type>() || &case_type != &*input_type ||
          expression.get_layout().get_size() != 1) {
        source.report(
            expression.get_anchor(),
            "Library match case must have the input's exact scalar Type."_view,
            "Keep every case in the same completed value domain."_view);
        case_failed = True;
      }
    }

    Option<Constant&> selected;
    if (!case_failed) {
      expression.fold().visit(
          [&](const Option<Model::Pack&>& folded) {
            if (folded) {
              selected = folded->select<Constant>();
            }
          },
          [&](const Expression::Error&) { case_failed = True; });
      if (!selected) {
        source.report(
            expression.get_anchor(),
            "Library match case did not fold to one Constant."_view,
            "Use a complete immutable case value."_view);
        case_failed = True;
      }
    }

    if (selected) {
      if (entry.constant && &entry.constant->get() != &*selected) {
        source.report(
            expression.get_anchor(),
            "Library match case selected a different Constant identity."_view,
            "Repeat linking with the same completed declaration graph."_view);
        case_failed = True;
      }

      for (Count previous = 0; previous < index; previous++) {
        const Case& retained = cases[previous];
        if (retained.constant && retained.constant->get() == *selected) {
          source.report(
              expression.get_anchor(),
              "Library match cannot retain one Constant case twice."_view,
              "Remove the later equal case."_view);
          case_failed = True;
          break;
        }
      }
      entry.constant = Reference<const Constant>(*selected);
    }

    case_failed |= !entry.body.get().link(source);
    failed |= case_failed;
  }

  default_body.visit(
      []() {},
      [&](Reference<Block>& selected) {
        failed |= !selected.get().link(source);
      });
  BAIL_IF(failed);

  auto has_complete_flag_coverage = [&]() -> Bool {
    if (!input_type->resolve().is<Ttx::Model::Types::Flag>()) {
      return False;
    }

    Bool has_false = False;
    Bool has_true = False;
    for (const Case& entry : cases.get_view()) {
      BAIL_IF(!entry.constant);
      auto flag = entry.constant->get().select<Language::Constants::Flag>();
      BAIL_IF(!flag);
      if (flag->get_value()) {
        has_true = True;
      } else {
        has_false = True;
      }
    }

    return has_false && has_true;
  };
  complete_coverage = Bool(default_body) || has_complete_flag_coverage();
  linked = True;
  return True;
}

auto Language::Flow::Match::finalize() -> void {
  input.get().finalize();
  for (Count index = 0; index < cases.get_size(); index++) {
    Case& entry = cases[index];
    entry.expression.visit(
        []() {},
        [](Reference<Expression>& selected) { selected.get().finalize(); });
    entry.body.get().finalize();
  }
  default_body.visit(
      []() {}, [](Reference<Block>& selected) { selected.get().finalize(); });
}

auto Language::Flow::Match::reaches_next_statement() const -> Bool {
  if (!complete_coverage) {
    return True;
  }

  for (const Case& entry : cases.get_view()) {
    if (entry.body.get().reaches_next_statement()) {
      return True;
    }
  }

  return default_body.visit(
      []() { return False; },
      [](const Reference<Block>& selected) {
        return selected.get().reaches_next_statement();
      });
}

auto Language::Flow::Match::get_case_constant(Count index) const
    -> Option<const Constant&> {
  if (index >= cases.get_size()) {
    return {};
  }

  return cases.get_view().get_data()[index].constant.visit(
      []() -> Option<const Constant&> { return {}; },
      [](const Reference<const Constant>& selected) -> Option<const Constant&> {
        return selected.get();
      });
}

auto Language::Flow::Match::get_case_kind(Count index) const
    -> Option<CaseKind> {
  if (index >= cases.get_size()) {
    return {};
  }

  return cases.get_view().get_data()[index].kind;
}

auto Language::Flow::Match::get_case_payload(Count index) const
    -> Option<const Addressable&> {
  if (index >= cases.get_size()) {
    return {};
  }

  return cases.get_view().get_data()[index].payload.visit(
      []() -> Option<const Addressable&> { return {}; },
      [](const Reference<Addressable>& selected) -> Option<const Addressable&> {
        return selected.get();
      });
}

auto Language::Flow::Match::get_case_body(Count index) const
    -> Option<const Block&> {
  if (index >= cases.get_size()) {
    return {};
  }

  return cases.get_view().get_data()[index].body.get();
}
