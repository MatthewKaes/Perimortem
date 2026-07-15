// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/target/system_v.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/hash.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/abi/type.hpp"
#include "tetrodotoxin/compiler/allocation/registers.hpp"
#include "tetrodotoxin/compiler/allocation/system_v.hpp"
#include "tetrodotoxin/compiler/assembler/x86_64.hpp"
#include "tetrodotoxin/compiler/execution/binary.hpp"
#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/compiler/execution/call.hpp"
#include "tetrodotoxin/compiler/execution/operand.hpp"
#include "tetrodotoxin/compiler/execution/return.hpp"
#include "tetrodotoxin/compiler/target/cpp.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;

// Lowers one immutable execution program into a native object transaction.
//
// Each function receives deterministic first-fit register allocation before
// its operations are emitted. Constants and external names are interned while
// instructions are written, but symbols and relocations are published only
// after every function succeeds. A failed function therefore leaves no partial
// object in the linker.
//
// The transaction keeps generated code and publication records in memory. This
// is simpler than patching linker state during instruction emission and fits
// the small programs produced today. Ordered vectors define publication order,
// while maps provide direct lookup for interning and export projection.
class SystemVLowerer {
 public:
  SystemVLowerer(
      const Program& program,
      Ttx::Lexical::Errors& errors,
      Linker::Linker& linker)
      : program(program), errors(errors), linker(linker) {}

  auto build() -> Bool;

 private:
  struct GeneratedFunction {
    View::Bytes name;
    Range range;
  };

  struct GeneratedString {
    View::Bytes name;
    View::Bytes value;
    Range range;
  };

  struct ExternalSymbol {
    View::Bytes name;
  };

  struct Relocation {
    enum class Target : Bits_8 {
      String,
      External,
    };

    Target target;
    Count target_index;
    Count code_offset;
  };

  auto lower_function(const Execution::Function& function) -> Bool;
  auto lower_binary(
      const Execution::Function& function,
      const Execution::Body& body,
      const Allocation::Registers& allocation,
      const Execution::Binary& binary) -> Bool;
  auto lower_call(
      const Execution::Function& function,
      const Execution::Body& body,
      const Allocation::Registers& allocation,
      const Execution::Call& call) -> Bool;
  auto lower_return(
      const Execution::Function& function,
      const Execution::Body& body,
      const Allocation::Registers& allocation,
      const Execution::Return& result) -> Bool;
  auto materialize(
      const Execution::Body& body,
      const Allocation::Registers& allocation,
      const Ttx::Type& type,
      const Execution::Operand& operand,
      Count component,
      Assembler::x86_64::Reg destination,
      Count stack_shift = 0) -> Bool;
  auto publish() -> void;
  auto report(const Execution::Function& function, View::Bytes message) -> Bool;
  auto string_index(View::Bytes value) -> Count;
  auto external_index(View::Bytes name) -> Count;
  auto local_string_name(View::Bytes value) -> View::Bytes;
  auto append_hex(Managed::Bytes& output, Bits_64 value) -> void;

  Allocator::Arena arena;
  const Program& program;
  Ttx::Lexical::Errors& errors;
  Linker::Linker& linker;
  Dynamic::Bytes machine_code;
  Dynamic::Bytes string_data;
  Dynamic::Vector<GeneratedFunction> functions;
  Dynamic::Map<View::Bytes, Count> function_indices;
  Dynamic::Vector<GeneratedString> strings;
  Dynamic::Map<View::Bytes, Count> string_indices;
  Dynamic::Vector<ExternalSymbol> externals;
  Dynamic::Map<View::Bytes, Count> external_indices;
  Dynamic::Vector<Relocation> relocations;
};

static constexpr Static::Vector<Assembler::x86_64::Reg, 6>
    allocation_registers = {{
      Assembler::x86_64::Reg::RBX,
      Assembler::x86_64::Reg::R12,
      Assembler::x86_64::Reg::R13,
      Assembler::x86_64::Reg::R14,
      Assembler::x86_64::Reg::R15,
      Assembler::x86_64::Reg::RBP,
    }};

