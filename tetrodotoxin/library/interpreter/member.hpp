// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/interpreter/parsed.hpp"
#include "tetrodotoxin/library/language/model/completion.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Library::Interpreter {

// Member selects the concrete parser named by one Definition qualifier. It
// retains no state and returns the real semantic object constructed by that
// owner. The receiving Composite alone decides whether to retain it.
class Member {
 public:
  // Result carries the exact declaration identity together with the category
  // selected by its qualifier. It exists only for this parser call and never
  // becomes a second declaration record in the retained graph.
  class Result {
   public:
    constexpr Result(
        Ttx::Concept::Abstract& semantic,
        Language::Types::Composite::Category category,
        ParseState state,
        Language::Model::Completion* completion = nullptr)
        : semantic(&semantic),
          category(category),
          state(state),
          completion(completion) {}

    constexpr auto get_semantic() const -> Ttx::Concept::Abstract& {
      return *semantic;
    }

    constexpr auto get_category() const
        -> Language::Types::Composite::Category {
      return category;
    }

    constexpr auto is_accepted() const -> Bool {
      return state == ParseState::Accepted;
    }

    constexpr auto needs_recovery() const -> Bool {
      return state == ParseState::Incomplete;
    }

    constexpr auto get_completion() const -> Language::Model::Completion* {
      return completion;
    }

   private:
    Ttx::Concept::Abstract* semantic;
    Language::Types::Composite::Category category;
    ParseState state;
    Language::Model::Completion* completion;
  };

  Member() = delete;

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Result>;
};

}  // namespace Tetrodotoxin::Library::Interpreter
