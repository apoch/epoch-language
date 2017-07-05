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
        public MSFStreamDBIModule(MSFStream rawstream, int index, int dbimoduleindex, uint symbytes, uint linesbytes, uint c13linesbytes)
            : base(rawstream.GetFlattenedBuffer())
        {
            Name = $"DBI Module {dbimoduleindex} ({index})";

            ParseAllSymbols(symbytes);
            ParseAllLines(linesbytes);
            ParseAllC13Lines(c13linesbytes);
        }


        private TypedByteSequence<uint> Header;

        private ByteSequence C13LinesSeq;

        private List<Symbol> Symbols = new List<Symbol>();



        protected override void SubclassPopulateAnalysis(List<ListViewItem> lvw, ListView lvwcontrol, TreeView tvw)
        {
            var hdrgroup = AddAnalysisGroup(lvwcontrol, tvw, "header", "Symbols header");
            AddAnalysisItem(lvw, tvw, "Header", hdrgroup, Header);


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


            var linesnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("lines", "Lines");

            var c13linesnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("c13lines", "C13 Lines");
            c13linesnode.Tag = C13LinesSeq;
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

        private void ParseAllLines(uint linesbytes)
        {
            ReadOffset += (int)linesbytes;
            // TODO - meaningful interpretation of this?
        }

        private void ParseAllC13Lines(uint c13linesbytes)
        {
            C13LinesSeq = new ByteSequence(FlattenedBuffer, ReadOffset, (int)c13linesbytes);
        }
    }
}
