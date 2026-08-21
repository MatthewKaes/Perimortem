// Perimortem Engine
// Copyright © Matt Kaes

// LLVM must enter before Perimortem so the standard placement declaration is
// visible before the freestanding fallback used by Perimortem headers.
// clang-format off
#include "llvm/IR/IRBuilder.h"
#include "tetrodotoxin/library/llvm/program.hpp"
// clang-format on

#include "perimortem/core/diagnostics/log.hpp"

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
#include "tetrodotoxin/library/llvm/header.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

Llvm::Program::Program(
    Memory::Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text,
    Target target,
    Debug::Level debug_level,
    Unit unit)
    : arena(arena),
      errors(errors),
      source_path(source_path),
      source_text(source_text),
      target(target),
      unit(unit),
      debug(debug_level),
      context(*LLVMContextCreate()),
      module(*LLVMModuleCreateWithNameInContext("tetrodotoxin", &context)),
      exports(arena),
      publications(arena) {}

auto Llvm::Program::get_name() const -> Core::View::Bytes {
  return "Program"_view;
}

auto Llvm::Program::get_documentation() const
    -> const Ttx::Concept::Documentation& {
  return Ttx::Concept::Documentation::get_empty();
}

Llvm::Program::~Program() {
  debug.release();
  target_machine.visit(
      []() {},
      [](Unsigned_8& machine) {
        delete reinterpret_cast<llvm::TargetMachine*>(&machine);
      });
  LLVMDisposeModule(&module);
  LLVMContextDispose(&context);
}

auto Llvm::Program::initialize() -> Bool {
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
            reinterpret_cast<const Unsigned_8*>(error.data()), error.size()));
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

  target_machine = *reinterpret_cast<Unsigned_8*>(machine);
  native_module.setTargetTriple(target_triple);
  native_module.setDataLayout(machine->createDataLayout());
  return debug.initialize(*this, source_path, source_text);
}

auto Llvm::Program::create_process_entry(Core::View::Bytes symbol) -> Bool {
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

auto Llvm::Program::compile() -> Utility::Result<Products, Failure> {
  if (!debug.finalize(*this)) {
    return Failure::ToolchainFailed;
  }

  llvm::Module& native_module = *llvm::unwrap(&module);

  llvm::SmallVector<char, 0> verification;
  llvm::raw_svector_ostream verification_stream(verification);
  if (llvm::verifyModule(native_module, &verification_stream)) {
    fail_backend(
        Core::View::Bytes(
            reinterpret_cast<const Unsigned_8*>(verification.data()),
            verification.size()));
  }

  if (source_failed) {
    return Failure::SourceRejected;
  }

  if (tool_failed) {
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

  auto header = Header::create(
      get_arena(), get_carriers(), get_functions(), get_globals(),
      exports.get_view());
  if (!header) {
    return Failure::ToolchainFailed;
  }

  Core::View::Bytes ir_view(
      reinterpret_cast<const Unsigned_8*>(ir.data()), ir.size());
  Core::View::Bytes object_view(
      reinterpret_cast<const Unsigned_8*>(object.data()), object.size());
  return Products(
      get_arena().proxy(ir_view), get_arena().proxy(object_view),
      header->get_view(), publications.get_view());
}

auto Llvm::Program::resolve_context(Core::View::Bytes) const
    -> const Ttx::Concept::Abstract& {
  return Ttx::Concept::Invalid::get_invalid();
}

auto Llvm::Program::add_publication(Publication value) -> void {
  for (const Publication& existing : publications.get_view()) {
    if (&existing.get_semantic() == &value.get_semantic() &&
        existing.get_symbol() == value.get_symbol()) {
      return;
    }
  }
  publications.insert(value);
}

auto Llvm::Program::add_export(Export value) -> void {
  for (const Export& existing : exports.get_view()) {
    if (&existing.get_callable() == &value.get_callable() &&
        existing.get_symbol() == value.get_symbol()) {
      return;
    }
  }
  exports.insert(value);
}

auto Llvm::Program::fail_backend(Core::View::Bytes message) -> Bool {
  Core::Diagnostics::Log::error(message);
  tool_failed = True;
  return False;
}

auto Llvm::Program::fail_source(
    Core::Option<Ttx::Lexical::Anchor> anchor,
    Core::View::Bytes message,
    Core::View::Bytes hint) -> Bool {
  Ttx::Lexical::Errors::Report report(
      errors, source_path, source_text,
      anchor ? *anchor : Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()));
  report << message;
  report.get_hint() << hint;
  source_failed = True;
  return False;
}
