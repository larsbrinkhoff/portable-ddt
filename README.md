# Portable DDT, and friends

This is a collection of portable, or implemented partly in C,
implementations of DDT.  They all originate from MIT.

- `ddt.68` is David Bridgham's Portable DDT for VAX.
- `ndgb` is Portable DDT, including a PDP-11 version.
- `nu-ddt` is DDT for the NuMachine; it's not portable but partly in C.

---

### About portable DDT

The variants can be viewed as a matrix. Along one side was the
architecture: it first ran on the PDP-11, and was later moved to (that
I know of) the MC68000, the VAX (from DAB's work), the Intel x86 (I
think; Proteon definitely had an x86 router product) and the AMD
29000. Along the other was the operating environment:

- basic, bare metal (could be used to debug the OS)

- as a process under MOS (shared with the basic one; you could set a
  breakpoint from the MOS instance, and when you hit it, the machine
  would drop into the basic one)

- as a process under Unix, to debug an inferior process (I think this
  was planned, but never done)

- a dump analyzer, running under Unix (i.e. doing reads/writes to a
  file, not memory)

- a cross-net debugger (the main part ran on one machine, talking to a
  'toehold' on the debuggee, which did the read/writes/etc - I'm
  pretty sure this one was never done, although the code was
  structured to allow it)

Not all the boxes in the matrix were filled in; I think the first two were
for almost all the machines (but I'm pretty sure the VAX one never has MOSDDT),
probably not the rest, though.