static constexpr Static::Vector<Assembler::x86_64::Reg, 6> integer_arguments = {
  {
    Assembler::x86_64::Reg::RDI,
    Assembler::x86_64::Reg::RSI,
    Assembler::x86_64::Reg::RDX,
    Assembler::x86_64::Reg::RCX,
    Assembler::x86_64::Reg::R8,
    Assembler::x86_64::Reg::R9,
  }};

static constexpr Static::Vector<Assembler::x86_64::Xmm, 8> real_arguments = {{
  Assembler::x86_64::Xmm::XMM0,
  Assembler::x86_64::Xmm::XMM1,
  Assembler::x86_64::Xmm::XMM2,
  Assembler::x86_64::Xmm::XMM3,
  Assembler::x86_64::Xmm::XMM4,
  Assembler::x86_64::Xmm::XMM5,
  Assembler::x86_64::Xmm::XMM6,
  Assembler::x86_64::Xmm::XMM7,
}};

static constexpr Static::Vector<Assembler::x86_64::Reg, 2> integer_results = {{
  Assembler::x86_64::Reg::RAX,
  Assembler::x86_64::Reg::RDX,
}};

static constexpr Static::Vector<Assembler::x86_64::Xmm, 2> real_results = {{
  Assembler::x86_64::Xmm::XMM0,
  Assembler::x86_64::Xmm::XMM1,
}};

auto Target::SystemV::backend() -> Backend {
  return Backend(
      [](const Program& program, Ttx::Lexical::Errors& errors,
         Linker::Linker& linker) -> Bool {
        SystemVLowerer compiler(program, errors, linker);
        return compiler.build();
      },
      Target::Cpp::build_header);
}

auto SystemVLowerer::build() -> Bool {
  View::Vector<Execution::Function> source_functions = program.get_functions();
  for (Count i = 0; i < source_functions.get_size(); i++) {
    Bool lowered = lower_function(source_functions[i]);
    if (!lowered) {
      return False;
    }
  }

  publish();
  return True;
}

