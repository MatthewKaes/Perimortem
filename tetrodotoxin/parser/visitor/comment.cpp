// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/visitor/comment.hpp"

#include <spanstream>
#include <sstream>

using namespace Tetrodotoxin::Parser;
using namespace Tetrodotoxin::Lexical;

// Supported comment blocks can't be more than 128 lines, and will break if they
// are ~16KB of text. This limit is arbitrary but larger comments start creating
// bloat in both the parser and LSP. The limit on a single block should be more
// than reasonible in practice.
auto Visitor::parse_comment(Context& ctx) -> Perimortem::Memory::ByteView {
  constexpr const int max_comment_size = 128;
  auto token = &ctx.current();
  if (token->klass != Classifier::Comment)
    return Perimortem::Memory::ByteView();

  // Precaculate the insertion points.
  std::array<uint32_t, max_comment_size> line_locations;
  uint32_t total_bytes = 0;
  uint32_t start_index = ctx.index();
  while (token->klass == Classifier::Comment) {
    // Add up the bytes for each line including a new line.
    line_locations[ctx.index() - start_index] = total_bytes;
    total_bytes += token->data.get_size();
    token = &ctx.advance();
  }
  uint32_t end_index = ctx.index();

  // Fill an arena buffer with the comment.
  auto buffer =
      reinterpret_cast<char*>(ctx.get_allocator().allocate(total_bytes));
  for (uint32_t i = start_index; i < end_index; i++) {
    std::memcpy(buffer + line_locations[i - start_index], ctx[i].data.get_data(), ctx[i].data.get_size());
  }
  return Perimortem::Memory::ByteView(std::string_view(buffer, total_bytes));
}
