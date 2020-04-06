---
title: 'xdpacket(1) | General Commands Manual'
order: 1
---

# NAME

xdpacket - the eXtremely Direct Packet handler (in userland)

A very flexible match-execute packet engine:
attach it to one or more interfaces,
it matches packets against your rules
and does whatever you want with them.

# SYNOPSIS

```bash
xdpacket [[-i IP_ADDRESS], ...]  # must run as root or have CAP_NET_RAW

Options:
	-i, --ip IP_ADDRESS	: open a CLI socket on IP_ADDRESS:7044
	-h, --help	        : print usage and exit
```

Invocation opens a YAML REPL on stdin/stdout,
that takes standard xdpacket grammar.

2 listening TCP sockets are opened as well, on `127.0.0.1` and `::1` port `7044`;
for use by e.g. a controller or a Python REPL.

These sockets can be tested with:

```bash
telnet localhost 7044
```

# GRAMMAR

The language chosen for xdpacket's grammar is [YAML](https://yaml.org),
for the following reasons:

-   It is easier to type than JSON or XML, and more readable.
-   The reference implementation [libyaml](https://github.com/yaml/libyaml)
is in C, with a decent API.
-   It can be abbreviated sufficiently to be effective as a CLI language,
avoiding dual implementations for API vs CLI.
See [CLI Usage Notes](#cli-usage-notes) below.

The grammar is intuitive enough that if you are reasonably familiar with YAML,
you will probably pick a lot of it up by reading through some configurations
in the [example dir](../example/README.md),
particularly [syntax.yaml](../example/syntax.yaml).

The following subsections look at the grammar in detail:

### YAML Nomenclature

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

## document toplevel: modes

The toplevel of an xdpacket grammar is the mode,
which tells xdpacket what action it should take on the ruleset in memory:

| mode     | action                                         |
| -------  | ---------------------------------------------- |
| `xdpk`   | add/set the given configuration mappings       |
| `print`  | print full ruleset, or only the given mappings |
| `delete` | delete the give configuration mappings         |

Each of these mappings must then contain a list of one or more of the following
*subsystems*:

| subsystem | description                                                    |
| --------- | -------------------------------------------------------------- |
| `iface`   | an I/O socket opened on a network interface                    |
| `field`   | `(offset, length, mask)` tuple used in matching and read/write |
| `rule`    | a directive for matching and altering packets                  |
| `process` | a list of rules to be executed, in sequence, on an `iface`     |

A YAML document with valid xdacket grammar contains one or more mappings
of the type:

```yaml
mode:
    - subsystem: subsystem_name
```

These are described in detail below:

## Iface

xdpacket sees no packets by default; each desired interface must be declared
as a specific node.

| key     | value  | description           |
| ------- | ------ | --------------------- |
| `iface` | string | system interface name |

```yaml
# to create a new interface, use 'xdpk'
xdpk:
  - iface: eth0  # open a socket for I/O on 'eth0'
```

### Iface Notes

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

1. VLAN tags (IEEE 802.1Q) are always stripped by the kernel and never shown
to raw sockets.
To match VLAN tags, set up an interface for each e.g. `eth0.42`.

## Field

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
- `len: 0` and `offt: 0` will match all packets

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

Note that no processing/alteration is done to incoming packets before matching.

## Rule

A `rule` uses lists of Operations (source-destination tuples, detailed below)
to describe a series of *matches* to test on packets,
and what to do with packets that match *all* of these:

| key     | value  | description                    | default        |
| ------- | ------ | ------------------------------ | -------------- |
| `rule`  | string | user-supplied unique string ID | N/A: mandatory |
| `match` | list   | what to match in packet        | []             |
| `write` | list   | how to modify matching packets | []             |

```yaml
# example rule:
# match any packet, reverse source and destination addresses, set ttl to 1
xdpk:
  - rule: reflect
    match:
      - dst: {field: any}
        src: {field: any}
    write:
      # swap MAC address fields
      - dst: {state: msrc}
        src: {field: mac src}
      - dst: {field: mac src}
        src: {field: mac dst}
      - dst: {field: mac dst}
        src: {state: msrc}
      # swap IP address fields
      - dst: {state: isrc}
        src: {field: ip src}
      - dst: {field: ip src}
        src: {field: ip dst}
      - dst: {field: ip dst}
        src: {state: isrc}
      # set TTL
      - dst: {field: ttl}
        src: {value: 1}
```

A rule is executed in the following sequence:

1. `match`:
    Packet contents must match all Operations given in the `match` sequence.

    Packets that don't match fail the rule and are not processed any further.

    Example:

    ```yaml
    # only packets that match both fields will be processed
    match:
      - dst: {field: eth_proto}
        src: {value: 0x0800}  # IPv4
      - dst: {field: ip_proto}
        src: {value: 0x01}  # ICMP
    ```

1. `write`:
    Each operation is executed in sequence,
    modifying the memory being pointed to.

    Example:

    ```yaml
    # write the literal value `1` to the TTL field
    write:
      - dst: {field: ttl}
        src: {value: 1}
    ```

### Rule source-destination tuples: Operations

Data to be matched or written is described using *operations*:

| element | type   | description                                         |
| ------- | ------ | --------------------------------------------------- |
| `dst`   | memref | area being checked (when matching) or written to    |
| `src`   | memref | comparison (when matching) or source (when writing) |

`dst` and `src` are described with *memref* (memory reference) tuples.
Valid memref tuples:

| element | type    | description                                         |
| ------- | ------- | --------------------------------------------------- |
| `field` | field   | where in the `src` or `dst` to match/write to/from  |
| `state` | name    | named global memory region for pesistent state      |
| `value` | literal | literal bytes to match against or write to packet   |

Some key points of note:

1. If a location (`state` or `value`) is not explicitly given,
    the packet under processing is referenced.

    Example:

    ```yaml
    # write the literal value `1` to the TTL field
    write:
      - dst: {field: ttl}  # destination: 'ttl' field of the packet itself
        src: {value: 1}  # source: literal value '1' stored in memory
    ```

1. It is not necessary to specify `field` for both `src` and `dst`,
    if only one field is specified it will be used for both.

    Example:

    ```yaml
    write:
      - dst: {state: msrc}  # 'mac src' field is used when writing into 'msrc'
        src: {field: mac src}  # source refers to the 'mac src' field of packet
    ```

1. When writing to or reading from `state`, the `offt` (offset) value
    of `field` is ignored and an offset of `0` if always used.

    Example:

    ```yaml
    write:
      - dst: {state: msrc}  # writes to offset 0 of 'msrc'
        src: {field: mac src}  # reads from offset 6 in packet
      - dst: {field: mac dst} # writes to offset 0 in packet
        src: {state: msrc} # reads from offset 0 of 'msrc'
    ```

1. `state` buffers are *global*, meaning they can be accessed by any rule.
    - Multiple rules can store to or copy from the same state buffer
    - Copies will see the latest store only
    - Stores and copies are atomic (a copy will not see a half-formed store)
    - Copies from a buffer that has not had a store will see zeroes

1. A `value` is a read-only memory location and cannot be used as a `dst`.

1. `value` is parsed depending on its content;
    e.g. `192.168.0.1` is parsed as an IPv4 address, not a string.

    This is done for simplicity and intuitiveness,
    see [value.c](../src/value.c) for exact regex matches and sequence used.

    Examples:

    ```yaml
    # example field-value pairs
    - 192.168.1.1                           # 4B IPv4 address
    - aa:bb:cc:dd:ee:ff                     # 6B MAC address
    - 2001:db8:85a3:8d3:1319:8a2e:370:7348  # 16B IPv6 address
    - 1                                     # integer
    - 0x40                                  # integer
    - 0xdeadbeef                            # hex-encoded byte array
    - "HTTP/1.1 200 OK"                     # string
    ```

### Rule Notes

1. All rule elements are executed in sequence, there is no error if a later
    instruction overwrites some portion of the packet that was written
    to by an earlier instruction.

## Process

A `process` applies rules to packets incoming on an interface,
specifies the order in which rules are applied,
and specifies the output interface for packets which match and successfully
execute each rule.

| key       | value  | description                    | default        |
| -------   | ------ | ------------------------------ | -------------- |
| `process` | iface  | ID of a valid `iface`          | N/A: mandatory |
| `rules`   | list   | sequence of rule-output tuples | []             |

A `rule-output` tuple (`rout`):

| element  | type   | description           |
| -------  | ------ | --------------------- |
| `rule`   | string | ID of a valid `rule`  |
| `output` | string | ID of a valid `iface` |

```yaml
# Example Process:
# 1. Process all packets incoming on 'enp0s3'.
# 2a.Those packets which match and successfully execute 'rule a'
#    are routed out of 'enp0s3'.
# 2b.Those packets which match and successfully execute 'rule b'
#    are routed out of 'enp0s8'.
# 3. Packets not matching any rules are discarded.
xdpk:
  - process: enp0s3
    rules:
      - rule a: enp0s3
      - rule b: enp0s8
```

### Process Notes

1. Rules are processed in the sequence given:
    - If a packet does not match a rule, the next rule is matched.
    - Once a packet matches a rule, that rule is executed and the packet
      is then discarded.
    - If a packet matches no rules, it is discarded.

1. When processing a `rules` sequence:
    - A rule which fails to match results in the next rule being checked.
    - A rule which fails to execute (`store`, `copy` and `write` stages)
      results in the packet being discarded.

    This is evident in the difference between `matches` and `writes` when
    printing process status:

    ```yaml
    # input to xdpacket
    print:
    - process: enp0s3

    # output from xdpacket
    ---
    print:
    - process: enp0s3
      nodes:
      - reflect: enp0s3
        matches: 2036
    errors: 0
    ...
    ```

# CLI usage notes

Modes and subsystems have abbreviations to reduce CLI typing requirements:

| mode     | three-letter | one-letter |
| -------  | ------------ | ---------- |
| `xdpk`   | `add`        | `a`        |
| `print`  | `prn`        | `p`        |
| `delete` | `del`        | `d`        |

| subsystem | one-letter |
| --------- | ---------- |
| `iface`   | `i`        |
| `field`   | `f`        |
| `rule`    | `r`        |
| `process` | `p`        |

| `rule` directive | one-letter |
| ---------------- | ---------- |
| `match`          | `m`        |
| `state`          | `t`        |
| `store`          | `s`        |
| `copy`           | `c`        |
| `write`          | `w`        |

Note that YAML allows a more terse format, similar to JSON:

```yaml
p: []  # print entire ruleset in memory
a: [i: enp0s3]  # add interface 'enp0s3'
a: [{f: ttl, offt: 22, len: 1}]  # add a 'ttl' field pointing to Byte 22
```

# AUTHORS

- Sirio Balmelli
- Alan Morrissett

# SEE ALSO

[xdpacket on github](https://siriobalmelli.github.io/xdpacket/)