auto SystemVLowerer::lower_function(const Execution::Function& function)
    -> Bool {
  const Ttx::Function& signature = function.get_signature();
  const Execution::Body& body = function.get_body();
  View::Vector<Execution::Binding> bindings = body.get_bindings();
  Dynamic::Vector<Count> widths;
  for (Count i = 0; i < bindings.get_size(); i++) {
    Count width = Allocation::SystemV::get_width(bindings[i].get_type());
    if (width == 0) {
      return report(
          function,
          "The x86-64 System V backend cannot allocate this value type."_view);
    }

    widths.insert(width);
  }

  Allocation::Registers allocation(
      body, widths.get_view(), allocation_registers.get_size());
  Count used_registers = 0;
  for (Count i = 0; i < bindings.get_size(); i++) {
    Count color = allocation.get_color(i);
    if (color == Count(-1)) {
      continue;
    }

    Count end = color + widths[i];
    if (end > used_registers) {
      used_registers = end;
    }
  }

  Assembler::x86_64 assembler(machine_code);
  const Count function_start = machine_code.get_size();
  for (Count i = 0; i < used_registers; i++) {
    assembler.push(allocation_registers[i]);
  }

  Count frame_slots = allocation.get_spill_count();
  if (((used_registers + frame_slots) & 1) == 0) {
    frame_slots++;
  }

  const Count frame_size = frame_slots * 8;
  if (frame_size != 0) {
    assembler.sub(Bits_32(frame_size), Assembler::x86_64::Reg::RSP);
  }

  View::Vector<Ttx::Member> parameters =
      signature.get_parameters().get_members();
  Allocation::SystemV convention(parameters);
  for (Count i = 0; i < parameters.get_size(); i++) {
    Count width = Allocation::SystemV::get_width(parameters[i].get_type());
    for (Count k = 0; k < width; k++) {
      Allocation::SystemV::Location location = convention.get(i, k);
      Assembler::x86_64::Reg source = Assembler::x86_64::Reg::R11;
      if (location.bank == Allocation::SystemV::Bank::Stack) {
        Signed_32 offset =
            Signed_32(frame_size + used_registers * 8 + 8 + location.index * 8);
        assembler.mov(Assembler::x86_64::Reg::RSP, offset, source);
      } else if (location.bank == Allocation::SystemV::Bank::Real) {
        assembler.mov_bits(real_arguments[location.index], source);
      } else {
        source = integer_arguments[location.index];
      }

      Count color = allocation.get_color(i, k);
      if (color < allocation_registers.get_size()) {
        assembler.mov(source, allocation_registers[color]);
      } else {
        Count spill = allocation.get_spill(i, k);
        if (spill == Count(-1)) {
          return report(function, "System V parameter has no location."_view);
        }

        assembler.mov(
            source, Assembler::x86_64::Reg::RSP, Signed_32(spill * 8));
      }
    }
  }

  View::Vector<Execution::Operation> operations = body.get_operations();
  for (Count i = 0; i < operations.get_size(); i++) {
    Bool lowered = False;
    Bool returned = False;
    operations[i].visit(
        []() {},
        [&](const Execution::Binary& binary) {
          lowered = lower_binary(function, body, allocation, binary);
        },
        [&](const Execution::Call& call) {
          lowered = lower_call(function, body, allocation, call);
        },
        [&](const Execution::Return& result) {
          lowered = lower_return(function, body, allocation, result);
          returned = True;
        });
    if (!lowered) {
      return False;
    }

    if (returned) {
      break;
    }
  }

  if (frame_size != 0) {
    assembler.add(Bits_32(frame_size), Assembler::x86_64::Reg::RSP);
  }

  for (Count i = used_registers; i > 0; i--) {
    assembler.pop(allocation_registers[i - 1]);
  }

  assembler.ret();
  function_indices.insert(function.get_symbol(), functions.get_size());
  functions.insert({
    function.get_symbol(),
    {function_start, machine_code.get_size() - function_start},
  });
  return True;
}

