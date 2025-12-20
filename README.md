# project0

A small, lightweight command-line file archiver written in C.

This repository contains a compact implementation of an archiving/extraction utility (a simple tar-like tool). The code is organized into components for archive creation, extraction, and buffered I/O. It is intended for learning, experimentation, or use on POSIX-like systems.

## Features

- Create simple archives containing multiple files
- Extract files from archives
- Lightweight single-binary CLI implementation
- Minimal dependencies — builds with a standard C compiler (gcc/clang)
- Buffered I/O implementation for efficient read/write

## Repository layout

- main.c        — CLI entrypoint and mode dispatch
- archive.c     — archive creation logic
- archive.h     — archive-related declarations
- extract.c     — archive extraction logic
- iobuf.c/.h    — buffered I/O helpers
- std.c/.h      — small standard helpers / utility functions
- arch_decs.h, ext_decs.h — additional declarations and data structures

## Requirements

- POSIX-compatible OS (Linux, macOS, *BSD)
- C compiler (gcc or clang)
- make (optional, for Makefile)

No external libraries beyond the C standard library are required.

## Build

Compile with gcc / clang.
