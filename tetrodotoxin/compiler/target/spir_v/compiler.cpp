// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/target/spir_v/compiler.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/assembler/spir_v.hpp"
#include "tetrodotoxin/model/callables/sample.hpp"
#include "tetrodotoxin/model/constants/aggregate.hpp"
#include "tetrodotoxin/model/interfaces/builtin.hpp"
#include "tetrodotoxin/model/interfaces/located.hpp"
#include "tetrodotoxin/model/operations/binary.hpp"
#include "tetrodotoxin/model/renderables/constant.hpp"
#include "tetrodotoxin/model/renderables/push.hpp"
#include "tetrodotoxin/model/renderables/resource.hpp"
#include "tetrodotoxin/model/stages/implemented.hpp"
#include "tetrodotoxin/model/terminal.hpp"
#include "tetrodotoxin/model/types/represented.hpp"
#include "tetrodotoxin/target/spir_v/validator.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/constants/flag.hpp"
#include "ttx/model/constants/real.hpp"
#include "ttx/model/constants/signed.hpp"
#include "ttx/model/constants/unsigned.hpp"
#include "ttx/model/types/flag.hpp"
#include "ttx/model/types/managed.hpp"
#include "ttx/model/types/real.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/terminal.hpp"
#include "ttx/model/types/unsigned.hpp"
#include "ttx/model/types/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;

using Assembler = Tetrodotoxin::Compiler::Assembler::SpirV;
using Metadata = Tetrodotoxin::Target::SpirV::Metadata;

// TypeRecord assigns one compilation local SPIR-V identity to a semantic Type.
class TypeRecord {
 public:
  constexpr TypeRecord(const Ttx::Model::Type& type, Unsigned_32 id)
      : type(type), id(id) {}

  Reference<Ttx::Model::Type> type;
  Unsigned_32 id;
};

// PointerRecord caches one storage class and pointee representation pair.
class PointerRecord {
 public:
  constexpr PointerRecord(
      Unsigned_32 pointee,
      Assembler::StorageClass storage,
      Unsigned_32 id)
      : pointee(pointee), storage(storage), id(id) {}

  Unsigned_32 pointee;
  Assembler::StorageClass storage;
  Unsigned_32 id;
};

// ConstantRecord assigns one compilation local identity to a semantic value.
class ConstantRecord {
 public:
  constexpr ConstantRecord(const Ttx::Model::Constant& value, Unsigned_32 id)
      : value(value), id(id) {}

  Reference<Ttx::Model::Constant> value;
  Unsigned_32 id;
};

// LengthRecord caches the scalar constants used by SPIR-V array Types.
class LengthRecord {
 public:
  constexpr LengthRecord(Count value, Unsigned_32 id) : value(value), id(id) {}

  Count value;
  Unsigned_32 id;
};

// InterfaceRecord joins one Stage Addressable to its planned storage and
// variable representation.
class InterfaceRecord {
 public:
  constexpr InterfaceRecord(
      const Ttx::Model::Addressable& addressable,
      const Ttx::Model::Type& type,
      Assembler::StorageClass storage,
      Unsigned_32 variable)
      : addressable(addressable),
        type(type),
        storage(storage),
        variable(variable) {}

  Reference<Ttx::Model::Addressable> addressable;
  Reference<Ttx::Model::Type> type;
  Assembler::StorageClass storage;
  Unsigned_32 variable;
};

// ReadRole records which Render declaration group owns one Stage read.
enum class ReadRole : Unsigned_8 {
  Constant,
  Push,
  Resource,
};

// ReadRecord joins one required Stage read to its planned SPIR-V variables.
class ReadRecord {
 public:
  constexpr ReadRecord(
      const Ttx::Model::Addressable& addressable,
      const Ttx::Model::Type& type,
      ReadRole role,
      Count index)
      : addressable(addressable), type(type), role(role), index(index) {}

  Reference<Ttx::Model::Addressable> addressable;
  Reference<Ttx::Model::Type> type;
  ReadRole role;
  Count index;
  Unsigned_32 variable = 0;
  Unsigned_32 private_variable = 0;
};

// StageCompiler owns one complete target transaction. It derives local
// representation records, emits ordered module sections, validates the final
// module, and retains no state after publication.
class StageCompiler {
 public:
  StageCompiler(const Model::Stages::Implemented& stage)
      : stage(stage),
        required(stage.get_required()),
        metadata(
            required.get_execution(),
            required.get_parameters().get_size(),
            required.get_results().get_size(),
            0,
            0) {}

  auto compile(Dynamic::Bytes& complete) -> Bool {
    // Validate the common Body shape supported by the current target profile.
    if (!stage.get_body().is_valid() ||
        stage.get_body().get_blocks().get_size() != 1) {
      return False;
    }

    // Plan every interface and Render read before assigning entry identities.
    Bool interfaces_valid = plan_interfaces();
    if (!interfaces_valid) {
      return False;
    }
    Bool reads_valid = plan_reads();
    if (!reads_valid) {
      return False;
    }

    // Emit the module only after semantic planning succeeds as one unit.
    entry_function = allocate_id();
    entry_label = allocate_id();
    emit_preamble();
    Bool body_valid = emit_body();
    if (!body_valid) {
      return False;
    }

    // Assemble sections in SPIR-V logical order and validate the promised
    // Metadata before returning bytes to the Package compiler.
    Assembler module(complete);
    module.begin_module(next_id);
    complete.concat(preamble);
    complete.concat(debug);
    complete.concat(annotations);
    complete.concat(declarations);
    complete.concat(functions);
    return Target::SpirV::Validator::validate(complete, metadata);
  }

