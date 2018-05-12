---
title: xDPacket Grammar
order: 1
---

# xDPacket Grammar

TODO:

- specify as BNF
- yacc file?

NOTES:

- matchers must be in user-defined sequence (linked list?)
- masks are in *bits*; positions are in *bytes*

## examples

Description:

- "src or dst ip 192.168.0.1"
- NAT out enp0s3

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
	- 16@tcp.dport;328@tcp.payload=80;"POST[...]"
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
