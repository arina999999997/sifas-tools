# Metadata String Editor
Rewrite of https://github.com/JeremieCHN/MetaDataStringEditor/blob/master/MetadataFile.cs in C++, with reference from https://github.com/Perfare/Il2CppDumper/blob/master/Il2CppDumper/Il2Cpp/MetadataClass.cs

This is made for SIFAS but it's general enough that it should work for other things too.

# How to use
Direct substitution mode:

```
metadata_string_editor -i <path/to/input/metadata.dat> -o <path/to/output/metadata.dat> -d <path/to/direct/substitution/file>
```

Config exchange mode:

```
metadata_string_editor -i <path/to/input/metadata.dat> -o <path/to/output/metadata.dat> -c <path/to/old/config> <path/to/new/config>
```
 

The output file default to always `./global-metadata.dat` if not provided.

The files can contain non-significant empty lines. More precisely, when seeking for a substitution or a declaration, an empty lines will be ignored.

## Direct substitution mode
The direct substitution mode directly write to a string literal, or directly substitute a string.

It should contain multiple lines, as multiple substitution to be done.

### String substitution
String substitution has 3 parts:

- Declaration of string substitution on one line: `str [separator]`
- The original string on one or more line and `[separator]`
- The new string on one or more line and `[separator]`

The `[separator]` can be set to any string without whitespace, and should be used when the original string contains LF. Otherwise, the LF is used as the separator and the whole structure should only have 3 lines.

For example:

```
str
abc
def
```
Will replace all appearances of `abc` to `def`. Here an appearance is a perfect match the exact string literal.

With separator:

```
str |

a string with a LF before it|
a string with a LF after it
|
```
Will replace all appearances of `\na string with a LF before it` to `a string with a LF after it\n`.

### Id substitution
Often String substitution is good enough, but there are case where you don't want to replace every appearance of a string. In that case, you need to replace the specific string using its id.

The id is saved in the metadata file itself and will not change. `metadata_string_editor` will output the ids of strings it modifies along with the before and after value, so you can run it using string substitution first to see the relevant ids.

An Id substitution has 3 parts:

- Declaration of id substitution on one line: `id <id> [separator]`
- The new string on one or more line and `[separator]`

`[separator]` is the same with string substitution.

For example:

```
id 1337
leet
```

will change the string literal with id `1337` to `leet`.

With separator: 


```
id 1337 '

leet
'
```

will change the string literal with id `1337` to `\nleet\n`.

## Config exchange mode
The config exchange mode imagine the string literals as configuration, and we can change them.

To do so, we define 2 files, old and new configs.

### Old config
The old config has to define the substitutions method like in direct substitution:

```
<name> str [separator]
<original string>[separator]
```

or 

```
<name> id <id>
```

For example:

```
URL str
https://server.old
TITLE id 1337 
```
### New config
The new config has to define the new string to replace the old one.

If a string is defined in the new config then it has to be in the old config. Configs not mentioned will not be changed.

The format is:

```
<name> [separator]
<string>[separator]
```

Note that the separator for the old and new config for the same `<name>` can be different.

## TODO
- Add a feature to search for metadata string(?)
- Add a feature to comments the files.
- Add a feature to have multiple ids(?)

## Notes

`run_all.bat` is an example of how to use this in config exchange mode:

- `<client>_global_metadata.dat` should be the relevant `global-metadat.dat` files. This can be obtained with `apktool`.
- The `.cfg`s are designed to work on a standard 3.12 `apk`.

Furthermore, the `.patch` should work in direct substitution mode.

