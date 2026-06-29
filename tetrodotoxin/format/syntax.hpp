// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/ast/layout.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/ast/param.hpp"
#include "tetrodotoxin/syntax/ast/statement.hpp"
#include "tetrodotoxin/syntax/ast/value.hpp"
#include "tetrodotoxin/syntax/pack.hpp"
#include "tetrodotoxin/syntax/package.hpp"
#include "tetrodotoxin/syntax/type/body.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Format {

enum class BodyOrder {
  Type,
  Package,
};

auto format(Formatter& ctx, const Syntax::Package& package) -> void;
auto format(Formatter& ctx, const Syntax::Pack& pack) -> void;
auto measure(const Syntax::Pack& pack) -> Count;

auto measure_fields(Perimortem::Core::View::Vector<Syntax::Pack::Field> fields)
    -> Count;
auto measure_args(Perimortem::Core::View::Vector<Syntax::ExpressionNode*> args)
    -> Count;
auto format_fields_after_open(
    Formatter& ctx,
    Perimortem::Core::View::Vector<Syntax::Pack::Field> fields,
    Perimortem::Core::View::Bytes close,
    Bool spaced_inline = False,
    Bool positional_rows = False,
    Bool break_named_fields = True) -> void;
auto format_args_after_open(
    Formatter& ctx,
    Perimortem::Core::View::Vector<Syntax::ExpressionNode*> args,
    Perimortem::Core::View::Bytes close,
    Bool spaced_inline = False,
    Bool positional_rows = False) -> void;

auto format(
    Formatter& ctx,
    const Syntax::Type::Body& body,
    BodyOrder order = BodyOrder::Type) -> void;
auto format(Formatter& ctx, const Syntax::Type::Declaration& type, Bool first)
    -> void;
auto format_block(
    Formatter& ctx,
    Perimortem::Core::View::Vector<Syntax::Type::Declaration*> types) -> void;
auto format(Formatter& ctx, const Syntax::Type::Ref& ref) -> void;
auto measure(const Syntax::Type::Ref& ref) -> Count;

auto format(Formatter& ctx, const Syntax::Ast::Attribute& attr) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Comment& comment) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Definition& definition) -> void;
auto format_header(
    Formatter& ctx,
    const Syntax::Ast::Definition& definition,
    Count name_width) -> void;
auto format(
    Formatter& ctx,
    const Syntax::Ast::Definition& definition,
    Count name_width,
    Count type_width) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Function& function) -> void;
auto format_declaration(
    Formatter& ctx,
    const Syntax::Ast::Function& function,
    Bool first) -> void;
auto format_block(
    Formatter& ctx,
    Perimortem::Core::View::Vector<Syntax::Ast::Function*> functions) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Import& import) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Import& import, Count name_width)
    -> void;
auto format(Formatter& ctx, const Syntax::Ast::Layout& layout) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Member& member) -> void;
auto format(
    Formatter& ctx,
    const Syntax::Ast::Member& member,
    Bool first,
    Count name_width,
    Count type_width) -> void;
auto format_block(
    Formatter& ctx,
    Perimortem::Core::View::Vector<Syntax::Ast::Member*> members) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Param& param) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Statement& statement) -> void;
auto format(
    Formatter& ctx,
    const Syntax::Ast::Statement& statement,
    Count name_width,
    Count type_width) -> void;
auto format_block(
    Formatter& ctx,
    Perimortem::Core::View::Vector<Syntax::Ast::Statement*> statements) -> void;
auto format(Formatter& ctx, const Syntax::Ast::Value& value) -> void;
auto measure(const Syntax::Ast::Value& value) -> Count;

}  // namespace Tetrodotoxin::Format
