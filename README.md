---
title: xDPacket
order: 100
---

# xDPacket [![Build Status](https://travis-ci.org/siriobalmelli/xdpacket.svg?branch=master)]

the eXtremely Direct Packet handler (in userland)

- [on github](https://github.com/siriobalmelli/xdpacket)
- [as a web page](https://siriobalmelli.github.io/xdpacket/)

## Raison D'etre

This project grew out of the desire to manipulate network packets in novel
and interesting ways, and the dismay at discovering that both OpenFlow and P4
could not match or mangle packet payloads past the first few bytes.

xdpacket's purposes are to:

-   Provide a terse, clean grammar with which to represent
    arbitrary matching and manipulation of packets.
-   Expose a full-featured API to allow control by higher-level services.
-   Expose a CLI interface for manual control and rule testing/debugging.
-   Be performant and lean (hot path with *no* mallocs, cache-optimized
    data structures and sane algorithms).

## Grammar

The language chosen for xdpacket's grammar is [YAML](https://yaml.org),
for the following reasons:

-   It is easier to type than JSON or XML, and more readable.
-   The reference implementation [libyaml](https://github.com/yaml/libyaml)
    is in C, with a decent API.
-   It can be abbreviated sufficiently to be effective as a CLI language
    (see [CLI usage][cli_usage_and_abbreviation]), avoiding dual
    implementations for API vs CLI.

The grammar is intuitive enough that if you are reasonably familiar with YAML,
you will probably pick a lot of it up by just reading through some of the
example configurations:

- [checksums.yaml](docs/checksums.yaml)
- [mdns.yaml](docs/mdns.yaml)
- [mirror.yaml](docs/mirror.yaml)
- [reflection.yaml](docs/reflection.yaml)

The following subsections look at the grammar in detail, please keep
the above examples handy as you read on.

### YAML Nomenclature
A quick note about the terminology used by the YAML standard:

An "object" or "dictionary" or "key-value pair" is known as a *mapping*
in YAML.

A "list" or "array" is known as a *sequence*.

```yaml
# this is a mapping with key 'hello' and value 'world'
hello: world
```

``` yaml
# this is a mapping with key 'the fox' and value '[quick, brown]'
the fox:
  - quick
  - brown
```

```yaml
# this is a mapping with two keys 'a' and 'b' and corresponding values
a: word
b:
  - kind
  - effective
```

### document toplevel

A YAML document with valid xdacket grammar contains one or more mappings
with one of these keys:

| mode    | action                                   |
| ------- | ---------------------------------------- |
| `xdpk`  | add/set the given configuration mappings |
| `print` | print configuration                      |
| `del`   | delete configuration mappings            |

Each of these mappings must then contain a list of one or more of the following
*subsystems*:

### iface

*TODO*: iface

### field

*TODO*: field

### rule

*TODO*: rule

### process

*TODO*: process

### CLI usage and abbreviation

*TODO*: CLI

## Usage Notes

1. xdpacket uses promiscuous sockets - all packets on the network are captured.

1. All packets visible in xdpacket are *also* visible to the host network stack.
This can lead to counterintutive behavior e.g. xdpacket processing
and outputting a packet *and* the host system replying with an *ICMP unreachable*.

## Codebase Notes

1. The build system is [Meson](https://mesonbuild.com/index.html).
If you don't know Meson yet don't be discouraged, it's the easiest of the lot.
    - Start by scanning through the toplevel file [meson.build]().
    - Read [bootstrap.sh]() to see how Meson is used

1. Functionality is broken down into "subsystems". A subsystem has:
    - a header in [include]() e.g. [include/field.h]()
    - a src file in [src]() e.g. [src/field.c]()
    - a test file in [test]() e.g. [test/field_test.c]()
    - all exported/visible (i.e. present in the header file) symbols
      beginning with the subsystem name e.g. `field_new()`

1. This codebase is targeted at Linux on all architectures:
    - use linux-specific extensions as desired
    - avoid architecture-specific code without a generic fallback

1. Use of the word *set*:
In this codebase the word *set* is used to describe structures which have
been "rendered" or "parsed" or "packed" for use in matching/handling packets.
Generally speaking, every structure in the program represents an element
in the YAML schema used for the CLI/API.
This makes parsing and logic very straightforward, but if used by itself would
cause a lot of pointer chasing and cache poisoning by pulling in unneeded strings
and references when handling packets.
To solve this, certain structs have a corresponding `[struct name]_set` struct
which is allocated and freed along with their "parent" (non-set) struct.
When the "parse chain" or "hot path" is then built, these `_set` structs are
copied or referenced (as appropriate).
Packets being handled must *only ever* touch *set* structures.
To be clear, the choice of the word *set* is purely arbitrary - other possible
choices such as "bytes", etc would have also worked.

## TODO

1. packaging

1. Documentation and man pages

1. checksums validated in all cases

1. REPL reworked for CLI style usability
    - backspace
    - arrow keys
    - tab completion
    - is there a library for this

1. Refine printing:
    - implement a "print everything unless specified" model
    - implement regex matching of names

1. Port over test cases, extenguish `src_old`, `include_old` and `test_old`
    (previous attempt under naive architecture assumptions).

1. Sanitizers for debug/testing builds

1. Multithreading:
    - fence API/CLI parser instances from each other with a mutex.
    - modify `process` installation into `iface`, and consumption thereof,
      to be an atomic pointer swap.
    - protect contentious state operations (`fref`) with a mutex
    - modify `iface` to be a thread spawn, carefully audit init()/free() paths
    - round-robin pin each new `iface` to one of the available CPUset;
      pin the socket to this interface as well with an `ioctl`.

1. migrate from AF_PACKET to AF_XDP as soon as it is upstreamed
  - <https://patchwork.ozlabs.org/cover/867937/>
  - <https://fosdem.org/2018/schedule/event/af_xdp/attachments/slides/2221/export/events/attachments/af_xdp/slides/2221/fosdem_2018_v3.pdf>

## Consider

1. Merging with <https://github.com/netsniff-ng/netsniff-ng>.
1. An alternate implementation that builds and loads eBPF filters
instead of processing packets in userspace.
