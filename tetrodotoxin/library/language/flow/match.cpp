// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/match.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

class Payload final : public Language::Model::Addressable {
 public:
  TTX_CONTRACT(Payload, Language::Model::Addressable);

  constexpr Payload(View::Bytes name) : name(name) {}

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  auto bind(const Language::Model::Type& selected) -> Bool {
    if (type && &type->get() != &selected) {
      return False;
    }
    type = Reference<const Language::Model::Type>(selected);
    return True;
  }

  auto resolve() const -> const Abstract& override {
    return type ? static_cast<const Abstract&>(*this)
                : static_cast<const Abstract&>(Invalid::get_invalid());
  }

  auto get_type() const -> const Language::Model::Type& override {
    return type->get();
  }

 private:
  View::Bytes name;
  Option<Reference<const Language::Model::Type>> type;
};

class PatternContext final : public Abstract {
 public:
  TTX_CONTRACT(PatternContext, Abstract);

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
    Cursor& cursor,
    Block& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope) -> Option<Match&> {
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.require(
      Code::Type::Match,
      "Library match statements require the `match` keyword."_view);
  BAIL_IF(!opening);
  Token input_opening = cursor.current();
  auto input_pack = Parser::Expression::parse(lexical_context, cursor);
  BAIL_IF(!input_pack);
  auto input = input_pack->select<Expression>();
  if (!input) {
    cursor.create_expression_error(
        Span(input_opening, cursor.peek(-1)),
        "Library match input must be one scalar Expression."_view,
        "Use one unlabelled value instead of empty, named, or composed Pack "
        "flow."_view);
    return {};
  }

  Token scope_opening = cursor.require(
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

  Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    Token case_token = cursor.require(
        Code::Type::Case,
        "Library match bodies contain only authored `case` forms."_view);
    BAIL_IF(!case_token);

    if (cursor.matches(Code::Type::Discard)) {
      if (result.default_body) {
        cursor.create_token_error(
            "A Library match may contain at most one `_` case."_view);
        return {};
      }
      cursor.consume();

      auto body = Block::interpret(
          cursor, lexical_context, function, access_scope, enclosing_loop);
      BAIL_IF(!body);
      result.default_body = Reference<Block>(*body);

      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
      if (!cursor.matches(Code::Type::ScopeEnd)) {
        cursor.create_token_error(
            "The `_` Library match case must be final."_view);
        return {};
      }
      continue;
    }

    if (cursor.matches(Code::Type::Addressable) &&
        (cursor.peek(1).get_code().get_type() == Code::Type::Define ||
         cursor.peek(1).get_code().get_type() == Code::Type::ScopeStart)) {
      // The payload context borrows its parent and exposes one private binding
      // only after Option linking proves this case is elimination. Until then
      // the same token remains a normal Identifier Constant candidate.
      Token value_token = cursor.consume();
      View::Bytes value_name =
          value_token.caculate_text(cursor.get_source_text());
      auto& binding = domain.construct<Payload>(value_name);
      auto& context =
          domain.construct<PatternContext>(lexical_context, binding);
      auto& expression = Expressions::Identifier::create_authored(
          cursor, value_token, Anchor::create(Span(value_token)));

      auto body = Block::interpret(
          cursor, context, function, access_scope, enclosing_loop);
      BAIL_IF(!body);
      result.cases.insert(
          Case{
            .kind = Match::CaseKind::Value,
            .expression = Reference<Expression>(expression),
            .body = Reference<Block>(*body),
            .payload = Reference<Language::Model::Addressable>(binding),
            .anchor = Anchor::create(Span(value_token)),
            .constant = {},
          });
      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
      continue;
    }

    Token expression_opening = cursor.current();
    auto case_pack = Parser::Expression::parse(lexical_context, cursor);
    BAIL_IF(!case_pack);
    auto expression = case_pack->select<Expression>();
    if (!expression) {
      cursor.create_expression_error(
          Span(expression_opening, cursor.peek(-1)),
          "Library match case must be one scalar Expression."_view,
          "Use one unlabelled value that can fold to a Constant."_view);
      return {};
    }

    auto body = Block::interpret(
        cursor, lexical_context, function, access_scope, enclosing_loop);
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
    Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  }

  Token closing = cursor.consume();
  result.anchor = Anchor::create(opening, Span(opening, closing));
  return result;
}

