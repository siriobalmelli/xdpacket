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
.

## TODO

1. write-side fvals pointing to other fields (not clobbered by cross/writing)

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

1. migrate from AF_PACKET to AF_XDP as soon as it is upstreamed
  - <https://patchwork.ozlabs.org/cover/867937/>
  - <https://fosdem.org/2018/schedule/event/af_xdp/attachments/slides/2221/export/events/attachments/af_xdp/slides/2221/fosdem_2018_v3.pdf>

## Consider

1. Merging with <https://github.com/netsniff-ng/netsniff-ng> which already
  does a lot of what this program does, except better
