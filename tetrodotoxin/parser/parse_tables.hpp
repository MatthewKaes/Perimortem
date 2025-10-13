// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <string_view>

#include "concepts/sparse_lookup.hpp"

#include "token.hpp"

using namespace Perimortem::Concepts;

namespace Tetrodotoxin::Language::Parser::KeywordTable {
using value_type = Classifier;
constexpr auto sparse_factor = 120;
constexpr auto seed = 5793162292815167211UL;
constexpr TablePair<const char*, value_type> data[] = {
    make_pair("if", Classifier::If),
    make_pair("else", Classifier::Else),
    make_pair("for", Classifier::For),
    make_pair("while", Classifier::While),
    make_pair("return", Classifier::Return),
    make_pair("func", Classifier::FuncDef),
    make_pair("type", Classifier::TypeDef),
    make_pair("this", Classifier::This),
    make_pair("from", Classifier::From),
    make_pair("true", Classifier::True),
    make_pair("false", Classifier::False),
    make_pair("new", Classifier::New),
    make_pair("init", Classifier::Init),
    make_pair("package", Classifier::Package),
    make_pair("requires", Classifier::Requires),
    make_pair("debug", Classifier::Debug),
    make_pair("error", Classifier::Error),
};

using lookup =
    SparseLookupTable<value_type, array_size(data), data, sparse_factor, seed>;

static_assert(lookup::has_perfect_hash());
static_assert(sizeof(lookup::sparse_table) <= 512,
              "Ideally keyword sparse table would be less than 512 bytes. "
              "Try to find a smaller size.");
}

// constexpr auto search_result =
//     SeedFinder<255, Classifier, array_size(able), table,
//     sparse_factor, seed, false >::candidate_search();
