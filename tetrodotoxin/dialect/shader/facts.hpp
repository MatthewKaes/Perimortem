// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/resolution/record.hpp"
#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/ast/param.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

namespace Tetrodotoxin::Dialect::Shader {

enum class Stage : Bits_8 {
  None,
  Vertex,
  Pixel,
};

struct Descriptor {
  Bool present = False;
  Count set = 0;
  Count slot = 0;
};

struct Builtin {
  Bool present = False;
  Perimortem::Core::View::Bytes name;
  Bool has_slot = False;
  Count slot = 0;
};

struct EntryPoint {
  const Syntax::Type::Declaration* type = nullptr;
  const Syntax::Ast::Function* function = nullptr;
  Stage stage = Stage::None;
};

struct InterfaceField {
  Perimortem::Core::View::Bytes name;
  const Syntax::Type::Ref* type = nullptr;
  Builtin builtin;
  Count location = 0;
  Bool named = False;
};

struct Interface {
  const Syntax::Ast::Function* function = nullptr;
  Perimortem::Memory::Dynamic::Vector<InterfaceField> inputs;
  Perimortem::Memory::Dynamic::Vector<InterfaceField> outputs;
};

struct HostField {
  Perimortem::Core::View::Bytes name;
  const Syntax::Type::Ref* type = nullptr;
  Perimortem::Core::View::Bytes type_name;
  Count offset = 0;
  Count size = 0;
  Count alignment = 1;
  const Syntax::Ast::Member* member = nullptr;
};

struct PushConstantBlock {
  Perimortem::Core::View::Bytes name;
  const Syntax::Type::Declaration* type = nullptr;
  Count byte_size = 0;
  Count alignment = 1;
  Count field_start = 0;
  Count field_count = 0;
};

struct PushConstantMetadata {
  Perimortem::Memory::Dynamic::Vector<PushConstantBlock> blocks;
  Perimortem::Memory::Dynamic::Vector<HostField> fields;
};

struct DescriptorBinding {
  Perimortem::Core::View::Bytes name;
  const Syntax::Type::Ref* type = nullptr;
  Perimortem::Core::View::Bytes type_name;
  Count set = 0;
  Count slot = 0;
  const Syntax::Type::Declaration* owner = nullptr;
  const Syntax::Ast::Member* member = nullptr;
};

class Facts {
 public:
  auto entry_points(const Resolution::Record& record) const
      -> Perimortem::Memory::Dynamic::Vector<EntryPoint>;
  auto interface_of(const Syntax::Ast::Function& function) const -> Interface;
  auto push_constants(const Resolution::Record& record) const
      -> PushConstantMetadata;
  auto descriptors(const Resolution::Record& record) const
      -> Perimortem::Memory::Dynamic::Vector<DescriptorBinding>;

  auto stage_of(const Syntax::Type::Declaration& type) const -> Stage;
  auto is_entry_point(const Syntax::Type::Declaration& type) const -> Bool;
  auto is_push_constant(const Syntax::Type::Declaration& type) const -> Bool;

  auto descriptor_of(const Syntax::Ast::Member& member) const -> Descriptor;
  auto builtin_of(const Syntax::Ast::Member& member) const -> Builtin;
  auto builtin_of(const Syntax::Ast::Param& param) const -> Builtin;
};

auto stage_name(Stage stage) -> Perimortem::Core::View::Bytes;

}  // namespace Tetrodotoxin::Dialect::Shader
