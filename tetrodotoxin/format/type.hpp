// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/syntax/type/body.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Format {

enum class BodyOrder {
  Type,
  Package,
};

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

}  // namespace Tetrodotoxin::Format