  constexpr auto get_metadata() const -> Metadata { return metadata; }

 private:
  auto allocate_id() -> Unsigned_32 { return next_id++; }

  static auto type_of(const Abstract& value) -> const Abstract& {
    const Abstract& resolved = value.resolve();
    return resolved.is<Ttx::Model::Type>() ? resolved : Invalid::get_invalid();
  }

  static auto type_of(const Ttx::Model::Addressable& value) -> const Abstract& {
    return type_of(value.get_type());
  }

  auto physical_u32() -> Unsigned_32 {
    if (u32_type != 0) {
      return u32_type;
    }
    u32_type = allocate_id();
    Assembler assembler(declarations);
    assembler.type_int(u32_type, 32, False);
    return u32_type;
  }

  auto physical_s32() -> Unsigned_32 {
    if (s32_type != 0) {
      return s32_type;
    }
    s32_type = allocate_id();
    Assembler assembler(declarations);
    assembler.type_int(s32_type, 32, True);
    return s32_type;
  }

  auto physical_real32() -> Unsigned_32 {
    if (real32_type != 0) {
      return real32_type;
    }
    real32_type = allocate_id();
    Assembler assembler(declarations);
    assembler.type_float(real32_type, 32);
    return real32_type;
  }

  auto physical_flag() -> Unsigned_32 {
    if (flag_type != 0) {
      return flag_type;
    }
    flag_type = allocate_id();
    Assembler assembler(declarations);
    assembler.type_bool(flag_type);
    return flag_type;
  }

  auto length_constant(Count length) -> Unsigned_32 {
    for (Count i = 0; i < lengths.get_size(); i++) {
      if (lengths[i].value == length) {
        return lengths[i].id;
      }
    }
    if (length > Unsigned_32(-1)) {
      return 0;
    }
    Unsigned_32 id = allocate_id();
    Assembler assembler(declarations);
    assembler.constant(physical_u32(), id, Unsigned_32(length));
    lengths.insert(LengthRecord(length, id));
    return id;
  }

  auto type_id(const Ttx::Model::Type& source) -> Unsigned_32 {
    const Abstract& resolved = source.resolve();
    if (!resolved.is<Ttx::Model::Type>()) {
      return 0;
    }
    const auto& type = resolved.assume<Ttx::Model::Type>();
    for (Count i = 0; i < types.get_size(); i++) {
      if (&types[i].type.get() == &type) {
        return types[i].id;
      }
    }

    if (type.is<Ttx::Model::Types::Managed>()) {
      return 0;
    }
    if (type.is<Model::Types::Represented>()) {
      const Ttx::Model::Type& shader_type =
          type.assume<Model::Types::Represented>().get_shader_type();
      Unsigned_32 represented = type_id(shader_type);
      if (represented != 0) {
        types.insert(TypeRecord(type, represented));
      }
      return represented;
    }
    if (type.is<Ttx::Model::Types::Unsigned>()) {
      const auto& terminal = type.assume<Ttx::Model::Types::Unsigned>();
      if (terminal.get_width() != 32) {
        return 0;
      }
      Unsigned_32 id = physical_u32();
      types.insert(TypeRecord(type, id));
      return id;
    }
    if (type.is<Ttx::Model::Types::Signed>()) {
      const auto& terminal = type.assume<Ttx::Model::Types::Signed>();
      if (terminal.get_width() != 32) {
        return 0;
      }
      Unsigned_32 id = physical_s32();
      types.insert(TypeRecord(type, id));
      return id;
    }
    if (type.is<Ttx::Model::Types::Real>()) {
      const auto& terminal = type.assume<Ttx::Model::Types::Real>();
      if (terminal.get_width() != 32) {
        return 0;
      }
      Unsigned_32 id = physical_real32();
      types.insert(TypeRecord(type, id));
      return id;
    }
    if (type.is<Ttx::Model::Types::Flag>()) {
      Unsigned_32 id = physical_flag();
      types.insert(TypeRecord(type, id));
      return id;
    }
    if (!type.is<Ttx::Model::Types::Vector>()) {
      return 0;
    }

    const auto& vector = type.assume<Ttx::Model::Types::Vector>();
    Unsigned_32 element = type_id(vector.get_element_type());
    Count count = vector.get_element_count();
    if (element == 0 || count == 0 || count > Unsigned_32(-1)) {
      return 0;
    }
    Unsigned_32 id = allocate_id();
    types.insert(TypeRecord(type, id));
    Assembler assembler(declarations);
    if (count >= 2 && count <= 4 &&
        vector.get_element_type().resolve().is<Ttx::Model::Types::Terminal>()) {
      assembler.type_vector(id, element, Unsigned_32(count));
      return id;
    }

    Unsigned_32 length = length_constant(count);
    if (length == 0) {
      return 0;
    }
    assembler.type_array(id, element, length);
    Assembler annotation(annotations);
    Unsigned_32 stride = size_of(vector.get_element_type());
    if (stride == 0) {
      return 0;
    }
    annotation.decorate(id, Assembler::Decoration::ArrayStride, stride);
    return id;
  }

