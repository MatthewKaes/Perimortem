// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/addressable.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::View {

// Slice borrows the available part of one requested contiguous interval.
class Slice : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "slice"_view;

  TTX_CONTRACT(Slice, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& count,
      const Language::Model::Type& result) -> Slice&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return results;
  }

  auto fold_call(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&> override;

 private:
  Slice(
      Ttx::Model::Layouts::Addressable& self,
      Ttx::Model::Layouts::Addressable& start,
      Ttx::Model::Layouts::Addressable& count,
      const Language::Model::Type& result);

  Perimortem::Core::Static::
      Vector<Ttx::Concept::Reference<const Ttx::Concept::Abstract>, 3>
          parameter_entries;
  Ttx::Model::Layouts::Named parameters;
  Ttx::Model::Layouts::Ranged results;
  const Language::Model::Type& result_type;

  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Borrows the available part of a requested contiguous interval."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::View
