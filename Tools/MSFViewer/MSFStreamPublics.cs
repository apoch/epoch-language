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


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var headergroup = lvw.Groups.Add("headers", "DBI Publics Header Info");
            AddAnalysisItem(lvw, "Hash bytes", headergroup, HashBytes);
            AddAnalysisItem(lvw, "Address map bytes", headergroup, AddressMapBytes);
            AddAnalysisItem(lvw, "Number of thunks", headergroup, NumThunks);
            AddAnalysisItem(lvw, "Thunk table section index", headergroup, ThunkTableSectionIndex);
            AddAnalysisItem(lvw, "Padding", headergroup, Padding);
            AddAnalysisItem(lvw, "Thunk table offset", headergroup, ThunkTableOffset);
            AddAnalysisItem(lvw, "Number of sections", headergroup, NumSections);
            AddAnalysisItem(lvw, "Signature", headergroup, Signature);
            AddAnalysisItem(lvw, "Version", headergroup, Version);
            AddAnalysisItem(lvw, "Version (desugared)", headergroup, $"{Version.ExtractedValue - 0xeffe0000}");
            AddAnalysisItem(lvw, "Hash record bytes", headergroup, HRFilesBytes);
            AddAnalysisItem(lvw, "Hash bucket bytes", headergroup, BucketBytes);
            AddAnalysisItem(lvw, "Mystery bitmap", headergroup, new BitmapByteSequence(Bitmap));

            int i = 0;
            foreach (var hrfile in HRFiles)
            {
                var hrgroup = lvw.Groups.Add($"hrf{i}", $"Hash Record {i}");
                AddAnalysisItem(lvw, "Record offset", hrgroup, hrfile.Offset);
                AddAnalysisItem(lvw, "Record ???", hrgroup, hrfile.Unknown);

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
                while (ReadOffset - begin < HRFilesBytes.ExtractedValue)
                {
                    var hrfile = new HRFile();
                    hrfile.Offset = ExtractUInt32();
                    hrfile.Unknown = ExtractUInt32();

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