  auto size_of(const Ttx::Model::Type& source) -> Unsigned_32 {
    const Abstract& resolved = source.resolve();
    if (!resolved.is<Ttx::Model::Type>()) {
      return 0;
    }
    const auto& type = resolved.assume<Ttx::Model::Type>();
    if (type.is<Model::Types::Represented>()) {
      return size_of(
          type.assume<Model::Types::Represented>().get_shader_type());
    }
    if (type.is<Ttx::Model::Types::Terminal>()) {
      Count size = type.assume<Ttx::Model::Types::Terminal>().get_size();
      return size <= Unsigned_32(-1) ? Unsigned_32(size) : 0;
    }
    if (type.is<Ttx::Model::Types::Vector>()) {
      const auto& vector = type.assume<Ttx::Model::Types::Vector>();
      Unsigned_32 element = size_of(vector.get_element_type());
      if (element == 0 ||
          vector.get_element_count() > Unsigned_32(-1) / element) {
        return 0;
      }
      return element * Unsigned_32(vector.get_element_count());
    }
    return 0;
  }

  auto alignment_of(const Ttx::Model::Type& source) -> Unsigned_32 {
    const Abstract& resolved = source.resolve();
    if (!resolved.is<Ttx::Model::Type>()) {
      return 0;
    }
    const auto& type = resolved.assume<Ttx::Model::Type>();
    if (type.is<Model::Types::Represented>()) {
      return alignment_of(
          type.assume<Model::Types::Represented>().get_shader_type());
    }
    if (type.is<Ttx::Model::Types::Terminal>()) {
      Count alignment =
          type.assume<Ttx::Model::Types::Terminal>().get_alignment();
      return alignment <= Unsigned_32(-1) ? Unsigned_32(alignment) : 0;
    }
    if (type.is<Ttx::Model::Types::Vector>()) {
      const auto& vector = type.assume<Ttx::Model::Types::Vector>();
      Unsigned_32 element = alignment_of(vector.get_element_type());
      Count count = vector.get_element_count();
      if (element == 0 || count == 0) {
        return 0;
      }
      return element * Unsigned_32(count == 3 ? 4 : count);
    }
    return 0;
  }

  auto pointer_type(Unsigned_32 pointee, Assembler::StorageClass storage)
      -> Unsigned_32 {
    if (pointee == 0) {
      return 0;
    }
    for (Count i = 0; i < pointers.get_size(); i++) {
      if (pointers[i].pointee == pointee && pointers[i].storage == storage) {
        return pointers[i].id;
      }
    }
    Unsigned_32 id = allocate_id();
    pointers.insert(PointerRecord(pointee, storage, id));
    Assembler assembler(declarations);
    assembler.type_pointer(id, storage, pointee);
    return id;
  }

  auto sampled_image_type() -> Unsigned_32 {
    if (sampled_image != 0) {
      return sampled_image;
    }
    image = allocate_id();
    sampled_image = allocate_id();
    Assembler assembler(declarations);
    assembler.type_image(
        image, physical_real32(), Assembler::Dim::D2, 0, 0, 0, 1,
        Assembler::ImageFormat::Unknown);
    assembler.type_sampled_image(sampled_image, image);
    return sampled_image;
  }

  auto constant_id(const Ttx::Model::Constant& value) -> Unsigned_32 {
    for (Count i = 0; i < constants.get_size(); i++) {
      if (&constants[i].value.get() == &value) {
        return constants[i].id;
      }
    }

    const Abstract& type = type_of(value.get_type());
    if (!type.is<Ttx::Model::Type>()) {
      return 0;
    }

    Unsigned_32 target_type = type_id(type.assume<Ttx::Model::Type>());
    if (target_type == 0) {
      return 0;
    }
    Unsigned_32 id = allocate_id();
    constants.insert(ConstantRecord(value, id));
    Assembler assembler(declarations);
    if (value.is<Ttx::Model::Constants::Unsigned>()) {
      Unsigned_64 payload =
          value.assume<Ttx::Model::Constants::Unsigned>().get_value();
      if (payload > Unsigned_32(-1)) {
        return 0;
      }
      assembler.constant(target_type, id, Unsigned_32(payload));
      return id;
    }
    if (value.is<Ttx::Model::Constants::Signed>()) {
      Signed_64 payload =
          value.assume<Ttx::Model::Constants::Signed>().get_value();
      if (payload < Signed_64(-2147483647) - 1 || payload > 2147483647) {
        return 0;
      }
      assembler.constant(target_type, id, Unsigned_32(Signed_32(payload)));
      return id;
    }
    if (value.is<Ttx::Model::Constants::Real>()) {
      Real_32 payload =
          Real_32(value.assume<Ttx::Model::Constants::Real>().get_value());
      assembler.constant(
          target_type, id, __builtin_bit_cast(Unsigned_32, payload));
      return id;
    }
    if (value.is<Ttx::Model::Constants::Flag>()) {
      // The current assembler deliberately does not expose OpConstantTrue and
      // OpConstantFalse because the selected shaders carry no Flag constants.
      return 0;
    }
    if (!value.is<Model::Constants::Aggregate>()) {
      return 0;
    }

    const auto& aggregate = value.assume<Model::Constants::Aggregate>();
    Dynamic::Vector<Unsigned_32> children;
    for (Count i = 0; i < aggregate.get_size(); i++) {
      const Abstract& child = aggregate.get_value(i);
      if (!child.is<Ttx::Model::Constant>()) {
        return 0;
      }
      Unsigned_32 child_id = constant_id(child.assume<Ttx::Model::Constant>());
      if (child_id == 0) {
        return 0;
      }
      children.insert(child_id);
    }
    assembler.constant_composite(target_type, id, children.get_view());
    return id;
  }

