# clam

Functional, bytecode interpreted language written in C

## Usage Instructions

### Build

```bash
# Release
meson setup builddir/release --buildtype release
meson compile -C builddir/release

# Debug
# You may also use "--buildtype=debug" but for me this causes weird `_FORTIFY_SOURCE` warnings with clang19Stdenv.
meson setup builddir/debug --buildtype debugoptimized -Db_sanitize=address,undefined
meson compile -C builddir/debug
```

### Run

```bash
./builddir/{debug,release}/clam
````

#### Nix

```bash
nix run github:jawadcode/clam
```

## Credits

The design and implementation of this interpreter is heavily inspired by [Clox (from Crafting Interpreters)](https://www.github.com/munificent/craftinginterpreters/tree/master/c), massive props to [Bob Nystrom](https://www.github.com/munificent) for writing such a useful book.
