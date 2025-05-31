Fork of MicroSocks - multithreaded, small, efficient SOCKS5 server.
===================================================================
This fork of MicroSocks adds two options:
- `-c <host>`
- `-C <port>`

These options allow the use of MicroSocks on a server that doesn't allow
incoming connections. On that server you run it with `-c <host>`. Instead of
listening on the port specified with `-p <port>`, it connects to
`<host>:<port>`, and then waits for a socks request to come in.

On the machine where your socks client (e.g., browser) runs, you run
MicroSocks with `-p <port> -C <port2>`. Make sure `<port>` is reachable from
the other machine (e.g., configure a portforward on your router). Configure
your browser to use `<port2>` as the socks proxy (e.g.,
`--proxy-server=socks5://localhost:<port2>`).

The life of a socks connection in this setup:
- The server's MicroSocks connects to the client's MicroSocks.
- The server's MicroSocks waits for a request to come in on that connection.
- The client's MicroSocks waits for a connection to come in from the browser
  (on port2).

(time passes)

- The browser connects to the client's MicroSocks (port2).
- The client's MicroSocks exchanges data with the server's MicroSocks (so
  the browser's socks request is sent to the server).
- The server handles the socks request, and creates a new connection to the
  client's MicroSocks, to get ready for the next connection.

In a diagram, with the arrows indicating how the TCP connections are created:
```
 ┌───────┐   ┌──────────┐   ┌─────────────┐
 │BROWSER│⇒ │  CLIENT  │⇐ │ NAT ROUTER  │
 │       │⇒ │MICROSOCKS│⇐ │W/PORTFORWARD│
 └───────┘   └──────────┘   └─────────────┘
                                ⇑⇑
 ┌───────┐   ┌──────────┐   ┌───────────────────────────┐
 │ SOME  │⇐ │  SERVER  │⇒ │  NAT ROUTER OR FIREWALL   │
 │WEBSITE│⇐ │MICROSOCKS│⇒ │(ONLY OUTGOING CONNECTIONS)│
 └───────┘   └──────────┘   └───────────────────────────┘
```

This fork also sets the TCP buffer sizes to 4 MByte, which is necessary to
achieve good throughput on high-latency links (you can also bump the defaults in
/proc/sys/net, but that requires root).



original README.md
==================


MicroSocks - multithreaded, small, efficient SOCKS5 server.
===========================================================

a SOCKS5 service that you can run on your remote boxes to tunnel connections
through them, if for some reason SSH doesn't cut it for you.

It's very lightweight, and very light on resources too:

for every client, a thread with a low stack size is spawned.
the main process basically doesn't consume any resources at all.

the only limits are the amount of file descriptors and the RAM.

It's also designed to be robust: it handles resource exhaustion
gracefully by simply denying new connections, instead of calling abort()
as most other programs do these days.

another plus is ease-of-use: no config file necessary, everything can be
done from the command line and doesn't even need any parameters for quick
setup.

History
-------

This is the successor of "rocksocks5", and it was written with
different goals in mind:

- prefer usage of standard libc functions over homegrown ones
- no artificial limits
- do not aim for minimal binary size, but for minimal source code size,
  and maximal readability, reusability, and extensibility.

as a result of that, ipv4, dns, and ipv6 is supported out of the box
and can use the same code, while rocksocks5 has several compile time
defines to bring down the size of the resulting binary to extreme values
like 10 KB static linked when only ipv4 support is enabled.

still, if optimized for size, *this* program when static linked against musl
libc is not even 50 KB. that's easily usable even on the cheapest routers.

command line options
--------------------

    microsocks -1 -q -i listenip -p port -u user -P passw -b bindaddr -w wl

all arguments are optional.
by default listenip is 0.0.0.0 and port 1080.

- option -q disables logging.
- option -b specifies which ip outgoing connections are bound to
- option -w allows to specify a comma-separated whitelist of ip addresses,
that may use the proxy without user/pass authentication.
e.g. -w 127.0.0.1,192.168.1.1.1,::1 or just -w 10.0.0.1
to allow access ONLY to those ips, choose an impossible to guess user/pw combo.
- option -1 activates auth_once mode: once a specific ip address
authed successfully with user/pass, it is added to a whitelist
and may use the proxy without auth.
this is handy for programs like firefox that don't support
user/pass auth. for it to work you'd basically make one connection
with another program that supports it, and then you can use firefox too.
for example, authenticate once using curl:

    curl --socks5 user:password@listenip:port anyurl


Supported SOCKS5 Features
-------------------------
- authentication: none, password, one-time
- IPv4, IPv6, DNS
- TCP (no UDP at this time)

Troubleshooting
---------------

if you experience segfaults, try raising the `THREAD_STACK_SIZE` in sockssrv.c
for your platform in steps of 4KB.

if this fixes your issue please file a pull request.

microsocks uses the smallest safe thread stack size to minimize overall memory
usage.
