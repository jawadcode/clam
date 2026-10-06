# clam

Functional, bytecode interpreted language written in C

## Usage Instructions

### Build

#### Release

```bash
meson setup builddir/release --buildtype release
meson compile -C builddir/release
```

#### Debug

##### Linux

Note: The Nix derivation and devShell both have `hardeningDisable = [ "fortify" ];` so that debug builds don't spew a billion fortify source warnings.

```bash
meson setup builddir/debug --buildtype debug -Db_sanitize=address,undefined
meson compile -C builddir/debug
```

#### Windows

Must be done in a VSDevShell with 'C++ Clang Compiler for Windows' enabled.

Note: Address Sanitiser doesn't seem to work with clang on windows yet.

```ps1
meson setup builddir/debug --buildtype debug -Db_sanitize=undefined --native-file=clang-windows.ini
meson compile -C builddir/debug
```

### Run

* Nix: `nix run github:jawadcode/clam`

* Other Linux: `./builddir/{debug,release}/clam`

* Windows: `.\builddir\{debug,release}\clam.exe`

## Credits

The design and implementation of this interpreter is heavily inspired by [Clox (from Crafting Interpreters)](https://www.github.com/munificent/craftinginterpreters/tree/master/c), massive props to [Bob Nystrom](https://www.github.com/munificent) for writing such a useful book.
