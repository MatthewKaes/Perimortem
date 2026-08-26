// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#if __has_include("llvm/IR/Module.h")
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#else
#error LLVM Module is required by Projection lowering
#endif

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/terminal/llvm/lowering/projections.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

auto Llvm::Lowering::Projections::lower(
    Module::Program& program,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Projection> projections)
    -> Bool {
  llvm::Module& module = *llvm::unwrap(&program.get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::IntegerType& count = *llvm::Type::getInt64Ty(context);
  Core::Static::Vector<llvm::Type*, 3> members = {{&pointer, &count, &count}};
  llvm::StructType& projection_type = *llvm::StructType::get(
      context,
      llvm::ArrayRef<llvm::Type*>(members.get_data(), members.get_size()));

  for (const Tetrodotoxin::Terminal::Abi::Projection& projection :
       projections) {
    const auto& shader = projection.get_program();
    const auto& instance = shader.get_instance();
    const auto& parameters = shader.get_instance_parameters_field();
    auto payload = program.get_carriers().get_payload(instance);
    auto field_index = program.get_carriers().get_field_index(parameters);
    auto parameter_type =
        program.get_carriers().get_type(parameters.get_type());
    BAIL_IF(!payload || !field_index || !parameter_type);

    auto& payload_type = *llvm::cast<llvm::StructType>(llvm::unwrap(*payload));
    const llvm::DataLayout& layout = module.getDataLayout();
    Count offset = layout.getStructLayout(&payload_type)
                       ->getElementOffset(U32(*field_index));
    Count size =
        layout.getTypeAllocSize(llvm::unwrap(*parameter_type)).getFixedValue();
    BAIL_IF(size == 0);

    llvm::GlobalVariable* module_symbol =
        module.getNamedGlobal(llvm_text(projection.get_module_symbol()));
    if (module_symbol == nullptr) {
      module_symbol = new llvm::GlobalVariable(
          module, llvm::Type::getInt8Ty(context), true,
          llvm::GlobalValue::ExternalLinkage, nullptr,
          llvm_text(projection.get_module_symbol()));
    }
    BAIL_IF(module.getNamedGlobal(llvm_text(projection.get_symbol())));

    Core::Static::Vector<llvm::Constant*, 3> values = {{
      module_symbol,
      llvm::ConstantInt::get(&count, offset),
      llvm::ConstantInt::get(&count, size),
    }};
    llvm::Constant& initializer = *llvm::ConstantStruct::get(
        &projection_type,
        llvm::ArrayRef<llvm::Constant*>(values.get_data(), values.get_size()));
    auto* emitted = new llvm::GlobalVariable(
        module, &projection_type, true, llvm::GlobalValue::ExternalLinkage,
        &initializer, llvm_text(projection.get_symbol()));
    emitted->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  }
  return True;
}
