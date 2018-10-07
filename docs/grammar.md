---
title: xDPacket Grammar
order: 1
---

# xDPacket Grammar

xDPacket parses valid YAML using [LibYAML](https://github.com/yaml/libyaml).

Reasons for the YAML approach:

1. JSON is valid input:
- This is straight compatibility with every web API ever.

```
{
"len": 2,
"mask": "0xff",
"offt": 56,
"val": "some \"string\" 'so-called' @;|$"
}
```

1. YAML is valid input:
- Every *other* web API we care about (let XML perish please).

```
---
len: 2
mask: "0xff"
offt: 56
val: "some \"string\" 'so-called' @;|$"
```

1. YAML lends itself to one-liners:
- It's best if sysadmins don't have to type longform rules
- Command-line rule manipulation must be terse but legible

```
{ len: 2, mask: 0xff, offt: 56, val: "some \"string\" 'so-called' @;|$" }
```

NOTES:

- matchers must be in user-defined sequence (linked list?)
- "mask" is optional - if not included assumed to be `0xff`

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
