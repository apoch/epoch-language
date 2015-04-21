# Release 14 #

The primary focus of this release was to remove the virtual machine that formerly powered running Epoch programs. This has been accomplished by converting to 100% native code generation via LLVM.

In addition to removing the VM, we have reimplemented critical runtime features such as the garbage collector to work with the new native code framework.

A complete test suite of 62 test programs is included with the [Release 14 installer](https://code.google.com/p/epoch-language/downloads/detail?name=EpochRelease14.exe&can=2&q=).

Additionally, a number of bugs have been addressed in this release; see the [issue tracker](https://code.google.com/p/epoch-language/issues/list) for full details.