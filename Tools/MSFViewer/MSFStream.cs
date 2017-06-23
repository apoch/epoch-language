using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class MSFStream
    {
        public string Name = "(Unknown)";

        protected byte[] FlattenedBuffer = null;
        protected int ReadOffset = 0;

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

        public void PopulateAnalysis(ListView lvw)
        {
            lvw.Items.Clear();
            lvw.Groups.Clear();
            var metagroup = lvw.Groups.Add("meta", "Metadata");

            string datasize = (FlattenedBuffer != null) ? $"{FlattenedBuffer.Length}" : "0";
            AddAnalysisItem(lvw, "Size of data", datasize, metagroup);

            SubclassPopulateAnalysis(lvw);
        }

        protected virtual void SubclassPopulateAnalysis(ListView lvw)
        {
        }

        protected void AddAnalysisItem(ListView lvw, string desc, string value, ListViewGroup group)
        {
            var item = new ListViewItem(new string[] { desc, value });
            item.Group = group;
            lvw.Items.Add(item);
        }

        public byte[] GetFlattenedBuffer()
        {
            return FlattenedBuffer;
        }

        protected string ExtractStringWithLength(int length)
        {
            var ret = Encoding.ASCII.GetString(FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray());
            ReadOffset += length;

            return ret;
        }

        protected string[] ExtractTerminatedStrings(int length)
        {
            var bytes = FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray();
            ReadOffset += length;

            int nullcount = bytes.Count(b => (b == 0));
            var ret = new string[nullcount];

            int read = 0;
            for (int i = 0; i < nullcount; ++i)
            {
                int substrend = read;
                while ((substrend < bytes.Length) && (bytes[substrend] != 0))
                    ++substrend;

                ret[i] = Encoding.ASCII.GetString(bytes.Skip(read).Take(substrend - read).ToArray());
                read = substrend + 1;
            }

            return ret;
        }

        protected byte ExtractByte()
        {
            var ret = FlattenedBuffer[ReadOffset];
            ++ReadOffset;

            return ret;
        }

        protected short ExtractInt16()
        {
            var ret = BitConverter.ToInt16(FlattenedBuffer, ReadOffset);
            ReadOffset += 2;

            return ret;
        }

        protected int ExtractInt32()
        {
            var ret = BitConverter.ToInt32(FlattenedBuffer, ReadOffset);
            ReadOffset += 4;

            return ret;
        }

        protected Guid ExtractGuid()
        {
            var bytes = FlattenedBuffer.Skip(ReadOffset).Take(16).ToArray();
            ReadOffset += 16;

            return new Guid(bytes);
        }
    }
}