auto Language::Flow::Match::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    const Language::Model::Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }

  Expression& retained_input = input.get();
  BAIL_IF(!retained_input.link(cursor, lexical_context, access_scope));
  // Match consumes one value domain. Type selection can link for contextual
  // access but cannot lend a fabricated value merely to enter pattern flow.
  if (&retained_input.resolve() != &retained_input) {
    cursor.create_expression_error(
        retained_input.get_anchor(),
        "Library match input did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }
  const Abstract& resolved_input_type = retained_input.get_type().resolve();
  auto input_type = resolved_input_type.select<Language::Model::Type>();
  if (!input_type || retained_input.get_layout().get_size() != 1) {
    cursor.create_expression_error(
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
      cursor.create_expression_error(
          anchor, "Option match requires one value case and one `_` case."_view,
          "Bind the present value first and keep `_` as the final absent branch."_view);
      failed = True;
    }

    for (Count index = 0; index < cases.get_size(); index++) {
      Case& entry = cases[index];
      if (entry.kind != CaseKind::Value || !entry.payload) {
        cursor.create_expression_error(
            entry.anchor,
            "Option match value case requires one local name."_view,
            "Use `case value:` to bind the exact payload."_view);
        failed = True;
        continue;
      }

      auto binding = entry.payload->get().select<Payload>();
      const Abstract& shadowed =
          binding ? lexical_context.resolve_context(binding->get_name())
                  : Invalid::get_invalid();
      if (!shadowed.is<Invalid>()) {
        auto report = cursor.create_report(entry.anchor);
        report
            << "Library match payload shadows a reachable lexical binding."_view;
        auto& note = report.get_hint();
        note << "Rename this payload so every enclosing name remains "
                "unambiguous."_view;
        auto original = cursor.get_associations().find(shadowed);
        if (original) {
          Token focus = original->get_token();
          if (!focus) {
            focus = original->get_span().get_start();
          }
          if (focus) {
            note << " Original declaration: "_view << cursor.get_source_path()
                 << ":"_view << focus.get_line() << ":"_view
                 << focus.get_column() << "."_view;
          }
        }
        failed = True;
      }
      if (!binding || !binding->bind(option_type->get_element_type())) {
        cursor.create_expression_error(
            entry.anchor,
            "Option match payload selected a different Type."_view,
            "Repeat linking with the same completed Option Type."_view);
        failed = True;
        continue;
      }

      cursor.get_associations().create(entry.anchor, *binding);
      failed |= !entry.body.get().link(cursor);
    }

    default_body.visit(
        []() {},
        [&](Reference<Block>& selected) {
          failed |= !selected.get().link(cursor);
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
    Bool case_failed = !expression.link(cursor, lexical_context, access_scope);
    // Cases obey the same value boundary as the input before constant folding
    // inspects any Layout or payload.
    if (!case_failed && &expression.resolve() != &expression) {
      cursor.create_expression_error(
          expression.get_anchor(),
          "Library match case did not produce value flow."_view,
          "Use a runtime value for each constant case."_view);
      case_failed = True;
    }

    if (!case_failed) {
      const Abstract& case_type = expression.get_type().resolve();
      if (!case_type.is<Language::Model::Type>() ||
          &case_type != &*input_type ||
          expression.get_layout().get_size() != 1) {
        cursor.create_expression_error(
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
        cursor.create_expression_error(
            expression.get_anchor(),
            "Library match case did not fold to one Constant."_view,
            "Use a complete immutable case value."_view);
        case_failed = True;
      }
    }

    if (selected) {
      if (entry.constant && &entry.constant->get() != &*selected) {
        cursor.create_expression_error(
            expression.get_anchor(),
            "Library match case selected a different Constant identity."_view,
            "Repeat linking with the same completed declaration graph."_view);
        case_failed = True;
      }

      for (Count previous = 0; previous < index; previous++) {
        const Case& retained = cases[previous];
        if (retained.constant && retained.constant->get() == *selected) {
          cursor.create_expression_error(
              expression.get_anchor(),
              "Library match cannot retain one Constant case twice."_view,
              "Remove the later equal case."_view);
          case_failed = True;
          break;
        }
      }
      entry.constant = Reference<const Constant>(*selected);
    }

    case_failed |= !entry.body.get().link(cursor);
    failed |= case_failed;
  }

  default_body.visit(
      []() {},
      [&](Reference<Block>& selected) {
        failed |= !selected.get().link(cursor);
      });
  BAIL_IF(failed);

  auto has_complete_flag_coverage = [&]() -> Bool {
    auto flag_type =
        input_type->resolve()
            .select<Tetrodotoxin::Library::Language::Model::Types::Flag>();
    if (!flag_type) {
      return False;
    }

    Bool has_false = False;
    Bool has_true = False;
    for (const Case& entry : cases.get_view()) {
      BAIL_IF(!entry.constant);
      auto validity = flag_type->get_validity(entry.constant->get());
      BAIL_IF(!validity);
      if (*validity) {
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

auto Language::Flow::Match::finalize(Cursor& cursor) -> void {
  input.get().finalize(cursor);
  for (Count index = 0; index < cases.get_size(); index++) {
    Case& entry = cases[index];
    entry.expression.visit(
        []() {},
        [&](Reference<Expression>& selected) {
          selected.get().finalize(cursor);
        });
    entry.body.get().finalize(cursor);
  }
  default_body.visit(
      []() {},
      [&](Reference<Block>& selected) { selected.get().finalize(cursor); });
}

auto Language::Flow::Match::lower(Llvm::Builder& target) const -> Bool {
  Bool input_lowered = input.get().lower(target);
  if (!input_lowered) {
    return False;
  }

  auto state = target.begin_match(input.get());
  if (!state) {
    return False;
  }

  for (const Case& entry : cases.get_view()) {
    Option<Llvm::Builder::MatchCase> selected;
    if (entry.kind == CaseKind::Constant) {
      if (!entry.constant) {
        return False;
      }

      Bool constant_lowered = entry.constant->get().lower(target);
      if (!constant_lowered) {
        return False;
      }

      selected = target.begin_constant_case(*state, entry.constant->get());
      if (!selected) {
        return False;
      }
    } else if (entry.kind == CaseKind::Value) {
      if (!entry.payload) {
        return False;
      }

      selected =
          target.begin_value_case(*state, entry.payload->get(), entry.anchor);
      if (!selected) {
        return False;
      }
    } else {
      return False;
    }

    Bool body_lowered = entry.body.get().lower(target);
    if (!body_lowered) {
      return False;
    }

    Bool case_ended = target.end_match_case(*state, *selected);
    if (!case_ended) {
      return False;
    }
  }

  if (default_body) {
    auto selected = target.begin_default_case();

    Bool default_lowered = default_body->get().lower(target);
    if (!default_lowered) {
      return False;
    }

    Bool default_ended = target.end_match_case(*state, selected);
    if (!default_ended) {
      return False;
    }
  }

  return target.end_match(*state, !complete_coverage);
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
    -> Option<const Language::Model::Addressable&> {
  if (index >= cases.get_size()) {
    return {};
  }

  return cases.get_view().get_data()[index].payload.visit(
      []() -> Option<const Language::Model::Addressable&> { return {}; },
      [](const Reference<Language::Model::Addressable>& selected)
          -> Option<const Language::Model::Addressable&> {
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

auto Language::Flow::Match::get_case_anchor(Count index) const
    -> Option<Ttx::Lexical::Anchor> {
  if (index >= cases.get_size()) {
    return {};
  }

  return cases.get_view().get_data()[index].anchor;
}