auto SystemVLowerer::lower_binary(
    const Execution::Function& function,
    const Execution::Body& body,
    const Allocation::Registers& allocation,
    const Execution::Binary& binary) -> Bool {
  const Ttx::Type& type = binary.get_operand_type();
  const Count binding = binary.get_result().get_id();
  Count color = allocation.get_color(binding);
  Count spill = allocation.get_spill(binding);
  Abi::Lowering lowering =
      Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned());
  switch (lowering) {
  case Abi::Lowering::Bool:
  case Abi::Lowering::Integer:
  case Abi::Lowering::Signed:
    break;

  default:
    return report(
        function, "System V binary operation requires an integer value."_view);
  }

  if (color == Count(-1) && spill == Count(-1)) {
    return report(function, "System V binary result has no location."_view);
  }

  Assembler::x86_64 assembler(machine_code);
  Assembler::x86_64::Reg destination = color < allocation_registers.get_size()
                                           ? allocation_registers[color]
                                           : Assembler::x86_64::Reg::R10;
  if (binary.get_operator() == Execution::Binary::Operator::Equal) {
    Bool materialized_left = materialize(
        body, allocation, type, binary.get_left(), 0,
        Assembler::x86_64::Reg::RAX);
    if (!materialized_left) {
      return report(
          function, "System V could not materialize comparison operands."_view);
    }

    Bool materialized_right = materialize(
        body, allocation, type, binary.get_right(), 0,
        Assembler::x86_64::Reg::R11);
    if (!materialized_right) {
      return report(
          function, "System V could not materialize comparison operands."_view);
    }

    assembler.zero(destination);
    assembler.compare(Assembler::x86_64::Reg::R11, Assembler::x86_64::Reg::RAX);
    assembler.set_equal(destination);
    if (spill != Count(-1)) {
      assembler.mov(
          destination, Assembler::x86_64::Reg::RSP, Signed_32(spill * 8));
    }

    return True;
  }

  Bool materialized_left =
      materialize(body, allocation, type, binary.get_left(), 0, destination);
  if (!materialized_left) {
    return report(
        function, "System V could not materialize a binary operand."_view);
  }

  if (binary.get_operator() == Execution::Binary::Operator::Divide ||
      binary.get_operator() == Execution::Binary::Operator::Remainder) {
    Bool materialized_dividend = materialize(
        body, allocation, type, binary.get_left(), 0,
        Assembler::x86_64::Reg::RAX);
    if (!materialized_dividend) {
      return report(
          function, "System V could not materialize division operands."_view);
    }

    Bool materialized_divisor = materialize(
        body, allocation, type, binary.get_right(), 0,
        Assembler::x86_64::Reg::R11);
    if (!materialized_divisor) {
      return report(
          function, "System V could not materialize division operands."_view);
    }

    if (lowering == Abi::Lowering::Signed) {
      assembler.signed_divide(Assembler::x86_64::Reg::R11);
    } else {
      assembler.divide(Assembler::x86_64::Reg::R11);
    }

    assembler.mov(
        binary.get_operator() == Execution::Binary::Operator::Remainder
            ? Assembler::x86_64::Reg::RDX
            : Assembler::x86_64::Reg::RAX,
        destination);
    if (spill != Count(-1)) {
      assembler.mov(
          destination, Assembler::x86_64::Reg::RSP, Signed_32(spill * 8));
    }

    return True;
  }

  Bool materialized_right = materialize(
      body, allocation, type, binary.get_right(), 0,
      Assembler::x86_64::Reg::R11);
  if (!materialized_right) {
    return report(
        function, "System V could not materialize a binary operand."_view);
  }

  switch (binary.get_operator()) {
  case Execution::Binary::Operator::Add:
    assembler.add(Assembler::x86_64::Reg::R11, destination);
    break;
  case Execution::Binary::Operator::Subtract:
    assembler.sub(Assembler::x86_64::Reg::R11, destination);
    break;
  case Execution::Binary::Operator::Multiply:
    assembler.multiply(Assembler::x86_64::Reg::R11, destination);
    break;
  case Execution::Binary::Operator::Divide:
  case Execution::Binary::Operator::Remainder:
  case Execution::Binary::Operator::Equal:
    break;
  }

  if (spill != Count(-1)) {
    assembler.mov(
        destination, Assembler::x86_64::Reg::RSP, Signed_32(spill * 8));
  }

  return True;
}

