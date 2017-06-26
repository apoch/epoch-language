# MSFViewer

`MSF` stands for *Multi-Stream File*, a container format that is used by Program Database (PDB) files on Windows. PDB files contain crucial metadata for debugging programs built with Microsoft Visual Studio. The Epoch programming language project plans to emit PDB data to allow programs written in Epoch to be debugged in Visual Studio, WinDbg, and other compatible Windows debuggers.

More information on the format can be found on [Microsoft's GitHub repository](https://github.com/Microsoft/microsoft-pdb), although much of it is incomplete. Another superb resource is the `llvm-pdbdump` tool that comes with LLVM distributions.

The goal of MSFViewer is to make viewing and understanding PDB data easier. Each "stream" of the MSF container is viewable as a contiguous sequence of bytes using a hex-editor style interface. Where certain fields and structures are well-understood, MSFViewer also provides a more friendly experience for reading the data in a PDB file.

A major motivation for the development of this tool is that the PDB format is tricky and hard to support. Since the official code is currently in a suboptimal state, it can be difficult to know *why* a given PDB works (or doesn't work!) in one of the target debuggers. Since the tools for PDB viewing mostly focus on the *semantic meaning* of PDB data, they are not helpful for understanding the *structural layout* of the files themselves. To address this, MSFViewer provides a simple but effective UI for browsing the low-level details of a PDB file.

## Screenshots

![Analysis View](https://github.com/apoch/epoch-language/raw/master/Images/Screenshots/MSFViewer-DBI-Analysis.png)

Above is shown the "analysis" mode of MSFViewer, which breaks down the known fields and structures of the PDB data file in question. Below is a shot of the hex viewer capabilities of the tool, looking at the same data.

![Hex View](https://github.com/apoch/epoch-language/raw/master/Images/Screenshots/MSFViewer-DBI-Hex.png)


## The MSF Directory

An MSF file is divided into *blocks*, which can take on different sizes (4KB being the common option). However all blocks in a file are the same size. One of the blocks contains a *hint* which points at the actual MSF "directory" data. Inside this directory is a list of *streams*, their sizes, and what blocks comprise the stream.

This mechanism is powerful because it allows the MSF container to interleave blocks from different streams. This minimizes the need to load the entire file into memory and parse it monolithically, which was especially important on computers of the past which had very limited operating RAM. These days the interleaving is more of a curiosity as near as I can tell, and less of a practical concern.

As an aside, tools like Visual Studio can and do interleave streams. However a lot of third-party apps (like LLVM's PDB generator and Epoch's PDB generator) tend to write all the blocks of a stream next to each other for simplicity. Of course in Epoch's case I am generating an entire PDB from whole cloth every build, whereas Visual Studio can actually retain the blocks that aren't changing and speed up build times a bit by doing so.

## Known Fixed Streams

Some streams have known, fixed indices in the directory:

 * **"Old style" MSF directory** - this seems unused but VS2015 still writes to it. It is "safe" to put junk data into this stream, which is what Epoch currently does.
 * **PDB Info Stream** - contains metadata about a PDB file and its component streams.
 * **TPI Stream** - a type information lookup table of some variety. Currently don't know much about this.
 * **DBI Stream** - a workhorse; stores *modules* that correspond (loosely) to input code files for the program.
 * **IPI Stream** - same structure as TPI but differing semantics. Also don't know much about it.

Additionally, the DBI stream contains pointers to other streams, namely *Globals*, *Publics*, and *Symbols*.

## Known DBI Streams

The globals, publics, and symbols streams are known from header data in the DBI stream, which is of course at a fixed index in the directory. All three streams appear to have *CodeView* format data in them, although the publics stream at least has other data structures as well (most of which are not clearly understood yet).

The intent behind separating these into three streams is not entirely clear, nor is the semantic difference between them.
