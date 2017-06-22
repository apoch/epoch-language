using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MSFViewer
{
    class MSF
    {
        public MSF(string filename)
        {
            Streams = new List<MSFStream>();

            EntireFile = new MSFStream(File.ReadAllBytes(filename));
            EntireFile.Name = "(Entire file)";

            ParseMagic();
            ParseSuperBlock();
            ParseBlockMap();
            ParseDirectoryHint();
            ParseDirectory();
        }

        public List<MSFStream> Streams;
        public MSFStream EntireFile;

        private int ReadOffset = 0;
        private int BlockSize = 0;
        private int DirectoryBlock = 0;
        private int FreeBlockMapIndex = 0;
        private int BlockCount = 0;
        private int DirectoryStreamLength = 0;
        private int HintBlock = 0;
        private int StreamCount = 0;

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
            ExtractInt32();     // Unknown?
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
                Streams.Add(new MSFStream(EntireFile.GetFlattenedBuffer(), streamsizes[i], blocks[i], BlockSize));
            }
        }


        private string ExtractStringWithLength(int length)
        {
            var buffer = EntireFile.GetFlattenedBuffer();
            var ret = Encoding.ASCII.GetString(buffer.Skip(ReadOffset).Take(length).ToArray());
            ReadOffset += length;

            return ret;
        }

        private byte ExtractByte()
        {
            var buffer = EntireFile.GetFlattenedBuffer();
            var ret = buffer[ReadOffset];
            ++ReadOffset;

            return ret;
        }

        private int ExtractInt32()
        {
            var buffer = EntireFile.GetFlattenedBuffer();
            var bytes = buffer.Skip(ReadOffset).Take(4).ToArray();
            ReadOffset += 4;

            return BitConverter.ToInt32(bytes, 0);
        }
    }
}
