// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/target/spir_v/validator.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/assembler/spir_v.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

using Assembler = Tetrodotoxin::Compiler::Assembler::SpirV;

// DecorationFact retains one parsed interface decoration for final comparison
// with the semantic Metadata promised by the compiler.
class DecorationFact {
 public:
  constexpr DecorationFact(Unsigned_32 target, Assembler::Decoration decoration)
      : target(target), decoration(decoration) {}

  Unsigned_32 target;
  Assembler::Decoration decoration;
};

static auto read_word(View::Bytes words, Count index) -> Unsigned_32 {
  Count byte = index * 4;
  if (byte + 4 > words.get_size()) {
    return 0;
  }

  return Unsigned_32(words[byte]) | (Unsigned_32(words[byte + 1]) << 8) |
         (Unsigned_32(words[byte + 2]) << 16) |
         (Unsigned_32(words[byte + 3]) << 24);
}

static auto section(Assembler::Op operation) -> Unsigned_8 {
  switch (operation) {
  case Assembler::Op::Capability:
    return 0;
  case Assembler::Op::MemoryModel:
    return 1;
  case Assembler::Op::EntryPoint:
  case Assembler::Op::ExecutionMode:
    return 2;
  case Assembler::Op::Name:
  case Assembler::Op::MemberName:
    return 3;
  case Assembler::Op::Decorate:
  case Assembler::Op::MemberDecorate:
    return 4;
  case Assembler::Op::TypeVoid:
  case Assembler::Op::TypeBool:
  case Assembler::Op::TypeInt:
  case Assembler::Op::TypeFloat:
  case Assembler::Op::TypeVector:
  case Assembler::Op::TypeImage:
  case Assembler::Op::TypeSampler:
  case Assembler::Op::TypeSampledImage:
  case Assembler::Op::TypeArray:
  case Assembler::Op::TypeStruct:
  case Assembler::Op::TypePointer:
  case Assembler::Op::TypeFunction:
  case Assembler::Op::Constant:
  case Assembler::Op::ConstantComposite:
  case Assembler::Op::Variable:
    return 5;
  default:
    return 6;
  }
}

static auto literal_end(View::Bytes words, Count start, Count end) -> Count {
  for (Count word = start; word < end; word++) {
    Unsigned_32 value = read_word(words, word);
    for (Count byte = 0; byte < 4; byte++) {
      if (((value >> (byte * 8)) & 0xFF) == 0) {
        return word + 1;
      }
    }
  }

  return Count(-1);
}

