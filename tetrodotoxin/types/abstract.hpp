// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace Tetrodotoxin::Language::Parser::Types {
// So you've made the mistake of reading Tetrodotoxin source and you've ended up
// here. Well now that means my problems are your problems so you get to read my
// diatribe on type systems for type systems.
//
// To start we must recall that TTX is based on the following proof:
//
// 1. Slow = salty
// 2. TTX = salt free
// ∴  TTX is fast.
//
// To avoid contridiction Tetrodotoxin must use a type system that enables both
// execute fast and make the developer fast. A fool’s errand in a world where
// there exists people who believe dynamicly typed systems are fast, but it's
// not everyone's job to be correct, just ours.
//
// Tetrodotoxin sets out to solve the "typedef" problem by pushing as much of
// the type system to as local a context as possible. All tokens are evaluated
// by the lexer independent of any parser feedback and is why the language makes
// a few unusual design choices, such as types and names being case sensitive.
//
// This allows Tetrodotoxin to provide the maximum amount of information
// possible from a single lexer pass in any context.
//
// This enables the parser to focus on progressive resolution without making
// major sacrifices on quality. In essence: more information / context should
// not fundementally change the correctness of any previous partial parses.
//
// This model conflicts heavily with most type systems that require a global
// context to ensure correctness. So instead we abandon 'types' and choose to
// instead represent everything as abstract contexts and treat the type system
// as an emergent property of context stacking.
//
// The benefit of this is the entire "type system" can be represented using only
// itself giving us a unified framework that blends the line between parsing and
// compiling.
//
// The downside of this framework is it's a ton of abstract concepts and state
// to juggle when working on the language, but hey, at least it's not Rust.
//
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠁⡀⣀⣤⡀⡀⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠁⡀⡀⡀⠉⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⣾⣿⣿⣿⣆⡀⠸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⡀⡀⣾⣿⣦⡀⡀⠉⢿⣿⣿⣿⣿⣿⡿⠛⠉⠉⠙⠿⣿⣿⡇⡀⣸⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⡀⡀⣾⣿⣿⣿⣿⣦⡀⡀⠻⣿⣿⣿⡿⡀⢠⣶⣶⣄⡀⠈⢿⡀⡀⣿⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⡀⡀⣾⣿⠋⡀⣿⣿⣿⣿⣄⡀⠘⣿⣿⡇⡀⣿⣿⣿⣿⣿⡄⡀⡀⡀⣿⣿⣿⣿⣿⣿⠅⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⣸⣿⠁⣾⡀⣿⣿⣿⣿⣿⣦⡀⠘⣿⡄⡀⣿⣿⣿⣿⣿⣿⡄⡀⡀⣿⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠁⡀⢠⣿⠁⣾⣿⡀⠟⠋⢁⡀⢘⣿⣧⡀⠹⡇⡀⠸⣿⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⠿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⡀⡀⣿⠃⣼⣿⣿⣤⣾⣿⠏⣠⣿⣿⣿⡆⡀⢿⡀⡀⣿⣿⣿⣿⣿⣿⣿⡀⠸⣿⣿⣿⣿⣿⡀⡀⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⠇⡀⡀⡀⡀⡀⡀⡀⠉⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⢠⡿⢀⣿⣿⣿⣿⣿⣯⡀⠙⠿⣿⣿⣿⡀⠈⣧⡀⠈⣿⣿⣿⣿⣿⣿⣷⡀⣿⣿⣿⣿⠟⡀⡀⡀⡀⠙⣿⣿⣿⣿⣿⣿⣿
// ⣿⡀⢸⣿⣿⣿⣿⣶⣶⣤⣀⡀⡀⠈⠛⢿⣿⣿⣿⣿⡀⡀⠈⠁⣈⣉⣉⡉⠉⠙⠛⠿⣿⣶⡤⡀⣿⡆⡀⣿⣦⡀⠈⣿⣿⣿⣿⣿⣿⣷⣿⣿⠟⡀⣠⣾⣿⣿⣦⡀⠈⣿⣿⣿⣿⣿⣿
// ⣿⡀⠘⣿⣿⠿⠟⠿⠿⣿⣿⣿⣷⣄⡀⡀⠈⠋⣀⣤⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣦⣄⡀⠛⢿⡇⡀⢸⣿⣦⡀⠈⠛⠛⠿⣿⣿⣿⡿⠁⢠⣿⣿⣿⣿⣿⣿⡇⡀⢹⣿⣿⣿⣿⣿
// ⣿⡇⡀⣿⣿⣦⣀⠑⠶⣤⣀⠙⠿⣿⠟⡀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣄⠙⣿⣿⣿⣿⣿⣿⣿⣷⣤⡀⡀⡀⠙⠋⡀⣠⣤⣤⣀⡀⠈⠻⡀⣰⣿⣿⣿⣿⣿⣿⣿⣿⡀⢸⣿⣿⣿⣿⣿
// ⣿⣧⡀⣿⣿⣿⣿⡿⠂⣸⣿⣿⡶⡀⣴⣿⣿⠃⣼⣿⣿⣿⣿⣿⣿⣿⣿⣦⠈⢿⣿⣿⣿⣿⣿⡇⢸⣿⣦⡀⡀⣴⣿⣿⣿⣿⣿⣿⣄⡀⢠⣿⣿⣿⣿⣿⣿⣿⣿⠃⡀⣾⣿⣿⣿⣿⣿
// ⣿⣿⡀⠸⣿⣿⠋⣠⣿⣿⣿⠟⢀⣿⣿⣿⣿⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⡀⠙⠿⠿⠛⠛⠃⠸⢿⣿⡇⢰⣿⣿⣿⣿⣿⣿⣿⣿⡀⢸⣿⣿⣿⣿⣿⣿⣿⠋⡀⡀⡀⠻⣿⣿⣿⣿
// ⣿⣿⡄⡀⣿⣧⣀⣤⡀⣸⠏⣠⣿⣿⣿⣿⡿⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⡀⣠⣶⡀⡀⡀⡀⢳⣦⡈⢳⠈⣿⣿⣿⣿⣿⣿⣿⣿⡀⠘⣿⣿⣿⣿⣿⠟⠁⣼⣿⣿⡀⡀⢿⣿⣿⣿
// ⣿⣿⣷⡀⠘⣿⣿⡟⢠⡟⢠⣿⣿⣿⣿⣿⣷⢸⣿⣿⣿⣿⡀⢿⣿⣿⠟⢀⣾⣿⣿⣿⡀⡀⡀⡀⡀⢻⣿⡀⣄⠈⠿⣿⣿⣿⡿⠛⡀⣠⣆⡀⠉⠉⢁⣠⣶⣿⣿⣿⣿⡇⡀⢸⣿⣿⣿
// ⣿⣿⣿⡆⡀⠹⣿⣇⣀⡀⣿⣿⣿⠃⠘⣿⣿⡀⣿⣿⣿⣿⡀⣀⠛⠃⣴⣿⣿⣿⣿⣿⡀⡀⡀⡀⡀⡀⣿⠇⡈⣶⣄⢀⣀⣀⣠⣴⣿⣿⣿⠁⣾⣿⣿⣿⣿⣿⣿⣿⣿⠁⡀⣾⣿⣿⣿
// ⣿⣿⣿⣿⡀⡀⠻⣿⡇⢰⣿⣿⣿⡀⣧⠈⠻⠂⠙⠉⠛⠃⣼⣿⡟⢰⣿⣿⣿⣿⣿⣿⣧⡀⡀⡀⡀⡀⠸⢀⣿⣦⠙⡀⣿⣿⣿⣿⣿⣿⣿⡀⢻⣿⣿⣿⣿⣿⣿⠟⠁⡀⣴⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⠹⡇⢸⣿⣿⡟⠘⢁⣤⡀⡀⡀⡀⠻⣷⣄⠙⣇⢸⣿⣿⣿⣿⣿⣿⣿⣦⡀⡀⡀⡀⣠⣿⣿⣿⠁⡀⠘⣿⣿⣿⣿⣿⣿⣷⡀⠈⠛⠛⠛⠉⡀⡀⣤⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⡀⡀⡀⢸⣿⠟⢀⣾⣿⣿⡄⡀⡀⡀⡀⠹⣿⡆⢻⡄⠛⣿⣿⣿⣿⣿⣿⣿⠿⡀⣠⣾⣿⣿⣿⣿⡀⣄⡀⡀⠙⠻⠿⣿⣿⡿⠟⠁⢀⣴⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡟⡀⡀⡀⠸⠃⣴⣿⣿⣿⣿⣷⡀⡀⡀⡀⡀⢻⡇⣸⣿⣷⣤⣀⣈⣉⣀⣠⣤⣾⠿⣿⣿⣿⣿⣿⡇⢸⣿⣦⡀⡀⡀⡀⡀⡀⡀⡀⡀⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⠃⡀⢰⡄⡀⣸⣿⣿⣿⣿⣿⣿⣦⡀⡀⡀⡀⠘⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⠘⣿⣿⣿⣿⠁⣼⣿⣿⣿⣷⣦⣄⣠⣤⡾⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⣼⡇⡀⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⡀⡀⡀⣾⣿⣿⣿⣿⠿⢿⣿⣿⠿⠿⠛⠁⣼⣿⣿⣿⠃⡀⣿⣿⣿⣿⣿⣿⣿⣿⡿⡀⡀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⣿⡇⡀⢻⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⣠⣿⣿⣿⣿⣿⡀⣶⣶⣶⣶⣶⣶⣿⣿⣿⣿⡿⠁⡀⡀⣿⣿⣿⣿⣿⣿⣿⡿⡀⡀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⣿⣿⡀⡀⠙⠿⢿⣿⠿⠿⠛⢁⣴⣿⡀⣿⣿⣿⣿⠁⣾⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⡀⣴⡇⢰⣿⣿⣿⣿⣿⣿⡿⡀⡀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⣦⣀⡀⡀⡀⡀⠺⣿⣿⣿⣷⠈⢿⠿⠋⢀⣾⣿⣿⣿⣿⣿⣿⠿⠋⡀⡀⣠⣿⣿⠁⣾⣿⣿⣿⣿⣿⣿⡀⡀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⣿⡿⢿⣿⣿⣤⡀⡀⠉⠛⠿⢷⣤⣶⣿⣿⣿⠿⠿⠛⠉⡀⡀⡀⣀⣴⣿⣿⣿⠃⢰⣿⣿⣿⣿⣿⣿⠁⡀⣰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⣿⣿⣿⠟⡀⡀⣿⣿⣿⣿⣶⣤⡀⡀⡀⡀⣀⣀⡀⡀⡀⡀⣀⣀⣤⣶⣿⣿⣿⣿⡿⠁⣶⠘⣿⣿⣿⣿⣿⠁⡀⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡀⡀⣿⠟⡀⡀⡄⡀⣿⣿⣿⣿⣿⠋⡀⣿⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣄⠹⣿⣿⠟⡀⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⡆⡀⡀⡀⣠⣿⣷⡀⠘⣿⣿⡿⠁⡀⡀⣿⡀⡀⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣤⣤⡶⡀⡀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⡀⣤⣿⣿⣿⣿⡀⡀⠘⠉⡀⢀⡆⡀⠘⠁⢀⡀⣄⡈⠛⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠇⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⡀⣴⣿⡿⡀⡀⣠⣿⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⣿⣿⠋⡀⢀⣾⣿⡏⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⡀⡀⣴⣿⣿⣿⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⡀⡀⣴⣿⣿⣿⣿⠇⣰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
// ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠁⡀⣠⣿⣿⣿⣿⣿⡿⡀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡀⡀⡀⠸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
//
//                             Good luck with debugging!
//
class Abstract {
 public:
  // The intended usage of the abstract.
  enum class Usage {
    Constant,    // [=/=]
    Dynamic,     // [=>>]
    Hidden,      // [=!=]
    Temporary,   // [***]
    Transitory,  // [???]
  };

