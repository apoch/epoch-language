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

            AddressMap = new List<TypedByteSequence<uint>>();
            Thunks = new List<TypedByteSequence<uint>>();
            Sections = new List<SectionMapEntry>();

            ParsePublicsHeader();
            ParseAddressMap();
            ParseThunkMap();
            ParseSectionMap();
        }


        private class HRFile
        {
            public TypedByteSequence<uint> Offset;
            public TypedByteSequence<uint> Unknown;

            public ByteSequence OriginalSequence;
        }


        private class BitmapByteSequence : ByteSequence
        {
            private uint BitCount = 0;

            public BitmapByteSequence(TypedByteSequence<byte[]> buffer)
                : base(buffer?.ExtractedValue)
            {
                if (buffer != null)
                {
                    foreach (byte b in buffer.ExtractedValue)
                    {
                        for (int i = 0; i < 8; ++i)
                        {
                            if ((b & (1 << i)) != 0)
                                ++BitCount;
                        }
                    }
                }
            }

            public override string ToString()
            {
                if (HasData())
                    return $"Present (occupies {GetRawBytes().Length} bytes)";

                return "Absent";
            }

            public uint CountBits()
            {
                return BitCount;
            }
        }

        private class SectionMapEntry
        {
            public TypedByteSequence<uint> Offset;
            public TypedByteSequence<ushort> SectionIndex;
            public TypedByteSequence<ushort> Padding;

            public ByteSequence OriginalSequence;
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

        private BitmapByteSequence Bitmap;

        private List<TypedByteSequence<uint>> AddressMap;
        private List<TypedByteSequence<uint>> Thunks;
        private List<SectionMapEntry> Sections;

        private int HRFBeginOffset;
        private int AddrBeginOffset;


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
            AddAnalysisItem(lvw, tvw, "Mystery bitmap", headergroup, Bitmap);

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

            var addrgroup = AddAnalysisGroup(lvwcontrol, tvw, "addrmapall", "Address Map");

            i = 0;
            foreach (var addr in AddressMap)
            {
                AddAnalysisItem(lvw, tvw, $"Address {i}", addrgroup, addr);
                ++i;
            }

            var thunkgroup = AddAnalysisGroup(lvwcontrol, tvw, "thunkall", "Thunk Map");

            i = 0;
            foreach (var thunk in Thunks)
            {
                AddAnalysisItem(lvw, tvw, $"Thunk {i}", thunkgroup, thunk);
                ++i;
            }

            var secmapgroup = AddAnalysisGroup(lvwcontrol, tvw, "secmapall", "Section Map");

            i = 0;
            foreach (var sm in Sections)
            {
                var smgroup = AddAnalysisGroup(lvwcontrol, tvw, $"section{i}", $"Section {i}", secmapgroup.Node);
                AddAnalysisItem(lvw, tvw, "Offset", smgroup, sm.Offset);
                AddAnalysisItem(lvw, tvw, "Section Index", smgroup, sm.SectionIndex);
                AddAnalysisItem(lvw, tvw, "Padding", smgroup, sm.Padding);

                smgroup.Node.Tag = sm.OriginalSequence;

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

            Bitmap = new BitmapByteSequence(ExtractBytes(516));         // TODO - magic number (seems constant in other tools though)

            for (uint i = 0; i < Bitmap.CountBits(); ++i)
                ExtractUInt32();
        }


        private void ParseAddressMap()
        {
            AddrBeginOffset = ReadOffset;
            uint numaddrs = AddressMapBytes.ExtractedValue / sizeof(uint);
            for (uint i = 0; i < numaddrs; ++i)
                AddressMap.Add(ExtractUInt32());
        }

        private void ParseThunkMap()
        {
            uint numthunks = NumThunks.ExtractedValue;
            for (uint i = 0; i < numthunks; ++i)
                Thunks.Add(ExtractUInt32());
        }

        private void ParseSectionMap()
        {
            uint numsections = NumSections.ExtractedValue;
            for (uint i = 0; i < numsections; ++i)
            {
                var seq = new ByteSequence(FlattenedBuffer, ReadOffset, 8);

                var offset = ExtractUInt32();
                var sectionindex = ExtractUInt16();
                var padding = ExtractUInt16();

                Sections.Add(new SectionMapEntry { Offset = offset, SectionIndex = sectionindex, Padding = padding, OriginalSequence = seq });
            }
        }
    }
}
