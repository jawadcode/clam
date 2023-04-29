# clam

Functional, bytecode interpreted language written in C

## Usage Instructions:

### Build:

```bash
meson setup builddir -Dbuildtype=release
meson compile -C builddir
```

### Run:

```bash
./builddir/clam
````

## Credits

Lots of the interpreter's frontend code and general semantics are inspired by [Clox (from Crafting Interpreters)](https://www.github.com/munificent/craftinginterpreters/tree/master/c), so huge thanks to [Bob Nystrom](https://www.github.com/munificent) for his awesome book.