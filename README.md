# naesh

a small shell written in C89 that apparently compiles with `-Weverything`. definitely not named after my friend naenae

## building

requires `clang` and `ninja`.

```
ninja -t clean && ninja
./out/naesh
```

## features

- raw mode line editing (arrow keys, backspace, ctrl-a/e/k/l)
- command history (up/down, ctrl-p/n, page up/down)
- single and double quoting with backslash escapes in double quotes
- pipelines (`|`)
- PATH lookup
- builtins: `cd`, `exit`, `history`
- signal handling (SIGINT, SIGTERM, SIGQUIT)
- more in [TODO](TODO.md)

## usage

```
$ echo hello | tr a-z A-Z
HELLO

$ ls | grep src | wc -l
```

## note
all code lives in headers and is compiled as one translation unit via `master.c`. this is intentional btw, known as unity builds.

## license

naesh is licensed under the [GNU General Public License Version 3](LICENSE)