  auto plan_interfaces() -> Bool {
    Bool parameters_valid = plan_interface_layout(
        required.get_parameters(), Assembler::StorageClass::Input);
    if (!parameters_valid) {
      return False;
    }
    return plan_interface_layout(
        required.get_results(), Assembler::StorageClass::Output);
  }

  auto plan_interface_layout(
      const Ttx::Concept::Layout& layout,
      Assembler::StorageClass storage) -> Bool {
    for (Count i = 0; i < layout.get_size(); i++) {
      const Abstract& slot = layout.get_abstract(i);
      if (!slot.is<Ttx::Model::Addressable>()) {
        return False;
      }
      const auto& addressable = slot.assume<Ttx::Model::Addressable>();
      const Abstract& type = type_of(addressable);
      if (!type.is<Ttx::Model::Type>() ||
          type.is<Ttx::Model::Types::Managed>()) {
        return False;
      }

      Unsigned_32 representation = type_id(type.assume<Ttx::Model::Type>());
      Unsigned_32 pointer = pointer_type(representation, storage);
      if (representation == 0 || pointer == 0) {
        return False;
      }

      Bool located = slot.is<Model::Interfaces::Located>();
      Bool builtin = slot.is<Model::Interfaces::Builtin>();
      if (located == builtin) {
        return False;
      }
      if (located) {
        Unsigned_32 location =
            slot.assume<Model::Interfaces::Located>().get_location();
        for (Count existing = 0; existing < interfaces.get_size(); existing++) {
          const Abstract& previous = interfaces[existing].addressable.get();
          if (interfaces[existing].storage == storage &&
              previous.is<Model::Interfaces::Located>() &&
              previous.assume<Model::Interfaces::Located>().get_location() ==
                  location) {
            return False;
          }
        }
      } else {
        auto role = slot.assume<Model::Interfaces::Builtin>().get_role();
        Bool valid_role =
            required.get_execution() ==
                Model::Stages::Required::Execution::Vertex &&
            ((storage == Assembler::StorageClass::Input &&
              role == Model::Interfaces::Builtin::Role::VertexIndex) ||
             (storage == Assembler::StorageClass::Output &&
              role == Model::Interfaces::Builtin::Role::Position));
        if (!valid_role) {
          return False;
        }
      }

      Unsigned_32 variable = allocate_id();
      interfaces.insert(InterfaceRecord(
          addressable, type.assume<Ttx::Model::Type>(), storage, variable));
      Assembler names(debug);
      names.name(variable, addressable.get_name());
      Assembler facts(annotations);
      if (located) {
        facts.decorate(
            variable, Assembler::Decoration::Location,
            slot.assume<Model::Interfaces::Located>().get_location());
      } else {
        auto role = slot.assume<Model::Interfaces::Builtin>().get_role();
        Assembler::BuiltIn target =
            role == Model::Interfaces::Builtin::Role::Position
                ? Assembler::BuiltIn::Position
                : Assembler::BuiltIn::VertexIndex;
        facts.decorate(
            variable, Assembler::Decoration::BuiltIn, Unsigned_32(target));
      }
      Assembler declarations_writer(declarations);
      declarations_writer.variable(pointer, variable, storage);
    }
    return True;
  }

  auto plan_reads() -> Bool {
    Count push_count = 0;
    Count resource_count = 0;
    for (Count i = 0; i < required.get_read_count(); i++) {
      const Abstract& read = required.get_read(i);
      if (!read.is<Ttx::Model::Addressable>()) {
        return False;
      }
      const auto& addressable = read.assume<Ttx::Model::Addressable>();
      const Abstract& type = type_of(addressable);
      if (!type.is<Ttx::Model::Type>()) {
        return False;
      }

      const auto& value_type = type.assume<Ttx::Model::Type>();
      if (read.is<Model::Renderables::Constant>()) {
        reads.insert(
            ReadRecord(addressable, value_type, ReadRole::Constant, 0));
      } else if (read.is<Model::Renderables::Push>()) {
        if (type.is<Ttx::Model::Types::Managed>()) {
          return False;
        }
        reads.insert(
            ReadRecord(addressable, value_type, ReadRole::Push, push_count));
        push_count++;
      } else if (read.is<Model::Renderables::Resource>()) {
        const Abstract& sample = type.resolve_context("sample"_view);
        if (!type.is<Ttx::Model::Types::Managed>() ||
            !sample.is<Model::Callables::Sample>()) {
          return False;
        }
        reads.insert(ReadRecord(
            addressable, value_type, ReadRole::Resource, resource_count));
        resource_count++;
      } else {
        return False;
      }
    }

    metadata = Metadata(
        required.get_execution(), required.get_parameters().get_size(),
        required.get_results().get_size(), push_count, resource_count);
    Bool pushes_valid = emit_push_storage();
    if (!pushes_valid) {
      return False;
    }
    return emit_resource_storage();
  }

