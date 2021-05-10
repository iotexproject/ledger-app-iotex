# IoTeX 0.1.8 - Security Review

- Application version: IoTeX 0.1.8 ([nanos_0.1.8_2.0.0-rc2_2021-02-28_10-31-01](https://github.com/LedgerHQ/app-iotex/releases/tag/nanos_0.1.8_2.0.0-rc2_2021-02-28_10-31-01))
- Reviewed by: Jean-Baptiste Bédrune
- Review date: May 4, 2021
- Approved by: -

| Date       | Version | Comments                          |
| ---------- | ------- | --------------------------------- |
| 05/03/2021 | 0.1     | Draft version, sent to developers |

References:

- https://docs.google.com/document/d/1sbK9w2VP2k-FElRFNbxP7JM5ZzES48dp3cs1jpFnPeo/edit#heading=h.rq0vani8f7lf

Several possible fixes can be found in the [security-fixes](https://github.com/LedgerHQ/app-iotex/tree/security-review) branch of the project. However, we recommend a full rewrite of the transaction parser.

## Vulnerabilities

Transactions are encoded in Protobuf. The Nano device receive an ActionCore message, as defined in [action.proto](https://github.com/iotexproject/iotex-proto/blob/9574ea51df66302662ea5a194b3821217d160590/proto/types/action.proto). Fields are extracted from the message, and displayed to the user. Parsing of the transaction data is performed through a custom Protobuf reader, which is not compliant with the standard and has several vulnerabilities.

### Various problems in Protobuf parser

#### Parser accepts incorrectly encoded integers

As shown above, to parse integers parser reads the input stream as long as the MSB of the current byte is set. This is incorrect. For example, the following data will be decoded as 0xffffffffffffffff while its value is actually 0x3fffffffffffffffff (bigger than the maximum size of `uint64_t`):

```
ff ff ff ff ff ff ff ff ff 7f
```

Such integers should be rejected by the parser.

:warning: This has not been fixed.

#### No canonical representation of the transaction

Parser internals are very basic: fields are read sequentially, and displayed in the order they are serialized. There are two problems with this method:

- Parser does not correctly handle unordered fields: if nonce is stored before the version in the serialized data, nonce will be displayed first. This is a bit strange: field ordering should be constant when displayed to the user.
- Parser does not detect repeated fields: fields can be repeated several times, and will be displayed on the screen several times. This is not an expected behavior: messages that contain repeated fields should be rejected.

:warning: This has not been fixed.

#### Bad prototype for decode_varint

The prototype of the `decode_varint` function, which reads Protobuf-encoded integers, takes an input size of type `uint8_t`. This is a problem as calls to this function provide a size computed from bigger variables:

```c
uint32_t len;
uint64_t i;
[...]
header = decode_varint(&pb_data[i], &skip_bytes, len - i);
```

`len-i` can be, for example, 256. Size will be cast to a byte, and resulting value will be 0. Value returned by `decode_varint` will then be incorrect.

The type of the `max_len` parameter has been changed to `size_t`.

#### No error is raised when decode_varint fails

The `decode_varint` function always return a value, and never fails, even if no valid integer has been parser, as can be seen in the code below:

```c
uint64_t
decode_varint(const uint8_t *buf, uint8_t *skip_bytes, uint8_t max_len) {
    uint64_t result = 0;
    uint64_t val;
    uint8_t idx = 0;

    while (idx < max_len)
    {
        val = buf[idx] & 0x7f;
        result |= (val << (idx * 7));

        // no more bytes
        if (!(buf[idx] & 0x80)) break;

        idx++;
    }

    (*skip_bytes) = idx + 1;
    return result;
}
```

It leads to an out-of bounds read when an invalid integer is encoded at the end of a transaction: if the transaction data ends with bytes whose most significant bit is set, the "no more bytes" case will be reached, and `skip_bytes` will be set to `idx +1`, which is an index after the end of the transaction.

This will produce and out-of-bounds read when the next field will be read.

Fix: https://github.com/LedgerHQ/app-iotex/commit/c1b5a50170500ea13f296ec71a7e2e0770e5ad32

#### No difference between uint32 and uint64 parsing

Protobuf defines integer of various sizes. For example, the header of the ActionCore message is defined as:

```protobuf
message ActionCore {
  uint32 version = 1;
  uint64 nonce = 2;
  uint64 gasLimit = 3;
  string gasPrice = 4;
  uint32 chainID = 5;
  ...
```

`uint32` and `uint64` are respectively unsigned integers of 32 and 64 bits. While these types are different, they are parsed with the same function in pb_parser.c: `decode_varint`. Invalid values for uint32, bigger than 32 bits, will be considered as valid and cast to a 32-bit integer by the parser. Non-compliant messages will the be accepted by the parser.

Fix: see https://github.com/LedgerHQ/app-iotex/commit/c1b5a50170500ea13f296ec71a7e2e0770e5ad32

#### Infinite loop in decode_varint

`decode_varint` iterates on the input buffer, until it finds a byte whose MSB is 0. The type of the iterator index is `uint8_t`. If 256 consecutive bytes have their MSB set, counter will loop back to zero, which will lead to an infinite loop.

This bug can be fixed by changing the type of the variable carrying the loop index.

#### Size of Protobuf containers is never verified

#### Infinite recursion

### Transactions Fuzzer

A transaction fuzzer has been added.

It simply calls the `decode_pb` function responsible for parsing the transaction data, encoded in Protobuf. Transactions are `ActionCore` messages, whose format is described in [action.proto](https://github.com/iotexproject/iotex-proto/blob/9574ea51df66302662ea5a194b3821217d160590/proto/types/action.proto).

Parser is fully implemented in the `pb_parser.c ` file. One good thing is that it is easy to fuzz. One bad aspect is that the parser is fully home made, and does not seem to have been written with security in mind.

The `decode_pb` function returns the number of fields to be displayed. Our fuzzer retrieves this number, and prints each of the returned fields to force the formatting of the fields to be displayed.

#### Overflow on gas price size

Gas price (`gasPrice`  field) is a string. It consists in a size, encoded as a varint, and data. varint is read as a signed integer. The amount of data to copy in the output buffer is truncated to the size of the output buffer using the `min` macro:

```c
int gas_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);;
i += skip_bytes;

if (curid == queryid) {
    int cpylen;
    cpylen = min(gas_str_len, tx_ctx.query.out_val_len - 1);
    snprintf(tx_ctx.query.out_key, tx_ctx.query.out_key_len, "Gas Price");
    strncpy(tx_ctx.query.out_val, (const char *)&pb_data[i], cpylen);
    tx_ctx.query.out_val[cpylen] = 0;
}
```

However, as it can be seen above, as `gas_str_len` can be negative, `cpylen` can also be negative, and passed to `snprintf`. This will lead to an overflow of the `tx_ctx.query.out_key` variable.

A simple test case is the following transaction data (see crash-e10e8db7a12da6c6abce6ccd83221405dcfd1c6f):

```text
22 ff ff ff ff 1a
```

Bug has been fixed by defining `gas_str_len` as an unsigned field:

```diff
diff --git a/src/lib/pb_parser.c b/src/lib/pb_parser.c
index 254e463..07a309a 100755
--- a/src/lib/pb_parser.c
+++ b/src/lib/pb_parser.c
@@ -523,7 +523,7 @@ decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields_out, int q

                 totalfields++;

-                int gas_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);;
+                size_t gas_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);;
                 i += skip_bytes;

                 if (curid == queryid) {
```

#### Out of bounds read when reading gas price

Additionally, there is also an out of bounds read in the code detailed above: the amount of data copied to `tx_ctx.query.out_val` is truncated to the size of this buffer. However, there is no guarantee that the input buffer is large enough to hold such data.

This leads to an out of bounds read in the transaction buffer. An attacker could read up to 192 bytes of data after the transaction buffer.

Testcase: see crash-a344e5f8ffe3c340393617831c1a290ced3ef78c

Bug can be fixed by properly checking there is enough remaining data in the input buffer:

```diff
diff --git a/src/lib/pb_parser.c b/src/lib/pb_parser.c
index 71a0e3b..98f4d78 100755
--- a/src/lib/pb_parser.c
+++ b/src/lib/pb_parser.c
@@ -526,6 +526,10 @@ decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields_out, int q
                 size_t gas_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);;
                 i += skip_bytes;

+                if (gas_str_len > len - i) { // overflow
+                    return -1;
+                }
+
                 if (curid == queryid) {
                     int cpylen;
                     cpylen = min(gas_str_len, tx_ctx.query.out_val_len - 1);
```

#### Integer overflow on input stream size when reading gas price

Finally, another overflow occurs after field size has been decoded. Indeed, the `decode_varint` function returns the value decoded by the Protobuf parser, and the number of bytes processed by the parser, to compute the current offset of the stream being read.

There is no check on the number of bytes read, which can be higher than the number of input bytes. Hence, after `skipped_bytes` is added to  `i`, `i` can be higher than `len`, the full input stream length. This leads to several integer overflows, that trigger out of bounds read.

Testcase: crash-6a2429c02c20ccab5aa1ad6a6a9058e8bcefe54a

A simple check has been added:

```diff
diff --git a/src/lib/pb_parser.c b/src/lib/pb_parser.c
index 98f4d78..16bec81 100755
--- a/src/lib/pb_parser.c
+++ b/src/lib/pb_parser.c
@@ -525,6 +525,9 @@ decode_pb(const uint8_t *pb_data, uint32_t len, uint32_t *totalfields_out, int q

                 size_t gas_str_len = decode_varint(&pb_data[i], &skip_bytes, len - i);;
                 i += skip_bytes;
+                if (i > len) {
+                    return -1;
+                }

                 if (gas_str_len > len - i) { // overflow
                     return -1;
```

#### Integer Overflow when Processing Action Length

The "action" field is a union of all the possible actions supported by IoTeX. Unions are prefixed by their type and their size. As in the snipped mentioned above, the field size is decoded using `decode_varint`:

```c
/* Action length */
uint64_t msg_len = decode_varint(&pb_data[i], &skip_bytes, len - i);
i += skip_bytes;

if (i + msg_len > len) {
    return -DECODE_E_EMBMSG_LEN;
}

if ((ret = decode_action(&pb_data[i], &skip_bytes, msg_len, &subtotalfields, queryid - curid, field_number)) != 0) {
    return ret;
}
```

`msg_len` is then directly added to `i`, the current parsing offset. There is no check for integer overflows. Hence, if `msg_len` is very big, `i + msg_len` will overflow, and the parsing function will not detect an error (and will not return `-DECODE_E_EMBMSG_LEN`).

Testcase: see crash-0468e66c3ada831cef39e4b11dcd3d87a0060404

Bug can be fixed by detecting integer overflow like:

```c
if ((i + msg_len > len) || (msg_len > UINT64_MAX - i)) {
    return -DECODE_E_EMBMSG_LEN;
}
```

#### Out of bounds read during Rau to IOTX conversion

The `utils_rau2iotx` function converts Rau to IOTX ($10^{18}$ Rau). First char of the buffer containing the Rau input buffer is always read, even if the input buffer has a 0 length.

Testcase: crash-a943b23db3c2bd946667b5a5a8c2ccea9b07698e

Bug has been fixed by returning NULL if the length of the Rau buffer is 0, which is the return value already used if conversion fails.

## Conclusion

This is a preliminary report. We strongly encourage developers to use a standard Protobuf parser, such as [Nanopb](https://github.com/nanopb/nanopb). Several problems detailed above are hard to tackle with a homemade parser.
