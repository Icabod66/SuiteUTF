# File: docs/README.md

# LibUTF Documentation Index

This directory contains the authoritative documentation for the LibUTF
library.

LibUTF provides low-level, explicit handling of Unicode and UTF-encoded text.
It is not a text layout, font rendering, or high-level Unicode transformation
library. Its focus is correctness, transparency, and control over encoding and
decoding behavior.

The documentation is structured to reflect the layering of the library itself,
from Unicode fundamentals, through strict UTF handling, to advanced diagnostic
and legacy processing.

A key design principle is that UTF encoding and decoding are the primary focus
of the library. The Unicode utilities exist to support reasoning about code
points and text rules, but most UTF encode/decode paths are intentionally
self-contained for performance and correctness reasons.

---

## Directory structure

- docs/
  - README.md  
    Overview of the documentation structure and how the pieces fit.

  - History.md  
    The origins and development history of this library.

  - unicode/
    - unicode_overview.md  
      Unicode concepts, classification, and utility functions used throughout
      the library. This layer operates on Unicode code points and does not
      perform UTF encoding or decoding.

  - utf/
    - choosing_a_utf_library.md  
      Guidance for selecting an appropriate UTF or Unicode library based on
      encoding, diagnostics, and policy requirements.
      
    - when_to_use_libutf.md  
      Situations where LibUTF is a good fit, and cases where other UTF libraries
      may be more appropriate.
      
    - std_overview.md  
      Strict, standards-compliant UTF handling. This is the recommended entry
      point for most users working with encoded text.

    - toolkit_overview.md  
      Advanced UTF toolkit for diagnostics, legacy encodings, and non-standard
      behavior.

    - handlers_usage.md  
      Practical usage patterns based on the IUTF and IUTFTK handler interfaces.
      Most real-world usage is expected to go through handlers rather than
      individual functions.

    - cp_errors.md  
      Detailed description of the cp_errors diagnostic model used by the UTF
      toolkit.

    - variants_utf_sub_type.md  
      Explanation of UTF_SUB_TYPE variants and their behavioral differences.

    - overlong_utf8.md  
      Overlong UTF-8 encodings, including rationale, risks, and supported use
      cases.

    - wchar_t_guidance.md  
      Guidance on handling wchar_t data when using LibUTF.

    - utf_helpers.md  
      Small helper predicates and utility functions used internally and by
      advanced users.

  - reference/
    - unicode_api.md  
      API reference for Unicode-related headers.

    - utf_std_api.md  
      API reference for utf_std.h.

    - utf_toolkit_api.md  
      API reference for utf_toolkit.h.

  - util/
    - text_hash.md  
      Standalone CCITT-16 based text hashing utilities.

  - comparisons/  
    - tinyutf8_comparison.md  
      UTF-8 policy and diagnostic differences between tinyutf8 and LibUTF.

---

## How to approach the documentation

If you are new to LibUTF, the recommended reading order is:

1) unicode/unicode_overview.md  
   Introduces Unicode concepts and classification used by the library. This
   provides semantic background, but is not required to understand UTF
   encoding and decoding behavior.

2) utf/std_overview.md  
   Describes strict UTF processing, error conventions, and the design goals of
   the standard layer. This establishes the core mental model for LibUTF.

3) utf/handlers_usage.md  
   Shows how most users interact with the library via handler abstractions.

At this point, many users will have all the information they need to use LibUTF
effectively.

---

## When to use the UTF toolkit

The UTF toolkit exists to support advanced and specialized scenarios, including:

- Diagnosing malformed or mixed-encoding text
- Processing legacy or non-standard encodings
- Handling overlong UTF-8 encodings deliberately
- Inspecting low-level decoding failures in detail

If you need these capabilities, continue with:

- utf/toolkit_overview.md
- utf/cp_errors.md
- utf/variants_utf_sub_type.md

The toolkit builds on the same conceptual foundations as the standard layer, but
exposes more control and more diagnostic information.

---

## Handlers versus direct function use

Both the standard layer and the toolkit provide handler interfaces:

- IUTF for utf_std
- IUTFTK for utf_toolkit

In practice, most code uses handlers rather than calling individual encode or
decode functions directly. Handlers centralize configuration, reduce error-prone
flag combinations, and provide a consistent streaming model.

Direct function use is primarily intended for:
- Low-level utilities
- Specialized tooling
- Diagnostic or experimental code
- Performance-critical loops where virtual dispatch is undesirable

This distinction is explained in detail in utf/handlers_usage.md.

---

## API reference versus narrative documentation

The Markdown files in this directory describe semantics, intent, and usage.

The files under docs/reference provide API-level reference material that maps
directly to header files. They are intended to complement, not replace, the
narrative documentation.

Header files remain the authoritative source for function signatures and exact
types.

---

## Documentation scope and intent

This documentation intentionally focuses on:

- Encoding and decoding behavior
- Error handling and diagnostics
- Design trade-offs and variant selection

It intentionally does not attempt to provide:
- Text shaping or rendering guidance
- Locale-aware transformations
- High-level string manipulation facilities

---

## Notes on formatting

All documentation files are ASCII-only and are intended to be readable in any
editor. Markdown features are used conservatively to ensure compatibility with
diff tools and simple viewers.