auto Target::SpirV::Validator::validate(
    View::Bytes words,
    const Metadata& expected) -> Bool {
  if (words.get_size() < 20 || words.get_size() % 4 != 0 ||
      read_word(words, 0) != Assembler::magic || read_word(words, 4) != 0) {
    return False;
  }

  Unsigned_32 bound = read_word(words, 3);
  if (bound < 2 || bound > 1'000'000) {
    return False;
  }

  Dynamic::Vector<Unsigned_8> definitions;
  definitions.resize(bound);
  Dynamic::Vector<Unsigned_8> storage;
  storage.resize(bound);
  for (Count i = 0; i < bound; i++) {
    definitions[i] = 0;
    storage[i] = 0xFF;
  }

  Dynamic::Vector<Unsigned_32> references;
  Dynamic::Vector<DecorationFact> decorations;
  auto define = [&](Unsigned_32 id) -> Bool {
    if (id == 0 || id >= bound || definitions[id] != 0) {
      return False;
    }
    definitions[id] = 1;
    return True;
  };
  auto reference = [&](Unsigned_32 id) -> Bool {
    if (id == 0 || id >= bound) {
      return False;
    }
    references.insert(id);
    return True;
  };

  Count capability_count = 0;
  Count memory_model_count = 0;
  Count entry_count = 0;
  Count function_count = 0;
  Count label_count = 0;
  Count return_count = 0;
  Count entry_interface_count = 0;
  Count push_offset_count = 0;
  Unsigned_32 entry_model = Unsigned_32(-1);
  Unsigned_8 last_section = 0;
  Bool in_function = False;
  Count word_count = words.get_size() / 4;
  for (Count offset = 5; offset < word_count;) {
    Unsigned_32 header = read_word(words, offset);
    Count size = Count(header >> 16);
    Assembler::Op operation = Assembler::Op(header & 0xFFFF);
    if (size == 0 || offset + size > word_count) {
      return False;
    }

    Unsigned_8 current_section = section(operation);
    if (current_section < last_section) {
      return False;
    }
    last_section = current_section;

    auto operand = [&](Count index) -> Unsigned_32 {
      return read_word(words, offset + index);
    };
    Bool valid = True;
    switch (operation) {
    case Assembler::Op::Capability:
      valid =
          size == 2 && operand(1) == Unsigned_32(Assembler::Capability::Shader);
      capability_count++;
      break;
    case Assembler::Op::MemoryModel:
      valid = size == 3;
      memory_model_count++;
      break;
    case Assembler::Op::EntryPoint: {
      if (size < 4) {
        return False;
      }
      entry_model = operand(1);
      valid = reference(operand(2));
      Count interface_start = literal_end(words, offset + 3, offset + size);
      if (interface_start == Count(-1)) {
        return False;
      }
      entry_interface_count = offset + size - interface_start;
      for (Count word = interface_start; word < offset + size; word++) {
        Bool inserted = reference(read_word(words, word));
        if (!inserted) {
          valid = False;
        }
      }
      entry_count++;
      break;
    }
    case Assembler::Op::ExecutionMode:
      valid = size == 3 && reference(operand(1));
      break;
    case Assembler::Op::Name:
      valid = size >= 3 && reference(operand(1)) &&
              literal_end(words, offset + 2, offset + size) != Count(-1);
      break;
    case Assembler::Op::MemberName:
      valid = size >= 4 && reference(operand(1)) &&
              literal_end(words, offset + 3, offset + size) != Count(-1);
      break;
    case Assembler::Op::Decorate:
      valid = (size == 3 || size == 4) && reference(operand(1));
      if (valid) {
        decorations.insert(
            DecorationFact(operand(1), Assembler::Decoration(operand(2))));
      }
      break;
    case Assembler::Op::MemberDecorate:
      valid = size == 5 && reference(operand(1));
      if (valid && operand(3) == Unsigned_32(Assembler::Decoration::Offset)) {
        push_offset_count++;
      }
      break;
    case Assembler::Op::TypeVoid:
    case Assembler::Op::TypeBool:
    case Assembler::Op::TypeSampler:
      valid = size == 2 && define(operand(1));
      break;
    case Assembler::Op::TypeInt:
      valid = size == 4 && define(operand(1));
      break;
    case Assembler::Op::TypeFloat:
      valid = size == 3 && define(operand(1));
      break;
    case Assembler::Op::TypeVector:
      valid = size == 4 && define(operand(1)) && reference(operand(2));
      break;
    case Assembler::Op::TypeImage:
      valid = size == 9 && define(operand(1)) && reference(operand(2));
      break;
    case Assembler::Op::TypeSampledImage:
      valid = size == 3 && define(operand(1)) && reference(operand(2));
      break;
    case Assembler::Op::TypeArray:
      valid = size == 4 && define(operand(1)) && reference(operand(2)) &&
              reference(operand(3));
      break;
    case Assembler::Op::TypeStruct:
      valid = size >= 2 && define(operand(1));
      for (Count i = 2; valid && i < size; i++) {
        valid = reference(operand(i));
      }
      break;
    case Assembler::Op::TypePointer:
      valid = size == 4 && define(operand(1)) && reference(operand(3));
      break;
    case Assembler::Op::TypeFunction:
      valid = size >= 3 && define(operand(1));
      for (Count i = 2; valid && i < size; i++) {
        valid = reference(operand(i));
      }
      break;
    case Assembler::Op::Constant:
      valid = size == 4 && reference(operand(1)) && define(operand(2));
      break;
    case Assembler::Op::ConstantComposite:
      valid = size >= 3 && reference(operand(1)) && define(operand(2));
      for (Count i = 3; valid && i < size; i++) {
        valid = reference(operand(i));
      }
      break;
    case Assembler::Op::Variable:
      valid = (size == 4 || size == 5) && reference(operand(1)) &&
              define(operand(2));
      if (valid) {
        storage[operand(2)] = Unsigned_8(operand(3));
        if (size == 5) {
          valid = reference(operand(4));
        }
      }
      break;
    case Assembler::Op::Function:
      valid = size == 5 && !in_function && reference(operand(1)) &&
              define(operand(2)) && reference(operand(4));
      in_function = True;
      function_count++;
      break;
    case Assembler::Op::Label:
      valid = size == 2 && in_function && define(operand(1));
      label_count++;
      break;
    case Assembler::Op::Load:
      valid = size == 4 && in_function && reference(operand(1)) &&
              define(operand(2)) && reference(operand(3));
      break;
    case Assembler::Op::Store:
      valid = size == 3 && in_function && reference(operand(1)) &&
              reference(operand(2));
      break;
    case Assembler::Op::AccessChain:
      valid = size >= 5 && in_function && reference(operand(1)) &&
              define(operand(2)) && reference(operand(3));
      for (Count i = 4; valid && i < size; i++) {
        valid = reference(operand(i));
      }
      break;
    case Assembler::Op::CompositeConstruct:
      valid = size >= 3 && in_function && reference(operand(1)) &&
              define(operand(2));
      for (Count i = 3; valid && i < size; i++) {
        valid = reference(operand(i));
      }
      break;
    case Assembler::Op::CompositeExtract:
      valid = size >= 5 && in_function && reference(operand(1)) &&
              define(operand(2)) && reference(operand(3));
      break;
    case Assembler::Op::ImageSampleImplicitLod:
      valid = size == 5 && in_function && reference(operand(1)) &&
              define(operand(2)) && reference(operand(3)) &&
              reference(operand(4));
      break;
    case Assembler::Op::ConvertUToF:
      valid = size == 4 && in_function && reference(operand(1)) &&
              define(operand(2)) && reference(operand(3));
      break;
    case Assembler::Op::FAdd:
    case Assembler::Op::FSub:
    case Assembler::Op::FMul:
    case Assembler::Op::FDiv:
      valid = size == 5 && in_function && reference(operand(1)) &&
              define(operand(2)) && reference(operand(3)) &&
              reference(operand(4));
      break;
    case Assembler::Op::Return:
      valid = size == 1 && in_function;
      return_count++;
      break;
    case Assembler::Op::FunctionEnd:
      valid = size == 1 && in_function;
      in_function = False;
      break;
    default:
      return False;
    }

    if (!valid) {
      return False;
    }
    offset += size;
  }

  if (in_function || capability_count != 1 || memory_model_count != 1 ||
      entry_count != 1 || function_count != 1 || label_count != 1 ||
      return_count != 1 ||
      entry_interface_count !=
          expected.get_input_count() + expected.get_output_count()) {
    return False;
  }

  Unsigned_32 expected_model =
      expected.get_execution() == Model::Stages::Required::Execution::Vertex
          ? Unsigned_32(Assembler::ExecutionModel::Vertex)
          : Unsigned_32(Assembler::ExecutionModel::Fragment);
  if (entry_model != expected_model ||
      push_offset_count != expected.get_push_count()) {
    return False;
  }

  for (Count i = 0; i < references.get_size(); i++) {
    if (definitions[references[i]] == 0) {
      return False;
    }
  }

  Count inputs = 0;
  Count outputs = 0;
  Count resources = 0;
  Count push_variables = 0;
  for (Count id = 1; id < bound; id++) {
    switch (Assembler::StorageClass(storage[id])) {
    case Assembler::StorageClass::Input:
      inputs++;
      break;
    case Assembler::StorageClass::Output:
      outputs++;
      break;
    case Assembler::StorageClass::UniformConstant:
      resources++;
      break;
    case Assembler::StorageClass::PushConstant:
      push_variables++;
      break;
    default:
      break;
    }
  }

  if (inputs != expected.get_input_count() ||
      outputs != expected.get_output_count() ||
      resources != expected.get_resource_count() ||
      push_variables != (expected.get_push_count() == 0 ? 0 : 1)) {
    return False;
  }

  Count interface_decorations = 0;
  Count bindings = 0;
  Count descriptor_sets = 0;
  Count blocks = 0;
  for (Count i = 0; i < decorations.get_size(); i++) {
    const DecorationFact& fact = decorations[i];
    switch (fact.decoration) {
    case Assembler::Decoration::Location:
    case Assembler::Decoration::BuiltIn:
      if (storage[fact.target] != Unsigned_8(Assembler::StorageClass::Input) &&
          storage[fact.target] != Unsigned_8(Assembler::StorageClass::Output)) {
        return False;
      }

      interface_decorations++;
      break;
    case Assembler::Decoration::Binding:
      bindings++;
      break;
    case Assembler::Decoration::DescriptorSet:
      descriptor_sets++;
      break;
    case Assembler::Decoration::Block:
      blocks++;
      break;
    default:
      break;
    }
  }

  return interface_decorations == inputs + outputs &&
         bindings == expected.get_resource_count() &&
         descriptor_sets == expected.get_resource_count() &&
         blocks == (expected.get_push_count() == 0 ? 0 : 1);
}
