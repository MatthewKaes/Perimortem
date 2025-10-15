// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <string_view>

#include "concepts/sparse_lookup.hpp"

#include "token.hpp"

using namespace Perimortem::Concepts;

namespace Tetrodotoxin::Language::Parser::KeywordTable {
using value_type = Classifier;
constexpr auto sparse_factor = 150;
constexpr auto seed = 5793162292815199453UL;
constexpr TablePair<const char*, value_type> data[] = {
    make_pair("if", Classifier::If),
    make_pair("for", Classifier::For),
    make_pair("new", Classifier::New),
    make_pair("via", Classifier::Via),
    make_pair("else", Classifier::Else),
    make_pair("func", Classifier::FuncDef),
    make_pair("init", Classifier::Init),
    make_pair("true", Classifier::True),
    make_pair("class", Classifier::TypeDef),
    make_pair("while", Classifier::While),
    make_pair("false", Classifier::False),
    make_pair("error", Classifier::Error),
    make_pair("debug", Classifier::Debug),
    make_pair("return", Classifier::Return),
    make_pair("on_load", Classifier::OnLoad),
    make_pair("package", Classifier::Package),
    make_pair("warning", Classifier::Warning),
    make_pair("requires", Classifier::Requires),
};

// constexpr auto search_result =
//     SeedFinder<255, Classifier, array_size(data), data,
//     sparse_factor, seed >::candidate_search();

using lookup =
    SparseLookupTable<value_type, array_size(data), data, sparse_factor, seed>;

static_assert(lookup::has_perfect_hash());
static_assert(sizeof(lookup::sparse_table) <= 512,
              "Ideally keyword sparse table would be less than 512 bytes. "
              "Try to find a smaller size.");

}  // namespace Tetrodotoxin::Language::Parser::KeywordTable

