# XDPacket grammar

TODO:
	- specify as BNF
	- yacc file?

NOTES:
	- matchers must be in user-defined sequence (linked list?)

rough function descriptions:
	- `#define mask(x) (x<<3)`
	- `#define field(matcher) "parse 'matcher' into a 'struct field'"`
	- `#define hash(obj;obj;obj) "fv1a over each 'obj'"`

## examples

description:
	- "src or dst ip 192.168.0.1"
	- NAT out enp0s3
matchers:
	- a = field(32@ip.saddr)
	- b = field(32@ip.daddr)
action keys:
	- hash(a;"192.168.0.1")
	- hash(b;"192.168.0.1")
actions:
	- output(ifindex("enp0s3"))
	- mangle(output.ip_addr:32@ip.saddr)

description:
	- "src and dst ip 192.168.0.1"
	- drop
matchers:
	- a = field(32@ip.saddr;32@ip.daddr)
action keys:
	- hash(a;"192.168.0.1";"192.168.0.1")
actions:
	- drop

description:
	- "src 192.168.0.0/24"
	- route through eth1
matchers:
	- a = field(24@ip.saddr)
action keys:
	- hash(a;"192.168.0")
actions:
	- output(ifindex("eth1"))

description:
	- "tcp dport 80"
	- `POST http://rx1.nonep.ngs/xml-rpc HTTP/1.1`
	- drop
matchers:
	- a = field(16@tcp.dport;mask(strlen("POST..."))@tcp.payload)
action keys:
	- hash(a;80;"POST...")
actions:
	- drop

description:
	- spoof mDNS from MAC 'x' to MAC 'y' (including last 4 bytes)
	- note that '48' is "mDNS flags"
matchers:
	- a = field(48@eth.src_mac;32@ip.daddr;16@ip.dport;16@48)
action keys:
	- hash(a;'x';224.0.0.251;5353;8400)
actions:
	- output(maciface('y'))
	- mangle(output.ip_addr:32@ip.saddr)
	- mangle(output.ip_addr:32@-4)

description:
	- spoof generic mDNS from MAC 'x' to MAC 'y'
matchers:
	- a = field(48@eth.src_mac;32@ip.daddr;16@ip.dport)
action keys:
	- hash(a;'x';224.0.0.251;5353)
actions:
	- output(maciface('y'))
	- mangle(output.ip_addr:32@ip.saddr)
