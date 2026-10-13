# Multithreaded Plugin Pipeline

A modular text-processing pipeline implemented in **C** on Linux, using **POSIX threads**, dynamically loaded shared libraries, and bounded producer–consumer queues.

## Overview

The command-line program dynamically loads a sequence of plugins (`.so` libraries), connects them into a processing pipeline, and passes text through the selected stages. Each stage processes input independently using synchronized producer–consumer queues. The `<END>` marker signals graceful shutdown.

Available plugins: `logger`, `typewriter`, `uppercaser`, `rotator`, `flipper`, and `expander`.

## Technology

C99, GCC, POSIX threads (`pthread`), mutexes and condition variables, `dlopen`/`dlsym`, shell scripting.

## Project structure

```text
main.c                    CLI and dynamic plugin loading
plugins/                  Plugin implementations and shared SDK
plugins/sync/             Monitor and producer–consumer queue components
build.sh                  Build script for executable and shared libraries
test.sh                   Local test suite
test_input.txt            Sample input
```

## Build and run

On a Linux system with GCC and Bash:

```bash
bash build.sh
printf 'hello world\n<END>\n' | ./output/analyzer 5 uppercaser logger
```

The first argument (`5`) is the queue capacity, followed by an ordered list of plugins. Run commands from the repository root because shared objects are loaded from `./output/`.

## Tests

```bash
bash build.sh
bash test.sh
```

The provided local test script contains 22 checks, including plugin combinations, input handling, argument validation, and shutdown. All 22 passed in one Linux test run. This is **not** a guarantee against race conditions, deadlocks under every schedule, or other production-quality issues.

## Attribution and status

Adapted for portfolio presentation from an Operating Systems course assignment. Source-code provenance, any collaborators' contributions, and course publication permissions should be verified before changing the repository from private to public. No license is granted at this stage.
