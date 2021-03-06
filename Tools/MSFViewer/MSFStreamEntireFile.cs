﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class MSFStreamEntireFile : MSFStream
    {
        public MSFStreamEntireFile(byte[] rawbuffer)
            : base(rawbuffer)
        {
            Streams = new List<MSFStream>();
            KnownStreamModules = new Dictionary<int, ModInfo>();
            
            ValidMagic = ParseMagic();
            ParseSuperBlock();
            ParseBlockMap();
            ParseDirectoryHint();
            ParseDirectory();

            RawBlocks = MSFBlock.CopyBlockData(rawbuffer, BlockSize.ExtractedValue);
        }


        public void RegisterDBIStreams(int globals, int publics, int symbols)
        {
            KnownStreamGlobals = globals;
            KnownStreamPublics = publics;
            KnownStreamSymbols = symbols;
        }

        public void RegisterDBIModuleStream(int modi, int streamindex, uint symbytes, uint linesbytes, uint c13linesbytes)
        {
            // TODO - understand the role of invalid stream index better
            if (streamindex < 0)
                return;

            KnownStreamModules.Add(modi, new ModInfo {
                StreamIndex = streamindex,
                NumBytesSymbols = symbytes,
                NumBytesLines = linesbytes,
                NumBytesC13Lines = c13linesbytes
            });
        }

        public List<MSFStream> Streams;

        private MSFStreamPDBInfo PDBInfoStream;
        private bool ValidMagic;

        private TypedByteSequence<int> BlockSize;
        private TypedByteSequence<int> DirectoryBlock;
        private TypedByteSequence<int> FreeBlockMapIndex;
        private TypedByteSequence<int> BlockCount;
        private TypedByteSequence<int> DirectoryStreamLength;
        private TypedByteSequence<int> HintBlock;
        private TypedByteSequence<int> StreamCount;
        private TypedByteSequence<int> Unknown;

        private int KnownStreamGlobals = -1;
        private int KnownStreamPublics = -1;
        private int KnownStreamSymbols = -1;

        class ModInfo
        {
            public int StreamIndex;
            public uint NumBytesSymbols;
            public uint NumBytesLines;
            public uint NumBytesC13Lines;
        }

        private Dictionary<int, ModInfo> KnownStreamModules;


        protected override void SubclassPopulateAnalysis(List<ListViewItem> lvw, ListView lvwcontrol, TreeView tvw)
        {
            var blockgroup = AddAnalysisGroup(lvwcontrol, tvw, "blocks", "Blocks");
            AddAnalysisItem(lvw, tvw, "Block size", blockgroup, BlockSize);
            AddAnalysisItem(lvw, tvw, "Block count", blockgroup, BlockCount);

            var directorygroup = AddAnalysisGroup(lvwcontrol, tvw, "directory", "Directory");
            AddAnalysisItem(lvw, tvw, "Directory hint block", directorygroup, HintBlock);
            AddAnalysisItem(lvw, tvw, "Directory data block", directorygroup, DirectoryBlock);
            AddAnalysisItem(lvw, tvw, "Directory stream length", directorygroup, DirectoryStreamLength);

            var additionalgroup = AddAnalysisGroup(lvwcontrol, tvw, "additional", "Additional Data");
            AddAnalysisItem(lvw, tvw, "Magic header", additionalgroup, new MaskedByteSequence(FlattenedBuffer, 0, 32, ValidMagic ? "Valid!" : "Invalid!"));       // TODO - magic length
            AddAnalysisItem(lvw, tvw, "Free block map", additionalgroup, FreeBlockMapIndex);
            AddAnalysisItem(lvw, tvw, "Unknown data field", additionalgroup, Unknown);

            var streamgroup = AddAnalysisGroup(lvwcontrol, tvw, "streams", "Streams");
            AddAnalysisItem(lvw, tvw, "Stream count", streamgroup, StreamCount);
        }

        private bool ParseMagic()
        {
            string MAGIC = "Microsoft C/C++ MSF 7.00";
            string filemagic = ExtractStringWithLength(MAGIC.Length).ExtractedValue;

            if (filemagic.CompareTo(MAGIC) != 0)
                return false;


            if ((ExtractByte().ExtractedValue != 13) ||            // Carriage return
                (ExtractByte().ExtractedValue != 10) ||            // Linefeed
                (ExtractByte().ExtractedValue != 26) ||            // Magic?
                (ExtractByte().ExtractedValue != 68) ||            // 'D'
                (ExtractByte().ExtractedValue != 83) ||            // 'S'
                (ExtractByte().ExtractedValue != 0) ||             // Null magic
                (ExtractByte().ExtractedValue != 0) ||             // Null magic
                (ExtractByte().ExtractedValue != 0)                // Null magic
            )
            {
                return false;
            }

            return true;
        }

        private void ParseSuperBlock()
        {
            BlockSize = ExtractInt32();
            FreeBlockMapIndex = ExtractInt32();
            BlockCount = ExtractInt32();
            DirectoryStreamLength = ExtractInt32();
            Unknown = ExtractInt32();
            HintBlock = ExtractInt32();
        }

        private void ParseBlockMap()
        {
            // TODO
        }

        private void ParseDirectoryHint()
        {
            ReadOffset = BlockSize.ExtractedValue * HintBlock.ExtractedValue;
            DirectoryBlock = ExtractInt32();
        }

        private void ParseDirectory()
        {
            ReadOffset = BlockSize.ExtractedValue * DirectoryBlock.ExtractedValue;
            StreamCount = ExtractInt32();

            var streamsizes = new int[StreamCount.ExtractedValue];
            for (int i = 0; i < StreamCount.ExtractedValue; ++i)
            {
                streamsizes[i] = ExtractInt32().ExtractedValue;
            }

            var blocks = new List<List<int>>();
            for (int i = 0; i < StreamCount.ExtractedValue;  ++i)
            {
                int extrablocks = streamsizes[i] / BlockSize.ExtractedValue;

                var blocklist = new List<int>();
                if (streamsizes[i] % BlockSize.ExtractedValue > 0)
                    blocklist.Add(ExtractInt32().ExtractedValue);

                for (int j = 0; j < extrablocks; ++j)
                    blocklist.Add(ExtractInt32().ExtractedValue);

                blocks.Add(blocklist);
            }

            for (int i = 0; i < StreamCount.ExtractedValue; ++i)
            {
                switch (i)
                {
                    case 1:
                        PDBInfoStream = new MSFStreamPDBInfo(i, FlattenedBuffer, streamsizes[i], blocks[i], BlockSize.ExtractedValue);
                        Streams.Add(PDBInfoStream);
                        break;

                    case 3:
                        Streams.Add(new MSFStreamDBI(i, FlattenedBuffer, streamsizes[i], blocks[i], BlockSize.ExtractedValue, this));
                        break;

                    default:
                        {
                            var stream = new MSFStream(i, FlattenedBuffer, streamsizes[i], blocks[i], BlockSize.ExtractedValue);
                            if((PDBInfoStream != null) && (stream.Name == null))
                                stream.Name = PDBInfoStream.GetNameOfStream(i);
                            Streams.Add(stream);
                        }
                        break;
                }
            }

            if (KnownStreamSymbols > 0)
                Streams[KnownStreamSymbols] = new MSFStreamSymbols(Streams[KnownStreamSymbols], KnownStreamSymbols);

            if (KnownStreamPublics > 0)
                Streams[KnownStreamPublics] = new MSFStreamPublics(Streams[KnownStreamPublics], KnownStreamPublics);

            foreach (var stream in KnownStreamModules)
            {
                Streams[stream.Value.StreamIndex] =
                    new MSFStreamDBIModule(
                        Streams[stream.Value.StreamIndex],
                        stream.Value.StreamIndex,
                        stream.Key,
                        stream.Value.NumBytesSymbols,
                        stream.Value.NumBytesLines,
                        stream.Value.NumBytesC13Lines
                    );
            }
        }
    }
}
