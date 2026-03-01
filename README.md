# multipart

Multipart/form-data parser for file uploads and mixed fields.

`multipart` provides a deterministic, minimal parser for HTTP
`multipart/form-data` request bodies:

- Parse parts using a boundary string
- Extract form fields and file uploads
- Case-insensitive header storage
- Access `name` and optional `filename`
- Preserve raw body bytes per part
- Configurable safety limits

Header-only. Zero external dependencies.

## Download

https://vixcpp.com/registry/pkg/gaspardkirira/multipart

## Why multipart?

Handling `multipart/form-data` manually often leads to:

- Incorrect boundary parsing
- Broken header parsing
- Mishandled CRLF vs LF line endings
- Unsafe file upload handling
- Missing limits on part size
- Inconsistent handling of repeated fields

This library provides:

- Deterministic boundary-based parsing
- Clean header extraction
- Explicit `name` and `filename` parsing
- Raw byte body access
- Configurable safety limits
- No hidden allocations beyond internal containers

No HTTP server.
No streaming layer.
No framework dependency.

Just explicit multipart primitives.

## Installation

### Using Vix Registry

```bash
vix add gaspardkirira/multipart
vix deps
```

### Manual

```bash
git clone https://github.com/GaspardKirira/multipart.git
```

Add the `include/` directory to your project.

## Dependency

Requires C++17 or newer.

No external dependencies.

## Quick Examples

### Basic Form Fields

```cpp
#include <multipart/multipart.hpp>
#include <iostream>

int main()
{
    const std::string boundary = "----BOUNDARY";

    const std::string body =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n"
        "\r\n"
        "gaspard\r\n"
        "--" + boundary + "--\r\n";

    auto res = multipart::parse(body, boundary);

    for (const auto& p : res.parts)
        std::cout << p.name << " = "
                  << std::string(p.body.begin(), p.body.end()) << "\n";
}
```

### File Upload

```cpp
#include <multipart/multipart.hpp>
#include <iostream>

int main()
{
    const std::string boundary = "----BOUNDARY";

    const std::string body =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"a.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello\r\n"
        "--" + boundary + "--\r\n";

    auto res = multipart::parse(body, boundary);

    const auto* file = res.find("file");

    if (file)
    {
        std::cout << "Filename: " << file->filename << "\n";
        std::cout << "Size: " << file->body.size() << " bytes\n";
    }
}
```

### Multiple Fields With Same Name

```cpp
auto tags = res.find_all("tag");
```

---

## API Overview

```cpp
// Parse
multipart::result res =
    multipart::parse(body, boundary, options);

// Access
res.parts;
res.find("name");
res.find_all("name");

// Options
multipart::options opts;
opts.max_parts = 10;
opts.max_part_bytes = 1024 * 1024;
```

## Parsing Semantics

- Boundary must be provided without leading `--`.
- Supports CRLF and LF line endings.
- Parses headers per part (stored lowercase).
- Extracts:
  - `name` from Content-Disposition
  - `filename` if present
- Raw body stored as `std::vector<std::uint8_t>`.
- Final boundary must be `--boundary--`.
- Throws `std::runtime_error` on malformed input.

## Security Controls

Optional limits:

- `max_parts`
- `max_part_bytes`

These help prevent:

- Multipart abuse
- Memory exhaustion attacks
- Oversized file uploads

## Complexity

Let:

- N = total body size
- P = number of parts

| Operation        | Time Complexity |
|-----------------|-----------------|
| Parse           | O(N)            |
| Lookup by name  | O(P)            |

Parsing is linear in body size.

## Design Principles

- Deterministic parsing
- Explicit boundary handling
- Minimal abstraction
- No implicit I/O
- Safe-by-default with limits
- Header-only simplicity

This library focuses strictly on multipart parsing.

If you need:

- Streaming multipart parsing
- Chunked transfer decoding
- Automatic file persistence
- HTTP server integration

Build them on top of this layer.

## Tests

```bash
vix build
vix test
```

Tests verify:

- Field extraction
- File uploads
- Repeated field names
- Boundary correctness
- Limit enforcement

## License

MIT License
Copyright (c) Gaspard Kirira

