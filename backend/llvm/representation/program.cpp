// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// LLVM must enter before Perimortem so the standard placement declaration is
// visible before the freestanding fallback used by Perimortem headers.
#if defined(__cplusplus)
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#endif
#include "perimortem/core/diagnostics/log.hpp"

#include "backend/llvm/abi/header.hpp"
#include "backend/llvm/representation/program.hpp"
#include "llvm-c/Core.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Backend;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto write_abi_value(
    llvm::raw_ostream& output,
    const llvm::Module& module,
    Core::View::Bytes symbol) -> void {
  output << llvm_text(symbol) << '|';
  const llvm::GlobalValue* value = module.getNamedValue(llvm_text(symbol));
  if (const auto* function = llvm::dyn_cast_or_null<llvm::Function>(value)) {
    output << "function|" << U64(function->getCallingConv()) << '|';
    function->getFunctionType()->print(output);
    output << '|';
    function->getAttributes().print(output);
  } else if (
      const auto* global =
          llvm::dyn_cast_or_null<llvm::GlobalVariable>(value)) {
    output << "state|" << (global->isConstant() ? "readonly|" : "writable|");
    global->getValueType()->print(output);
  } else {
    output << "missing";
  }
  output << '\n';
}

static auto create_abi_fingerprint(
    const llvm::Module& module,
    Llvm::Target target,
    Core::View::Bytes header,
    Core::View::Vector<Llvm::Abi::Export> exports,
    Core::View::Vector<Llvm::Abi::Publication> publications,
    Core::View::Vector<Tetrodotoxin::Linker::Import> imports)
    -> Tetrodotoxin::Linker::Fingerprint {
  llvm::SmallVector<char, 0> description;
  llvm::raw_svector_ostream output(description);
  output << llvm_text(Llvm::get_name(target)) << '\n'
         << module.getDataLayoutStr() << '\n'
         << llvm_text(header) << '\n';
  for (const Llvm::Abi::Export& exported : exports) {
    write_abi_value(output, module, exported.get_symbol());
  }
  for (const Llvm::Abi::Publication& publication : publications) {
    write_abi_value(output, module, publication.get_symbol());
  }
  for (const Tetrodotoxin::Linker::Import& import : imports) {
    output << U64(import.get_kind()) << '|' << llvm_text(import.get_abi())
           << '|';
    write_abi_value(output, module, import.get_symbol());
  }
  return Tetrodotoxin::Linker::Fingerprint::create(
      Core::View::Bytes(
          reinterpret_cast<const U8*>(description.data()), description.size()));
}

Llvm::Representation::Program::Program(
    Memory::Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text,
    Target target,
    Debug::Level debug_level,
    Llvm::Abi::Unit unit)
    : Emission(Emission::Kind::Module),
      arena(arena),
      errors(errors),
      source_path(source_path),
      source_text(source_text),
      target(target),
      unit(unit),
      debug(debug_level),
      context(*LLVMContextCreate()),
      module(*LLVMModuleCreateWithNameInContext("tetrodotoxin", &context)),
      exports(arena),
      publications(arena),
      imports(arena) {}

Llvm::Representation::Program::~Program() {
  debug.release();
  target_machine.visit(
      []() {},
      [](U8& machine) {
        delete reinterpret_cast<llvm::TargetMachine*>(&machine);
      });
  LLVMDisposeModule(&module);
  LLVMContextDispose(&context);
}

auto Llvm::Representation::Program::initialize() -> Bool {
  if (target != Llvm::Target::X86_64SysV) {
    return fail_backend(
        "The LLVM request selected an unsupported target."_view);
  }

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  constexpr llvm::StringLiteral triple_name("x86_64-pc-linux-gnu");
  std::string error;
  llvm::Triple target_triple(triple_name);
  llvm::Module& native_module = *llvm::unwrap(&module);
  const llvm::Target* selected =
      llvm::TargetRegistry::lookupTarget(target_triple, error);
  if (!selected) {
    return fail_backend(
        Core::View::Bytes(
            reinterpret_cast<const U8*>(error.data()), error.size()));
  }

  llvm::TargetOptions options;
  options.UseInitArray = true;
  llvm::TargetMachine* machine = selected->createTargetMachine(
      target_triple, "x86-64", "", options, llvm::Reloc::PIC_,
      llvm::CodeModel::Small, llvm::CodeGenOptLevel::None);
  if (!machine) {
    return fail_backend(
        "LLVM could not create the selected target machine."_view);
  }

  target_machine = *reinterpret_cast<U8*>(machine);
  native_module.setTargetTriple(target_triple);
  native_module.setDataLayout(machine->createDataLayout());
  return debug.initialize(*this, source_path, source_text);
}

