// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/enum/size.hpp"

#include "tetrodotoxin/library/language/constants/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Enum::Size::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Types::Unsigned& type,
    Count count) -> Size& {
  Language::Constants::Unsigned& constant =
      Language::Constants::Unsigned::create_synthetic(domain, type, U64(count));
  return domain.construct_from<Size>(
      [&]() -> Size { return Size(type, constant); });
}
