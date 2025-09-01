# ScopedTimer Utility

## Overview
`ScopedTimer` is a lightweight, header‑only C++ utility for quick performance timing.  
It uses the RAII pattern: the timer starts when the object is constructed and automatically logs the elapsed time when it goes out of scope.

This makes it ideal for measuring:
- The runtime of a **single function call**.
- The total time spent in a **code block**.
- Any scope where you want timing without manual start/stop calls.

## Features
- **RAII‑based** — no need to remember to stop the timer.
- **Header‑only** — just include and use.
- **Flexible label handling** — accepts string literals (zero‑copy) or `std::string`/`std::string_view` (safe copy).
- **Selectable output stream** — defaults to `std::cerr`, but can be redirected to any `std::ostream`.
- **Two macros** for convenience:
  - `PROFILE_CALL(label, expr [, ostream])` — time a specific function call or expression.
  - `PROFILE_SCOPE(label [, ostream])` — time an entire block or scope.

## Usage

### 1. Include the header
```cpp
#include "util/scoped_timer.h"
```

### 2. Time a single call (default to `std::cerr`)
```cpp
PROFILE_CALL("expensiveOperation", expensiveOperation());
```
**Example output:**
```
[Telemetry] expensiveOperation took 12.347 ms
```

### 3. Time a single call with custom output stream
```cpp
PROFILE_CALL("expensiveOperation", expensiveOperation(), std::cout);
PROFILE_CALL("expensiveOperation", expensiveOperation(), logFile);
```

### 4. Time a block of code (default to `std::cerr`)
```cpp
{
    PROFILE_SCOPE("Full update sequence");

    loadData();
    processData();
    renderOutput();
} // Timer logs here
```

### 5. Time a block with custom output stream
```cpp
{
    PROFILE_SCOPE("Full update sequence", logFile);

    loadData();
    processData();
    renderOutput();
}
```

## Label Parameter Details
The updated `ScopedTimer` supports:
- **String literals / `const char*`** — zero‑copy.
- **`std::string` or substrings** — safely copied into owned storage.
- **`std::string_view`** — accepted directly, with safe lifetime handling.

Examples:
```cpp
ScopedTimer t1("Literal label");           // zero‑copy, default to std::cerr
std::string s = "Dynamic label";
ScopedTimer t2(s);                         // safe copy
ScopedTimer t3(s.substr(0, 6));            // safe copy
ScopedTimer t4(std::string("Temporary"));  // safe copy
```

## Notes
- **Label**: Used in the log output to identify the timed section.
- **Output**: Printed to the selected `std::ostream` in milliseconds with three decimal places.
- **Default stream**: `std::cerr` — keeps timing output separate from normal program output.
- **Overhead**: Negligible for typical profiling use; suitable for development and debugging.
- **Thread safety**: Each timer instance is independent; safe to use in multiple threads.

## When to Use
- Profiling hot paths during development.
- Comparing performance of alternative implementations.
- Gathering quick telemetry without setting up a full profiler.

## License

See the LICENSE file in the repository root for full terms.  
All source files include SPDX identifiers for license clarity and compliance.