  virtual ~Abstract() = 0;
  constexpr virtual auto get_name() const -> std::string_view = 0;
  constexpr virtual auto get_doc() const -> std::string_view = 0;
  constexpr virtual auto get_uuid() const -> uint32_t = 0;
  constexpr virtual auto get_usage() const -> Usage = 0;
  virtual auto get_size() const -> uint32_t = 0;

  virtual auto resolve() const -> const Abstract* { return this; };
  virtual auto resolve_context(std::string_view name) const -> const Abstract* {
    return nullptr;
  };
  virtual auto resolve_scope(std::string_view name) const -> const Abstract* {
    return nullptr;
  };
  virtual auto resolve_host() const -> const Abstract* { return nullptr; };
  virtual auto expand_context(
      const std::function<void(const Abstract* const)>& fn) const -> void {};
  virtual auto expand_scope(
      const std::function<void(const Abstract* const)>& fn) const -> void {};

  // Used for optimizations
  template <typename T>
  auto is() -> bool {
    return get_uuid() == T::uuid;
  }

  template <typename T>
  auto cast() -> T* {
    if (!is<T>())
      return nullptr;

    return static_cast<T*>(this);
  }
};

// Pure abstract destructor implmentation.
inline Abstract::~Abstract() {}

}  // namespace Tetrodotoxin::Language::Parser::Types