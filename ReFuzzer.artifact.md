# ReFuzzer Workflow Guide

This guide explains how to use the ReFuzzer toolchain to generate, compile, sanitize, and refuzz C++ programs.  
**Replace `~/demo` with your desired working directory.**

---

## 1. Generate C++ Programs

Generate new C++ programs using your chosen model.  
This will create `.cpp` files in the target directory (e.g., `~/demo`):

```sh
./program generate --dir=~/demo
```

---

## 2. Compile the Programs

Compile all `.cpp` files in the directory.  
This will organize files into `~/demo_/correct`, `~/demo_/incorrect`, etc.:

```sh
./program compile ~/demo
```

---

## 3. Sanitize the Compiled Programs

Run sanitizer checks on the compiled programs.  
**Use the `correct` directory from the previous step:**  
(e.g., `~/demo_/correct`)

```sh
./program sanitize --dir=~/demo_/correct
```

This will produce:
- `~/demo_/correct/correct/` (sanitizer-passed files)
- `~/demo_/correct/incorrect/` (files with sanitizer errors)
- `~/demo_/correct/object/` (executables)
- `~/demo_/correct/sanitizer_log/` (logs)

---

## 4. Refuzz (Fix and Re-sanitize)

Run the refuzz process to automatically fix compilation and sanitizer errors using the model.

**Example command:**

```sh
./program refuzz \
  --dir=~/demo \
  --compile-log=~/demo_/log \
  --sanitizer-log=~/demo_/correct/sanitizer_log
```

- `--dir=~/demo` : The original source directory.
- `--compile-log=~/demo_/log` : The log directory from the compile step.
- `--sanitizer-log=~/demo_/correct/sanitizer_log` : The sanitizer log directory from the sanitize step.

---

## Summary

**Order of commands:**
1. `generate` (creates source files)
2. `compile` (sorts files by compilation success)
3. `sanitize` (checks for sanitizer issues)
4. `refuzz` (fixes errors and rechecks)

**Directory flow:**
- Start with `~/demo`
- Compile to `~/demo_/correct`, `~/demo_/incorrect`, etc.
- Sanitize in `~/demo_/correct`
- Refuzz using logs from `~/demo_/log` and `~/demo_/correct/sanitizer_log