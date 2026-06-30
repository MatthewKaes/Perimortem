// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

namespace Tetrodotoxin::Format {

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

}  // namespace Tetrodotoxin::Format
