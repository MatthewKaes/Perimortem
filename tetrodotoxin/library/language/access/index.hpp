// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Index is the reference only scalar or ranged selector for Access storage.
// Runtime bounds engage a scalar address or the complete requested interval.
// an invalid target writes nothing. Index never manufactures an Addressable or
// Option and never supplies an ordinary read. Safe value selection belongs to
// Slice's `:[...]` forms.
class Index : public Expression {
 public:
  TTX_CONTRACT(Index, Expression);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Index"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_write_type(const Model::Type& access_scope) const
      -> Perimortem::Core::Option<const Model::Type&> override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

  auto lower_write_target(Llvm::Builder& body) const -> Bool override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }
  constexpr auto get_index() const -> const Expression& { return first; }
  constexpr auto get_count() const
      -> Perimortem::Core::Option<const Expression&> {
    return count.visit(
        []() -> Perimortem::Core::Option<const Expression&> { return {}; },
        [](const Ttx::Concept::Reference<Expression>& selected)
            -> Perimortem::Core::Option<const Expression&> {
          return selected.get();
        });
  }
  constexpr auto get_range_count() const -> Perimortem::Core::Option<Count> {
    return range_count;
  }
  auto get_element_type() const -> const Ttx::Concept::Abstract&;

 protected:
  auto link_write_target(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      const Model::Type& access_scope) -> Bool override;
  auto accepts_write(const Model::Pack& source, const Model::Type& access_scope)
      const -> Bool override;

 private:
  constexpr Index(
      Expression& receiver,
      Expression& index,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), first(index) {}
  constexpr Index(
      Expression& receiver,
      Expression& start,
      Expression& count,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        receiver(receiver),
        first(start),
        count(Ttx::Concept::Reference<Expression>(count)) {}

  auto link_target(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope)
      -> Bool;

  Expression& receiver;
  Expression& first;
  Perimortem::Core::Option<Ttx::Concept::Reference<Expression>> count;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      element_type;
  Perimortem::Core::Option<Count> range_count;
};

}  // namespace Tetrodotoxin::Library::Language::Access
