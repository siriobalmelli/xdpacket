---
title: xDPacket
order: 100
---

# xDPacket [![Build Status](https://travis-ci.org/siriobalmelli/xdpacket.svg?branch=master)]

the eXtremely Direct Packet handler (in userland)

- [on github](https://github.com/siriobalmelli/xdpacket)
- [as a web page](https://siriobalmelli.github.io/xdpacket/)

## Raison D'etre

## TODO

1. migrate from AF_PACKET to AF_XDP as soon as it is upstreamed
	- <https://patchwork.ozlabs.org/cover/867937/>
	- <https://fosdem.org/2018/schedule/event/af_xdp/attachments/slides/2221/export/events/attachments/af_xdp/slides/2221/fosdem_2018_v3.pdf>

1. "client cookie" or "token" field to allow for rule searches or comments

## Consider

1. Merging with <https://github.com/netsniff-ng/netsniff-ng> which already
	does a lot of what this program does, except better
