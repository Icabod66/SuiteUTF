# File: docs/utf/when_to_use_suite__utf.md

## When SuiteUTF is a good fit

In SuiteUTF, strict handling means adherence to the UTF standard, with any
deviation treated as a failure rather than silently corrected. This behavior is
available through both utf_std and designated strict sub-types in utf_toolkit.

SuiteUTF also supports non-standard encoding and decoding policies by design.
The choice between strict and permissive behavior is explicit rather than
implicit in the API.

SuiteUTF is well-suited to situations where:

- UTF encoding and decoding behavior must be explicit and controllable.
- Strict standards compliance is required, with deviations treated as failure.
- Legacy, non-standard, or custom UTF forms must be encoded or decoded in a
  controlled and intentional way.
- Detailed diagnostics are required, including byte-level error reporting.
- Allocation-free and deterministic behavior is important.
- Encoding and decoding policy must be separated from container semantics.

Typical use cases include engines, validators, converters, diagnostic tools, and
systems that must interoperate with legacy or constrained environments.

---

## When SuiteUTF may not be the right choice

SuiteUTF may not be the best fit if:

- You only need a UTF-8 string type and prefer a drop-in container.
- Your input and output are guaranteed to be well-formed, canonical UTF, and
  diagnostics are unnecessary.
- Convenience and minimal integration effort are higher priorities than
  explicit control over encoding and decoding behavior.
- You need high-level Unicode algorithms such as collation, normalization, or
  locale-aware text processing.

In these cases, higher-level or more specialized libraries may provide a better
fit with less integration effort.
