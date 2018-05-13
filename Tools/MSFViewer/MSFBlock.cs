using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MSFViewer
{
    class MSFBlock
    {
        private byte[] RawBlock = null;
        private int MyIndex;

        private MSFBlock(byte[] entirefile, int block, int blocksize)
        {
            MyIndex = block;
            RawBlock = new byte[blocksize];

            Array.Copy(entirefile, block * blocksize, RawBlock, 0, blocksize);
        }

        public override string ToString()
        {
            return $"Block {MyIndex}";
        }

        public int Index
        {
            get { return MyIndex; }
        }

        public byte[] Bytes
        {
            get { return RawBlock; }
        }

        public static List<MSFBlock> CopyBlockData(byte[] entirefile, int blocksize)
        {
            var ret = new List<MSFBlock>();

            int numblocks = entirefile.Length / blocksize;
            for (int i = 0; i < numblocks; ++i)
                ret.Add(new MSFBlock(entirefile, i, blocksize));

            return ret;
        }

        public static List<MSFBlock> CopyBlockData(byte[] entirefile, List<int> blockindices, int blocksize)
        {
            var ret = new List<MSFBlock>();

            foreach (int block in blockindices)
                ret.Add(new MSFBlock(entirefile, block, blocksize));

            return ret;
        }
    }
}
