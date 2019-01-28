---
title: xDPacket
order: 100
---

# xDPacket [![Build Status](https://travis-ci.org/siriobalmelli/xdpacket.svg?branch=master)]

the eXtremely Direct Packet handler (in userland)

- [on github](https://github.com/siriobalmelli/xdpacket)
- [as a web page](https://siriobalmelli.github.io/xdpacket/)

## Raison D'etre

TODO

## Grammar

TODO: currently the following files show the grammar in action;
from them it should be relatively easy to intuit (until proper docs are prepared):

- [checksums.yaml](docs/checksums.yaml)
- [mdns.yaml](docs/mdns.yaml)
- [mirror.yaml](docs/mirror.yaml)
- [reflection.yaml](docs/reflection.yaml)

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

1. regex-matcher fields with backreference 'write'

1. Mechanism to store/write state

1. write-side fvals pointing to other fields (not clobbered by cross/writing)

1. masquerade/snat flag on interfaces

1. checksums validated in all cases

1. REPL reworked for CLI style usability
    - backspace
    - arrow keys
    - tab completion
    - is there a library for this

1. Port over test cases, extenguish `src_old`, `include_old` and `test_old`
    (previous attempt under naive architecture assumptions).

1. Documentation and man pages

1. packaging

1. Sanitizers for debug/testing builds

1. migrate from AF_PACKET to AF_XDP as soon as it is upstreamed
  - <https://patchwork.ozlabs.org/cover/867937/>
  - <https://fosdem.org/2018/schedule/event/af_xdp/attachments/slides/2221/export/events/attachments/af_xdp/slides/2221/fosdem_2018_v3.pdf>

## Consider

1. Merging with <https://github.com/netsniff-ng/netsniff-ng> which already
  does a lot of what this program does, except better
