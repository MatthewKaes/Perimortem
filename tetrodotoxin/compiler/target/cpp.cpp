// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/target/cpp.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/member.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Compiler;

auto get_type_name(const Ttx::Type& type) -> View::Bytes {
  const Ttx::Attribute* cpp = type.resolve_attribute("cpp"_view);
  return cpp == nullptr ? "<unsupported>"_view : cpp->get_value();
}

auto Target::Cpp::build_header(const Execution::Program& program)
    -> Dynamic::Bytes {
  Dynamic::Bytes header;
  Stream::Textual<Dynamic::Bytes> output(header);
  output << "#pragma once\n\n"
            "#include \"perimortem/core/perimortem.hpp\"\n"
            "#include \"perimortem/core/view/bytes.hpp\"\n\n"
            "namespace Ttx {\n\n"_view;

  View::Vector<Execution::Function> functions = program.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    const Execution::Function& function = functions[i];
    const Ttx::Function& signature = function.get_signature();
    View::Vector<Ttx::Member> parameters =
        signature.get_parameters().get_members();
    View::Vector<Ttx::Member> results = signature.get_result().get_members();

    View::Bytes return_type = "void"_view;
    Dynamic::Bytes result_name;
    if (results.get_size() > 1) {
      result_name.concat(function.get_symbol());
      result_name.concat("_result"_view);
      return_type = result_name;

      output << "struct "_view << result_name << " {\n"_view;
      for (Count j = 0; j < results.get_size(); j++) {
        output << "  "_view << get_type_name(results[j].get_type()) << " "_view;
        if (results[j].get_name().is_empty()) {
          output << "value"_view << j;
        } else {
          output << results[j].get_name();
        }

        output << ";\n"_view;
      }

      output << "};\n\n"_view;
    } else if (
        results.get_size() == 1 &&
        !results[0].get_type().attribute_equals("abi"_view, "void"_view)) {
      return_type = get_type_name(results[0].get_type());
    }

    output << "extern \"C\" "_view << return_type << " "_view
           << function.get_symbol() << "("_view;
    for (Count j = 0; j < parameters.get_size(); j++) {
      if (j != 0) {
        output << ", "_view;
      }

      output << get_type_name(parameters[j].get_type());
      if (!parameters[j].get_name().is_empty()) {
        output << " "_view << parameters[j].get_name();
      }
    }

    output << ");\n"_view;
  }

  output << "\n}  // namespace Ttx\n"_view;
  return header;
}
