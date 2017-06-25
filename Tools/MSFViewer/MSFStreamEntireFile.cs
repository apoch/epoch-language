using System;
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

            ParseMagic();
            ParseSuperBlock();
            ParseBlockMap();
            ParseDirectoryHint();
            ParseDirectory();
        }


        public void RegisterDBIStreams(int globals, int publics, int symbols)
        {
            KnownStreamGlobals = globals;
            KnownStreamPublics = publics;
            KnownStreamSymbols = symbols;
        }


        public List<MSFStream> Streams;

        private MSFStreamPDBInfo PDBInfoStream;

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

        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var blockgroup = lvw.Groups.Add("blocks", "Blocks");
            AddAnalysisItem(lvw, "Block size", blockgroup, BlockSize);
            AddAnalysisItem(lvw, "Block count", blockgroup, BlockCount);

            var directorygroup = lvw.Groups.Add("directory", "Directory");
            AddAnalysisItem(lvw, "Directory hint block", directorygroup, HintBlock);
            AddAnalysisItem(lvw, "Directory data block", directorygroup, DirectoryBlock);
            AddAnalysisItem(lvw, "Directory stream length", directorygroup, DirectoryStreamLength);

            var additionalgroup = lvw.Groups.Add("additional", "Additional Data");
            AddAnalysisItem(lvw, "Free block map", additionalgroup, FreeBlockMapIndex);
            AddAnalysisItem(lvw, "Unknown data field", additionalgroup, Unknown);

            var streamgroup = lvw.Groups.Add("streams", "Streams");
            AddAnalysisItem(lvw, "Stream count", streamgroup, StreamCount);
        }

        private void ParseMagic()
        {
            string MAGIC = "Microsoft C/C++ MSF 7.00";
            string filemagic = ExtractStringWithLength(MAGIC.Length).ExtractedValue;

            if (filemagic.CompareTo(MAGIC) != 0)
                throw new Exception("Invalid PDB magic");


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
                throw new Exception("Invalid PDB magic");
            }
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
        }
    }
}
