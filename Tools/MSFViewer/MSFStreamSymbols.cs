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
        
        private List<Symbol> Symbols = new List<Symbol>();
        private int UnknownSymbols = 0;


        protected override void SubclassPopulateAnalysis(List<ListViewItem> lvw, ListView lvwcontrol, TreeView tvw)
        {
            var statgroup = AddAnalysisGroup(lvwcontrol, tvw, "stats", "Statistics");
            AddAnalysisItem(lvw, tvw, "Symbols of unrecognized type", statgroup, $"{UnknownSymbols}");


            var symnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("symbolsall", "Symbols");


            int i = 0;
            foreach (var sym in Symbols)
            {
                var symgroup = AddAnalysisGroup(lvwcontrol, tvw, $"symbols{i}", $"Symbol {i} \"{sym}\"", symnode);
                AddAnalysisItem(lvw, tvw, "Symbol size", symgroup, sym.Size);
                AddAnalysisItem(lvw, tvw, "Symbol type", symgroup, sym.Type);

                sym.PopulateAnalysis(this, lvw, symgroup, tvw);

                symgroup.Node.Tag = sym.CorrespondingByteSequence;

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

                    var sym = Symbol.MakeSymbol(this, symsize, symtype);
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
    }
}