  auto emit_push_storage() -> Bool {
    Dynamic::Vector<Unsigned_32> members;
    Unsigned_32 offset = 0;
    Count member_index = 0;
    for (Count i = 0; i < reads.get_size(); i++) {
      ReadRecord& read = reads[i];
      if (read.role != ReadRole::Push) {
        continue;
      }
      Unsigned_32 member_type = type_id(read.type.get());
      Unsigned_32 alignment = alignment_of(read.type.get());
      Unsigned_32 size = size_of(read.type.get());
      if (member_type == 0 || alignment == 0 || size == 0) {
        return False;
      }
      Unsigned_32 remainder = offset % alignment;
      if (remainder != 0) {
        offset += alignment - remainder;
      }
      members.insert(member_type);
      read.index = member_index;
      member_index++;
      offset += size;
    }
    if (members.get_size() == 0) {
      return True;
    }

    push_struct = allocate_id();
    push_variable = allocate_id();
    Assembler names(debug);
    names.name(push_variable, "push"_view);
    Assembler facts(annotations);
    facts.decorate(push_struct, Assembler::Decoration::Block);
    offset = 0;
    for (Count i = 0; i < reads.get_size(); i++) {
      ReadRecord& read = reads[i];
      if (read.role != ReadRole::Push) {
        continue;
      }
      Unsigned_32 alignment = alignment_of(read.type.get());
      Unsigned_32 remainder = offset % alignment;
      if (remainder != 0) {
        offset += alignment - remainder;
      }
      facts.member_decorate(
          push_struct, Unsigned_32(read.index), Assembler::Decoration::Offset,
          offset);
      names.member_name(
          push_struct, Unsigned_32(read.index),
          read.addressable.get().get_name());
      read.variable = push_variable;
      offset += size_of(read.type.get());
    }

    Assembler declarations_writer(declarations);
    declarations_writer.type_struct(push_struct, members.get_view());
    Unsigned_32 push_pointer =
        pointer_type(push_struct, Assembler::StorageClass::PushConstant);
    if (push_pointer == 0) {
      return False;
    }
    declarations_writer.variable(
        push_pointer, push_variable, Assembler::StorageClass::PushConstant);
    return True;
  }

  auto emit_resource_storage() -> Bool {
    Bool has_resource = False;
    for (Count i = 0; i < reads.get_size(); i++) {
      if (reads[i].role == ReadRole::Resource) {
        has_resource = True;
        break;
      }
    }
    if (!has_resource) {
      return True;
    }
    Unsigned_32 resource_type = sampled_image_type();
    Unsigned_32 resource_pointer =
        pointer_type(resource_type, Assembler::StorageClass::UniformConstant);
    if (resource_type == 0 || resource_pointer == 0) {
      return False;
    }
    for (Count i = 0; i < reads.get_size(); i++) {
      ReadRecord& read = reads[i];
      if (read.role != ReadRole::Resource) {
        continue;
      }
      const auto& resource =
          read.addressable.get().assume<Model::Renderables::Resource>();
      read.variable = allocate_id();
      Assembler names(debug);
      names.name(read.variable, resource.get_name());
      Assembler facts(annotations);
      facts.decorate(
          read.variable, Assembler::Decoration::DescriptorSet,
          resource.get_descriptor_set());
      facts.decorate(
          read.variable, Assembler::Decoration::Binding,
          resource.get_binding());
      Assembler declarations_writer(declarations);
      declarations_writer.variable(
          resource_pointer, read.variable,
          Assembler::StorageClass::UniformConstant);
    }
    return True;
  }

  auto emit_preamble() -> void {
    Dynamic::Vector<Unsigned_32> interface_ids;
    for (Count i = 0; i < interfaces.get_size(); i++) {
      interface_ids.insert(interfaces[i].variable);
    }
    Assembler assembler(preamble);
    assembler.capability(Assembler::Capability::Shader);
    assembler.memory_model(
        Assembler::AddressingModel::Logical, Assembler::MemoryModel::GLSL450);
    Assembler::ExecutionModel execution =
        required.get_execution() == Model::Stages::Required::Execution::Vertex
            ? Assembler::ExecutionModel::Vertex
            : Assembler::ExecutionModel::Fragment;
    assembler.entry_point(
        execution, entry_function, "main"_view, interface_ids);
    if (execution == Assembler::ExecutionModel::Fragment) {
      assembler.execution_mode(
          entry_function, Assembler::ExecutionMode::OriginUpperLeft);
    }
    Assembler names(debug);
    names.name(entry_function, "main"_view);
  }

  auto find_interface(View::Bytes name, Assembler::StorageClass storage) const
      -> const InterfaceRecord* {
    for (Count i = 0; i < interfaces.get_size(); i++) {
      if (interfaces[i].storage == storage &&
          interfaces[i].addressable.get().get_name() == name) {
        return &interfaces[i];
      }
    }
    return nullptr;
  }

