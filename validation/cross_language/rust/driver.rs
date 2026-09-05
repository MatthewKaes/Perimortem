// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

mod echo;
mod providers;
mod ttx;

#[no_mangle]
pub extern "C" fn rust_test_exports(bytes_terminal: echo::BytesTerminal) -> echo::Exports {
    echo::create(bytes_terminal)
}
