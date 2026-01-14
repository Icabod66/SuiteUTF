# File: docs/utf/cp_errors.md

# cp_errors (toolkit diagnostic result)

## Purpose and audience

`cp_errors` is the diagnostic result type used throughout the UTF toolkit layer
to report detailed outcomes from code-point encode and decode operations. It is
returned by low-level encoding and decoding functions and by the IUTFTK handler
interface.

This type is intended for callers that require more than a simple success or
failure signal. It provides structured information about what went wrong (or
what was unusual), guidance on recovery strategies such as replacement-character
substitution, and compact diagnostic detail suitable for logging or user-facing
error reporting.

The standard UTF layer does not expose `cp_errors` directly. Instead it presents
simplified return conventions, while remaining conceptually aligned with the
same error model and classification philosophy.

## Structure of cp_errors

`cp_errors` is a lightweight value type that wraps a 32-bit unsigned integer.
The value is treated as a bitmask with three conceptual regions:

- High-order bits represent named error and warning flags.
- A small group of low-order bits encodes a byte-relative index.
- All other bits are unused or reserved.

The type supports bitwise composition, masking, and querying operations, making
it inexpensive to pass by value and easy to filter or combine.

### Diagnostic flags

The `bits` enumeration defines the set of possible diagnostic flags. Each flag
describes a specific condition encountered during encoding or decoding.

At a high level, the flags fall into the following groups.

Operation status:

- `Failed` indicates that the operation failed because one or more error-class
  conditions were encountered.

Buffer and structural errors:

- `InvalidBuffer` indicates a null text buffer.
- `InvalidOffset` indicates that the current offset exceeds the buffer length.
- `MisalignedOffset` indicates that the offset is misaligned for the encoding.
- `MisalignedLength` indicates that the buffer length is misaligned.
- `WriteOverflow` indicates that a write would exceed buffer bounds.
- `ReadTruncated` indicates that a read would overrun the buffer.
- `ReadExhausted` indicates that the read is positioned at end of buffer.

General encoding and decoding errors:

- `NotEncodable` indicates that a code point cannot be encoded in the
  selected encoding.
- `NotDecodable` indicates that a code point cannot be decoded using the
  selected encoding.

Code-point classification warnings:

- `InvalidPoint` indicates a value in the invalid Unicode range
  (U+80000000 to U+FFFFFFFF).
- `ExtendedUCS4` indicates a value in the extended UCS4 range
  (U+00110000 to U+7FFFFFFF).
- `Supplementary` indicates a value in the supplementary planes
  (U+00010000 to U+0010FFFF).
- `NonCharacter` indicates a Unicode non-character.

Surrogate-related warnings:

- `TruncatedPair` indicates a high surrogate from a truncated pair.
- `SurrogatePair` indicates that a code point was encoded as a surrogate pair.
- `HighSurrogate` indicates an unpaired high surrogate.
- `LowSurrogate` indicates an unpaired low surrogate.

UTF-8 form and compliance warnings:

- `IrregularForm` indicates a processable but non-compliant encoding form.
- `ModifiedUTF8` indicates a modified UTF-8 form such as the two-byte NULL.
- `OverlongUTF8` indicates an overlong UTF-8 encoding.
- `ExtendedUTF8` indicates a UTF-8 encoding longer than four bytes.
- `BadSizeUTF8` indicates an invalid explicit UTF-8 size during encoding.

Diagnostic byte-level errors:

- `DisallowedByte` indicates an illegal byte for the encoding.
- `UnexpectedByte` indicates a byte that is valid in isolation but appears in
  an unexpected position.

Transformation and representational limits:

- `Untransformable` indicates that the value cannot be transformed between
  Unicode and the selected encoding.
- `NotEnoughBits` indicates that the encoding cannot represent the value.

## Errors vs warnings

`cp_errors` distinguishes errors from warnings using internal masks.

- `failed()` checks whether the `Failed` flag is set.
- `error()` checks whether any error-class flags are present.
- `errors_only()` returns a new `cp_errors` containing only error flags.
- `warnings_only()` returns a new `cp_errors` containing only warning flags.

This allows callers to implement policy decisions such as:

- Treat all warnings as non-fatal and continue processing.
- Escalate specific warnings to errors in strict modes.
- Log warnings while aborting only on true errors.

### Buffer errors

Buffer-related conditions can be queried directly using `buffer_error()`. This
covers structural problems with the text buffer itself, such as null pointers,
out-of-range offsets, or misalignment.

These conditions typically indicate a contract or usage error rather than a
content or encoding issue, and are often handled separately from other failures.

## Rune validity queries

`cp_errors` provides helper queries that allow callers to determine whether a
decoded Unicode value is a valid rune, and whether that rune was produced using
a strictly Unicode-compliant encoding form.

These queries encapsulate both error-state and warning-state interpretation and
should be preferred over manual flag inspection when reasoning about Unicode
validity.

---

### is_rune_value()

