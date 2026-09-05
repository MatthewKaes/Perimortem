// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/source.hpp"

#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "puffer/invocation.hpp"
#include "puffer/publisher.hpp"
#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/build/dialect.hpp"
#include "tetrodotoxin/environment/dialect.hpp"
#include "tetrodotoxin/environment/provider_closure.hpp"
#include "tetrodotoxin/environment/sdk.hpp"
#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"
#include "tetrodotoxin/shader/dialect.hpp"
#include "tetrodotoxin/terminal/query.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/query.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static volatile sig_atomic_t interrupted = 0;

static void interrupt(int) {
  interrupted = 1;
}

class InterruptGuard {
 public:
  InterruptGuard() {
    struct sigaction action = {};
    action.sa_handler = interrupt;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, &previous);
    interrupted = 0;
  }

  ~InterruptGuard() { sigaction(SIGINT, &previous, nullptr); }

 private:
  struct sigaction previous = {};
};

static auto write_error(Core::View::Bytes message) -> void {
  fwrite(message.get_data(), 1, CppSize(message.get_size()), stderr);
  fwrite("\n", 1, 1, stderr);
}

static auto report_errors(const Ttx::Lexical::Errors& errors) -> void {
  Memory::Allocator::Arena arena;
  for (Count index = 0; index < errors.get_size(); index++) {
    Core::View::Bytes message = errors.render_message(arena, index);
    write_error(message);
  }
}

static auto report_error(
    ttx_abstract error,
    Ttx::Lexical::Errors& errors,
    Core::View::Bytes source,
    Core::View::Bytes contents) -> void {
  auto local = Ttx::Concept::Abstract::from_handle(error);
  auto described = local ? local->select<Language::Error>()
                         : Core::Option<const Language::Error&>();
  if (described) {
    Ttx::Lexical::Errors::Report report(
        errors, source, contents,
        Ttx::Lexical::Anchor::create(Ttx::Lexical::Span()));
    described->describe(report);
  } else if (Language::Error::recognizes(error)) {
    const ttx_borrowed_bytes name = error.operations->name(error);
    write_error(
        name.data != nullptr && name.size != 0
            ? Core::View::Bytes(name.data, Count(name.size))
            : "puffer: provider reported an unnamed Error"_view);
  } else {
    write_error("puffer: provider returned an invalid Error"_view);
  }
}

static auto cancel(const std::vector<tetrodotoxin_product_request>& requests)
    -> void {
  for (tetrodotoxin_product_request request : requests) {
    Terminal::cancel(request);
  }
}

