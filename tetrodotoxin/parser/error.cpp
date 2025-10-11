// Perimortem Engine
// Copyright © Matt Kaes

#include "error.hpp"

#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

using namespace Tetrodotoxin::Language::Parser;

bool Error::colorful = true;

constexpr const char* clear_color = "\x1b[0m";
std::string Error::error_color_primary = "\x1b[38;2;227;62;60m";
std::string Error::error_color_secondary = "\x1b[38;2;222;122;101m";
std::string Error::error_color_tertiary = "\x1b[38;2;245;147;85m";
std::string Error::error_color_highlight = "\x1b[38;2;255;201;107m";

Error::Error(std::string_view source_map,
             std::string_view details_,
             const ByteView& source,
             std::optional<Location> loc,
             std::optional<std::string_view> line_range,
             std::optional<uint32_t> error_range) {
  std::stringstream details;
  const char* err_primary = Error::colorful ? error_color_primary.c_str() : "";
  const char* err_secondary =
      Error::colorful ? error_color_secondary.c_str() : "";
  const char* err_tertiary =
      Error::colorful ? error_color_tertiary.c_str() : "";
  const char* err_highlight =
      Error::colorful ? error_color_highlight.c_str() : "";

  details << err_primary << "\x1b[1m[ERROR] " << err_secondary
          << "\x1b[3m\x1b[38;2;143;28;0m" << source_map << ":";
  if (loc) {
    details << loc->line << ":" << loc->column << ":";
  }

  details << std::endl << clear_color;
  details << err_secondary << "\x1b[1m" << details_ << std::endl;
  if (loc) {
    std::stringstream soruce_stream;
    soruce_stream << (line_range ? *line_range : "");

    std::string line;
    bool had_lines = false;
    while (std::getline(soruce_stream, line)) {
      details << err_highlight << std::setw(5) << std::right << loc->line++
              << err_tertiary << " | " << line << std::endl;
      had_lines = true;
    }

    if (!had_lines) {
      details << err_highlight << std::setw(5) << std::right << loc->line++
              << err_tertiary << " | " << std::endl;
    }

    details << "      | " << err_highlight << std::setw(loc->column)
            << std::right << "^";
    if (error_range) {
      details << std::setfill('-') << std::setw(*error_range) << ""
              << std::endl;
    }
  }

  details << clear_color;
  msg = details.str();
}
