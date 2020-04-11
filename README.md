# RiscOS.ea-convert
Renames the files in a directory with matching entries in a RISCOS.EA file to have the correct riscos file type suffix (",xxx")

The code only uses standard C routines so although it is currently supplied with a Visual Studio solution to build it, it should build using other compilers without too much work.

## Example Usage
It can be run from a windows command line to find directories and process them with a command like:

Convert all the directories in the current directory
```
for /D %i in (*) do EA-Convert %i
```

or convert the entire directory tree (including subdirectories) starting in the current directory

```
for /R %i in (.) do EA-Convert %i
```

## To Do

* Currently it will always append the filetype to the file unless it is the default filetype (text) even if it already has a valid PC file extension. eg `intro8.wav` is renamed to `intro8.wav,fe4`.
