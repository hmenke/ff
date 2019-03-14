# ff

Simplified version of *find* using the PCRE and libgit2 libraries.

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
