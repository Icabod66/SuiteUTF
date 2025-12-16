# File: docs/comparisons/tinyutf8_comparison.md

# Comparison: tinyutf8 and LibUTF UTF-8 handling

## Scope and intent

This document compares tinyutf8 and LibUTF at the level of UTF-8 encoding and
decoding behavior.

The focus is on low-level policy choices, edge cases, and diagnostics. It is not
a comparison of API ergonomics, container design, or suitability as a
std::string replacement.

Descriptions of tinyutf8 behavior are based on analysis of the tinyutf8 source
code as it existed at the time this document was written. No claims are made
about future versions or about code paths not exercised by the analyzed
implementations.

LibUTF behavior is described according to its documented guarantees and intended
semantics.

---

## Library focus and layering

tinyutf8 is a UTF-8 only library built around an owning UTF-8 string type. It is
designed to behave as a drop-in replacement for std::string, with UTF-8 aware
iteration and access.

LibUTF is a lower-level, allocation-free library focused on explicit UTF encode
and decode operations. It supports multiple UTF encodings and separates strict,
standards-compliant handling from a more permissive diagnostic toolkit.

In practice, this means tinyutf8 emphasizes convenience and internal
normalization, while LibUTF emphasizes explicit policy selection and observable
behavior.

---

## Allocation and ownership

tinyutf8 performs memory allocation as part of its core design. Operations such
as producing std::string outputs necessarily allocate and copy data.

LibUTF core encode and decode operations perform no allocation. All buffers,
offsets, and limits are provided explicitly by the caller. Any higher-level
ownership or string abstraction is expected to be built on top if required.

---

## UTF-8 sequence length policies

Strict Unicode UTF-8 permits sequence lengths of 1 to 4 bytes.

tinyutf8 permits extended UTF-8 forms and advertises support for sequence
lengths of 1 to 7 bytes, allowing code point values up to 0xFFFFFFFF.

LibUTF intentionally caps UTF-8 encoding at 1 to 6 bytes. This allows handling
of legacy and extended UTF-8 like forms up to 31 bits, while avoiding the
7-byte space that requires universally forbidden lead byte values.

In LibUTF, attempting to encode a UTF-8 sequence longer than 6 bytes is a
write-side error and reports BadSizeUTF8.

During decoding, LibUTF always treats lead bytes 0xFE and 0xFF as illegal input
and reports DisallowedByte.

---

## Overlong and non-canonical encodings

tinyutf8 does not treat overlong UTF-8 encodings as a distinct condition at the
low-level decode primitive. Sequence length is determined from the lead byte,
and the decoded value is reconstructed mechanically from the selected number of
bytes.

As a result, overlong UTF-8 encodings decode to their canonical code point
values without emitting a dedicated diagnostic.

LibUTF distinguishes overlong encodings explicitly. Overlong UTF-8 sequences can
be decoded in toolkit modes, but are reported via the OverlongUTF8 diagnostic
flag. In strict modes, overlong encodings are treated as errors.

LibUTF also provides a controlled mechanism for treating overlong encodings as
application-defined indices. This allows deliberate use while preserving
diagnostic visibility.

---

## Continuation byte validation and diagnostics

tinyutf8 determines UTF-8 sequence length from the lead byte and reconstructs
the code point from the resulting byte sequence. On the inspected decode paths,
there is no requirement that subsequent bytes conform to the UTF-8 continuation
byte pattern, and no structural validation is performed before decoding.

LibUTF validates UTF-8 structure in both strict and toolkit modes. If a byte is
illegal for the encoding, DisallowedByte is reported. If a byte is allowed by
the encoding but appears in an unexpected position, UnexpectedByte is reported.

LibUTF also records the relative offset of the offending byte within the
attempted decode using reserved diagnostic bits. LibUTF does not attempt to
reconstruct a code point from structurally invalid byte sequences.

---

## BOM handling

tinyutf8 provides a convenience function that prefixes the UTF-8 BOM byte
sequence EF BB BF when producing a std::string. It does not perform BOM
detection or stripping as part of its UTF-8 decode operations.

LibUTF treats BOM handling explicitly. BOMs may be emitted deliberately, and
their appearance during decoding, including mid-stream, can be detected and
reported as part of normal diagnostic processing.

---

## Rationale for the LibUTF 6-byte UTF-8 cap

LibUTF caps extended UTF-8 at 6 bytes as a deliberate policy choice.

Six-byte UTF-8 like forms cover the historically known extended space up to
31-bit values, while avoiding the 7-byte range that necessarily collides with
lead byte values 0xFE and 0xFF. These byte values are widely treated as forbidden
in UTF-8 processing and can cause compatibility problems with other software.

This cap aligns with LibUTF goals of controlled permissiveness. Legacy and
non-standard encodings can be decoded, diagnosed, and reasoned about, while
universally invalid byte classes are always rejected during decoding.

The result is a bounded, diagnosable extension surface that supports real-world
data without silently expanding into further extended encodings with little
practical use.

---
