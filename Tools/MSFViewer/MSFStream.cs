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
