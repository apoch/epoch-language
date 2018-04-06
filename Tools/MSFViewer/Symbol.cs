using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class Symbol
    {
        public TypedByteSequence<ushort> Size;
        public TypedByteSequence<ushort> Type;

        public MaskedByteSequence CorrespondingByteSequence;

        public virtual void PopulateAnalysis(MSFStream stream, List<ListViewItem> lvw, AnalysisGroup group, TreeView tvw)
        {
        }

        public override string ToString()
        {
            return "{unknown symbol type}";
        }

        public static Symbol MakeSymbol(MSFStream stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type)
        {
            var seq = new MaskedByteSequence(stream.GetFlattenedBuffer(), stream.GetReadOffset() - 4, size.ExtractedValue + 2, "Symbol");

            switch (type.ExtractedValue)
            {
                case 0x1108:     // S_UDT
                    return new SymbolUDT(stream, size, type, seq);

                case 0x110e:     // S_PUB32
                    return new SymbolPublic(stream, size, type, seq);

                case 0x1110:    // S_GPROC32
                    return new SymbolProc(stream, size, type, seq);

                case 0x1125:     // S_PROCREF
                    return new SymbolProcRef(stream, size, type, seq);

                case 0x1136:    // S_SECTION
                    return new SymbolSection(stream, size, type, seq);
            }

            // TODO - count unknown symbols in the stream
            return new Symbol { Size = size, Type = type, CorrespondingByteSequence = seq };
        }
    }

    class SymbolPublic : Symbol
    {
        public TypedByteSequence<int> PublicType;
        public TypedByteSequence<int> Offset;
        public TypedByteSequence<ushort> SectionIndex;
        public TypedByteSequence<string> Name;

        public SymbolPublic(MSFStream stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type, MaskedByteSequence seq)
        {
            CorrespondingByteSequence = seq;
            Size = size;
            Type = type;

            PublicType = stream.ExtractInt32();
            Offset = stream.ExtractInt32();
            SectionIndex = stream.ExtractUInt16();
            Name = stream.ExtractTerminatedString();

            stream.Extract4ByteAlignment();
        }

        public override void PopulateAnalysis(MSFStream stream, List<ListViewItem> lvw, AnalysisGroup group, TreeView tvw)
        {
            MSFStream.AddAnalysisItem(lvw, tvw, "Public symbol type", group, PublicType);
            MSFStream.AddAnalysisItem(lvw, tvw, "Start offset", group, Offset);
            MSFStream.AddAnalysisItem(lvw, tvw, "Section index", group, SectionIndex);
            MSFStream.AddAnalysisItem(lvw, tvw, "Name", group, Name);
        }

        public override string ToString()
        {
            return Name.ExtractedValue;
        }
    }

    class SymbolUDT : Symbol
    {
        public TypedByteSequence<int> TypeIndex;
        public TypedByteSequence<string> Name;

        public SymbolUDT(MSFStream stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type, MaskedByteSequence seq)
        {
            CorrespondingByteSequence = seq;
            Size = size;
            Type = type;

            TypeIndex = stream.ExtractInt32();
            Name = stream.ExtractTerminatedString();

            stream.Extract4ByteAlignment();
        }

        public override void PopulateAnalysis(MSFStream stream, List<ListViewItem> lvw, AnalysisGroup group, TreeView tvw)
        {
            MSFStream.AddAnalysisItem(lvw, tvw, "UDT symbol type index", group, TypeIndex);
            MSFStream.AddAnalysisItem(lvw, tvw, "UDT name", group, Name);
        }

        public override string ToString()
        {
            return Name.ExtractedValue;
        }
    }

    class SymbolProc : Symbol
    {
        public TypedByteSequence<uint> Parent;
        public TypedByteSequence<uint> End;
        public TypedByteSequence<uint> Next;
        public TypedByteSequence<uint> CodeSize;
        public TypedByteSequence<uint> DebugStart;
        public TypedByteSequence<uint> DebugEnd;
        public TypedByteSequence<uint> TypeIndex;
        public TypedByteSequence<uint> CodeOffset;
        public TypedByteSequence<ushort> Segment;
        public TypedByteSequence<byte> Flags;
        public TypedByteSequence<string> Name;

        public SymbolProc(MSFStream stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type, MaskedByteSequence seq)
        {
            CorrespondingByteSequence = seq;
            Size = size;
            Type = type;

            Parent = stream.ExtractUInt32();
            End = stream.ExtractUInt32();
            Next = stream.ExtractUInt32();
            CodeSize = stream.ExtractUInt32();
            DebugStart = stream.ExtractUInt32();
            DebugEnd = stream.ExtractUInt32();
            TypeIndex = stream.ExtractUInt32();
            CodeOffset = stream.ExtractUInt32();
            Segment = stream.ExtractUInt16();
            Flags = stream.ExtractByte();
            Name = stream.ExtractTerminatedString();

            stream.Extract4ByteAlignment();
        }

        public override void PopulateAnalysis(MSFStream stream, List<ListViewItem> lvw, AnalysisGroup group, TreeView tvw)
        {
            MSFStream.AddAnalysisItem(lvw, tvw, "Parent", group, Parent);
            MSFStream.AddAnalysisItem(lvw, tvw, "End", group, End);
            MSFStream.AddAnalysisItem(lvw, tvw, "Next", group, Next);
            MSFStream.AddAnalysisItem(lvw, tvw, "Code size", group, CodeSize);
            MSFStream.AddAnalysisItem(lvw, tvw, "Debug start", group, DebugStart);
            MSFStream.AddAnalysisItem(lvw, tvw, "Debug end", group, DebugEnd);
            MSFStream.AddAnalysisItem(lvw, tvw, "Type index", group, TypeIndex);
            MSFStream.AddAnalysisItem(lvw, tvw, "Code offset", group, CodeOffset);
            MSFStream.AddAnalysisItem(lvw, tvw, "Segment index", group, Segment);
            MSFStream.AddAnalysisItem(lvw, tvw, "Flags", group, Flags);
            MSFStream.AddAnalysisItem(lvw, tvw, "Name", group, Name);
        }

        public override string ToString()
        {
            return Name.ExtractedValue;
        }

    }

    class SymbolProcRef : Symbol
    {
        public TypedByteSequence<uint> SumName;
        public TypedByteSequence<uint> SymOffset;
        public TypedByteSequence<ushort> Module;
        public TypedByteSequence<string> Name;

        public SymbolProcRef(MSFStream stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type, MaskedByteSequence seq)
        {
            CorrespondingByteSequence = seq;
            Size = size;
            Type = type;

            SumName = stream.ExtractUInt32();
            SymOffset = stream.ExtractUInt32();
            Module = stream.ExtractUInt16();
            Name = stream.ExtractTerminatedString();

            stream.Extract4ByteAlignment();
        }

        public override void PopulateAnalysis(MSFStream stream, List<ListViewItem> lvw, AnalysisGroup group, TreeView tvw)
        {
            MSFStream.AddAnalysisItem(lvw, tvw, "ProcRef sum name", group, SumName);
            MSFStream.AddAnalysisItem(lvw, tvw, "Symbol offset", group, SymOffset);
            MSFStream.AddAnalysisItem(lvw, tvw, "Module index", group, Module);
            MSFStream.AddAnalysisItem(lvw, tvw, "Name", group, Name);
        }

        public override string ToString()
        {
            return Name.ExtractedValue;
        }
    }

    class SymbolSection : Symbol
    {
        public TypedByteSequence<ushort> SectionIndex;
        public TypedByteSequence<ushort> Alignment;
        public TypedByteSequence<uint> RVA;
        public TypedByteSequence<uint> Length;
        public TypedByteSequence<uint> Characteristics;
        public TypedByteSequence<string> Name;

        public SymbolSection(MSFStream stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type, MaskedByteSequence seq)
        {
            CorrespondingByteSequence = seq;
            Size = size;
            Type = type;

            SectionIndex = stream.ExtractUInt16();
            Alignment = stream.ExtractUInt16();
            RVA = stream.ExtractUInt32();
            Length = stream.ExtractUInt32();
            Characteristics = stream.ExtractUInt32();
            Name = stream.ExtractTerminatedString();

            stream.Extract4ByteAlignment();
        }

        public override void PopulateAnalysis(MSFStream stream, List<ListViewItem> lvw, AnalysisGroup group, TreeView tvw)
        {
            MSFStream.AddAnalysisItem(lvw, tvw, "Section index", group, SectionIndex);
            MSFStream.AddAnalysisItem(lvw, tvw, "Alignment", group, Alignment);
            MSFStream.AddAnalysisItem(lvw, tvw, "RVA", group, RVA);
            MSFStream.AddAnalysisItem(lvw, tvw, "Length", group, Length);
            MSFStream.AddAnalysisItem(lvw, tvw, "Characteristics", group, Characteristics);
            MSFStream.AddAnalysisItem(lvw, tvw, "Name", group, Name);
        }

        public override string ToString()
        {
            return Name.ExtractedValue;
        }
    }
}
