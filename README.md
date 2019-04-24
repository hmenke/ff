# ff

[![AppVeyor Build Status][appveyor-svg]][appveyor-link]

Simplified version of *find* using the PCRE library.

*ff* has been inspired by [*fd*](https://github.com/sharkdp/fd).

The problem with [*fd*](https://github.com/sharkdp/fd) is that it is
written in Rust and not everyone is willing to download nearly 1GB of
compiler for something that simple.  *ff* on the other hand is written
in C99 and can be compiled with the C compiler that already comes with
your POSIX system.

Some source files used by *ff* are actually quite generic and don't
have any dependencies outside the C, POSIX, and GNU standard libraries
(except the shared header `macros.h`).  These are all stored in the
folder generic and can be reused easily.

## Features

- Convenient syntax: `ff PATTERN` instead of `find -iname '*PATTERN*'`
- Ignores hidden directories and files, by default
- Regular expressions and shell glob patterns
- Parallel directory traversal
- No heavy build system
- Respect `.gitignore`

## Future features (hopefully)

- Command execution
- Exclude files and directories

## Building from source

Building from source is very easy.  The only build dependencies are GCC, make,
and optionally PCRE.  If PCRE is installed, you can build the default `pcre`
target.
```console
$ make
```
If you do not have PCRE and don't want to install it, you can fall back to
POSIX regex by builing the `posix` target instead.
```console
$ make posix
```

[appveyor-svg]: https://ci.appveyor.com/api/projects/status/03dntgenr4yvofrv/branch/master?svg=true
[appveyor-link]: https://ci.appveyor.com/project/hmenke/ff/branch/master
