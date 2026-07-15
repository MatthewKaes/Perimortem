// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/program.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Compiler;

auto Program::define(
    Ttx::Lexical::Source source,
    View::Bytes symbol,
    const Ttx::Function& function,
    const Execution::Body& body) -> Bool {
  if (symbol.is_empty() || body.get_blocks().is_empty()) {
    return False;
  }

  if (function_indices.find(&function) != nullptr ||
      function_symbols.find(symbol) != nullptr) {
    return False;
  }

  function_indices.insert(&function, functions.get_size());
  function_symbols.insert(symbol, functions.get_size());
  functions.insert(Execution::Function(source, symbol, function, body));
  return True;
}

auto Program::expose(Abi::Type type) -> Bool {
  const auto* existing = type_paths.find(type.get_path());
  if (existing != nullptr) {
    return &types[existing->value].get_type() == &type.get_type();
  }

  type_paths.insert(type.get_path(), types.get_size());
  types.insert(type);
  return True;
}

auto Program::expose(const Abi::Export& export_) -> Bool {
  for (Count i = 0; i < exports.get_size(); i++) {
    Bool same_public_name = exports[i].get_path() == export_.get_path() &&
                            exports[i].get_function().get_name() ==
                                export_.get_function().get_name();
    if (same_public_name) {
      return False;
    }

    if (exports[i].get_symbol() == export_.get_symbol()) {
      Bool same_callable =
          &exports[i].get_function() == &export_.get_function() &&
          exports[i].get_target_symbol() == export_.get_target_symbol();
      if (!same_callable) {
        return False;
      }
    } else if (
        &exports[i].get_function() == &export_.get_function() ||
        exports[i].get_target_symbol() == export_.get_target_symbol()) {
      return False;
    }
  }

  const Count index = exports.get_size();
  if (export_indices.find(&export_.get_function()) == nullptr) {
    export_indices.insert(&export_.get_function(), index);
  }

  if (exported_symbols.find(export_.get_target_symbol()) == nullptr) {
    exported_symbols.insert(export_.get_target_symbol(), export_.get_symbol());
  }

  exports.insert(export_);
  return True;
}

auto Program::find(const Ttx::Function& function) const
    -> const Execution::Function* {
  const auto* entry = function_indices.find(&function);
  return entry == nullptr ? nullptr : &functions[entry->value];
}

auto Program::find_export(const Ttx::Function& function) const
    -> const Abi::Export* {
  const auto* entry = export_indices.find(&function);
  return entry == nullptr ? nullptr : &exports[entry->value];
}

auto Program::resolve_symbol(View::Bytes symbol) const -> View::Bytes {
  const auto* entry = exported_symbols.find(symbol);
  return entry == nullptr ? symbol : entry->value;
}
