---
title: xDPacket
order: 100
---

# xDPacket [![Build Status](https://travis-ci.org/siriobalmelli/xdpacket.svg?branch=master)]

the eXtremely Direct Packet handler (in userland)

- [on github](https://github.com/siriobalmelli/xdpacket)
- [as a web page](https://siriobalmelli.github.io/xdpacket/)

A very flexible match-execute packet engine:
attach it to one or more interfaces,
it matches packets against your rules
and does whatever you want with them.

- It knows nothing about protocols: it just sees packets as fields of bytes.
- It sees *every* packet, broadcast, multicast, non-IP: everything.
- It drops everything it can't match against your rules:
think of it as a reverse-firewall.
- It can do any brilliant or stupid or intentionally
wrong thing you can think of.

Example usages:

- Passing Ethernet PAUSE frames between L2 segments.
- Transparently connecting only 2 hosts between separate L2 segments.
- Selective NATing of broacast or multicast frames (e.g. mDNS, Bonjour)
between L3 networks.
- Refelcting/tapping (making a second copy) every packet received on an interface
and shunting it off to a second L2 segment for IDS/analysis.

## Quick Start

1. Build or install xdpacket:
    -   from source with [meson](https://mesonbuild.com):
        ```bash
        git clone https://github.com/siriobalmelli/xdpacket.git
        cd xdpacket
        meson --buildtype=release build-release
        cd release
        ninja
        sudo ninja install
        ```

    -   with [nix](https://nixos.org/nix/):
        TODO: confirm merged into nixpkgs Master
        ```bash
        nix-env --install xdpacket
        ```

    -   on Debian/Ubuntu:
        TODO: ppa info
        ```bash
        sudo apt-get install xdpacket
        ```

    -   on Fedora/RedHat:
        *TODO*

1. Run `sudo xdpacket` and you are in a YAML REPL which takes the
    [xdpacket grammar](man/xdpacket.md).

1. Input a very basic set of directives (see the [examples dir](../example/README.md)),
    for example:

    ```yaml
    ---
    # NOTE: your interface name may vary
    xdpk:
      - iface: enp0s3
      - field: ethertype
        offt: 12
        len: 2
      - field: ttl
        offt: 22
        len: 1
      # match any IP packet; set it's TTL to 1
      - rule: set ttl
        match:
          - ethertype: 0x0800
        write:
          - ttl: 1
      # "reflect" IP packets back out 'enp0s3' with ttl set to 1
      - process: enp0s3
        rules:
          - set ttl: enp0s3
    ...
    ```

1. Print process stats to verify it's functional:

    ```yaml
    # input to xdpacket:
    ---
    print:
    - process: enp0s3
    ...

    # output from xdpacket
    ---
    print:
    - process: enp0s3
      nodes:
      - set ttl: enp0s3
        matches: 4
    errors: 0
    ...
    ```

## Raison D'etre

This project grew out of the desire to manipulate network packets in novel
and interesting ways, and the dismay at discovering that both OpenFlow and P4
could not match or mangle packet payloads past the first few bytes.

The linux netfilter architecture is also of limited help, since it depends on
ip routing, so cannot be used to e.g. NAT multicast packets between subnets
without installing and configuring a routing daemon.

eBPF on the other hand is extremely powerful but the toolchain still has
teething pains and the compile-insert workflow doesn't lend itself well
as an API.

xdpacket's purposes are to:

-   Provide a terse, clean grammar with which to represent
    arbitrary matching and manipulation of packets.

-   Expose a full-featured API to allow control by higher-level services.

-   Expose a CLI interface for manual control and rule testing/debugging.

-   Be performant and lean (hot path with *no* mallocs, cache-optimized
    data structures and sane algorithms).

## Manual

[The man page](man/xdpacket.md) for xdpacket is in markdown at [man/xdpacket.md]();
it is also available on the command line with `man 1 xdpacket`.

## Codebase Notes

1. The build system is [Meson](https://mesonbuild.com/index.html).
    If you don't know Meson yet don't be discouraged, it's the easiest of the lot.
    -   Start by scanning through the toplevel file [meson.build]()
    -   Read [bootstrap.sh]() to see how Meson is used

1. Functionality is broken down into "subsystems". A subsystem has:
    -   a header in [include]() e.g. [include/field.h]()
    -   a src file in [src]() e.g. [src/field.c]()
    -   a test file in [test]() e.g. [test/field_test.c]()
    -   all exported/visible (i.e. present in the header file) symbols
        beginning with the subsystem name e.g. `field_new()`

1. This codebase is targeted at Linux on all architectures:
    -   use linux-specific extensions as desired
    -   avoid architecture-specific code without a generic fallback

1. Use of the word *set*:
    In this codebase the word *set* is used to describe structures which have
    been "compiled" or "parsed" or "packed" for use in matching/handling packets.

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

## BUGS

1. xdpacket does not currently handle IP fragmentation.
    Large fragmented IP packets incoming *may* be reassembled by the network
    driver and kernel and delivered to xdpacket, but when mangled
    and/or forwarded by xdpacket will be dropped by the receiving RAW socket.

    The fix for this will entail:
    - ascertaining whether the kernel will *always* reassemble fragmented
      IP packets on the way in (or whether we need to conditionally handle
      fragmentation).

    - reading the RFC and generating fragmented packets when required
      (a better understanding of PMTU will be necessary here).

## TODO

1. packaging

1. checksums validated in all cases (IPv6)

1. Fix CLI I/O:
    - BUG: SIGPIPE from a client FD kills entire program
    - piping a set of commands to xdpacket at startup is broken

1. REPL reworked for CLI style usability
    - backspace
    - arrow keys
    - tab completion
    - is there a library for this

1. Refine printing:
    - implement regex matching of print names

1. Produce a canonical AppArmor profile (use `audit` on a full run)
    and add to repo.

1. Multithreading:
    - fence API/CLI parser instances from each other with a mutex.

    - modify `process` installation into `iface`, and consumption thereof,
      to be an atomic pointer swap.

    - Protect contentious state operations (`fref`) with a mutex.
      Consider a naive RCU-like behavior rotating a circular buffer
      of write regions.

    - modify `iface` to be a thread spawn, carefully audit init()/free() paths

    - round-robin pin each new `iface` to one of the available CPUset;
      pin the socket to this interface as well with an `ioctl`.

1. migrate from AF_PACKET to AF_XDP as soon as it is upstreamed
    - <https://patchwork.ozlabs.org/cover/867937/>
    - <https://fosdem.org/2018/schedule/event/af_xdp/attachments/slides/2221/export/events/attachments/af_xdp/slides/2221/fosdem_2018_v3.pdf>

1. Replace all `_JQ` "queue-type" lists with a generic packed list tool
(see if glibc provides something, or otherwise implement in nonlibc?).

## other useful projects

1. <https://github.com/netsniff-ng/netsniff-ng>.
1. <https://github.com/newtools/ebpf>
1. <https://github.com/openvswitch/ovs>
1. <https://github.com/p4lang/PI>
