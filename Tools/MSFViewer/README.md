# MSFViewer

`MSF` stands for *Multi-Stream File*, a container format that is used by Program Database (PDB) files on Windows. PDB files contain crucial metadata for debugging programs built with Microsoft Visual Studio. The Epoch programming language project plans to emit PDB data to allow programs written in Epoch to be debugged in Visual Studio, WinDbg, and other compatible Windows debuggers.

More information on the format can be found on [Microsoft's GitHub repository](https://github.com/Microsoft/microsoft-pdb), although much of it is incomplete. Another superb resource is the `llvm-pdbdump` tool that comes with LLVM distributions.

The goal of MSFViewer is to make viewing and understanding PDB data easier. Each "stream" of the MSF container is viewable as a contiguous sequence of bytes using a hex-editor style interface. Where certain fields and structures are well-understood, MSFViewer also provides a more friendly experience for reading the data in a PDB file.
