# libinterpret_hse

A library for converting between various representations of Handshaking Expansions (HSE) and their graph models.

## Overview

The `libinterpret_hse` library provides parsers and interpreters for translating between different representations of asynchronous circuit specifications. It serves as a core component of the Loom compiler, enabling the transformation between:

- **ASTG** (Asynchronous Signal Transition Graph): Formal models representing signal transitions in asynchronous circuits
- **DOT**: GraphViz format for visualizing circuit graphs
- **CHP** (Communicating Hardware Processes): High-level description language for asynchronous circuits
- **COG**: Language for specifying concurrent operations with regions, parallel composition, and control structures
- **Custom expression formats**: Additional representations for boolean expressions and other circuit elements

## Key Components

### Import Modules

Import modules translate external representations into HSE graph models:

| Module | Description |
|--------|-------------|
| `import_astg` | Parses ASTG specifications into HSE graph structures |
| `import_dot` | Converts GraphViz DOT files into HSE graphs |
| `import_chp` | Transforms CHP process descriptions into corresponding HSE representations |
| `import_cog` | Converts COG specifications with regions, parallel compositions, and control structures |
| `import_expr` | Handles custom expression formats for direct graph construction |

### Export Modules

Export modules translate HSE graph models into external representations:

| Module | Description |
|--------|-------------|
| `export_astg` | Generates ASTG specifications from HSE graphs |
| `export_dot` | Creates GraphViz DOT files for visualization |
| `export_chp` | Produces CHP process descriptions from HSE graphs |
| `export_expr` | Exports to custom expression formats |

## Usage Examples

### Importing COG Specifications

```cpp
// Set up tokenizer with appropriate syntax definitions
tokenizer tokens;
tokens.register_token<parse::block_comment>(false);
tokens.register_token<parse::line_comment>(false);
parse_cog::composition::register_syntax(tokens);

// Insert input source
tokens.insert("string_input", input_string, nullptr);

// Parse input
tokens.increment(false);
tokens.expect<parse_cog::composition>();

if (tokens.decrement(__FILE__, __LINE__)) {
    // Create syntax tree
    parse_cog::composition syntax(tokens);
    
    // Convert to HSE graph
    boolean::cover covered;
    bool hasRepeat = false;
    hse::graph g = hse::import_hse(syntax, covered, hasRepeat, 0, &tokens, true);
    
    // Now work with the resulting HSE graph
    // ...
}
```

### Exporting to DOT Format

```cpp
// Given an HSE graph 'g'
hse::graph g;
// ... populate the graph ...

// Export to DOT format
stringstream ss;
hse::export_dot(g, ss);

// ss now contains the DOT representation that can be visualized with GraphViz
// cout << ss.str() << endl;
```

### Processing CHP Descriptions

```cpp
// Parse CHP input
parse_chp::composition chp_syntax(tokens);

// Convert to HSE graph
hse::graph g = hse::import_hse(chp_syntax, 0, &tokens, true);

// Process the graph
// ...
```

## Building and Testing

The library includes a comprehensive test suite that verifies all import and export functionality:

```bash
# Build and run tests
cd lib/interpret_hse
make test

# Run individual test cases
./test --gtest_filter=CogImport.BasicSequence
```

## Dependencies

- **parse_hse**: Parser for HSE syntax
  - parse_boolean: Boolean expression parser
    - parse: Core parsing utilities
      - common: Common utilities
- **hse**: HSE graph representation
  - boolean: Boolean algebra utilities
    - common: Common utilities
- **interpret_boolean**: Boolean expression interpreter
  - parse_boolean: Boolean expression parser
    - parse: Core parsing utilities
      - common: Common utilities
  - boolean: Boolean algebra utilities
    - common: Common utilities
- **parse_cog**: Parser for COG language
- **parse_chp**: Parser for CHP language
- **parse_astg**: Parser for ASTG format
- **parse_dot**: Parser for DOT format
- **parse_expression**: Parser for expression formats

## License

Licensed by Cornell University under GNU GPL v3.

Written by Ned Bingham.
Copyright © 2020 Cornell University.

Haystack is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Haystack is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License may be found in COPYRIGHT.
Otherwise, see <https://www.gnu.org/licenses/>.
