using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class MSFStreamPublics : MSFStream
    {
        public MSFStreamPublics(MSFStream rawstream, int index)
            : base(rawstream.GetFlattenedBuffer())
        {
            Name = $"DBI Publics ({index})";

            ParsePublicsHeader();
        }


        private class HRFile
        {
            public TypedByteSequence<uint> Offset;
            public TypedByteSequence<uint> Unknown;

            public ByteSequence OriginalSequence;
        }


        private class BitmapByteSequence : ByteSequence
        {
            public BitmapByteSequence(TypedByteSequence<byte[]> buffer)
                : base(buffer?.ExtractedValue)
            {
            }

            public override string ToString()
            {
                if (HasData())
                    return $"Present (occupies {GetRawBytes().Length} bytes)";

                return "Absent";
            }
        }


        private TypedByteSequence<uint> HashBytes;
        private TypedByteSequence<uint> AddressMapBytes;
        private TypedByteSequence<uint> NumThunks;
        private TypedByteSequence<uint> SizeOfThunk;
        private TypedByteSequence<ushort> ThunkTableSectionIndex;
        private TypedByteSequence<ushort> Padding;
        private TypedByteSequence<uint> ThunkTableOffset;
        private TypedByteSequence<uint> NumSections;

        private TypedByteSequence<uint> Signature;
        private TypedByteSequence<uint> Version;
        private TypedByteSequence<uint> HRFilesBytes;
        private TypedByteSequence<uint> BucketBytes;

        private List<HRFile> HRFiles = new List<HRFile>();

        private TypedByteSequence<byte[]> Bitmap;

        private int HRFBeginOffset;


        protected override void SubclassPopulateAnalysis(List<ListViewItem> lvw, ListView lvwcontrol, TreeView tvw)
        {
            var headergroup = AddAnalysisGroup(lvwcontrol, tvw, "headers", "DBI Publics Header Info");
            AddAnalysisItem(lvw, tvw, "Hash bytes", headergroup, HashBytes);
            AddAnalysisItem(lvw, tvw, "Address map bytes", headergroup, AddressMapBytes);
            AddAnalysisItem(lvw, tvw, "Number of thunks", headergroup, NumThunks);
            AddAnalysisItem(lvw, tvw, "Thunk table section index", headergroup, ThunkTableSectionIndex);
            AddAnalysisItem(lvw, tvw, "Padding", headergroup, Padding);
            AddAnalysisItem(lvw, tvw, "Thunk table offset", headergroup, ThunkTableOffset);
            AddAnalysisItem(lvw, tvw, "Number of sections", headergroup, NumSections);
            AddAnalysisItem(lvw, tvw, "Signature", headergroup, Signature);
            AddAnalysisItem(lvw, tvw, "Version", headergroup, Version);
            AddAnalysisItem(lvw, tvw, "Version (desugared)", headergroup, $"{Version.ExtractedValue - 0xeffe0000}");
            AddAnalysisItem(lvw, tvw, "Hash record bytes", headergroup, HRFilesBytes);
            AddAnalysisItem(lvw, tvw, "Hash bucket bytes", headergroup, BucketBytes);
            AddAnalysisItem(lvw, tvw, "Mystery bitmap", headergroup, new BitmapByteSequence(Bitmap));

            var hrall = tvw.Nodes.Find("root", false)[0].Nodes.Add("hrfall", "Hash Records");
            hrall.Tag = new ByteSequence(FlattenedBuffer, HRFBeginOffset, (int)HRFilesBytes.ExtractedValue);

            int i = 0;
            foreach (var hrfile in HRFiles)
            {
                var hrgroup = AddAnalysisGroup(lvwcontrol, tvw, $"hrf{i}", $"Hash Record {i}", hrall);
                AddAnalysisItem(lvw, tvw, "Record offset", hrgroup, hrfile.Offset);
                AddAnalysisItem(lvw, tvw, "Record ???", hrgroup, hrfile.Unknown);

                hrgroup.Node.Tag = hrfile.OriginalSequence;

                ++i;
            }
        }


        private void ParsePublicsHeader()
        {
            HashBytes = ExtractUInt32();
            AddressMapBytes = ExtractUInt32();
            NumThunks = ExtractUInt32();
            SizeOfThunk = ExtractUInt32();
            ThunkTableSectionIndex = ExtractUInt16();
            Padding = ExtractUInt16();
            ThunkTableOffset = ExtractUInt32();
            NumSections = ExtractUInt32();

            Signature = ExtractUInt32();
            Version = ExtractUInt32();
            HRFilesBytes = ExtractUInt32();
            BucketBytes = ExtractUInt32();

            if (HRFilesBytes.ExtractedValue % 8 == 0)
            {
                int begin = ReadOffset;
                HRFBeginOffset = begin;

                while (ReadOffset - begin < HRFilesBytes.ExtractedValue)
                {
                    var seq = new ByteSequence(FlattenedBuffer, ReadOffset, sizeof(uint) * 2);

                    var hrfile = new HRFile();
                    hrfile.Offset = ExtractUInt32();
                    hrfile.Unknown = ExtractUInt32();
                    hrfile.OriginalSequence = seq;

                    HRFiles.Add(hrfile);
                }
            }
            else
            {
                ReadOffset += (int)HRFilesBytes.ExtractedValue;
            }

            Bitmap = ExtractBytes(516);         // TODO - magic number (seems constant in other tools though)
        }
    }
}