class ReobserveWake {
 public:
  ReobserveWake()
      : operations({
          .header =
              {
                .size = sizeof(tetrodotoxin_reobserve_callback_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .changed = changed,
        }) {}

  auto handle() -> tetrodotoxin_reobserve_callback {
    return {
      .operations = &operations,
      .self = reinterpret_cast<tetrodotoxin_reobserve_callback_self*>(this),
    };
  }

  auto was_called() const -> bool {
    return called.load(std::memory_order_acquire);
  }

 private:
  static void TTX_CALL changed(tetrodotoxin_reobserve_callback_self* self) {
    reinterpret_cast<ReobserveWake*>(self)->called.store(
        true, std::memory_order_release);
  }

  tetrodotoxin_reobserve_callback_ops operations;
  std::atomic<bool> called = false;
};

static auto drive(tetrodotoxin_product_request request)
    -> Terminal::ProductObservation {
  Terminal::ProductObservation observation = Terminal::observe(request);
  while (observation.state == Terminal::ProductState::Unknown && !interrupted) {
    ReobserveWake wake;
    const Terminal::ReobservePrepareObservation prepared =
        Terminal::prepare_reobserve(request, wake.handle());
    if (prepared.state == Terminal::ReobservePrepareState::Unavailable) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      observation = Terminal::observe(request);
      continue;
    }
    if (prepared.state != Terminal::ReobservePrepareState::Prepared) {
      return {
        .state = Terminal::ProductState::Failed,
        .products = {},
        .closure = {},
        .error = prepared.error,
      };
    }

    observation = Terminal::observe(request);
    if (observation.state != Terminal::ProductState::Unknown) {
      const Terminal::ReobserveCloseObservation closed =
          Terminal::close_reobserve(prepared.reobserve);
      if (closed.state != Terminal::ReobserveCloseState::Closed) {
        return {
          .state = Terminal::ProductState::Failed,
          .products = {},
          .closure = {},
          .error = closed.error,
        };
      }
      break;
    }

    const Terminal::ReobserveCommitObservation committed =
        Terminal::commit_reobserve(prepared.reobserve);
    if (committed.state == Terminal::ReobserveCommitState::Armed) {
      while (!wake.was_called() && !interrupted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    } else if (committed.state != Terminal::ReobserveCommitState::Changed) {
      Terminal::close_reobserve(prepared.reobserve);
      return {
        .state = Terminal::ProductState::Failed,
        .products = {},
        .closure = {},
        .error = committed.error,
      };
    }

    const Terminal::ReobserveCloseObservation closed =
        Terminal::close_reobserve(prepared.reobserve);
    if (closed.state != Terminal::ReobserveCloseState::Closed) {
      return {
        .state = Terminal::ProductState::Failed,
        .products = {},
        .closure = {},
        .error = closed.error,
      };
    }
    if (!interrupted) {
      observation = Terminal::observe(request);
    }
  }
  return observation;
}

static auto coherent(const std::vector<Terminal::ClosureObservation>& closures)
    -> bool {
  std::vector<Terminal::AuthorityRevisionObservation> observed;
  for (const Terminal::ClosureObservation& closure : closures) {
    if (!closure.valid) {
      return false;
    }
    for (const Terminal::AuthorityRevisionObservation& authority :
         closure.authorities) {
      if (authority.authority.operations->is_current(
              authority.authority.self) == 0) {
        return false;
      }
      for (const Terminal::AuthorityRevisionObservation& prior : observed) {
        if (prior.authority.self == authority.authority.self &&
            prior.revision != authority.revision) {
          return false;
        }
      }
      observed.push_back(authority);
    }
  }
  return true;
}

static auto install_bootstrap(Environment::Toolchain& toolchain) -> Bool {
  auto environment =
      toolchain.install<Environment::Dialect>("Environment"_view);
  auto build = environment ? toolchain.install<Build::Dialect>(
                                 "Build"_view, *environment)
                           : Core::Option<Build::Dialect&>();
  return environment && build;
}

static auto is_package_source(Core::View::Bytes source, Core::View::Bytes path)
    -> Bool {
  Memory::Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena, source, path);
  auto tokens = tokenizer.get_tokens();
  for (Count index = 0; index + 2 < tokens.get_size(); index++) {
    if (tokens.get_data()[index].get_code() ==
            Ttx::Lexical::Code::Type::Dialect &&
        tokens.get_data()[index + 1].get_code() ==
            Ttx::Lexical::Code::Type::Define &&
        tokens.get_data()[index + 2].caculate_text(source) == "Package"_view) {
      return True;
    }
  }
  return False;
}

static auto requirement(const char* package, const char* route, uint8_t digest)
    -> Plugin::RequirementDescriptor {
  Plugin::RequirementDescriptor descriptor = {
    .package_coordinate = package,
    .exported_route = route,
    .contract_version = "1.0",
    .content_sha256 = {},
  };
  descriptor.content_sha256[0] = digest;
  return descriptor;
}

static auto local_dialect(
    tetrodotoxin_dialect_provider provider,
    ttx_abstract cpp_owner_requirement) -> Core::Option<Language::Dialect&> {
  const ttx_abstract candidate = provider.operations->candidate(provider.self);
  const ttx_interface_relation dialect =
      Ttx::relation(candidate, tetrodotoxin_dialect_requirement());
  if ((dialect != TTX_INTERFACE_SATISFIED &&
       dialect != TTX_INTERFACE_EQUIVALENT) ||
      Ttx::relation(candidate, cpp_owner_requirement) !=
          TTX_INTERFACE_SATISFIED) {
    return {};
  }
  Ttx::Abstract* owner =
      Ttx::Abstract::local_owner(candidate, cpp_owner_requirement);
  return owner == nullptr ? Core::Option<Language::Dialect&>()
                          : Core::Option<Language::Dialect&>(
                                static_cast<Language::Dialect&>(*owner));
}

static auto direct_requirements(
    Memory::Allocator::Arena& arena,
    Ttx::Lexical::Errors& errors,
    Core::View::Bytes path,
    Core::View::Bytes source)
    -> Core::Option<Memory::Managed::Vector<Package::RequiredDialect>> {
  Ttx::Lexical::Tokenizer tokenizer(arena, source, path);
  Ttx::Lexical::Associations associations(arena);
  Ttx::Lexical::Cursor cursor(tokenizer, errors, associations);
  Language::Parser::Comment::parse(cursor);
  if (Language::Parser::Dialect::parse(cursor) != "Package"_view) {
    return {};
  }
  Language::Parser::Comment::parse(cursor);
  return Package::Dialect::parse_requirements(cursor);
}

static auto install_package_dialects(
    Environment::Toolchain& toolchain,
    Environment::ProviderClosure& providers,
    const Environment::Sdk& sdk,
    Core::View::Vector<Package::RequiredDialect> requirements) -> Bool {
  const ttx_abstract cpp_owner = Ttx::Abstract::owner_requirement();
  for (const Package::RequiredDialect& required : requirements) {
    const std::string artifact(
        reinterpret_cast<const char*>(required.package.get_data()),
        required.package.get_size());
    auto path = sdk.select(artifact);
    Environment::ProviderAdmissionFailure failure =
        Environment::ProviderAdmissionFailure::InvalidProtocol;
    BAIL_IF(!path || !providers.admit(*path, failure));
    const std::string route(
        reinterpret_cast<const char*>(required.route.get_data()),
        required.route.get_size());
    auto provider = providers.find_dialect(*path, route);
    BAIL_IF(!provider);
    auto dialect = local_dialect(*provider, cpp_owner);
    BAIL_IF(!dialect || !toolchain.install(*provider, *dialect));
  }
  auto installed_library = toolchain.find("Library"_view);
  auto library = installed_library
                     ? installed_library->select<Library::Dialect>()
                     : Core::Option<Library::Dialect&>();
  return library &&
                 toolchain.install<Package::Dialect>("Package"_view, *library)
             ? True
             : False;
}

auto Puffer::Source::run() const -> S32 {
  auto contents = System::File::read(source);
  if (!contents) {
    write_error("puffer: source could not be read"_view);
    return 1;
  }

  const Bool package_source = is_package_source(contents->get_view(), source);
  std::unique_ptr<Environment::ProviderClosure> direct_providers;
  Environment::Toolchain toolchain;
  if (!install_bootstrap(toolchain)) {
    write_error("puffer: built-in Dialect installation failed"_view);
    return 1;
  }

  Memory::Allocator::Arena preflight_arena;
  Ttx::Lexical::Errors preflight_errors;
  if (package_source) {
    auto required = direct_requirements(
        preflight_arena, preflight_errors, source, contents->get_view());
    const std::string sdk(
        reinterpret_cast<const char*>(sdk_root.get_data()),
        sdk_root.get_size());
    auto manifest = Environment::Sdk::open(sdk);
    direct_providers = std::make_unique<Environment::ProviderClosure>(
        std::vector<Environment::InstalledRequirement>{
          {
            .descriptor = requirement("Tetrodotoxin.Cpp", "Owner", 0x43),
            .identity = Ttx::Abstract::owner_requirement(),
          },
        });
    if (!required || !manifest ||
        !install_package_dialects(
            toolchain, *direct_providers, *manifest, required->get_view())) {
      report_errors(preflight_errors);
      write_error(
          "puffer: Package requirements could not be satisfied by the installed SDK"_view);
      return 1;
    }
  }

  Memory::Allocator::Arena products;
  Ttx::Lexical::Errors errors;
  Invocation invocation(terminal_root, sdk_root, arguments);
  Environment::Workspace workspace(toolchain, {}, &repository, &invocation);
  const Language::Monograph* monograph = nullptr;
  Bool completed = False;
  if (package_source) {
    System::Path source_path(source);
    Core::View::Bytes root = source_path.get_directory();
    if (root.is_empty()) {
      root = "."_view;
    }
    Core::View::Bytes route = source_path.get_file();
    auto imported = workspace.import_package(errors, root, source, route);
    if (imported) {
      monograph = &*imported;
      completed = True;
    } else {
      auto retained = workspace.get_monograph(root, route);
      if (retained) {
        monograph = &*retained;
      }
    }
  } else {
    auto interpreted = workspace.interpret_source(
        errors, source, source, contents->get_view());
    if (interpreted) {
      monograph = &*interpreted;
      completed = True;
    } else {
      auto retained = workspace.get_monograph(source);
      if (retained) {
        monograph = &*retained;
      }
    }
  }

  if (monograph == nullptr) {
    report_errors(errors);
    return 1;
  }
  if (!completed) {
    report_errors(errors);
    return 1;
  }
  const ttx_context context = ttx_context_create();
  if (context.operations == nullptr) {
    write_error("puffer: product Context could not be created"_view);
    return 1;
  }
  const Language::ProductionObservation produced =
      workspace.produce(monograph->get_handle(), context, products);
  if (produced.state == Language::ProductionState::Failed) {
    report_error(produced.error, errors, source, contents->get_view());
    context.operations->release(context);
    report_errors(errors);
    return 1;
  }
  if (produced.state != Language::ProductionState::Planned) {
    context.operations->release(context);
    write_error(
        produced.state == Language::ProductionState::Unknown
            ? "puffer: selected product is still indeterminate"_view
        : produced.state == Language::ProductionState::Invalid
            ? "puffer: selected product provider violated its result "
              "protocol"_view
            : "puffer: selected Dialect has no default product"_view);
    return 1;
  }

  Language::ProductionPlanObservation plan =
      Language::observe(produced.production);
  if (!plan.valid) {
    if (plan.error.operations != nullptr) {
      report_error(plan.error, errors, source, contents->get_view());
    } else {
      write_error("puffer: source returned an invalid production plan"_view);
    }
    produced.production.operations->release(produced.production.self);
    context.operations->release(context);
    report_errors(errors);
    return 1;
  }

  InterruptGuard interrupt_guard;
  std::vector<ttx_pack> product_packs;
  std::vector<Terminal::ClosureObservation> closures;
  product_packs.reserve(plan.requests.size());
  closures.reserve(plan.requests.size());
  for (tetrodotoxin_product_request request : plan.requests) {
    Terminal::ProductObservation observation = drive(request);
    if (interrupted) {
      cancel(plan.requests);
      produced.production.operations->release(produced.production.self);
      context.operations->release(context);
      write_error("puffer: production cancelled"_view);
      return 130;
    }
    if (observation.state == Terminal::ProductState::Failed) {
      cancel(plan.requests);
      report_error(observation.error, errors, source, contents->get_view());
      produced.production.operations->release(produced.production.self);
      context.operations->release(context);
      report_errors(errors);
      return 1;
    }
    if (observation.state != Terminal::ProductState::Produced) {
      cancel(plan.requests);
      produced.production.operations->release(produced.production.self);
      context.operations->release(context);
      write_error(
          observation.state == Terminal::ProductState::None
              ? "puffer: selected Terminal declined its assigned product"_view
              : "puffer: selected Terminal violated its request protocol"_view);
      return 1;
    }
    product_packs.push_back(observation.products);
    closures.push_back(Terminal::observe_closure(observation.closure));
  }

  if (!coherent(closures)) {
    cancel(plan.requests);
    produced.production.operations->release(produced.production.self);
    context.operations->release(context);
    write_error(
        "puffer: product closures did not describe one coherent observation"_view);
    return 1;
  }

  const Core::View::Bytes publication_root(
      plan.publication_root.data(), plan.publication_root.size());
  const Bool published =
      Publisher(terminal_root, publication_root)
          .publish(
              Core::View::Vector<ttx_pack>(
                  product_packs.data(), product_packs.size()));
  produced.production.operations->release(produced.production.self);
  context.operations->release(context);
  if (!published) {
    write_error("puffer: product publication failed"_view);
    return 1;
  }
  return 0;
}