Returns true if the decoded value represents a valid Unicode scalar value (a rune).

This function returns false if any error state is present, including failure
states. In all such cases, the decoded value must be treated as invalid Unicode
and not a rune.

When this function returns true:

- The decoded value lies within the Unicode scalar value range.
- The value is not a surrogate code point.
- No error-class conditions are present.
- A limited subset of informational warnings may be present, provided they are
  compatible with rune validity.

This function does not imply that the encoding form was strictly
Unicode-compliant. It indicates only that the resulting value itself is a
valid rune.

---

### is_strict_rune(UTF_SUB_TYPE utfSubType)

Returns true if the decoded value is a valid Unicode scalar value and the
encoding used to produce it is strictly Unicode-compliant for the specified
UTF sub-type.

This function returns false if any error state is present, including
failure states.

When this function returns true:

- The decoded value is a valid rune.
- The encoding obeys all Unicode rules applicable to the given UTF sub-type.
- Only informational warnings compatible with strict Unicode compliance may
  be present.

The validity guarantees provided by this function are equivalent to those of a
successful decode in the standard UTF layer.

---

### Relationship between the queries

- is_strict_rune() implies is_rune_value()
- is_rune_value() does not imply is_strict_rune()

A decoded value may be rune-valid while originating from a non-canonical,
legacy, or otherwise non-strict encoding form.

---

### Interaction with error and warning states

Both is_rune_value() and is_strict_rune() treat all error states, including
failed() conditions, as invalid Unicode. In such cases, the decoded value must
not be interpreted as a rune.

A restricted subset of informational warnings may be present while still
allowing rune validity. Which warnings are permitted depends on the selected
UTF sub-type and its strictness rules.

Callers should not infer encoding round-trip safety or encoder support from
is_rune_value() alone.

Refer to docs/utf/variants_utf_sub_type.md for the strictness and warning
interpretation rules associated with each UTF sub-type.

## Replacement-character guidance

`use_replacement_character()` provides a focused policy hint for decoders. It
returns true when the diagnostic state includes any of the following conditions:

- The value is not decodable in the selected encoding.
- The value is a Unicode non-character.
- The encoding form is irregular but processable.

When this function returns true, callers are expected to substitute a
replacement character rather than propagating the decoded value unchanged.

This guidance is intentionally narrow. Other warnings, such as overlong or
modified UTF-8 forms, are not included and must be handled according to the
caller own policy.

Whether replacement is applied, and whether it is treated as fatal or
recoverable, depends on the strictness and error-consumption behavior of the
selected `UTF_SUB_TYPE`. See docs/utf/variants_utf_sub_type.md for how strict,
non-skipping, and permissive modes interpret decode failures.

## Byte index encoding

A small byte index is encoded into the low-order bits of `cp_errors`. This index
is relative to the start of the code point being decoded, specifically relative
to `text.buffer[text.offset]` at the time the decode operation was invoked.

The index is intended for byte-oriented diagnostics, such as identifying which
byte within a UTF-8 sequence was illegal or unexpected.

The stored value can range from 0 to 7. In practice, the toolkit layer only uses
this index for UTF-8 decoding, where valid values are in the range 0 to 5. For
other encodings or non-byte-oriented failures, the index may be zero and should
not be assumed to carry meaning.

The index is accessed using `get_byte_index()` and set using `set_byte_index()`.

## Recommended usage pattern

A typical handling pattern when working with `cp_errors` is:

1. Check for buffer errors first.
   Structural issues with the text buffer should usually be handled
   immediately.

2. Check for other errors.
   If `failed()` or `error()` is true, decide whether to abort or recover based
   on application policy.

3. Decide how to treat warnings.
   Use `warnings_only()` to inspect non-fatal conditions and apply any
   strictness rules required by your application.

4. Apply replacement-character guidance.
   If `use_replacement_character()` is true, substitute a replacement code
   point during decoding.

5. Preserve diagnostic detail.
   When reporting errors, include the byte-relative index where relevant to
   improve diagnostics.

## Examples

Checking for errors and warnings:

    cp_errors e = result;

    if (e.buffer_error())
    {
        // Structural buffer error.
    }
    else if (e.error())
    {
        cp_errors hard = e.errors_only();
        (void)hard;
    }
    else if (e.warnings_only().any())
    {
        // Non-fatal warnings present.
    }

Using replacement-character guidance:

    cp_errors e = result;

    if (e.use_replacement_character())
    {
        // Substitute a replacement character.
    }

Reporting a byte-relative diagnostic:

    cp_errors e = result;

    if (e.any(cp_errors::bits::UnexpectedByte) ||
        e.any(cp_errors::bits::DisallowedByte))
    {
        uint32_t index = e.get_byte_index();
        // Include index in diagnostic output.
        (void)index;
    }

## Cross-links

- Toolkit overview: docs/utf/toolkit_overview.md
- Handler usage patterns: docs/utf/handlers_usage.md
- Encoding variant reference: docs/utf/variants_utf_sub_type.md
