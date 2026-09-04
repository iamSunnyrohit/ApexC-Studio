
<div align="center">

<img src="./" alt="ApexC Studio Logo" width="128" style="border-radius: 24px;" />

# ApexC Studio

**A lightweight, mobile-native C11 compiler targeting AArch64 (ARM64) assembly with an embedded AST runtime engine.**

[![Platform](https://img.shields.io/badge/Platform-Android_arm64--v8a-0284C7?style=for-the-badge&logo=android&logoColor=white)](#)
[![Language](https://img.shields.io/badge/Standard-C11_Subset-38BDF8?style=for-the-badge&logo=c&logoColor=white)](#)
[![ABI](https://img.shields.io/badge/ABI-AAPCS64-22C55E?style=for-the-badge)](#)
[![UI](https://img.shields.io/badge/UI-Jetpack_Compose-818CF8?style=for-the-badge&logo=jetpackcompose&logoColor=white)](#)
[![License](https://img.shields.io/badge/License-MIT-slate?style=for-the-badge)](#)

<br/>

[Key Highlights](#-key-highlights) •
[Architecture](#-system-architecture) •
[Installation](#-installation) •
[Quickstart](#-example-programs) •
[Building Locally](#-building-locally)

</div>

---

## Overview

**ApexC** is a standalone, mobile-native compiler developed from scratch in pure C, coupled with a Jetpack Compose IDE. Engineered specifically for `arm64-v8a` environments, it avoids third-party parser generators (no Lex, Yacc, or Bison) to demonstrate bare-metal compilation pipeline principles—from lexical tokenization to recursive AST evaluation and native AArch64 machine emission.

---

## Key Highlights

* **Pure Recursive Descent Parser**: Zero toolchain dependencies. Implements an explicit precedence-climbing Abstract Syntax Tree (AST) generator.
* **Dual Execution Model**:
  * **AST Runtime Evaluator**: Interprets control flow, recursive stacks, and lexical scopes directly in memory for instant program return evaluation.
  * **AArch64 Disassembly**: Generates structured, readable AAPCS64-compliant GNU assembly output.
* **Hardware ABI Conformance**: Enforces strict 16-byte stack frame boundaries across ARM64 hardware, properly saving and restoring Frame Pointer (`x29`) and Link Register (`x30`).
* **Systems IDE Frontend**: Monospaced gutter editor with line counts, one-tap code presets, and live disassembly inspectors.

---

## System Architecture

ApexC splits compilation into a high-performance C NDK layer and a Kotlin Compose UI via JNI:

```text
  ┌────────────────────────────────────────────────────────┐
  │                 Jetpack Compose IDE                    │
  │     (Gutter Editor, Presets Selector, Status Terminal) │
  └──────────────────────────┬─────────────────────────────┘
                             │  JNI (native-lib.c)
                             ▼
  ┌────────────────────────────────────────────────────────┐
  │                   ApexC Core Engine                    │
  │                                                        │
  │   [Source Code] ──► Lexer ──► AST Parser               │
  │                                  │                     │
  │                 ┌────────────────┴───────────────┐     │
  │                 ▼                                ▼     │
  │         [AST Evaluator]                 [Codegen AArch64]
  │        Direct Return Code               AAPCS64 Assembly
  └────────────────────────────────────────────────────────┘

```

| Component | Implementation Details |
| --- | --- |
| **Frontend Parser** | Handwritten recursive descent; token scanner, dynamic symbol table |
| **Control Flow** | Conditionals (`b.eq`, `b.ne`, `cmp`, `cset`), while-loops, scoped local offsets |
| **Calling Convention** | AAPCS64 standard (`w0`–`w7` parameter registers, callee-saved stack alignment) |
| **JNI Bridge** | High-throughput `open_memstream` I/O buffering without intermediate file writes |

---

## Example Programs

ApexC handles arithmetic, looping structures, and multi-depth recursion.

### 1. Recursive Fibonacci

```c
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    return fib(7); // Evaluates to 13
}

```

### 2. While-Loop Accumulator

```c
int main() {
    int sum = 0;
    int i = 1;
    while (i <= 10) {
        sum = sum + i;
        i = i + 1;
    }
    return sum; // Evaluates to 55
}

```

---

## Installation

Download the pre-compiled, signed APK directly from the repository releases:

1. Navigate to **[Releases](https://www.google.com/search?q=../../releases)**.
2. Download `ApexC-v1.0.0-arm64.apk`.
3. Sideload onto any Android device running **Android 7.0 (API 24)+** on an ARM64 chipset:
```bash
adb install ApexC-v1.0.0-arm64.apk

```



---

## Building Locally

### Prerequisites

* **Android Studio Ladybug (or newer)**
* **Android NDK** (`r26` or newer)
* **CMake** `3.22.1+`
* **JDK 17+**

### Steps

1. Clone the repository:
```bash
git clone [https://github.com/](https://github.com/)<YOUR_USERNAME>/ApexC.git
cd ApexC

```


2. Sync and assemble the debug build:
```bash
./gradlew assembleDebug

```


3. Locate the compiled APK at:
```text
app/build/outputs/apk/debug/app-debug.apk

```



---

## Repository Structure

```text
ApexC/
├── app/
│   ├── src/
│   │   ├── main/
│   │   │   ├── cpp/
│   │   │   │   ├── apex/
│   │   │   │   │   ├── lexer.[c|h]      # Lexical scanner & token definitions
│   │   │   │   │   ├── parser.[c|h]     # Recursive descent AST parser
│   │   │   │   │   ├── eval.[c|h]       # Dynamic AST interpreter
│   │   │   │   │   └── codegen.[c|h]    # AAPCS64 assembly generator
│   │   │   │   ├── CMakeLists.txt       # NDK build targets
│   │   │   │   └── native-lib.c         # JNI bridge interface
│   │   │   ├── java/.../MainActivity.kt # Jetpack Compose UI & navigation
│   │   │   └── AndroidManifest.xml
│   └── build.gradle.kts
├── logo.png
└── README.md

```

---

## License

This project is licensed under the [MIT License](https://www.google.com/search?q=LICENSE).
