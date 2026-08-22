// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/inlay_hints.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/expression.hpp"

using namespace Perimortem;
using namespace Puffer;
using namespace Tetrodotoxin::Library::Language;

auto Puffer::Lsp::inlay_hints_for(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes source,
    const PositionEncoding& encoding,
    const Ttx::Lexical::Associations& associations,
    const PositionEncoding::Position& start,
    const PositionEncoding::Position& end) -> Serialization::Json::Node {
  Memory::Managed::Vector<Serialization::Json::Node> hints(arena);
  // Associations gives us the authored Calls that already completed fitting.
  // Reading those identities keeps hint placement tied to the same expressions
  // the compiler accepted.
  for (const Ttx::Lexical::Associations::Entry& association :
       associations.get_entries()) {
    auto call = association.get_semantic().select<Access::Call>();
    if (!call || !call->get_callable()) {
      continue;
    }

    const Model::Pack& arguments = call->get_arguments();
    for (Count index = 0; index < arguments.get_layout().get_size(); index++) {
      auto parameter = call->get_argument_parameter(index);
      auto produced = arguments.get_produced(index);
      auto expression = produced ? produced->producer.select<Expression>()
                                 : Core::Option<const Expression&>();
      auto anchor = expression ? expression->get_anchor()
                               : Core::Option<Ttx::Lexical::Anchor>();
      if (!parameter || parameter->get_name().is_empty() ||
          parameter->get_name() == "self"_view || !anchor) {
        continue;
      }

      Count offset = anchor->get_span().get_offset();
      auto position = encoding.locate(source, offset);
      if (!position || position->is_before(start) ||
          !position->is_before(end)) {
        continue;
      }

      Memory::Managed::Bytes label(arena, "."_view);
      label.concat(parameter->get_name());
      label.concat(" ="_view);
      hints.insert(
          Serialization::Json::Blueprint{
            {
              {"position"_view,
               {
                 {"line"_view, position->get_line()},
                 {"character"_view, position->get_character()},
               }},
              {"label"_view, label.get_view()},
              {"kind"_view, S64(2)},
              {"paddingRight"_view, True},
            }}.construct(arena));
    }
  }

  return Serialization::Json::Node(hints.get_view());
}
