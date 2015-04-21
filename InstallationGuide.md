_**Please note that this page is out of date. For the latest information, see the release notes for the Epoch release you wish to install.**_


# Installation Guide #
Welcome to the Epoch language project! This document will guide you through the process of installing and using the Epoch Software Development Kit. Simply follow the instructions and you'll be ready to write Epoch programs.


## Obtaining the SDK ##
The first thing to do is download the SDK archive. The latest stable version of the SDK is always listed on the [Front Page](http://code.google.com/p/epoch-language/). Additional versions of the SDK can be found on the [Downloads List](http://code.google.com/p/epoch-language/downloads/list).

> ### Note ###
> _Older versions of the SDK may lack features, contain bugs, and generally not be as good as the latest versions. We strongly recommend that you stick to the newest release unless absolutely certain that you need an older version._

For release 10 and later, releases come in two parts: an executable installer which will install the compiler and virtual machine, and the source code for the Epoch release. It is not necessary to download or install the source code to use the Epoch SDK.

The installer should be fairly self-explanatory.


## Testing the Installation ##
Assuming you accepted the default options for the SDK installer, you should find the example pack under Start/Epoch Release 10/Examples. Double-click a .epoch file to open it in a text editor; you can also right-click on a file and choose Execute to run the program directly.


## Congratulations! ##
The Epoch SDK is now installed and ready for use. Enjoy!


## Optional: Building the SDK from Sources ##
Each distribution of the Epoch SDK includes (as an optional component) the complete source code for the SDK. Prior to building the sources, check the file `Dependencies.txt` and ensure that all the required packages are installed.

You may also wish to check out the current source tree to have the latest SDK code; you can find instructions for accessing our repository [here](http://code.google.com/p/epoch-language/source/checkout). Please be warned that these sources are under active development and may not be stable! In general if you are writing software for Epoch we strongly suggest that you use a stable release rather than the development sources.

The SDK is currently designed for use with Microsoft Visual Studio 2005. More recent versions should handle the SDK just fine, once the solution is converted to the updated format.

All configurations should be compilable and usable directly out of the box. For best results, we recommend performing a batch build of all configurations to ensure that all dependencies are installed correctly.

By default, all projects emit their final binaries to the directory `Bin` in the SDK installation folder. All release configurations create their own subdirectory, so you can easily tell different versions apart.