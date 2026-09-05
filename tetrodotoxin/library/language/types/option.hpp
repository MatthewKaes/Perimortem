// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/admission.hpp"
#include "tetrodotoxin/library/language/model/completion.hpp"
#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/propagation.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"

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
class Option : public Model::Type, public Model::Completion {
 public:
  enum class Kind : U8 {
    Absent,
    Present,
  };


  constexpr Option(
      Perimortem::Core::View::Bytes name,
      const Model::Type& element,
      const Model::Types::Flag& flag)
      : name(name),
        element(element),
        flag(flag),
        propagation(element),
        admission(*this),
        initialization(*this) {}

  TTX_NAME(name);

  TTX_DOCUMENTATION(documentation);

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&>;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  auto accepts(const Model::Pack& source) const -> Bool;

  auto create_admitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const -> Perimortem::Core::Option<Model::Pack&>;

  auto validate_layout(Ttx::Lexical::Cursor& cursor) const -> Bool override;

  constexpr auto get_element_type() const -> const Model::Type& {
    return element;
  }

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return element.get_declaration_anchor();
  }

  constexpr auto get_flag_type() const -> const Model::Types::Flag& {
    return flag;
  }

 private:
  Perimortem::Core::View::Bytes name;
  const Model::Type& element;
  const Model::Types::Flag& flag;
  Model::Propagation propagation;
  Model::OwnedAdmission<Option> admission;
  Model::OwnedInitialization<Option> initialization;
  static constexpr Ttx::Documentations::Comment documentation{
    "Carries either no value or one exact payload value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
