# r2-gorecover
## Description
gorecover is a radare2 plugin that can recover symbols from stripped go executables. It could be useful
for ctf challenges, malware analysis or even day to day reverse engineeing of proprietary software.

## Installation and Usage
To install simply clone the repository and run
```
$ make all
$ make install
```

You can then use the plugin from inside radare2 by running the "gorec" command

```
$ r2 test
 -- Welcome, "reenigne"
[0x0044f4d0]> fs
0 2422 * strings
1    1 * symbols
2   26 * sections
3   16 * segments
4    0 * relocs
[0x0044f4d0]> gorec
[gorec] Section .gopclntab at: 0x4c56e0
[gorec] Now resolving 1768 symbols...
[0x0044f4d0]> fs
0 2422 . strings
1 1749 * symbols
2   26 . sections
3   16 . segments
4    0 . relocs
[0x0044f4d0]> s sym.main.main
[0x00482070]> # start your analysis...
```

## Recovery process
The recovery process is simple. Every go executable (from what I have seen) has a .gopclntab. It is a section
containing debug informations about functions (such as virtual address, file of origin, etc...). When a go executable is stripped only the symtab is remove, and from this debugging info we can thus recover the symbols.
The section is structured as such:

```
- magic: u8
- padding_zeroes: u16
- quantum: u8
- usize: u8
- functab_count: u8
- functab_array: [functab entry] * func
- ... whole lot of data we don't care about

# A functab entry is simple 2*usize vars
- functab_entry
    - function_pc: usize
    - function_info_offset: usize
```

To get a symbol address and name the process is really simple. For every entry we keep the function pc and info offset (from section base address). We get the u32 variable at (base + offset + usize), it corresponds to the offset
to the start of the function name string. And that's all !