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


        private class Checksum
        {
            public TypedByteSequence<uint> FileIndex;
            public TypedByteSequence<byte> ByteCount;
            public TypedByteSequence<byte> Type;
            public TypedByteSequence<byte[]> ChecksumBytes;

            public ByteSequence OriginalSequence;
        }

        private class LinePair
        {
            public TypedByteSequence<uint> CodeOffset;
            public TypedByteSequence<uint> Flags;
        }

        private class LineBlock
        {
            public TypedByteSequence<uint> NameIndex;
            public TypedByteSequence<uint> NumLines;
            public TypedByteSequence<uint> BlockSize;

            public List<LinePair> Pairs;
        }

        private class LineData
        {
            public TypedByteSequence<uint> FunctionOffset;
            public TypedByteSequence<ushort> SegmentOfContribution;
            public TypedByteSequence<ushort> Flags;
            public TypedByteSequence<uint> CodeSize;

            public List<LineBlock> LineBlocks;
            public ByteSequence OriginalSequence;
        }


        private TypedByteSequence<uint> Header;

        private ByteSequence C13LinesSeq;

        private List<Symbol> Symbols = new List<Symbol>();
        private List<Checksum> Checksums = new List<Checksum>();
        private List<LineData> Lines = new List<LineData>();


        protected override void SubclassPopulateAnalysis(List<ListViewItem> lvw, ListView lvwcontrol, TreeView tvw)
        {
            if (Header != null)
            {
                var hdrgroup = AddAnalysisGroup(lvwcontrol, tvw, "header", "Symbols header");
                AddAnalysisItem(lvw, tvw, "Header", hdrgroup, Header);
            }


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


            var linesnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("lines", "Lines");

            var c13linesnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("c13lines", "C13 Lines");
            c13linesnode.Tag = C13LinesSeq;

            var checksumsnode = c13linesnode.Nodes.Add("c13checksums", "Checksums");
            i = 0;
            foreach (var cs in Checksums)
            {
                var csgroup = AddAnalysisGroup(lvwcontrol, tvw, $"checksum{i}", $"Checksum {i}", checksumsnode);
                AddAnalysisItem(lvw, tvw, "File index", csgroup, cs.FileIndex);
                AddAnalysisItem(lvw, tvw, "Byte count", csgroup, cs.ByteCount);
                AddAnalysisItem(lvw, tvw, "Type", csgroup, cs.Type);
                AddAnalysisItem(lvw, tvw, "Checksum", csgroup, new MaskedByteSequence(cs.ChecksumBytes.GetRawBytes(), 0, cs.ByteCount.ExtractedValue, "{binary string}"));

                csgroup.Node.Tag = cs.OriginalSequence;

                ++i;
            }

            var lpnode = c13linesnode.Nodes.Add("c13linepairs", "Line Data");
            i = 0;
            foreach (var ld in Lines)
            {
                var ldgroup = AddAnalysisGroup(lvwcontrol, tvw, $"c13linedata{i}", $"Line data {i}", lpnode);
                AddAnalysisItem(lvw, tvw, "Function offset", ldgroup, ld.FunctionOffset);
                AddAnalysisItem(lvw, tvw, "Segment of contribution", ldgroup, ld.SegmentOfContribution);
                AddAnalysisItem(lvw, tvw, "Flags", ldgroup, ld.Flags);
                AddAnalysisItem(lvw, tvw, "Code size", ldgroup, ld.CodeSize);

                int j = 0;
                foreach (var lb in ld.LineBlocks)
                {
                    var lbgroup = AddAnalysisGroup(lvwcontrol, tvw, $"c13lb{j}", $"Line block {j}", ldgroup.Node);
                    AddAnalysisItem(lvw, tvw, "Name index", lbgroup, lb.NameIndex);
                    AddAnalysisItem(lvw, tvw, "Number of lines", lbgroup, lb.NumLines);
                    AddAnalysisItem(lvw, tvw, "Block code size", lbgroup, lb.BlockSize);

                    int k = 0;
                    foreach (var lp in lb.Pairs)
                    {
                        AddAnalysisItem(lvw, tvw, $"Line pair {k} code offset", lbgroup, lp.CodeOffset);
                        AddAnalysisItem(lvw, tvw, $"Line pair {k} packed data", lbgroup, lp.Flags);

                        ++k;
                    }

                    ++j;
                }

                ldgroup.Node.Tag = ld.OriginalSequence;

                ++i;
            }
        }



        private void ParseAllSymbols(uint symbytes)
        {
            if (symbytes < 4)
            {
                ReadOffset += (int)symbytes;
                return;
            }

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

            int begin = ReadOffset;
            while (ReadOffset < begin + c13linesbytes)
            {
                int chunkbegin = ReadOffset;
                var chunksig = ExtractUInt32();
                var chunksize = ExtractUInt32();
                var chunkend = ReadOffset + chunksize.ExtractedValue;

                if (chunksig.ExtractedValue == 0xf4)
                {
                    while (ReadOffset < chunkend)
                    {
                        int csbegin = ReadOffset;

                        var checksumfile = ExtractUInt32();
                        var bytecount = ExtractByte();
                        var checksumtype = ExtractByte();
                        var checksum = ExtractBytes(bytecount.ExtractedValue);

                        var seq = new ByteSequence(FlattenedBuffer, csbegin, ReadOffset - csbegin);

                        var csr = new Checksum { FileIndex = checksumfile, ByteCount = bytecount, Type = checksumtype, ChecksumBytes = checksum, OriginalSequence = seq };
                        Checksums.Add(csr);

                        Extract4ByteAlignment();
                    }
                }
                else if (chunksig.ExtractedValue == 0xf2)
                {
                    while (ReadOffset < chunkend)
                    {
                        int ldbegin = ReadOffset;
                        var functionoffset = ExtractUInt32();
                        var segcontrib = ExtractUInt16();
                        var flags = ExtractUInt16();
                        var codesize = ExtractUInt32();

                        var ld = new LineData
                        {
                            FunctionOffset = functionoffset,
                            SegmentOfContribution = segcontrib,
                            Flags = flags,
                            CodeSize = codesize,
                            LineBlocks = new List<LineBlock>()
                        };

                        while (ReadOffset < chunkend)
                        {
                            var nameindex = ExtractUInt32();
                            var numlines = ExtractUInt32();
                            var blocksize = ExtractUInt32();

                            var lb = new LineBlock
                            {
                                NameIndex = nameindex,
                                NumLines = numlines,
                                BlockSize = blocksize,
                                Pairs = new List<LinePair>()
                            };

                            for (uint i = 0; i < numlines.ExtractedValue; ++i)
                            {
                                var offset = ExtractUInt32();
                                var lpflags = ExtractUInt32();

                                var lp = new LinePair { CodeOffset = offset, Flags = lpflags };
                                lb.Pairs.Add(lp);
                            }

                            if ((flags.ExtractedValue & 1) != 0)
                            {
                                for (uint i = 0; i < numlines.ExtractedValue; ++i)
                                    ExtractUInt32();
                            }

                            ld.LineBlocks.Add(lb);
                        }

                        ld.OriginalSequence = new ByteSequence(FlattenedBuffer, ldbegin, ReadOffset - ldbegin);

                        Lines.Add(ld);
                    }
                }
                else
                {
                    ReadOffset = (int)(begin + c13linesbytes);
                    //ReadOffset += (int)chunksize.ExtractedValue;
                }
            }
        }
    }
}
