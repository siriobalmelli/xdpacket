---
title: 'xdpacket(1) | General Commands Manual'
order: 1
---

# NAME

xdpacket - the eXtremely Direct Packet handler (in userland)

# SYNOPSIS

```bash
xdpacket
```

Invocation opens a YAML REPL on stdin/stdout,
that takes standard xdpacket grammar.

2 listening TCP sockets are opened as well, on `127.0.0.1` and `::1` port 7044;
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

The grammar is intuitive enough that if you are reasonably familiar with YAML,
you will probably pick a lot of it up by reading through some configurations
in the [example dir](../example/README.md).

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

## document toplevel

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

## Iface

xdpacket sees no packets by default; each desired interface must be declared
as a specific node.

| key     | value  | description           |
| ------- | ------ | --------------------- |
| `iface` | string | system interface name |

```yaml
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

It is worthwhile to note that no processing/alteration is done to incoming
packets before matching.
For example, the "mac source" field of a packet with an 802.11q VLAN tag
is at a different offset in the header; xdpacket does nothing to account for
this, it matches and writes precisely where it is told to.

## Field-Value Tuple (fval)

Data to be matched or written is described using *field-value* tuples:

| element | type   | description                                         |
| ------- | ------ | --------------------------------------------------- |
| `field` | string | must correspond to ID of a valid `field`            |
| `value` | string | parsed to a big-endian array of bytes               |

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

### Fval Notes

1. `value` is parsed depending on its content (see above examples);
    e.g. `192.168.0.1` is parsed as an IPv4 address, not a string.

    This is done for simplicity and intuitiveness, see [fval.c](../src/fval.c) for
    exact regex matches and sequence used.

1. Since *field-value tuple* is a big mouthful (or keyboardful), these are
    referred to as *fvals* elsewhere, including the sources
    (see [fval.h](../include/fval.h)).

## Rule

A `rule` uses lists of field-value tuples to describe a series of *matches*
to test on packets, and what to do with packets that match *all* of these:

| key     | value  | description                           | default        |
| ------- | ------ | ------------------------------------- | -------------- |
| `rule`  | string | user-supplied unique string ID        | N/A: mandatory |
| `match` | list   | fval literals to match in packet      | []             |
| `store` | list   | fields to store into global state     | []             |
| `copy`  | list   | fields to copy from state into packet | []             |
| `write` | list   | fval literals to write to packet      | []             |

```yaml
# example rule:
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

    - All fields of the fvals are hashed in the packet,
      using a [64-bit fnv1a hash](https://github.com/siriobalmelli/nonlibc/blob/master/include/fnv.h).
    - The resulting hash is then checked against the pre-calculated hash
      of the values given in the fvals.

    Example:

    ```yaml
    # only packets that match both fields will be processed
    match:
      - ip src: 192.168.1.1
      - mac src: aa:bb:cc:dd:ee:ff
    ```

1. `store`:
    For each fval in the `store` sequence, store the contents of `field` into
    the global state buffer named `value`.

    ```yaml
    # example:
    store:
      # store the contents of 'ip dst' field into global 'idst' state variable
      - ip dst: idst
    ```

1. `copy`:
    For each fval in the `copy` sequence, copy the contents of
    the global state buffer named by `value` into `field` bytes of the packet.

    ```yaml
    # Example: use state variables to swap ip source and destination
    store:
      - ip src: isrc
      - ip dst: idst
    copy:
      - ip src: idst
      - ip dst: isrc
    ```

    State buffers are *global*, meaning they can be accessed by any rule.
    - Multiple rules can store to or copy from the same state buffer
    - Copies will see the latest store only
    - Stores and copies are atomic (a copy will not see a half-formed store)
    - Copies from a buffer that has not had a store will see zeroes

1. `write`:
    For each fval in the `write` sequence, write the literal `value` to `field`
    bytes of the packet.

    Example:

    ```yaml
    # set TTL to 1
    write:
      - ip ttl: 1
    ```

### Rule Notes

1. All rule elements are executed in sequence, there is no error if a later
    instruction overwrites some portion of the packet that was written
    to by an earlier instruction.
    For example, this is legal:

    ```yaml
    # copy the entire IP header (stored e.g. by a previous rule)
    copy:
      - ip header: iphdr
    # clobber the TTL field of the header
    write:
      - ip ttl: 1
    ```

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

# AUTHORS

Sirio Balmelli
Alan Morrissett

# SEE ALSO

[xdpacket on github](https://siriobalmelli.github.io/xdpacket/)
