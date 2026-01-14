# File: docs/utf/std_overview.md

# LibUTF std layer overview

The utf_std layer provides a small, strict, standards-aligned interface for
working with UTF encodings and a small set of related 8-bit encodings.

It is designed for:

- Well-formed, standards-compliant UTF data.
- Simple, predictable behavior.
- No exceptions and no dynamic allocation.
- Caller-owned buffers and explicit sizes.

It is not a full text-processing or collation library. It focuses on low-level
encode and decode primitives and a small handler abstraction (IUTF) that makes
sequential processing easier.


## Design goals

The utf_std layer follows a few consistent rules:

- Strict UTF
  - Only encodes and decodes values that are valid for the target encoding.
  - Rejects non-compliant sequences.
  - Java-style UTF-8 support for the null code point is available and is the
    default behavior when using UTF_TYPE::OTHER.
  - The strict behavior of utf_std follows the same general acceptance goals
    as the toolkit strict variants, while using simpler reporting.

- Predictable error reporting
  - Functions return bool to indicate success or failure.
  - On failure, unicode and bytes are set according to documented rules.
  - Error signaling uses unicode_t values rather than exceptions or separate
    error objects. This signaling convention applies only within utf_std.

- Buffer ownership is external
  - All functions work on caller-supplied buffers.
  - Sizes are always passed explicitly.
  - There is no dynamic allocation inside utf_std.

- Thin abstractions
  - The main abstraction is the IUTF handler interface.
  - Free functions remain available for direct use where needed.


## Core types

### unicode_t

All Unicode scalar values and related codes in utf_std use:

    typedef int32_t unicode_t;

In this layer, unicode_t serves two purposes:

- Representing decoded Unicode code points.
- Carrying error information when a decode or encode operation fails.

This dual use is a deliberate design choice in utf_std and does not apply to
the utf_toolkit layer.

#### Unicode sentinel error values

Because utf_std does not return explicit error codes, certain invalid unicode_t
values are reserved to signal decode failures and exceptional conditions.

- Valid Unicode code points are always non-negative.

- Unicode values in the range 0x80000000 to 0xFFFFFFFF (negative unicode_t
  values) are always considered invalid and are reserved by the library as
  sentinel values.

- Sentinel values are used only by the utf_std layer to signal decode failures
  or exceptional conditions where no explicit error code is available.

- The utf_toolkit layer never returns sentinel unicode values; it reports all
  error and diagnostic information via cp_errors instead.

- Attempts to write or encode sentinel unicode values will always fail in both
  the standard and toolkit layers.

- Because unicode_t is signed, a simple sign check (unicode < 0) can be used to
  reliably identify sentinel values.

- Sentinel values are never valid Unicode and must not be treated as
  characters.


### utf_text

    struct utf_text
    {
        uint32_t    length;
        uint32_t    offset;
        uint8_t*    buffer;
    };

utf_text is a small, non-owning view over an encoded byte buffer.

length is the total buffer size in bytes. offset is a byte index into the
buffer and acts as a read or write cursor.

utf_text does not impose null-termination. Functions that operate on
null-terminated strings are explicitly documented as such.


### UTF_TYPE

    enum class UTF_TYPE : int32_t
    {
        UTF8,
        UTF16le,
        UTF16be,
        UTF32le,
        UTF32be,
        OTHER,
        COUNT
    };

UTF_TYPE identifies the primary UTF encodings. UTF_TYPE::OTHER indicates a
non-UTF or unknown encoding and defaults to Java-style UTF-8 unless a specific
UTF_OTHER_TYPE is selected.

A note on wchar_t:

The utf_std layer does not treat wchar_t as a Unicode encoding. The meaning
and width of wchar_t are implementation-defined and vary by platform.

Passing wchar_t buffers directly to utf_std functions without an explicit
understanding of their encoding can lead to incorrect results.

For guidance on safely handling wchar_t and integrating it with LibUTF,
see docs/utf/wchar_t_guidance.md.

### UTF_OTHER_TYPE

    enum class UTF_OTHER_TYPE : int32_t
    {
        JUTF8,
        ISO8859_1,
        Ascii,
        CP1252,
        COUNT
    };

UTF_OTHER_TYPE selects specific non-UTF or non-strict encodings supported by
utf_std.


## Error and validation conventions

Low-level encode and decode functions follow these conventions:

- bool return value
  - true on success.
  - false on failure or non-compliance.

- On decode failure or non-compliance
  - For UTF-8 and "other" encodings:
    - unicode is set to 0x800000XX, where XX is the original lead byte.
  - For UTF-16 and UTF-32:
    - unicode is set to 0x80000000.
  - bytes is set to the code-unit size in bytes.
  - If the buffer is at end-of-buffer, bytes is set to 0.

- If the buffer is nullptr or too small
  - unicode is set to 0.
  - The return value is false.

Callers that use these functions directly are expected to interpret unicode
error values according to this convention. Handler-based APIs encapsulate this
logic for most use cases.


## The IUTF handler interface

The IUTF interface is the primary abstraction most users should build around.
It provides a uniform way to:

- Inspect encoding metadata.
- Encode and decode single code points.
- Validate buffers.
- Process streams line by line.

Usage patterns and examples are covered in docs/utf/handlers_usage.md.
