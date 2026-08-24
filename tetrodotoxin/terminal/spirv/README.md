# SPIR-V Terminal

Shader finishes with concrete GPU meaning, not target words. The SPIR-V
Terminal is where that meaning becomes a module with selected storage classes,
decorations, identifiers, and instructions.

The producer starts with each Stage required by the selected Render contract,
finds the exact Library Function that satisfies it, and walks that Function's
real Block, Statements, Packs, and Expressions. Render locations become module
decorations, Library Types become physical SPIR-V Types, and Shader Bridges
make the intended CPU and GPU relationship available without equating their
identities.

All physical state lasts for one request. Type, Constant, interface, and value
tables use the original semantic objects as lookup keys, then disappear when
the completed module leaves the Workspace. Nothing is translated back into the
Shader, Render, or Library graph.

The word assembler beneath this folder owns portable binary framing and typed
instruction spellings. It deliberately performs only structural checks. The
complete product is also suitable for an independent SPIR-V validator, while
Vulkan remains the later consumer that owns device resources, commands,
synchronization, and presentation.

When a completed Library operation has no valid mapping for the selected GPU
target, the Terminal reports that authored operation instead of approximating
its behavior. This keeps the supported surface honest while letting the same
Library execution model continue growing across CPU and GPU products.
