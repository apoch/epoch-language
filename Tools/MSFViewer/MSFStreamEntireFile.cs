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

        public List<MSFStream> Streams;

        private int BlockSize = 0;
        private int DirectoryBlock = 0;
        private int FreeBlockMapIndex = 0;
        private int BlockCount = 0;
        private int DirectoryStreamLength = 0;
        private int HintBlock = 0;
        private int StreamCount = 0;
        private int Unknown = 0;

        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var blockgroup = lvw.Groups.Add("blocks", "Blocks");
            AddAnalysisItem(lvw, "Block size", $"{BlockSize}", blockgroup);
            AddAnalysisItem(lvw, "Block count", $"{BlockCount}", blockgroup);

            var directorygroup = lvw.Groups.Add("directory", "Directory");
            AddAnalysisItem(lvw, "Directory hint block", $"{HintBlock}", directorygroup);
            AddAnalysisItem(lvw, "Directory data block", $"{DirectoryBlock}", directorygroup);
            AddAnalysisItem(lvw, "Directory stream length", $"{DirectoryStreamLength}", directorygroup);

            var additionalgroup = lvw.Groups.Add("additional", "Additional Data");
            AddAnalysisItem(lvw, "Free block map", $"{FreeBlockMapIndex}", additionalgroup);
            AddAnalysisItem(lvw, "Unknown data field", $"{Unknown}", additionalgroup);

            var streamgroup = lvw.Groups.Add("streams", "Streams");
            AddAnalysisItem(lvw, "Stream count", $"{StreamCount}", streamgroup);
        }

        private void ParseMagic()
        {
            string MAGIC = "Microsoft C/C++ MSF 7.00";
            string filemagic = ExtractStringWithLength(MAGIC.Length);

            if (filemagic.CompareTo(MAGIC) != 0)
                throw new Exception("Invalid PDB magic");


            if ((ExtractByte() != 13) ||            // Carriage return
                (ExtractByte() != 10) ||            // Linefeed
                (ExtractByte() != 26) ||            // Magic?
                (ExtractByte() != 68) ||            // 'D'
                (ExtractByte() != 83) ||            // 'S'
                (ExtractByte() != 0) ||             // Null magic
                (ExtractByte() != 0) ||             // Null magic
                (ExtractByte() != 0)                // Null magic
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
            ReadOffset = BlockSize * HintBlock;
            DirectoryBlock = ExtractInt32();
        }

        private void ParseDirectory()
        {
            ReadOffset = BlockSize * DirectoryBlock;
            StreamCount = ExtractInt32();

            var streamsizes = new int[StreamCount];
            for (int i = 0; i < StreamCount; ++i)
            {
                streamsizes[i] = ExtractInt32();
            }

            var blocks = new List<List<int>>();
            for (int i = 0; i < StreamCount; ++i)
            {
                int extrablocks = streamsizes[i] / BlockSize;

                var blocklist = new List<int>();
                if (streamsizes[i] % BlockSize > 0)
                    blocklist.Add(ExtractInt32());

                for (int j = 0; j < extrablocks; ++j)
                    blocklist.Add(ExtractInt32());

                blocks.Add(blocklist);
            }

            for (int i = 0; i < StreamCount; ++i)
            {
                Streams.Add(new MSFStream(i, FlattenedBuffer, streamsizes[i], blocks[i], BlockSize));
            }
        }
    }
}