auto SystemVLowerer::lower_call(
    const Execution::Function& function,
    const Execution::Body& body,
    const Allocation::Registers& allocation,
    const Execution::Call& call) -> Bool {
  Range argument_range = call.get_arguments();
  View::Vector<Execution::Operand> operands = body.get_operands();
  View::Vector<Ttx::Member> parameters =
      call.get_signature().get_parameters().get_members();
  if (argument_range.size != parameters.get_size()) {
    return report(function, "System V call argument layout is invalid."_view);
  }

  Assembler::x86_64 assembler(machine_code);
  Allocation::SystemV convention(parameters);
  for (Count i = 0; i < argument_range.size; i++) {
    const Execution::Operand& operand = operands[argument_range.start + i];
    const Ttx::Type& type = parameters[i].get_type();
    Count width = Allocation::SystemV::get_width(type);
    if (convention.get(i, 0).bank == Allocation::SystemV::Bank::Stack) {
      continue;
    }

    for (Count k = 0; k < width; k++) {
      Allocation::SystemV::Location location = convention.get(i, k);
      Bool lowered;
      if (location.bank == Allocation::SystemV::Bank::Real) {
        lowered = materialize(
            body, allocation, type, operand, k, Assembler::x86_64::Reg::R11);
        if (lowered) {
          assembler.mov_bits(
              Assembler::x86_64::Reg::R11, real_arguments[location.index]);
        }
      } else {
        lowered = materialize(
            body, allocation, type, operand, k,
            integer_arguments[location.index]);
      }

      if (!lowered) {
        return report(
            function,
            "The x86-64 System V backend could not lower a call argument."_view);
      }
    }
  }

  const Count stack_components = convention.get_stack_count();
  const Bool padded = (stack_components & 1) != 0;
  if (padded) {
    assembler.sub(Bits_32(8), Assembler::x86_64::Reg::RSP);
  }

  Count stack_shift = padded ? 1 : 0;
  for (Count i = argument_range.size; i > 0; i--) {
    if (convention.get(i - 1, 0).bank != Allocation::SystemV::Bank::Stack) {
      continue;
    }

    const Execution::Operand& operand = operands[argument_range.start + i - 1];
    const Ttx::Type& type = parameters[i - 1].get_type();
    Count width = Allocation::SystemV::get_width(type);
    for (Count k = width; k > 0; k--) {
      Bool lowered = materialize(
          body, allocation, type, operand, k - 1, Assembler::x86_64::Reg::R11,
          stack_shift);
      if (!lowered) {
        return report(
            function,
            "The x86-64 System V backend could not lower a call argument."_view);
      }

      assembler.push(Assembler::x86_64::Reg::R11);
      stack_shift++;
    }
  }

  const Count target =
      external_index(program.resolve_symbol(call.get_symbol()));
  assembler.call();
  relocations.insert({
    Relocation::Target::External,
    target,
    machine_code.get_size(),
  });
  Count call_stack = (stack_components + (padded ? 1 : 0)) * 8;
  if (call_stack != 0) {
    assembler.add(Bits_32(call_stack), Assembler::x86_64::Reg::RSP);
  }

  Range results = call.get_results();
  View::Vector<Ttx::Member> result_members =
      call.get_signature().get_result().get_members();
  Count result_components = 0;
  for (Count i = 0; i < result_members.get_size(); i++) {
    result_components +=
        Allocation::SystemV::get_width(result_members[i].get_type());
  }

  if (results.size != result_members.get_size() || result_components > 2) {
    return report(
        function,
        "The x86-64 System V backend supports two register results."_view);
  }

  Count integer_index = 0;
  Count real_index = 0;
  for (Count i = 0; i < result_members.get_size(); i++) {
    const Ttx::Type& type = result_members[i].get_type();
    Count width = Allocation::SystemV::get_width(type);
    Abi::Lowering lowering =
        Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned());
    for (Count k = 0; k < width; k++) {
      Assembler::x86_64::Reg source;
      if (lowering == Abi::Lowering::Real) {
        source = Assembler::x86_64::Reg::R11;
        assembler.mov_bits(real_results[real_index++], source);
      } else {
        source = integer_results[integer_index++];
      }

      Count binding = results.start + i;
      Count color = allocation.get_color(binding, k);
      if (color < allocation_registers.get_size()) {
        assembler.mov(source, allocation_registers[color]);
        continue;
      }

      Count spill = allocation.get_spill(binding, k);
      if (spill == Count(-1)) {
        return report(function, "System V call result has no location."_view);
      }

      assembler.mov(source, Assembler::x86_64::Reg::RSP, Signed_32(spill * 8));
    }
  }

  return True;
}

