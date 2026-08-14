// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Model {
class Pack;
}

namespace Tetrodotoxin::Library::Language::Types {

// Option is one nonnullable optional value Type. Its target fitting owns the
// transition from produced flow into one of its two runtime states:
//
// Pack with no values  to  Option target fit  to  absent Option[T]
// absent Option[T]     to  postfix question   to  return a Pack with no values
// absent Option[T]     to  postfix bang       to  Pack containing default(T)
//
// The Pack is empty only on the flow edge. Option and its element always keep
// nonempty Type Layouts, so absence never erases either semantic Type.
class Option : public Ttx::Model::Type {
 public:
  enum class Kind : Unsigned_8 {
    Absent,
    Present,
  };

  TTX_CONTRACT(Option, Ttx::Model::Type);

  constexpr Option(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& element)
      : name(name), element(element) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_element_type() const -> const Ttx::Model::Type& {
    return element;
  }

  auto accepts(const Model::Pack& source) const -> Bool;

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Model::Type& element;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Carries either no value or one exact payload value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
