// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Option is one nonnullable optional value Type. It specializes the receiving
// Type protocol to turn produced flow into one of its two runtime states:
//
// Pack with no values  to  Option target fit  to  absent Option[T]
// absent Option[T]     to  postfix question   to  return a Pack with no values
// absent Option[T]     to  postfix bang       to  Pack containing default(T)
//
// The Pack is empty only on the flow edge. Option and its element always keep
// nonempty Type Layouts, so absence never erases either semantic Type.
class Option : public Model::Type {
 public:
  enum class Kind : Unsigned_8 {
    Absent,
    Present,
  };

  TTX_CONTRACT(Option, Model::Type);

  constexpr Option(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element)
      : name(name), element(element) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto accepts(const Model::Pack& source) const -> Bool override;

  auto create_fitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_element_type() const -> const Model::Type& {
    return element;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Carries either no value or one exact payload value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