auto SystemVLowerer::materialize(
    const Execution::Body& body,
    const Allocation::Registers& allocation,
    const Ttx::Type& type,
    const Execution::Operand& operand,
    Count component,
    Assembler::x86_64::Reg destination,
    Count stack_shift) -> Bool {
  Assembler::x86_64 assembler(machine_code);
  Abi::Lowering lowering =
      Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned());
  const Execution::Addressable* addressable =
      operand.find<Execution::Addressable>();
  if (addressable != nullptr) {
    Count color = allocation.get_color(addressable->get_id(), component);
    if (color < allocation_registers.get_size()) {
      assembler.mov(allocation_registers[color], destination);
      return True;
    }

    Count spill = allocation.get_spill(addressable->get_id(), component);
    if (spill == Count(-1)) {
      return False;
    }

    assembler.mov(
        Assembler::x86_64::Reg::RSP, Signed_32((spill + stack_shift) * 8),
        destination);
    return True;
  }

  const Execution::Constant* constant = operand.find<Execution::Constant>();
  if (constant == nullptr) {
    return False;
  }

  if (const View::Bytes* bytes = constant->find<View::Bytes>()) {
    if (lowering != Abi::Lowering::ViewBytes || component > 1) {
      return False;
    }

    if (component == 1) {
      assembler.mov(Bits_64(bytes->get_size()), destination);
      return True;
    }

    const Count target = string_index(*bytes);
    assembler.read_only(destination);
    relocations.insert({
      Relocation::Target::String,
      target,
      machine_code.get_size(),
    });
    return True;
  }

  if (component != 0) {
    return False;
  }

  if (const Bits_64* integer = constant->find<Bits_64>()) {
    if (lowering != Abi::Lowering::Bool && lowering != Abi::Lowering::Integer &&
        lowering != Abi::Lowering::Signed) {
      return False;
    }

    assembler.mov(*integer, destination);
    return True;
  }

  if (const Signed_64* integer = constant->find<Signed_64>()) {
    if (lowering != Abi::Lowering::Bool && lowering != Abi::Lowering::Integer &&
        lowering != Abi::Lowering::Signed) {
      return False;
    }

    Bits_64 bits;
    Data::copy(Data::cast<Bits_8>(&bits), *integer);
    assembler.mov(bits, destination);
    return True;
  }

  if (const Real_64* real = constant->find<Real_64>()) {
    if (lowering != Abi::Lowering::Real) {
      return False;
    }

    Bits_64 bits;
    Data::copy(Data::cast<Bits_8>(&bits), *real);
    assembler.mov(bits, destination);
    return True;
  }

  const Bool* flag = constant->find<Bool>();
  if (flag == nullptr || lowering != Abi::Lowering::Bool) {
    return False;
  }

  assembler.mov(Bits_64(*flag ? 1 : 0), destination);
  return True;
}

auto SystemVLowerer::lower_return(
    const Execution::Function& function,
    const Execution::Body& body,
    const Allocation::Registers& allocation,
    const Execution::Return& result) -> Bool {
  Perimortem::Utility::Range values = result.get_values();
  View::Vector<Ttx::Member> members =
      function.get_signature().get_result().get_members();
  if (values.size != members.get_size()) {
    return report(function, "System V return layout is invalid."_view);
  }

  Count components = 0;
  for (Count i = 0; i < members.get_size(); i++) {
    components += Allocation::SystemV::get_width(members[i].get_type());
  }

  if (components > 2) {
    return report(
        function,
        "The x86-64 System V backend supports two register returns."_view);
  }

  Assembler::x86_64 assembler(machine_code);
  Count integer_index = 0;
  Count real_index = 0;
  View::Vector<Execution::Operand> operands = body.get_operands();
  for (Count i = 0; i < members.get_size(); i++) {
    const Ttx::Type& type = members[i].get_type();
    const Execution::Operand& operand = operands[values.start + i];
    Count width = Allocation::SystemV::get_width(type);
    Abi::Lowering lowering =
        Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned());
    for (Count k = 0; k < width; k++) {
      if (lowering == Abi::Lowering::Real) {
        Bool materialized = materialize(
            body, allocation, type, operand, k, Assembler::x86_64::Reg::R11);
        if (!materialized) {
          return report(
              function, "System V could not materialize a real return."_view);
        }

        assembler.mov_bits(
            Assembler::x86_64::Reg::R11, real_results[real_index++]);
        continue;
      }

      Bool materialized = materialize(
          body, allocation, type, operand, k, integer_results[integer_index++]);
      if (!materialized) {
        return report(
            function, "System V could not materialize an integer return."_view);
      }
    }
  }

  return True;
}

