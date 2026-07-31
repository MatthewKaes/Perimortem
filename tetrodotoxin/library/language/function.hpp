// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/callables/static.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Function is one Library defined Static Callable. Reservation fixes its graph
// identity while completion installs the signature and consumes the authored
// definition without retaining parser state or an executable body.
class Function : public Callables::Static {
 private:
  struct Construction {};

 public:
  using ClassCatagory = Function;
  static constexpr Perimortem::System::Uuid contract_id{
    0x6c76a9165a2640bf,
    0xbbebc5e45cc6bd0f,
  };

  static auto reserve(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation)
      -> Perimortem::Utility::Option<Function&>;

  Function(
      Construction,
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility);

  auto complete(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context) -> Bool;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Callables::Static::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_parameters() const -> const Ttx::Concept::Layout& override;

  auto get_results() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_visibility() const -> Visibility { return visibility; }

  constexpr auto is_complete() const -> Bool { return parameters && results; }

 private:
  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Visibility visibility;
  Perimortem::Utility::Option<const Ttx::Concept::Layout&> parameters;
  Perimortem::Utility::Option<const Ttx::Concept::Layout&> results;
};

}  // namespace Tetrodotoxin::Library::Language