  auto find_read(const Ttx::Model::Addressable& addressable) -> ReadRecord* {
    for (Count i = 0; i < reads.get_size(); i++) {
      if (&reads[i].addressable.get() == &addressable) {
        return &reads[i];
      }
    }
    return nullptr;
  }

  auto private_constant(ReadRecord& read) -> Unsigned_32 {
    if (read.private_variable != 0) {
      return read.private_variable;
    }
    if (read.role != ReadRole::Constant ||
        !read.addressable.get().is<Model::Renderables::Constant>()) {
      return 0;
    }
    const auto& constant = read.addressable.get()
                               .assume<Model::Renderables::Constant>()
                               .get_value();
    Unsigned_32 initializer = constant_id(constant);
    Unsigned_32 target_type = type_id(read.type.get());
    Unsigned_32 pointer =
        pointer_type(target_type, Assembler::StorageClass::Private);
    if (initializer == 0 || pointer == 0) {
      return 0;
    }
    read.private_variable = allocate_id();
    Assembler names(debug);
    names.name(read.private_variable, read.addressable.get().get_name());
    Assembler declarations_writer(declarations);
    declarations_writer.variable(
        pointer, read.private_variable, Assembler::StorageClass::Private,
        initializer);
    return read.private_variable;
  }

  auto body_value_type(
      const Ttx::Model::Body& body,
      Ttx::Model::Bodies::ValueId id) const -> const Abstract& {
    if (!id.is_valid() || id.get_value() >= body.get_values().get_size()) {
      return Invalid::get_invalid();
    }

    return body.get_values()[id.get_value()].get_type();
  }

  auto operation_operands(
      const Ttx::Model::Body& body,
      Ttx::Model::Bodies::Range range,
      Dynamic::Vector<Unsigned_32>& output,
      const Dynamic::Vector<Unsigned_32>& values) const -> Bool {
    if (!range.fits(body.get_operands().get_size())) {
      return False;
    }
    for (Count i = 0; i < range.get_size(); i++) {
      Unsigned_32 local =
          body.get_operands()[range.get_start() + i].get_value();
      if (local >= values.get_size() || values[local] == 0) {
        return False;
      }
      output.insert(values[local]);
    }
    return True;
  }

  auto field_index(
      const Ttx::Model::Type& receiver,
      const Ttx::Model::Addressable& field) const -> Count {
    const Ttx::Concept::Layout& layout = receiver.get_layout();
    for (Count i = 0; i < layout.get_size(); i++) {
      if (&layout.get_abstract(i) == &field) {
        return i;
      }
    }
    return Count(-1);
  }

