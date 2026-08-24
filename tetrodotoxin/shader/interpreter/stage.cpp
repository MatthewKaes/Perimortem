// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/interpreter/stage.hpp"

#include "tetrodotoxin/library/interpreter/member.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/render/language/attributes.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Shader;

auto Interpreter::Stage::parse(
    Shader::Language::Program& program,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto member = Library::Interpreter::Member::parse(cursor, definition);
  BAIL_IF(!member);
  auto function = member->get_semantic().select<Library::Language::Function>();
  if (!function) {
    cursor.create_expression_error(
        definition.get_anchor(),
        "Shader Stage qualifiers create one Library Function."_view);
    return False;
  }

  // The Function parser owns the complete Signature and Block. Shader adds
  // only Render admission policy around that real executable identity.
  Bool attributes_valid = Render::Language::Attributes::validate(
      cursor, definition.get_attributes(),
      Render::Language::Attributes::Placement::Stage);
  auto validate_slots = [&](const Library::Language::Model::Layout& layout) {
    for (Count i = 0; i < layout.get_size(); i++) {
      attributes_valid &= Render::Language::Attributes::validate(
          cursor, layout.get_slot_attributes(i),
          Render::Language::Attributes::Placement::StageEntry);
    }
  };
  validate_slots(function->get_signature().get_parameters());
  validate_slots(function->get_signature().get_results());

  Bool retained = program.retain_authored_definition(
      *function, definition,
      Library::Language::Types::Composite::Category::Callable, cursor);
  if (member->needs_recovery()) {
    cursor.recover_to_scoped_statement();
  }
  return attributes_valid && retained && member->is_accepted();
}
