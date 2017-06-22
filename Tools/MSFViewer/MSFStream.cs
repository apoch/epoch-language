using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MSFViewer
{
    class MSFStream
    {
        public string Name = "(Unknown)";

        private byte[] FlattenedBuffer = null;

        public MSFStream(byte[] rawbuffer)
        {
            FlattenedBuffer = rawbuffer;
        }

        public MSFStream(int streamindex, byte[] entirefile, int streamsize, List<int> blocks, int blocksize)
        {
            if (streamsize > 0)
            {
                FlattenedBuffer = new byte[streamsize];
                int writeindex = 0;

                for (int i = 0; i < blocks.Count - 1; ++i)
                {
                    Array.Copy(entirefile, blocks[i] * blocksize, FlattenedBuffer, writeindex, blocksize);
                    writeindex += blocksize;
                }

                Array.Copy(entirefile, blocks[blocks.Count - 1] * blocksize, FlattenedBuffer, writeindex, streamsize % blocksize);
            }

            switch (streamindex)
            {
                case 0:
                    Name = "Old MSF Directory";
                    break;

                case 1:
                    Name = "PDB Stream";
                    break;

                case 2:
                    Name = "TPI Stream";
                    break;

                case 3:
                    Name = "DBI Stream";
                    break;

                case 4:
                    Name = "IPI Stream";
                    break;

                default:
                    Name = $"Unknown Stream {streamindex}";
                    break;
            }
        }

        public override string ToString()
        {
            return Name;
        }

        public byte[] GetFlattenedBuffer()
        {
            return FlattenedBuffer;
        }
    }
}