  auto emit_body() -> Bool {
    const Ttx::Model::Body& body = stage.get_body();

    // Reserve dense maps from common Body values to their emitted SSA IDs and
    // optional private pointers. Zero remains local target-planning failure and
    // never becomes a semantic graph value.
    Dynamic::Vector<Unsigned_32> values;
    Dynamic::Vector<Unsigned_32> pointers_by_value;
    values.resize(body.get_values().get_size());
    pointers_by_value.resize(body.get_values().get_size());
    for (Count i = 0; i < body.get_values().get_size(); i++) {
      values[i] = 0;
      pointers_by_value[i] = 0;
    }

    // Declare the stage entry function after every target Type and interface
    // record has been planned. The semantic Callable continues to own its
    // signature while this function always uses SPIR-V's void entry ABI.
    Unsigned_32 void_type = allocate_id();
    Unsigned_32 function_type = allocate_id();
    Assembler declarations_writer(declarations);
    declarations_writer.type_void(void_type);
    declarations_writer.type_function(function_type, void_type);
    Assembler assembler(functions);
    assembler.function(
        void_type, entry_function, Assembler::FunctionControl::None,
        function_type);
    assembler.label(entry_label);

    // Materialize each real input interface as the corresponding parameter
    // value. Name lookup is confined to matching the already validated Stage
    // interface records and does not select semantic identity.
    const Ttx::Concept::Layout& implemented_parameters = stage.get_parameters();
    if (body.get_parameter_count() != implemented_parameters.get_size()) {
      return False;
    }
    for (Count i = 0; i < implemented_parameters.get_size(); i++) {
      const Abstract& parameter = implemented_parameters.get_abstract(i);
      if (!parameter.is<Ttx::Model::Addressable>()) {
        return False;
      }
      const InterfaceRecord* interface =
          find_interface(parameter.get_name(), Assembler::StorageClass::Input);
      const Abstract& parameter_type =
          type_of(parameter.assume<Ttx::Model::Addressable>());
      if (interface == nullptr || !parameter_type.is<Ttx::Model::Type>() ||
          type_id(interface->type.get()) !=
              type_id(parameter_type.assume<Ttx::Model::Type>())) {
        return False;
      }

      Unsigned_32 result = allocate_id();
      assembler.load(
          type_id(parameter_type.assume<Ttx::Model::Type>()), result,
          interface->variable);
      values[i] = result;
    }

    // Lower the closed common Body union one operation at a time. Each
    // supported alternative either publishes all of its target IDs or fails
    // without changing the meaning of the common operation.
    Bool returned = False;
    for (Count operation_index = 0;
         operation_index < body.get_operations().get_size();
         operation_index++) {
      const auto& operation = body.get_operations()[operation_index];
      Bool emitted = operation.visit(
          []() -> Bool { return False; },
          [&](const Ttx::Model::Bodies::Operations::Constant& value) -> Bool {
            Unsigned_32 result = constant_id(value.get_value());
            if (result == 0) {
              return False;
            }

            values[value.get_result().get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::Aggregate& value) -> Bool {
            Dynamic::Vector<Unsigned_32> elements;
            Bool collected = operation_operands(
                body, value.get_elements(), elements, values);
            const Abstract& result_type =
                body_value_type(body, value.get_result());
            if (!collected || !result_type.is<Ttx::Model::Type>()) {
              return False;
            }

            Unsigned_32 target_type =
                type_id(result_type.assume<Ttx::Model::Type>());
            if (target_type == 0) {
              return False;
            }

            Unsigned_32 result = allocate_id();
            assembler.composite_construct(
                target_type, result, elements.get_view());
            values[value.get_result().get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::Projection& value) -> Bool {
            const Abstract& receiver_type =
                body_value_type(body, value.get_receiver());
            const Abstract& result_type =
                body_value_type(body, value.get_result());
            if (!receiver_type.is<Ttx::Model::Type>() ||
                !result_type.is<Ttx::Model::Type>()) {
              return False;
            }

            Count index = field_index(
                receiver_type.assume<Ttx::Model::Type>(),
                value.get_addressable());
            if (index == Count(-1)) {
              return False;
            }

            Unsigned_32 result = allocate_id();
            const Unsigned_32 index_word = Unsigned_32(index);
            assembler.composite_extract(
                type_id(result_type.assume<Ttx::Model::Type>()), result,
                values[value.get_receiver().get_value()],
                View::Vector<Unsigned_32>(&index_word, 1));
            values[value.get_result().get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::Load& value) -> Bool {
            if (!value.get_receivers().is_empty()) {
              return False;
            }

            ReadRecord* read = find_read(value.get_addressable());
            if (read == nullptr) {
              return False;
            }

            Unsigned_32 result = 0;
            switch (read->role) {
            case ReadRole::Constant: {
              const auto& constant = read->addressable.get()
                                         .assume<Model::Renderables::Constant>()
                                         .get_value();
              result = constant_id(constant);
              pointers_by_value[value.get_result().get_value()] =
                  private_constant(*read);
              break;
            }
            case ReadRole::Push: {
              Unsigned_32 member_index = length_constant(read->index);
              Unsigned_32 member_pointer = pointer_type(
                  type_id(read->type.get()),
                  Assembler::StorageClass::PushConstant);
              Unsigned_32 access = allocate_id();
              assembler.access_chain(
                  member_pointer, access, read->variable,
                  View::Vector<Unsigned_32>(&member_index, 1));
              result = allocate_id();
              assembler.load(type_id(read->type.get()), result, access);
              break;
            }
            case ReadRole::Resource:
              result = allocate_id();
              assembler.load(sampled_image_type(), result, read->variable);
              break;
            }

            if (result == 0) {
              return False;
            }

            values[value.get_result().get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::Call& value) -> Bool {
            const auto body_operands = body.get_operands();
            if (!value.get_arguments().fits(body_operands.get_size()) ||
                !value.get_results().fits(body.get_values().get_size())) {
              return False;
            }

            // Calls retain authored or object-owned semantic identities. The
            // selected Shader slice currently lowers the Image sample method.
            if (!value.get_callable().is<Model::Callables::Sample>() ||
                value.get_arguments().get_size() != 2 ||
                value.get_results().get_size() != 1) {
              return False;
            }

            Dynamic::Vector<Unsigned_32> arguments;
            Bool collected = operation_operands(
                body, value.get_arguments(), arguments, values);
            Ttx::Model::Bodies::ValueId result_id(
                value.get_results().get_start());
            const Abstract& result_type = body_value_type(body, result_id);
            if (!collected || !result_type.is<Ttx::Model::Type>()) {
              return False;
            }

            Unsigned_32 result = allocate_id();
            assembler.image_sample_implicit_lod(
                type_id(result_type.assume<Ttx::Model::Type>()), result,
                arguments[0], arguments[1]);
            values[result_id.get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::Binary& value) -> Bool {
            if (!Tetrodotoxin::Model::Operations::Binary::is_valid(value)) {
              return False;
            }

            const Abstract& result_type =
                body_value_type(body, value.get_result());
            if (!result_type.is<Ttx::Model::Type>()) {
              return False;
            }

            Unsigned_32 type = type_id(result_type.assume<Ttx::Model::Type>());
            Unsigned_32 result = allocate_id();
            Unsigned_32 left = values[value.get_left().get_value()];
            Unsigned_32 right = values[value.get_right().get_value()];
            switch (
                Tetrodotoxin::Model::Operations::Binary::get_operator(value)) {
            case Tetrodotoxin::Model::Operations::Binary::Operator::Add:
              assembler.fadd(type, result, left, right);
              break;
            case Tetrodotoxin::Model::Operations::Binary::Operator::Subtract:
              assembler.fsub(type, result, left, right);
              break;
            case Tetrodotoxin::Model::Operations::Binary::Operator::Multiply:
              assembler.fmul(type, result, left, right);
              break;
            case Tetrodotoxin::Model::Operations::Binary::Operator::Divide:
              assembler.fdiv(type, result, left, right);
              break;
            }

            values[value.get_result().get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::Convert& value) -> Bool {
            const Abstract& result_type =
                body_value_type(body, value.get_result());
            if (!result_type.is<Ttx::Model::Type>()) {
              return False;
            }

            Unsigned_32 result = allocate_id();
            assembler.convert_u_to_f(
                type_id(result_type.assume<Ttx::Model::Type>()), result,
                values[value.get_source().get_value()]);
            values[value.get_result().get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::IndexedRead& value)
              -> Bool {
            const Abstract& result_type =
                body_value_type(body, value.get_result());
            Unsigned_32 collection_pointer =
                pointers_by_value[value.get_receiver().get_value()];
            Unsigned_32 index = values[value.get_index().get_value()];
            if (!result_type.is<Ttx::Model::Type>() ||
                collection_pointer == 0 || index == 0) {
              return False;
            }

            Unsigned_32 access = allocate_id();
            Unsigned_32 result_pointer = pointer_type(
                type_id(result_type.assume<Ttx::Model::Type>()),
                Assembler::StorageClass::Private);
            assembler.access_chain(
                result_pointer, access, collection_pointer,
                View::Vector<Unsigned_32>(&index, 1));
            Unsigned_32 result = allocate_id();
            assembler.load(
                type_id(result_type.assume<Ttx::Model::Type>()), result,
                access);
            values[value.get_result().get_value()] = result;
            return True;
          },
          [&](const Ttx::Model::Bodies::Operations::Return& value) -> Bool {
            if (returned || value.get_values().get_size() !=
                                stage.get_results().get_size()) {
              return False;
            }

            Dynamic::Vector<Unsigned_32> returned_ids;
            Bool collected = operation_operands(
                body, value.get_values(), returned_ids, values);
            if (!collected) {
              return False;
            }

            for (Count i = 0; i < stage.get_results().get_size(); i++) {
              const Abstract& result_slot = stage.get_results().get_abstract(i);
              const InterfaceRecord* interface = find_interface(
                  result_slot.get_name(), Assembler::StorageClass::Output);
              if (interface == nullptr) {
                return False;
              }

              assembler.store(interface->variable, returned_ids[i]);
            }

            assembler.return_void();
            returned = True;
            return True;
          },
          [](const auto&) -> Bool {
            // Shader legality rejects common operations which this target
            // cannot represent instead of changing their common meaning.
            return False;
          });
      if (!emitted) {
        return False;
      }
    }

    // A Shader entry Body must publish exactly one terminal return path before
    // the function can become part of the module transaction.
    if (!returned) {
      return False;
    }

    assembler.function_end();
    return True;
  }

  const Model::Stages::Implemented& stage;
  const Model::Stages::Required& required;
  Metadata metadata;
  Dynamic::Bytes preamble;
  Dynamic::Bytes debug;
  Dynamic::Bytes annotations;
  Dynamic::Bytes declarations;
  Dynamic::Bytes functions;
  Dynamic::Vector<TypeRecord> types;
  Dynamic::Vector<PointerRecord> pointers;
  Dynamic::Vector<ConstantRecord> constants;
  Dynamic::Vector<LengthRecord> lengths;
  Dynamic::Vector<InterfaceRecord> interfaces;
  Dynamic::Vector<ReadRecord> reads;
  Unsigned_32 next_id = 1;
  Unsigned_32 u32_type = 0;
  Unsigned_32 s32_type = 0;
  Unsigned_32 real32_type = 0;
  Unsigned_32 flag_type = 0;
  Unsigned_32 image = 0;
  Unsigned_32 sampled_image = 0;
  Unsigned_32 push_struct = 0;
  Unsigned_32 push_variable = 0;
  Unsigned_32 entry_function = 0;
  Unsigned_32 entry_label = 0;
};

static auto module_path(const Model::Shader& shader, View::Bytes stage)
    -> Dynamic::Bytes {
  Dynamic::Bytes path("shader/"_view);
  path.concat(shader.get_name());
  path.append('/');
  path.concat(stage);
  path.concat(".spv"_view);
  return path;
}

auto Target::SpirV::Compiler::compile(
    const Model::Shader& shader,
    Dynamic::Vector<Module>& modules) -> Bool {
  Dynamic::Vector<Module> completed;
  if (shader.get_stage_count() == 0) {
    return False;
  }
  for (Count i = 0; i < shader.get_stage_count(); i++) {
    const Abstract& abstract = shader.get_stage(i);
    if (!abstract.is<Model::Stages::Implemented>()) {
      return False;
    }
    const auto& stage = abstract.assume<Model::Stages::Implemented>();
    StageCompiler compiler(stage);
    Dynamic::Bytes words;
    Bool compiled = compiler.compile(words);
    if (!compiled) {
      return False;
    }
    Dynamic::Bytes path = module_path(shader, stage.get_name());
    if (!Model::Terminal::is_valid_path(path)) {
      return False;
    }
    completed.emplace(Module(
        path, static_cast<Dynamic::Bytes&&>(words), compiler.get_metadata()));
  }
  modules = static_cast<Dynamic::Vector<Module>&&>(completed);
  return True;
}