auto SystemVLowerer::publish() -> void {
  Bits_16 program_section = 0;
  if (!machine_code.is_empty()) {
    program_section = linker.add_section(
        Linker::Object::Section::Type::Program, machine_code);
  }

  Bits_16 string_section = 0;
  if (!string_data.is_empty()) {
    string_section =
        linker.add_section(Linker::Object::Section::Type::Strings, string_data);
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    Linker::Object::Symbol symbol = Linker::Object::Symbol::create_function(
        functions[i].name, program_section,
        Linker::Object::Symbol::Visibility::Global);
    symbol.set_range(functions[i].range);
    linker.add_symbol(symbol);
  }

  View::Vector<Abi::Export> exports = program.get_exports();
  Dynamic::Set<View::Bytes> published_exports;
  for (Count i = 0; i < exports.get_size(); i++) {
    Bool inserted = published_exports.insert(exports[i].get_symbol());
    if (!inserted) {
      continue;
    }

    const auto* target = function_indices.find(exports[i].get_target_symbol());
    if (target == nullptr) {
      continue;
    }

    Linker::Object::Symbol symbol = Linker::Object::Symbol::create_function(
        exports[i].get_symbol(), program_section,
        Linker::Object::Symbol::Visibility::Global);
    symbol.set_range(functions[target->value].range);
    linker.add_symbol(symbol);
  }

  Count string_symbols = Count(-1);
  for (Count i = 0; i < strings.get_size(); i++) {
    Count symbol = linker.add_symbol(
        Linker::Object::Symbol::create_string(
            strings[i].name, string_section, strings[i].range));
    if (i == 0) {
      string_symbols = symbol;
    }
  }

  Count external_symbols = Count(-1);
  for (Count i = 0; i < externals.get_size(); i++) {
    Count symbol = linker.add_symbol(
        Linker::Object::Symbol::create_external(
            externals[i].name, Linker::Object::Symbol::Type::Function));
    if (i == 0) {
      external_symbols = symbol;
    }
  }

  for (Count i = 0; i < relocations.get_size(); i++) {
    const Relocation& relocation = relocations[i];
    Count symbol =
        (relocation.target == Relocation::Target::String ? string_symbols
                                                         : external_symbols) +
        relocation.target_index;
    if (relocation.target == Relocation::Target::String) {
      linker.add_relocation(
          Linker::Object::Relocation::create_pc32(
              program_section, symbol, relocation.code_offset));
    } else {
      linker.add_relocation(
          Linker::Object::Relocation::create_plt32(
              program_section, symbol, relocation.code_offset));
    }
  }
}

auto SystemVLowerer::report(
    const Execution::Function& function,
    View::Bytes message) -> Bool {
  errors.insert(function.get_source(), message);
  return False;
}

auto SystemVLowerer::string_index(View::Bytes value) -> Count {
  const auto* existing = string_indices.find(value);
  if (existing != nullptr) {
    return existing->value;
  }

  const Count offset = string_data.get_size();
  string_data.concat(value);
  const Count index = strings.get_size();
  string_indices.insert(value, index);
  strings.insert({local_string_name(value), value, {offset, value.get_size()}});
  return index;
}

auto SystemVLowerer::external_index(View::Bytes name) -> Count {
  const auto* existing = external_indices.find(name);
  if (existing != nullptr) {
    return existing->value;
  }

  const Count index = externals.get_size();
  external_indices.insert(name, index);
  externals.insert({name});
  return index;
}

auto SystemVLowerer::local_string_name(View::Bytes value) -> View::Bytes {
  Managed::Bytes output(arena);
  output.concat(".Lttx_"_view);
  append_hex(output, Hash(value).get_value());
  output.append('_');
  append_hex(output, string_data.get_size());
  return output;
}

auto SystemVLowerer::append_hex(Managed::Bytes& output, Bits_64 value) -> void {
  constexpr View::Bytes digits = "0123456789abcdef"_view;
  for (Signed_32 shift = 60; shift >= 0; shift -= 4) {
    output.append(digits[(value >> shift) & 0x0F]);
  }
}
