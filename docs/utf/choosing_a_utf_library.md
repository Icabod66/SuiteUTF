# File: docs/utf/choosing_a_utf_library.md

## Choosing a UTF library

Use the following guide to decide which class of UTF or Unicode library best
fits your needs.

In LibUTF, strict handling means adherence to the UTF standard, with any
deviation treated as a failure rather than silently corrected.

- If you want Unicode text to "just work" and do not need control over encoding
  or diagnostics at a low level,
  use the facilities provided by your language runtime.

- If you need a UTF-8 string container with minimal setup,
  use tinyutf8 or similar UTF-8 aware string abstractions.

- If your input is guaranteed to be well-formed and performance is the primary
  concern for validation and conversion,
  use fast libraries such as simdutf.

- If you need basic conversion between encodings but do not require detailed
  diagnostics or strict guarantees,
  use system libraries such as iconv.

- If you need to encode or decode UTF data strictly according to the relevant
  standard, with any deviation treated as failure,
  use LibUTF through the utf_std interfaces.

- If you need the same strict behavior but also want access to detailed
  diagnostic information,
  use LibUTF through the strict sub-types provided by utf_toolkit.

- If you need to encode or decode legacy, non-standard, or custom UTF forms, or
  require controlled permissiveness and recovery,
  use LibUTF through the permissive utf_toolkit sub-types or free functions.

- If you need high-level Unicode algorithms such as collation, normalization, or
  locale-aware text processing,
  use ICU as a comprehensive solution.
