# Scripted hex editor
Manually editing hex sometime get annoying, plus it's hard to keep track of what a binary has in it.

Using a script file make it easier to edit file and make it easier to see what's going on with each edit. 

## Usage
```
shed -i <path/to/input/file> -o <path/to/output/file> -s <path/to/script/file> -c <command>
```

- Input is file required.
- Output file default to input file if not provided.
- Script file or command must be provided.
- Script file can contain multiple commands.
- Command argument is ignored if script file is provided.

## Command syntax
Command parsing rule:

- Commands passed by `-c` is treated as if it's a single line in a script file.
- Commands are parsed line-by-line, LF (`'\n'`) is used to end the line.
- `'#'` can be used for commenting. More precisely, `'#'` will be treated as EOL, then the rest of the line is ignored.
- Empty lines are ignored.
- Each line can contain multiple command, splited with `';'`.
- Spaces inside commands are ignored, except for qouted spaces. Space is tested with `isspace` in `ctype.h`.

- Commands must be in the form of:
    ```
    [address] = value
    ```

Address parsing rule:

- Address must use hex.
- Address can be a single value, in which case it's considered the starting point to write to. Example ``[10ab]``, ``[0x10AB]``.
- Address can be a list, in which case the value will be written to every listed address. Example ``[10ab, 0x20CD]``.
- Address can be a slice, in which case the value for assignment must be a single byte that will be assigned to the whole range. Example ``[0x10ab..20CD]``. Slice include both endpoints.
- List of slices (or mixed addresses and slices) are also possible, although it can get confusing so use at your own risk.


Value parsing rule:

- For unqouted values, space are removed, the value are then parsed as bytes written in hex. For example, ``0a 1b 2c 3d`` and `0 x 0 a 1 b 2c3d` are the same values. Each byte must be 2 hexdigits, i.e. leading zero is necessary.
- For qouted values, user need to provide value type as a suffix after the qoute.
- String: 

    - There is no extra type specificier necessary.
    - Interpreted as a narrow strings `"abc", "127.0.0.1"`.
    - The character `"` can be escaped as a double `""`. So `"a""b"` is interpreted as `a"b`.
- The following are possible quotes type but not implemented for now.

    - (not implemented) Integer: interger in decimal followed by type: `"10"i32 "20"i64, "0"i32, "255"i8`.
    - (not implemented) Byte: similar to integer, `"255"b` is the same as `"255"i8`
    - (not implemented) Char: interpret it as a byte using ASCII: `"a"c "b"c`.
    - (not implemented) Wide string: Interepeted as a narrow string when reading, but each character is 16 bits instead of 8 bits. `"abc"w`.

## Notes

`run_all.bat` is an example of how to use this:

- `<client>_<32/64>_libil2cpp.so` should be the relevant `libil2cpp.so` files. `libil2cpp.so` can be obtained by using `apktool` to dump the `apk`.
- The `.shed` scripts are designed to work on a standard 3.12 `apk`, just download from some apk dumping site if you need it.