// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/index.hpp"

#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto parse_index(const Abstract& context, Cursor& cursor)
    -> Core::Option<Language::Expression&> {
  auto pack = Language::Parser::Expression::parse(context, cursor);
  return pack.visit(
      []() -> Core::Option<Language::Expression&> { return {}; },
      [](Language::Model::Pack& selected) {
        return selected.select<Language::Expression>();
      });
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
  auto direct = output.select<Language::Model::Type>();
  const Abstract& type =
      direct ? static_cast<const Abstract&>(*direct) : output.resolve();
  return type.is<Tetrodotoxin::Library::Language::Model::Types::Signed>() ||
         type.is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>();
}

auto Language::Access::Index::parse(
    const Abstract& context,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.require(
      Code::Type::BracketStart,
      "Index reference access requires an opening `[`."_view);
  BAIL_IF(!opening);

  auto selected = parse_index(context, cursor);
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
  Index& result = Expression::create_authored<Index>(
      domain, anchor, [&](auto authored) -> Index {
        return Index(receiver, *selected, authored);
      });
  return result;
}

auto Language::Access::Index::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  BAIL_IF(!index.link(cursor, lexical_context, access_scope));

  // Index links both Expressions before reading their output domains. The
  // element Type is reference metadata only, so this owner never performs
  // bounds checks, default selection, or ordinary value materialization.
  auto access = select_access(receiver.get_type());
  if (!access || !is_integer(index)) {
    cursor.create_expression_error(
        get_anchor(),
        "Index access requires Access storage and one integer index."_view,
        "Use bracket access with Access[T] and a signed or unsigned integer."_view);
    return False;
  }

  const Language::Model::Type& selected = access->get_element_type();
  if (element_type && &element_type->get() != &selected) {
    cursor.create_expression_error(
        get_anchor(),
        "Index access cannot change its referenced element Type."_view,
        "Keep one exact Access element Type bound to this authored index."_view);
    return False;
  }

  element_type = Reference<const Language::Model::Type>(selected);

  // The exact element Type completes Index's Pack with one value. Runtime
  // bounds decide whether its writable address is engaged. They do not change
  // the semantic output shape or introduce an Option Type.
  return Expression::link(cursor, lexical_context, access_scope);
}

auto Language::Access::Index::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  index.finalize(cursor);
}

auto Language::Access::Index::get_element_type() const -> const Abstract& {
  return element_type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Language::Model::Type>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Language::Access::Index::get_type() const -> const Abstract& {
  return get_element_type();
}

auto Language::Access::Index::get_write_type(const Language::Model::Type&) const
    -> Core::Option<const Language::Model::Type&> {
  return element_type.visit(
      []() -> Core::Option<const Language::Model::Type&> { return {}; },
      [](const Reference<const Language::Model::Type>& selected)
          -> Core::Option<const Language::Model::Type&> {
        return selected.get();
      });
}
