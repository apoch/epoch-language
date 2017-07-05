using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class MSFStreamDBIModule : MSFStream
    {
        public MSFStreamDBIModule(MSFStream rawstream, int index, int dbimoduleindex, uint symbytes)
            : base(rawstream.GetFlattenedBuffer())
        {
            Name = $"DBI Module {dbimoduleindex} ({index})";

            ParseAllSymbols(symbytes);
        }


        private TypedByteSequence<uint> Header;

        private List<Symbol> Symbols = new List<Symbol>();



        protected override void SubclassPopulateAnalysis(List<ListViewItem> lvw, ListView lvwcontrol, TreeView tvw)
        {
            var hdrgroup = AddAnalysisGroup(lvwcontrol, tvw, "header", "Header");
            AddAnalysisItem(lvw, tvw, "Chunk header", hdrgroup, Header);


            var symnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("symbolsall", "Symbols");


            int i = 0;
            foreach (var sym in Symbols)
            {
                var symgroup = AddAnalysisGroup(lvwcontrol, tvw, $"symbols{i}", $"Symbol {i}", symnode);
                AddAnalysisItem(lvw, tvw, "Symbol size", symgroup, sym.Size);
                AddAnalysisItem(lvw, tvw, "Symbol type", symgroup, sym.Type);

                sym.PopulateAnalysis(this, lvw, symgroup, tvw);

                symgroup.Node.Tag = sym.CorrespondingByteSequence;

                ++i;
            }
        }



        private void ParseAllSymbols(uint symbytes)
        {
            Header = ExtractUInt32();

            while (ReadOffset < symbytes)
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
