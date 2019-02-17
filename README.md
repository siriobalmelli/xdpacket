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

## Grammar

The language chosen for xdpacket's grammar is [YAML](https://yaml.org),
for the following reasons:

-   It is easier to type than JSON or XML, and more readable.
-   The reference implementation [libyaml](https://github.com/yaml/libyaml)
    is in C, with a decent API.
-   It can be abbreviated sufficiently to be effective as a CLI language
    (see [CLI usage][CLI usage and abbreviation]), avoiding dual
    implementations for API vs CLI.

The grammar is intuitive enough that if you are reasonably familiar with YAML,
you will probably pick a lot of it up by reading through some configurations
in the [example dir](example/README.md)

- [checksums.yaml](example/checksums.yaml)
- [mdns.yaml](example/mdns.yaml)
- [mirror.yaml](example/mirror.yaml)
- [reflection.yaml](example/reflection.yaml)

The following subsections look at the grammar in detail, please keep
the above examples handy as you read on.

#### YAML Nomenclature

A quick note about the terminology used by the YAML standard:

An "object" or "dictionary" or "key-value pair" is known as a *mapping* in YAML.

A "list" or "array" is known as a *sequence*.

```yaml
# this is a mapping with key 'hello' and value 'world'
hello: world
```

``` yaml
# this is a mapping with key 'the fox' and value '[quick, brown]'
# (NOTE that the value is a sequence, not a scalar)
the fox:
  - quick
  - brown
```

```yaml
# this is a mapping with two keys 'a' and 'b' and corresponding values for each
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

| subsystem | description                                                |
| --------- | ---------------------------------------------------------- |
| `iface`   | an I/O socket opened on a network interface                |
| `field`   | a set of bytes inside a packet                             |
| `rule`    | directives for matching and altering packets               |
| `process` | a list of rules to be executed, in sequence, on an `iface` |

These are described in detail below:

### iface

xdpacket sees no packets by default; each desired interface must be declared
as a specific node.

| key     | value  | description           |
| ------- | ------ | --------------------- |
| `iface` | string | system interface name |

```yaml
xdpk:
  - iface: eth0  # open a socket for I/O on 'eth0'
```

#### NOTES

1. xdpacket uses promiscuous sockets - all packets on the network are received,
    regardless of protocol or routing.

    Packets are also received regardless of firewall (netfilter) configuration.  
    If it's on the wire, xdpacket will see it.

1. All packets visible in xdpacket are *also* visible to the host network stack.
    This can lead to counterintutive behavior e.g. xdpacket processing
    and outputting a packet *and* the host system replying
    to the same packet with an *ICMP unreachable*.

    When xdpacket is used to NAT or forward traffic coming from an interface,
    it may be useful to set a netfilter rule to drop all incoming traffic
    so the host network stack doesn't see it.

    For an interface `eth9`, an [iptables](https://linux.die.net/man/8/iptables)
    rule might look like:
    ```bash
    iptables -A INPUT -i eth9 -j DROP
    ```

    An [nftables](https://wiki.nftables.org/wiki-nftables/index.php/Main_Page)
    rule might look like:
    ```bash
    nft insert rule filter input iif eth9 drop
    ```

### field

A `field` is used to describe a set of bytes in a packet,
both when reading and writing, and give that description an arbitrary name.

| key     | value  | description                    | default        |
| ------- | ------ | ------------------------------ | -------------- |
| `field` | string | user-supplied unique string ID | N/A: mandatory |
| `offt`  | int    | offset in Ethernet Frame       | `0`            |
| `len`   | uint   | length in bytes                | `0`            |
| `mask`  | uchar  | mask applied to trailing byte  | `0xff`         |

- a field will not match if `offt` is higher than the size of the packet
- negative `offt` means offset from end of packet
- a `len` of `0` simply matches whether a packet is at least `offt` long
- `len: 0` at `offt: 0` will match all packets

```yaml
# field examples
xdpk:
  - field: any

  - field: ttl
    offt: 22
    len: 1

  - field: ip dont fragment
    offt: 20
    len: 1
    mask: 0x40
```

```yaml
# Some more field examples.
# NOTE: .pcap files (tcpdump, wireshark) are useful to check offsets, lengths.
# The offsets in a .pcap are exactly those seen by xdpacket.
xdpk:
  - field: mac src
    offt: 6
    len: 6
  - field: ip src
    offt: 26
    len: 4
  - field: mac dst
    offt: 0
    len: 6
  - field: ip dst
    offt: 30
    len: 4
```

It is worthwhile to note that no processing/alteration is done to incoming
packets before matching.
For example, the "mac source" field of a packet with an 802.11q VLAN tag
is at a different offset in the header; xdpacket does nothing to account for
this, it matches and writes precisely where it is told to.

### field-value tuple (fval)

Data to be matched or written is described using *field-value* tuples:

| element | type   | description                                         |
| ------- | ------ | --------------------------------------------------- |
| `field` | string | must correspond to ID of a valid `field`            |
| `value` | string | parsed to a big-endian array of bytes               |

examples:

```yaml
# example field-value pairs
- ip dst: 192.168.1.1                             # 4B IPv4 address
- mac src: aa:bb:cc:dd:ee:ff                      # 6B MAC address
- ipv6 dst: 2001:db8:85a3:8d3:1319:8a2e:370:7348  # 16B IPv6 address
- ttl: 1                                          # integer
- ip dont fragment: 0x40                          # integer
- some field: 0xdeadbeef                          # hex-encoded byte array
- http: "HTTP/1.1 200 OK"                         # string
```

#### NOTES

1. `value` is parsed depending on its content (see above examples);
    e.g. `192.168.0.1` is parsed as an IPv4 address, not a string.

    This is done for simplicity and intuitiveness, see [fval.c](src/fval.c) for
    exact regex matches and sequence used.

1. Since *field-value tuple* is a big mouthful (or keyboardful), these are
    referred to as *fvals* elsewhere, including the sources
    (see [fval.h](include/fval.h)).

### rule

A `rule` uses lists of field-value tuples to describe a series of *matches*
to test on packets, and what to do with packets that match *all* of these
(AND relationship).

| key     | value  | description                           | default        |
| ------- | ------ | ------------------------------------- | -------------- |
| `rule`  | string | user-supplied unique string ID        | N/A: mandatory |
| `match` | list   | fval literals to match in packet      | []             |
| `store` | list   | fields to store into global state     | []             |
| `copy`  | list   | fields to copy from state into packet | []             |
| `write` | list   | fval literals to write to packet      | []             |

Here is an example rule:

```yaml
# match any packet, reverse source and destination addresses, set ttl to 1
xdpk:
  - rule: reflect
    match:
      - any:
    store:
      - mac src: msrc
      - ip src: isrc
      - mac dst: mdst
      - ip dst: idst
    copy:
      - mac src: mdst
      - ip src: idst
      - mac dst: msrc
      - ip dst: isrc
    write:
      - ttl: 1
```

A rule is executed in the following sequence:

1. `match`:
    Packet contents must match all fvals given in the `match` sequence.

    Packets that don't match fail the rule and are not processed any further.

    The implementation:
    - All fields of the fvals are hashed in the packet,
      using a [64-bit fnv1a hash](https://github.com/siriobalmelli/nonlibc/blob/master/include/fnv.h).
    - The resulting hash is then checked against the pre-calculated hash
      of the values given in the fvals.

    Example:

    ```yaml
    # only packets that match both
    match:
      - ip src: 192.168.1.1
      - mac src: aa:bb:cc:dd:ee:ff
    ```

1. `store`:
    For each fval in the `store` sequence: store the contents of `field` into
    the global state buffer `value`.

1. `copy`:

1. `write`:

#### NOTES

1. *TODO*: global state variables

### process

*TODO*: process

#### NOTES

1. *TODO*: rule ordering is important
1. When processing a `rules` sequence:
    - A rule which fails to match results in the next rule being checked.
    - A rule which fails to execute (`store`, `copy` and `write` stages)
      results in the packet being discarded.

    This is evident in the difference between `matches` and `writes` when
    printing process status:

    ```yaml
    ---
    print:
    - process: enp0s3
      nodes:
      - reflect: enp0s3
        matches: 2036
        writes: 1773
    errors: 0
    ...
    ```


### CLI usage and abbreviation

*TODO*: CLI

## Codebase Notes

1. The build system is [Meson](https://mesonbuild.com/index.html).
    If you don't know Meson yet don't be discouraged, it's the easiest of the lot.
    - Start by scanning through the toplevel file [meson.build]()
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

1. Documentation and man pages

1. checksums validated in all cases (IPv6)

1. REPL reworked for CLI style usability
    - backspace
    - arrow keys
    - tab completion
    - is there a library for this

1. Refine printing:
    - implement a "print everything unless specified" model
    - implement regex matching of names

1. Port over test cases, extinguish `src_old`, `include_old` and `test_old`
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

## other useful projects

1. <https://github.com/netsniff-ng/netsniff-ng>.
1. <https://github.com/newtools/ebpf>
1. <https://github.com/openvswitch/ovs>
1. <https://github.com/p4lang/PI>
