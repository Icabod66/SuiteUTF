# File: docs/utf/rune_handling.md

# Rune usage guide

## What is a rune?

A rune is a Unicode scalar value: any value in the range U+0000 to U+10FFFF,
excluding the surrogate range U+D800 to U+DFFF.

This document is a brief guide to how LibUTF encoders and decoders relate to
runes (valid Unicode scalar values).

## utf_std rune behavior

For the following UTF_TYPE values:

- UTF_TYPE::UTF8
- UTF_TYPE::UTF16le
- UTF_TYPE::UTF16be
- UTF_TYPE::UTF32le
- UTF_TYPE::UTF32be

Decoding:
- A successful decode returns a rune.
- A failed decode returns a non-rune (the output value must not be treated as a
  valid rune).

Encoding:
- Non-runes are rejected by the encoder.
- Encoded output is always well formed, standard UTF for the selected UTF_TYPE.

## utf_toolkit rune behavior

utf_toolkit uses cp_errors to report detailed outcomes for encode and decode.
This document does not describe cp_errors in detail. For the full semantics of
cp_errors and the rune queries, see docs/utf/cp_errors.md.

### Strict rune compliant UTF_SUB_TYPE values

The following UTF_SUB_TYPE values are strict rune compliant for both decoding
and encoding:

- UTF_SUB_TYPE::UTF8ns
- UTF_SUB_TYPE::UTF8st
- UTF_SUB_TYPE::UTF16le
- UTF_SUB_TYPE::UTF16be
- UTF_SUB_TYPE::UTF32le
- UTF_SUB_TYPE::UTF32be

For these sub-types:

Decoding:
- If cp_errors.failed() is false, the decoded value is guaranteed to be a rune.

Encoding:
- Non-runes are rejected by the encoder.
- Encoded output is always well formed, standard UTF for the selected sub-type.

The function cp_errors.is_strict_rune(utfSubType) is a formalization of this
rule. It returns true only for these strict sub-types (and only when rune
validity is guaranteed by the diagnostic state). For any other UTF_SUB_TYPE it
returns false.

### Other UTF_SUB_TYPE values

For all other UTF_SUB_TYPE values, rune compliance is not guaranteed by
cp_errors.failed() alone.

If you need to determine whether the decoded value is a rune for these sub-types,
use cp_errors.is_rune_value().

For strictness rules and behavioral differences between UTF sub-types, see:
docs/utf/variants_utf_sub_type.md.
