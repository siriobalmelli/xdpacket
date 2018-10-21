---
title: xDPacket Grammar
order: 1
---

# xDPacket Grammar

xDPacket parses valid YAML using [LibYAML](https://github.com/yaml/libyaml).

Reasons for the YAML approach:

1. JSON is valid input:
    - This is straight compatibility with every web API ever.
    ```json
    {
    "len": 2,
    "mask": "0xff",
    "offt": 56,
    "val": "some \"string\" 'so-called' @;|$"
    }
```
1. YAML is valid input:
    - Every *other* web API we care about (let XML perish please).
    ```yaml
    ---
    len: 2
    mask: "0xff"
    offt: 56
    val: "some \"string\" 'so-called' @;|$"
    ```

1. YAML lends itself to one-liners:
    - It's best if sysadmins don't have to type longform rules
    - Command-line rule manipulation must be terse but legible
    ```yaml
    { len: 2, mask: 0xff, offt: 56, val: "some \"string\" 'so-called' @;|$" }
    ```

## Command Set

Input is parsed as either:
    - a single `command` object
    - a list of `command` objects inside an `xdpk` object

```yaml
# single command: add an interface
if: { add: eth0 }

# multi-command list example
xdpk: [ { if: { add: eth0 } }, { if: { add: eth1 } } ]
```

```yaml
# Some more examples showing various commands.
# until such time as a proper spec is completed,
# this is the "unofficial guide" to xdpk grammar

xdpk:
  - if:
    - add: eth0  # capture packets on eth0
    - show: eth.*  # show counters for any "eth" interface (POSIX EREs)
    - del: eth0  # stop capture and output on eth0

  - field:
    - add: joes_ip
      offt: ip.saddr
      len: 4
      val: 192.168.1.1
    - add: isp
      offt: ip.daddr
      len: 2
      val: 10.68
    - add: flagged
      offt: tcp.flags
      len: 1
      mask: 0x80
      val: 0x80
    - show: .*

  - action:
    - add: reflect
      out: in.if  # send packet back to input interface
    # NOTE that 'action' semantics are identical to those of 'field'
    - add: set_flag
      offt: tcp.flags
      len: 1
      val: 0xC0

  - match:
    add: to_joe
    if: eth0
    dir: in
    sel: [joes_ip, isp]  # joes_ip || isp
    act: reflect

  - match:
    add: to_isp
    if: eth1
    dir: out
    sel: [joes_ip, [joes_isp, flagged]]  # joes_ip || (joes_isp && flagged)
    act: set_flag
```

NOTES:

- "mask" is optional - if not included assumed to be `0xff`

NOTE: EVERYTHING BELOW HERE IS OLD AND NEEDS REWRITING

## examples

1. Matching multiple IP addresses, simple NAT:

```yaml
---
# 'match' is a list of matchers; *any* of which may match (OR)
match:
  # a matcher is a list of fields which must *all* match (AND)
  -
    # this is a field
    - len: 4
      offt: ip.saddr
      val: "192.168.0.1"
    - len: 4
      offt: tcp.dport
      val: 80
  # this is another matcher
  -
    - len: 4
      offt: ip.daddr
      val: "192.168.0.1"

action:
  - mangle:
      from:
        if: enp0s3
        attr: ip
      to:
        len: 4
        offt: ip.saddr
  - out:
      if: enp0s3
```

TODO: rewrite all other examples with this grammar:

1. matchers:
  - 32@ip.saddr="192.168.0.1"
  - 32@ip.daddr="192.168.0.1"
1. actions:
  - output(ifindex("enp0s3"))
  - mangle(output.ip_addr:32@ip.saddr)

Description:

- "src and dst ip 192.168.0.1"
- drop

1. matchers:
  - 32@ip.saddr;32@ip.daddr="192.168.0.1";"192.168.0.1"
1. actions:
  - drop

Description:

- "src 192.168.0.0/24"
- route through eth1

1. matchers:
  - 24@ip.saddr="192.168.0"
1. actions:
  - output(ifindex("eth1"))

Description:

- "tcp dport 80"
- constains string `POST http://hostname.com/xml-rpc HTTP/1.1`
- drop

1. matchers:
  - 16@tcp.dport;328@tcp.payload=80;"POST..."
1. actions:
  - drop

Description:

- spoof mDNS from MAC `x` to MAC `y` (including last 4 bytes)
- note that '48' is "mDNS flags"

1. matchers:
  - 48@eth.src_mac;32@ip.daddr;16@ip.dport;16@48=`x`;224.0.0.251;5353;8400
1. actions:
  - output(maciface(`y`))
  - mangle(output.ip_addr:32@ip.saddr)
  - mangle(output.ip_addr:32@-4)

Description:

- spoof generic mDNS from MAC `x` to MAC `y`

1. matchers:
  - 48@eth.src_mac;32@ip.daddr;16@ip.dport=`x`;224.0.0.251;5353
1. actions:
  - output(maciface(`y`))
  - mangle(output.ip_addr:32@ip.saddr)
