// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/language/program.hpp"

#include "tetrodotoxin/shader/language/contract.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Shader::Language::Program::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Tetrodotoxin::Language::TypeReference contract,
    Abstract& context) -> Program& {
  return domain.construct_from<Program>(
      [&]() { return Program(domain, definition, contract, context); });
}

auto Shader::Language::Program::retain_shader_binding(
    Library::Language::Field& field,
    Render::Language::Binding::Kind kind) -> void {
  bindings.insert(Binding(field, kind));
}

auto Shader::Language::Program::validate_contract(Cursor& cursor) -> Bool {
  auto selected = contract.resolve(cursor, context);
  auto render_contract = selected
                             ? selected->select<Render::Language::Structure>()
                             : Option<const Render::Language::Structure&>();
  if (!render_contract) {
    cursor.create_expression_error(
        contract.get_anchor(),
        "Shader contract route must select one Render Structure."_view);
    return False;
  }

  Contract negotiator;
  BAIL_IF(!negotiator.validate(cursor, *render_contract, *this));
  contract_type =
      Reference<const Render::Language::Structure>(*render_contract);
  return True;
}
