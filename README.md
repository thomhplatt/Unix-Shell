# Unix-Shell

A Unix-style command-line shell implemented in C, supporting foreground and background process execution, basic job control, signal handling, and command history.

## Overview

This project implements a minimal Unix shell that accepts user commands, executes external programs, and manages process lifecycles using standard POSIX system calls.

The implementation distinguishes between internal commands that must be handled by the shell itself and external commands executed via child processes.

## Features

- Foreground and background command execution
- Process creation using `fork` and execution via `execvp`
- Background job tracking using a linked list
- Child process synchronization with `wait` / `waitpid`
- Signal handling to manage interactive behavior (`SIGINT`)
- Command history and line editing via GNU Readline
- Internal command handling for shell state management

## Process & Job Model

- Each command is executed in a child process created with `fork`
- Foreground processes block the shell until completion
- Background processes execute concurrently and are tracked internally
- Completed background jobs are periodically cleaned up
- The shell ignores `SIGINT` (Ctrl-C) while child processes restore default signal behavior

## Signal Handling

- The shell process ignores `SIGINT` to prevent accidental termination
- Foreground child processes restore default signal handling
- This ensures Ctrl-C interrupts running commands without killing the shell

## Error Handling

The shell performs robust error checking to handle:
- invalid commands
- failed system calls
- incorrect usage or malformed input

Errors are reported without crashing the shell, allowing continued interaction.

## Build & Run

    make
    ./ssi
