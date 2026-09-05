// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/binding.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

Language::Binding::Binding(
    View::Bytes name,
    const Ttx::Concept::Abstract& target)
    : Binding(name, target, target.get_documentation()) {}

Language::Binding::Binding(
    View::Bytes name,
    const Ttx::Concept::Abstract& target,
    const Ttx::Concept::Documentation& documentation)
    : name(name), target(&target), documentation(documentation) {}
