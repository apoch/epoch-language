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
        public MSFStreamSymbols(MSFStream rawstream)
            : base(rawstream.GetFlattenedBuffer())
        {
            Name = "DBI Symbols";

            ParseAllSymbols();
        }


        private class Symbol
        {
            public ushort Size;
            public ushort Type;

            public virtual void PopulateAnalysis(ListView lvw, ListViewGroup group)
            {
            }
        }

        private class SymbolPublic : Symbol
        {
            public int PublicType;
            public int Offset;
            public ushort SectionIndex;
            public string Name;

            public SymbolPublic(MSFStreamSymbols stream, ushort size, ushort type)
            {
                Size = size;
                Type = type;

                PublicType = stream.ExtractInt32();
                Offset = stream.ExtractInt32();
                SectionIndex = (ushort)stream.ExtractInt16();
                Name = stream.ExtractTerminatedString();

                stream.Extract4ByteAlignment();
            }

            public override void PopulateAnalysis(ListView lvw, ListViewGroup group)
            {
                AddAnalysisItem(lvw, "Public symbol type", $"{PublicType} (0x{PublicType:X})", group);
                AddAnalysisItem(lvw, "Start offset", $"{Offset} (0x{Offset:X})", group);
                AddAnalysisItem(lvw, "Section index", $"{SectionIndex} (0x{SectionIndex:X})", group);
                AddAnalysisItem(lvw, "Name", Name, group);
            }
        }

        private class SymbolUDT : Symbol
        {
            public int TypeIndex;
            public string Name;

            public SymbolUDT(MSFStreamSymbols stream, ushort size, ushort type)
            {
                Size = size;
                Type = type;

                TypeIndex = stream.ExtractInt32();
                Name = stream.ExtractTerminatedString();

                stream.Extract4ByteAlignment();
            }

            public override void PopulateAnalysis(ListView lvw, ListViewGroup group)
            {
                AddAnalysisItem(lvw, "UDT symbol type index", $"{TypeIndex} (0x{TypeIndex:X})", group);
                AddAnalysisItem(lvw, "UDT name", Name, group);
            }
        }

        
        private List<Symbol> Symbols = new List<Symbol>();
        private int UnknownSymbols = 0;


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var statgroup = lvw.Groups.Add("stats", "Statistics");
            AddAnalysisItem(lvw, "Symbols of unrecognized type", $"{UnknownSymbols}", statgroup);

            int i = 0;
            foreach (var sym in Symbols)
            {
                var symgroup = lvw.Groups.Add($"symbols{i}", $"Symbol {i}");
                AddAnalysisItem(lvw, "Symbol size", $"{sym.Size} (0x{sym.Size:X})", symgroup);
                AddAnalysisItem(lvw, "Symbol type", $"{sym.Type} (0x{sym.Type:X})", symgroup);

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
                    ushort symsize = (ushort)ExtractInt16();
                    ushort symtype = (ushort)ExtractInt16();

                    var sym = MakeSymbol(symsize, symtype);
                    Symbols.Add(sym);

                    ReadOffset = begin + symsize + 2;
                    while ((ReadOffset & 3) != 0)
                        ++ReadOffset;
                }
                catch
                {
                    break;
                }
            }
        }


        private Symbol MakeSymbol(ushort size, ushort type)
        {
            switch (type)
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
