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

        protected List<MSFBlock> RawBlocks = null;

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


                RawBlocks = MSFBlock.CopyBlockData(entirefile, blocks, blocksize);
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

        public void PopulateAnalysis(ListView lvw, TreeView tvw, List<ListViewItem> lvwitems)
        {
            tvw.BeginUpdate();
            lvw.BeginUpdate();

            tvw.Nodes.Clear();
            var rootnode = tvw.Nodes.Add("root", "MSF Stream");

            lvw.Items.Clear();
            lvw.Groups.Clear();
            var metagroup = AddAnalysisGroup(lvw, tvw, "meta", "Metadata");

            if (FlattenedBuffer != null)
                AddAnalysisItem(lvwitems, tvw, "Size of data", metagroup, new TypedByteSequence<int>(FlattenedBuffer, 0, FlattenedBuffer.Length, FlattenedBuffer.Length));
            else
                AddAnalysisItem(lvwitems, tvw, "Size of data", metagroup, "0");

            SubclassPopulateAnalysis(lvwitems, lvw, tvw);

            rootnode.Expand();
            lvw.Items.AddRange(lvwitems.ToArray());

            lvw.EndUpdate();
            tvw.EndUpdate();
        }

        public void PopulateRawBlocks(ListBox blocklist)
        {
            blocklist.Items.Clear();
            if (RawBlocks != null)
            {
                foreach (var rawblock in RawBlocks)
                    blocklist.Items.Add(rawblock);
            }
        }

        protected virtual void SubclassPopulateAnalysis(List<ListViewItem> lvwitems, ListView lvw, TreeView tvw)
        {
        }

        internal static AnalysisGroup AddAnalysisGroup(ListView lvw, TreeView tvw, string key, string desc, TreeNode parent = null)
        {
            var ret = new AnalysisGroup();
            ret.LVGroup = lvw.Groups.Add(key, desc);
            ret.LVGroup.Tag = key;

            if (parent != null)
                ret.Node = parent.Nodes.Add(key, desc);
            else
                ret.Node = tvw.Nodes.Find("root", true)[0].Nodes.Add(key, desc);

            return ret;
        }

        internal static void AddAnalysisItem(List<ListViewItem> lvwitems, TreeView tvw, string desc, AnalysisGroup group, ByteSequence originaldata)
        {
            var item = new ListViewItem(new string[] { desc, originaldata.ToString() });
            item.Group = group.LVGroup;
            item.Tag = originaldata;
            lvwitems.Add(item);

            var tnode = new TreeNode(desc);
            tnode.Tag = originaldata;

            group.Node.Nodes.Add(tnode);
        }

        internal static void AddAnalysisItem(List<ListViewItem> lvwitems, TreeView tvw, string desc, AnalysisGroup group, string nonpreviewdata)
        {
            var item = new ListViewItem(new string[] { desc, nonpreviewdata });
            item.Group = group.LVGroup;
            lvwitems.Add(item);

            var tnode = new TreeNode(desc);

            group.Node.Nodes.Add(tnode);
        }

        public byte[] GetFlattenedBuffer()
        {
            return FlattenedBuffer;
        }

        public int GetReadOffset()
        {
            return ReadOffset;
        }

        internal TypedByteSequence<string> ExtractStringWithLength(int length)
        {
            var ret = new TypedByteSequence<string>(FlattenedBuffer, ReadOffset, length, Encoding.ASCII.GetString(FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray()));
            ReadOffset += length;

            return ret;
        }

        internal TypedByteSequence<string> ExtractTerminatedString()
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

        internal Dictionary<int, string> ExtractTerminatedStrings(int length)
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

        internal TypedByteSequence<byte> ExtractByte()
        {
            var b = FlattenedBuffer[ReadOffset];
            var ret = new TypedByteSequence<byte>(FlattenedBuffer, ReadOffset, 1, b);
            ++ReadOffset;

            return ret;
        }

        internal TypedByteSequence<byte[]> ExtractBytes(int length)
        {
            var b = FlattenedBuffer.Skip(ReadOffset).Take(length).ToArray();
            var ret = new TypedByteSequence<byte[]>(FlattenedBuffer, ReadOffset, length, b);
            ReadOffset += length;

            return ret;
        }

        internal TypedByteSequence<short> ExtractInt16()
        {
            var s = BitConverter.ToInt16(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<short>(FlattenedBuffer, ReadOffset, sizeof(short), s);
            ReadOffset += sizeof(short);

            return ret;
        }

        internal TypedByteSequence<ushort> ExtractUInt16()
        {
            var s = BitConverter.ToUInt16(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<ushort>(FlattenedBuffer, ReadOffset, sizeof(ushort), s);
            ReadOffset += sizeof(ushort);

            return ret;
        }

        internal TypedByteSequence<int> ExtractInt32()
        {
            var i = BitConverter.ToInt32(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<int>(FlattenedBuffer, ReadOffset, sizeof(int), i);
            ReadOffset += sizeof(int);

            return ret;
        }

        internal TypedByteSequence<uint> ExtractUInt32()
        {
            var u = BitConverter.ToUInt32(FlattenedBuffer, ReadOffset);
            var ret = new TypedByteSequence<uint>(FlattenedBuffer, ReadOffset, sizeof(uint), u);
            ReadOffset += sizeof(uint);

            return ret;
        }

        internal TypedByteSequence<Guid> ExtractGuid()
        {
            var bytes = FlattenedBuffer.Skip(ReadOffset).Take(16).ToArray();
            var ret = new TypedByteSequence<Guid>(FlattenedBuffer, ReadOffset, 16, new Guid(bytes));
            ReadOffset += 16;

            return ret;
        }

        internal void Extract4ByteAlignment()
        {
            while ((ReadOffset & 3) != 0)
                ++ReadOffset;
        }
    }
}
