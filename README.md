# LibUTF

LibUTF is a compact suite of Unicode and UTF encoders and decoders with support
for strict standards compliance, permissive and custom behaviours, and legacy
encodings. It offers both simple fail-fast operation and rich diagnostic modes
for analysing, debugging, and correcting invalid text.

The library is designed with a strong emphasis on correctness, deterministic
behaviour, and long-term maintainability, making it suitable for use in both
runtime and tooling contexts.

The project originated between 2010 and 2016 as part of a larger game-engine
architecture and was revisited and modernised between 2024 and 2025. Current
work focuses on documentation, structural clarity, and consolidation of
long-standing design decisions.

LibUTF is provided as a reference-quality implementation and is not intended to
be a full-spectrum Unicode or internationalisation solution.

---

## Scope and Intent

LibUTF provides **low-level encoding and decoding of UTF-encoded Unicode**.

It is **not**:
- a general Unicode manipulation library
- a text shaping or font rendering library
- a high-level internationalisation framework

If you require complex Unicode operations (such as case folding, collation,
normalisation, or locale-aware processing), a different class of library is
likely more appropriate.

LibUTF is intentionally conservative in scope and is designed to integrate
cleanly into larger systems that already impose their own architectural or
policy constraints.

Diagnostic return values are explicitly marked `[[nodiscard]]`. Discarding them
constitutes an explicit opt-out from correctness guarantees.

---

## Design Principles

Key design principles include:

- explicit, no-exceptions error handling
- deterministic behaviour and table generation
- stable, intentionally narrow public APIs
- emphasis on auditability and correctness over convenience
- suitability for performance-sensitive and low-level environments

The library is written in C++ and does not dictate higher-level application
structure.

---

## Library Structure and Components

LibUTF is split into a small number of focused components. The following overview
is intended as a technical reference for users navigating the codebase.

---

### `libutf.h`

General umbrella include for all LibUTF public headers.

---

### `unicode_type.h`

Defines the fundamental `unicode_t` type used throughout the library.

---

### `utf_std.h` / `utf_std.cpp`

Depends on `unicode_type.h`.

The `.cpp` file also depends on `unicode_utilities.h`.

Provides lightweight handling of **well-formed, standards-compliant**
UTF-encoded byte streams, including:

- identification of UTF encoding forms
- basic error detection

Use `utf_std` when you require strict, standard UTF processing and validation.

---

### `utf_toolkit.h` / `utf_toolkit.cpp`

Depends on `utf_std.h` (and transitively on `unicode_type.h`).

The `.cpp` file also depends on `unicode_utilities.h`.

Provides lower-level manipulation of UTF-encoded byte streams, including support
for non-standard or non-conforming encodings and more detailed parsing and
writing analysis.

Use `utf_toolkit` only when you need to create, analyse, or process non-standard
UTF encodings.

---

### `unicode_classification.h` / `unicode_classification.cpp`

Depends on `unicode_type.h`.

Provides basic classification of Unicode code points, including:

- general classification
- ASCII-compatible code point classification
- XML-specific classification
- JSON-specific classification

---

### `unicode_utilities.h` / `unicode_utilities.cpp`

Depends on `unicode_type.h`.

Provides utility functionality, including:

- conversion of hexadecimal characters to and from numeric values
- translation of general short escape codes between `\x` and control code-point forms
- translation of JSON-compatible short escape codes
- transcoding between Unicode and Windows code page 1252

---

### `utf_helpers.h`

Depends on `unicode_type.h`.

Provides identifiers and helper functions for detecting UTF and non-UTF encoded
byte streams. Unless you are extending the library to support additional
encodings, most users will not need to interact with this header directly.

---

### `text_hash.h` / `text_hash.cpp`

Provides a small, self-contained implementation of the CRC-CCITT-FALSE checksum,
with optional conversion of the resulting hash value to an ASCII hexadecimal
representation.

This component is **not intended for cryptographic use**.

It is frequently used alongside LibUTF for tasks such as:

- stable identification of text content
- lightweight integrity checks
- hashing of UTF-encoded byte streams for diagnostics or tooling

The implementation is intentionally simple and has no external dependencies.

---

## Project Status

LibUTF is published to document a mature internal component and to make it
available for inspection and reuse.

While the codebase is actively maintained, the project should not be interpreted
as a consumer-facing product or as a commitment to ongoing feature expansion.
Issue reports and corrections are welcome; feature requests may or may not align
with the project’s intended scope.

---

## History, Authorship, and Provenance

LibUTF was originally developed by **Ritchie Brannan** between **2010 and 2016**
as part of a broader game-engine architecture.

Development was resumed between **November 2024 and 2025**, focusing on
modernisation, correctness, structural refinement, and documentation.

Design, implementation, and documentation are authored by the same individual.
Tool-assisted review, drafting, and coordination support were provided during the
modernisation phase using **ChatGPT** and **ChatGPT Codex**. All final technical
decisions, design judgements, policies, and stylistic choices were made by the
author.

None of the code in this library was written by ChatGPT or any other AI.

LibUTF is the result of long-term, independent development and substantially
predates its public release. The project was not derived from, nor based on,
third-party implementations.

A more detailed project history and revision narrative can be found in
`docs/History.md`.

---

## Licence

LibUTF is released under the MIT License. See `LICENSE.txt` for details.
