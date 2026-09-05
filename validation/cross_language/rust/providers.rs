// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// Composition lists providers here so their implementations and the ABI driver
// can be extended independently.
#[path = "second.rs"]
mod second;

#[path = "temporary.rs"]
mod temporary;
