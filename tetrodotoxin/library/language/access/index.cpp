// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/index.hpp"

#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto parse_index(
    Memory::Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Core::Option<Language::Expression&> {
  // Operand parsing uses a private Cursor so an incomplete index never moves
  // the enclosing postfix transaction or publishes its local diagnostic path.
  Errors operand_errors;
  auto operand_cursor = cursor.branch(operand_errors);
  auto parsed = Language::Parser::Expression::parse(
      domain, materializations, operand_cursor, source_context);
  if (parsed) {
    cursor.join(operand_cursor);
  }

  return parsed;
}

static auto select_access(const Abstract& output)
    -> Core::Option<const Language::Types::Access&> {
  auto direct = output.select<Language::Types::Access>();
  if (direct) {
    return direct;
  }

  return output.resolve().select<Language::Types::Access>();
}

static auto is_integer(const Language::Expression& expression) -> Bool {
  const Abstract& output = expression.get_type();
  auto direct = output.select<Ttx::Model::Type>();
  const Abstract& type =
      direct ? static_cast<const Abstract&>(*direct) : output.resolve();
  return type.is<Ttx::Model::Types::Signed>() ||
         type.is<Ttx::Model::Types::Unsigned>();
}

auto Language::Access::Index::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& receiver) -> Core::Option<Expression&> {
  Token opening = cursor.consume();
  auto selected = parse_index(domain, materializations, cursor, source_context);
  if (!selected || !cursor.matches(Code::Type::BracketEnd)) {
    cursor.create_expression_error(
        Span(opening, cursor.current()),
        "Index access requires one complete index Expression."_view,
        "Use `[index]` with one signed or unsigned integer Expression."_view);
    return {};
  }

  Token closing = cursor.consume();
  const auto& receiver_anchor = receiver.get_anchor();
  const auto& index_anchor = selected->get_anchor();
  if (!receiver_anchor || !index_anchor) {
    cursor.create_expression_error(
        Span(opening, closing),
        "Index access requires authored receiver and index Anchors."_view);
    return {};
  }

  Anchor anchor =
      Anchor::create(opening, receiver_anchor->get_span(), Span(closing));
  return Expression::create_authored<Index>(
      domain, anchor,
      [&](auto source) -> Index { return Index(receiver, *selected, source); });
}

auto Language::Access::Index::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Materializations& materializations,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  BAIL_IF(
      !receiver.link(source, lexical_context, materializations, access_scope));
  BAIL_IF(!index.link(source, lexical_context, materializations, access_scope));

  // Index links both Expressions before reading their output domains. The
  // element Type is reference metadata only, so this owner never performs
  // bounds checks, default selection, or ordinary value materialization.
  auto access = select_access(receiver.get_type());
  if (!access || !is_integer(index)) {
    source.report(
        get_anchor(),
        "Index access requires Access storage and one integer index."_view,
        "Use bracket access with Access[T] and a signed or unsigned integer."_view);
    return False;
  }

  const Ttx::Model::Type& selected = access->get_element_type();
  if (element_type && &element_type->get() != &selected) {
    source.report(
        get_anchor(),
        "Index access cannot change its referenced element Type."_view,
        "Keep one exact Access element Type bound to this authored index."_view);
    return False;
  }

  element_type = Reference<const Ttx::Model::Type>(selected);

  // Index is complete once its optional reference metadata is known. It does
  // not call Expression link because Invalid is the intentional ordinary value
  // Type for a reference that only a consuming statement may observe.
  return True;
}

auto Language::Access::Index::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Access::Index::get_type() const -> const Abstract& {
  return Invalid::get_invalid();
}

auto Language::Access::Index::get_inputs() const -> const Layout& {
  return inputs;
}

auto Language::Access::Index::get_element_type() const -> const Abstract& {
  return element_type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Ttx::Model::Type>& selected) -> const Abstract& {
        return selected.get();
      });
}
