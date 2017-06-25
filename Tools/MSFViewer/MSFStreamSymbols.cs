using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class MSFStreamSymbols : MSFStream
    {
        public MSFStreamSymbols(MSFStream rawstream, int index)
            : base(rawstream.GetFlattenedBuffer())
        {
            Name = $"DBI Symbols ({index})";

            ParseAllSymbols();
        }


        private class Symbol
        {
            public TypedByteSequence<ushort> Size;
            public TypedByteSequence<ushort> Type;

            public virtual void PopulateAnalysis(ListView lvw, ListViewGroup group)
            {
            }
        }

        private class SymbolPublic : Symbol
        {
            public TypedByteSequence<int> PublicType;
            public TypedByteSequence<int> Offset;
            public TypedByteSequence<ushort> SectionIndex;
            public TypedByteSequence<string> Name;

            public SymbolPublic(MSFStreamSymbols stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type)
            {
                Size = size;
                Type = type;

                PublicType = stream.ExtractInt32();
                Offset = stream.ExtractInt32();
                SectionIndex = stream.ExtractUInt16();
                Name = stream.ExtractTerminatedString();

                stream.Extract4ByteAlignment();
            }

            public override void PopulateAnalysis(ListView lvw, ListViewGroup group)
            {
                AddAnalysisItem(lvw, "Public symbol type", group, PublicType);
                AddAnalysisItem(lvw, "Start offset", group, Offset);
                AddAnalysisItem(lvw, "Section index", group, SectionIndex);
                AddAnalysisItem(lvw, "Name", group, Name);
            }
        }

        private class SymbolUDT : Symbol
        {
            public TypedByteSequence<int> TypeIndex;
            public TypedByteSequence<string> Name;

            public SymbolUDT(MSFStreamSymbols stream, TypedByteSequence<ushort> size, TypedByteSequence<ushort> type)
            {
                Size = size;
                Type = type;

                TypeIndex = stream.ExtractInt32();
                Name = stream.ExtractTerminatedString();

                stream.Extract4ByteAlignment();
            }

            public override void PopulateAnalysis(ListView lvw, ListViewGroup group)
            {
                AddAnalysisItem(lvw, "UDT symbol type index", group, TypeIndex);
                AddAnalysisItem(lvw, "UDT name", group, Name);
            }
        }

        
        private List<Symbol> Symbols = new List<Symbol>();
        private int UnknownSymbols = 0;


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var statgroup = lvw.Groups.Add("stats", "Statistics");
            AddAnalysisItem(lvw, "Symbols of unrecognized type", statgroup, $"{UnknownSymbols}");

            int i = 0;
            foreach (var sym in Symbols)
            {
                var symgroup = lvw.Groups.Add($"symbols{i}", $"Symbol {i}");
                AddAnalysisItem(lvw, "Symbol size", symgroup, sym.Size);
                AddAnalysisItem(lvw, "Symbol type", symgroup, sym.Type);

                sym.PopulateAnalysis(lvw, symgroup);

                ++i;
            }
        }

        private void ParseAllSymbols()
        {
            while (ReadOffset < FlattenedBuffer.Length)
            {
                try
                {
                    int begin = ReadOffset;
                    var symsize = ExtractUInt16();
                    var symtype = ExtractUInt16();

                    var sym = MakeSymbol(symsize, symtype);
                    Symbols.Add(sym);

                    ReadOffset = begin + symsize.ExtractedValue + 2;
                    while ((ReadOffset & 3) != 0)
                        ++ReadOffset;
                }
                catch
                {
                    break;
                }
            }
        }


        private Symbol MakeSymbol(TypedByteSequence<ushort> size, TypedByteSequence<ushort> type)
        {
            switch (type.ExtractedValue)
            {
                case 0x1108:     // S_UDT
                    return new SymbolUDT(this, size, type);

                case 0x110e:     // S_PUB32
                    return new SymbolPublic(this, size, type);
            }

            ++UnknownSymbols;
            return new Symbol { Size = size, Type = type };
        }
    }
}
