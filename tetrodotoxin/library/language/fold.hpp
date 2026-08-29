// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/pack.hpp"

namespace Tetrodotoxin::Library::Language {

// Ask the current producer for its ordinary fold concept. The query retains no
// answer: live producers are asked again on every call, while a Constant
// answers axiomatically with itself.
auto query_fold(const Model::Pack& source) -> const Ttx::Concept::Abstract&;

// Recover the Library flow owned by a Constant fold answer. A Constant from
// another Dialect remains a valid TTX answer but is intentionally not coerced
// into Library's value domain.
auto query_folded_pack(Model::Pack& source)
    -> Perimortem::Core::Option<Model::Pack&>;
auto query_folded_pack(const Model::Pack& source)
    -> Perimortem::Core::Option<const Model::Pack&>;

// A caller that owns an allocation domain may materialize identity-free empty
// or composed flow as one aggregate Constant. Every child is still queried at
// the time of this call; the aggregate contains only the resulting immutable
// identities.
auto query_folded_pack(
    Perimortem::Memory::Allocator::Arena& domain,
    Model::Pack& source) -> Perimortem::Core::Option<Model::Pack&>;

// Convert a Library computation outcome into the public concept answer. Only
// one aggregate Constant identity may escape; a recognized non-foldable
// result is the shared None fact.
auto fold_answer(Perimortem::Core::Option<const Model::Pack&> result)
    -> const Ttx::Concept::Abstract&;
auto fold_answer(Perimortem::Core::Option<Model::Pack&> result)
    -> const Ttx::Concept::Abstract&;

}  // namespace Tetrodotoxin::Library::Language
