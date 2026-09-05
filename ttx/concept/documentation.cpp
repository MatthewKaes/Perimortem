// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/documentation.hpp"

#include <cstddef>
#include <cstdlib>
#include <utility>

using namespace Ttx;

auto Documentation::get_empty() -> const Documentation& {
  static constexpr Documentation documentation;
  return documentation;
}

auto Documentation::size() const -> uint64_t {
  return line_count();
}

void Documentation::visit(ttx_bytes_sink result) const {
  for (Count index = 0; index < line_count(); ++index) {
    const Perimortem::Core::View::Bytes line = get_line(index);
    result.operations->bytes(
        result, {
                  .data = line.get_data(),
                  .size = line.get_size(),
                });
  }
  result.operations->completed(result);
}

auto Documentation::select(ttx_documentation self) -> const Documentation& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  const auto& selected = *reinterpret_cast<const Documentation*>(self.self);
  if (&selected.binding.operations != self.operations) {
    std::abort();
  }
  return selected;
}

auto TTX_CALL Documentation::get_size(ttx_documentation self) -> uint64_t {
  return select(self).size();
}

void TTX_CALL
    Documentation::visit_bytes(ttx_documentation self, ttx_bytes_sink result) {
  select(self).visit(result);
}

DocumentationLines::DocumentationLines(std::vector<std::vector<uint8_t>> lines)
    : lines(std::move(lines)) {}

auto DocumentationLines::size() const -> uint64_t {
  return lines.size();
}

void DocumentationLines::visit(ttx_bytes_sink result) const {
  for (const std::vector<uint8_t>& line : lines) {
    result.operations->bytes(
        result, {
                  .data = line.data(),
                  .size = line.size(),
                });
  }
  result.operations->completed(result);
}

auto DocumentationLines::get_line(Count index) const
    -> Perimortem::Core::View::Bytes {
  if (index >= lines.size()) {
    return {};
  }
  const std::vector<uint8_t>& line = lines[index];
  return line.empty() ? Perimortem::Core::View::Bytes()
                      : Perimortem::Core::View::Bytes(line.data(), line.size());
}

auto DocumentationLines::line_count() const -> Count {
  return lines.size();
}
