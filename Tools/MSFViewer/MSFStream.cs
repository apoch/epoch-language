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

        private int StreamIndex = 0;

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
                    Name = "Old MSF Directory (0)";
                    break;

                case 1:
                    Name = "PDB Stream (1)";
                    break;

                case 2:
                    Name = "TPI Stream (2)";
                    break;

                case 3:
                    Name = "DBI Stream (3)";
                    break;

                case 4:
                    Name = "IPI Stream (4)";
                    break;

                default:
                    Name = null;
                    break;
            }

            StreamIndex = streamindex;
        }

        public override string ToString()
        {
            if (Name == null)
                return $"Unknown Stream {StreamIndex}";

            return Name;
        }

        public void PopulateAnalysis(ListView lvw, TreeView tvw)
        {
            tvw.BeginUpdate();
            lvw.BeginUpdate();

            tvw.Nodes.Clear();
            var rootnode = tvw.Nodes.Add("root", "MSF Stream");

            lvw.Items.Clear();
            lvw.Groups.Clear();
            var metagroup = AddAnalysisGroup(lvw, tvw, "meta", "Metadata");

            if (FlattenedBuffer != null)
                AddAnalysisItem(lvw, tvw, "Size of data", metagroup, new TypedByteSequence<int>(FlattenedBuffer, 0, FlattenedBuffer.Length, FlattenedBuffer.Length));
            else
                AddAnalysisItem(lvw, tvw, "Size of data", metagroup, "0");

            SubclassPopulateAnalysis(lvw, tvw);

            rootnode.Expand();

            lvw.EndUpdate();
            tvw.EndUpdate();
        }

        protected virtual void SubclassPopulateAnalysis(ListView lvw, TreeView tvw)
        {
        }

        protected static ListViewGroup AddAnalysisGroup(ListView lvw, TreeView tvw, string key, string desc, string parent = "root")
        {
            var ret = lvw.Groups.Add(key, desc);
            ret.Tag = key;

            tvw.Nodes.Find(parent, true)[0].Nodes.Add(key, desc);

            return ret;
        }

        protected static void AddAnalysisItem(ListView lvw, TreeView tvw, string desc, ListViewGroup group, ByteSequence originaldata)
        {
            var item = new ListViewItem(new string[] { desc, originaldata.ToString() });
            item.Group = group;
            item.Tag = originaldata;
            lvw.Items.Add(item);

            var tnode = new TreeNode(desc);
            tnode.Tag = originaldata;
            AddTreeNodeByParentKey(tvw, group.Tag as string, tnode);
        }

        protected static void AddAnalysisItem(ListView lvw, TreeView tvw, string desc, ListViewGroup group, string nonpreviewdata)
        {
            var item = new ListViewItem(new string[] { desc, nonpreviewdata });
            item.Group = group;
            lvw.Items.Add(item);

            var tnode = new TreeNode(desc);
            AddTreeNodeByParentKey(tvw, group.Tag as string, tnode);
        }

        private static void AddTreeNodeByParentKey(TreeView tvw, string key, TreeNode node)
        {
            var matching = tvw.Nodes.Find(key, true);
            matching[0].Nodes.Add(node);
        }


        public byte[] GetFlattenedBuffer()
        {
            return FlattenedBuffer;
        }

        protected TypedByteSequence<string> ExtractStringWithLength(int length)
        {
            var ret = new TypedByteSequence<string>(FlattenedBuffer, ReadOffset, length, Encoding.ASCII.GetString(FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray()));
            ReadOffset += length;

            return ret;
        }

        protected TypedByteSequence<string> ExtractTerminatedString()
        {
            int length = 0;
            while (FlattenedBuffer[ReadOffset + length] != 0)
                ++length;

            var str = Encoding.ASCII.GetString(FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray());
            ++length;

            var ret = new TypedByteSequence<string>(FlattenedBuffer, ReadOffset, length, str);

            ReadOffset += length;

            return ret;
        }

        protected Dictionary<int, string> ExtractTerminatedStrings(int length)
        {
            var bytes = FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray();
            ReadOffset += length;

            int nullcount = bytes.Count(b => (b == 0));
            var ret = new Dictionary<int, string>();

            int read = 0;
            for (int i = 0; i < nullcount; ++i)
            {
                int substrend = read;
                while ((substrend < bytes.Length) && (bytes[substrend] != 0))
                    ++substrend;

                ret[read] = Encoding.ASCII.GetString(bytes.Skip(read).Take(substrend - read).ToArray());
                read = substrend + 1;
            }

            return ret;
        }

        protected TypedByteSequence<byte> ExtractByte()
        {
            var b = FlattenedBuffer[ReadOffset];
            var ret = new TypedByteSequence<byte>(FlattenedBuffer, ReadOffset, 1, b);
            ++ReadOffset;

            return ret;
        }

        protected TypedByteSequence<byte[]> ExtractBytes(int length)
        {
            var b = FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray();
            var ret = new TypedByteSequence<byte[]>(FlattenedBuffer, ReadOffset, length, b);
            ReadOffset += length;

            return ret;
        }

        protected TypedByteSequence<short> ExtractInt16()
        {
            var s = BitConverter.ToInt16(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<short>(FlattenedBuffer, ReadOffset, sizeof(short), s);
            ReadOffset += sizeof(short);

            return ret;
        }

        protected TypedByteSequence<ushort> ExtractUInt16()
        {
            var s = BitConverter.ToUInt16(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<ushort>(FlattenedBuffer, ReadOffset, sizeof(ushort), s);
            ReadOffset += sizeof(ushort);

            return ret;
        }

        protected TypedByteSequence<int> ExtractInt32()
        {
            var i = BitConverter.ToInt32(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<int>(FlattenedBuffer, ReadOffset, sizeof(int), i);
            ReadOffset += sizeof(int);

            return ret;
        }

        protected TypedByteSequence<uint> ExtractUInt32()
        {
            var u = BitConverter.ToUInt32(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<uint>(FlattenedBuffer, ReadOffset, sizeof(uint), u);
            ReadOffset += sizeof(uint);

            return ret;
        }

        protected TypedByteSequence<Guid> ExtractGuid()
        {
            var bytes = FlattenedBuffer.Skip(ReadOffset).Take(16).ToArray();
            var ret = new TypedByteSequence<Guid>(FlattenedBuffer, ReadOffset, 16, new Guid(bytes));
            ReadOffset += 16;

            return ret;
        }

        protected void Extract4ByteAlignment()
        {
            while ((ReadOffset & 3) != 0)
                ++ReadOffset;
        }
    }
}
