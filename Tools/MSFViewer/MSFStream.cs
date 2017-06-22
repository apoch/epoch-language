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

        public MSFStream(byte[] entirefile, int streamsize, List<int> blocks, int blocksize)
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