auto Llvm::Representation::Program::create_process_entry(
    Core::View::Bytes symbol) -> Bool {
  if (symbol.is_empty()) {
    return fail_backend(
        "The App entry requires one nonempty native symbol."_view);
  }

  llvm::Module& native_module = *llvm::unwrap(&module);
  if (native_module.getFunction("main") ||
      native_module.getFunction(llvm_text(symbol))) {
    return fail_backend(
        "The App entry collides with an existing native symbol."_view);
  }

  llvm::LLVMContext& native_context = *llvm::unwrap(&context);
  llvm::FunctionType& entry_type = *llvm::FunctionType::get(
      llvm::Type::getVoidTy(native_context), {}, false);
  llvm::Function& entry = *llvm::Function::Create(
      &entry_type, llvm::GlobalValue::ExternalLinkage, llvm_text(symbol),
      native_module);
  llvm::FunctionType& main_type = *llvm::FunctionType::get(
      llvm::Type::getInt32Ty(native_context), {}, false);
  llvm::Function& main = *llvm::Function::Create(
      &main_type, llvm::GlobalValue::ExternalLinkage, "main", native_module);
  llvm::BasicBlock& block =
      *llvm::BasicBlock::Create(native_context, "entry", &main);
  llvm::IRBuilder<> builder(&block);
  builder.CreateCall(&entry_type, &entry);
  builder.CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(native_context), 0));
  return True;
}

auto Llvm::Representation::Program::compile()
    -> Utility::Result<Products, Failure> {
  if (!debug.finalize(*this)) {
    return Failure::ToolchainFailed;
  }

  llvm::Module& native_module = *llvm::unwrap(&module);

  llvm::SmallVector<char, 0> verification;
  llvm::raw_svector_ostream verification_stream(verification);
  if (llvm::verifyModule(native_module, &verification_stream)) {
    fail_backend(
        Core::View::Bytes(
            reinterpret_cast<const U8*>(verification.data()),
            verification.size()));
  }

  if (has_source_failure()) {
    return Failure::SourceRejected;
  }

  if (has_tool_failure()) {
    return Failure::ToolchainFailed;
  }

  llvm::SmallVector<char, 0> ir;
  llvm::raw_svector_ostream ir_stream(ir);
  native_module.print(ir_stream, nullptr);

  llvm::SmallVector<char, 0> object;
  if (!target_machine) {
    fail_backend("LLVM object emission requires one target machine."_view);
    return Failure::ToolchainFailed;
  }

  llvm::TargetMachine& native_machine =
      *reinterpret_cast<llvm::TargetMachine*>(&*target_machine);
  llvm::raw_svector_ostream object_stream(object);
  llvm::legacy::PassManager passes;
  if (native_machine.addPassesToEmitFile(
          passes, object_stream, nullptr, llvm::CodeGenFileType::ObjectFile)) {
    fail_backend("LLVM cannot emit an object for the selected target."_view);
    return Failure::ToolchainFailed;
  }

  passes.run(native_module);

  auto header = Llvm::Abi::Header::create(
      get_arena(), get_carriers(), get_functions(), get_globals(), get_unit(),
      exports.get_view());
  if (!header) {
    return Failure::ToolchainFailed;
  }

  Tetrodotoxin::Linker::Fingerprint abi_fingerprint = create_abi_fingerprint(
      native_module, target, header->get_view(), exports.get_view(),
      publications.get_view(), imports.get_view());
  Core::View::Bytes header_view = header->get_view();
  if (!unit.is_package_member() && !unit.get_package().is_empty()) {
    auto identified = Llvm::Abi::Header::identify(
        get_arena(), header_view, unit.get_package(), abi_fingerprint);
    if (!identified) {
      return Failure::ToolchainFailed;
    }
    header_view = identified->get_view();
  }

  Core::View::Bytes ir_view(reinterpret_cast<const U8*>(ir.data()), ir.size());
  Core::View::Bytes object_view(
      reinterpret_cast<const U8*>(object.data()), object.size());
  return Products(
      get_arena().proxy(ir_view), get_arena().proxy(object_view), header_view,
      publications.get_view(), abi_fingerprint, imports.get_view());
}

auto Llvm::Representation::Program::add_publication(
    Llvm::Abi::Publication value) -> void {
  for (const Llvm::Abi::Publication& existing : publications.get_view()) {
    if (&existing.get_semantic() == &value.get_semantic() &&
        existing.get_symbol() == value.get_symbol()) {
      return;
    }
  }
  publications.insert(value);
}

auto Llvm::Representation::Program::add_import(
    Tetrodotoxin::Linker::Import value) -> Bool {
  for (const Tetrodotoxin::Linker::Import& existing : imports.get_view()) {
    if (existing.get_symbol() != value.get_symbol()) {
      continue;
    }
    if (existing == value) {
      return True;
    }
    return fail_backend(
        "One native import symbol carries conflicting ABI declarations."_view);
  }
  imports.insert(value);
  return True;
}

auto Llvm::Representation::Program::add_export(Llvm::Abi::Export value)
    -> void {
  for (const Llvm::Abi::Export& existing : exports.get_view()) {
    if (&existing.get_callable() == &value.get_callable() &&
        existing.get_symbol() == value.get_symbol()) {
      return;
    }
  }
  exports.insert(value);
}

auto Llvm::Representation::Program::fail_backend(Core::View::Bytes message)
    -> Bool {
  Core::Diagnostics::Log::error(message);
  failures |= U8(FailureFlag::Tool);
  return False;
}

auto Llvm::Representation::Program::fail_source(
    Core::Option<Ttx::Lexical::Anchor> anchor,
    Core::View::Bytes message,
    Core::View::Bytes hint) -> Bool {
  Ttx::Lexical::Errors::Report report(
      errors, source_path, source_text,
      anchor ? *anchor : Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()));
  report << message;
  report.get_hint() << hint;
  failures |= U8(FailureFlag::Source);
  return False;
}
