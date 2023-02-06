# FUSE File System

Created a FUSE file system for a systems class. Although the extra credit tests tend to crash the virtual machine we were programming on, the basic functionality of the file system is complete. Written by both me and a partner, Alexander Malakov.

Completed 12/11/2022.

- [Makefile](Makefile)   - Targets are explained in the assignment text
- [README.md](README.md) - This README
- [helpers](helpers)     - Helper code implementing access to bitmaps and blocks
- [hints](hints)         - Incomplete bits and pieces that you might want to use as inspiration
- [nufs.c](nufs.c)       - The main file of the file system driver
- [test.pl](test.pl)     - Tests to exercise the file system

## Running the tests

You might need install an additional package to run the provided tests:

```
$ sudo apt-get install libtest-simple-perl
```

Then using `make test` will run the provided tests.


