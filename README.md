# bread 🍞

`bread` is an expression based, object oriented (?), dynamically typed
scripting language, and an interpreter for that language. The design
goals for `bread` are as follows:

* Get a good grade on my final project in my programming languages class

## Building bread

`bread` has no dependencies outside of the C standard library, so simply run

```
make release
```

to create an optimized build. The generated binary will be ./release/bread.
To install `bread`, run

```
make install
```

By default `bread` will be installed in ~/.local/bin/bread. To run a `bread` file,
the command is

```
bread file.brd
```

To start a REPL, just run `bread` with no arguments.

## Acknowledgements

[Crafting Interpreters](https://craftinginterpreters.com/) for reference/inspiration
in implementing various parts of the interpreter, especially the virtual
machine and garbage collector.

The [Lua](https://www.lua.org/) programming language, for inspiration on syntax
and in the virtual machine.
