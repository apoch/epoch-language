using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class MSFStreamPDBInfo : MSFStream
    {
        public MSFStreamPDBInfo(int streamindex, byte[] entirefile, int streamsize, List<int> blocks, int blocksize)
            : base(streamindex, entirefile, streamsize, blocks, blocksize)
        {
            BitVectors = new List<int>[2];

            ParsePDBHeader();
            ParseNamedStreams();
            ParseBitVectors();
            ParseStreamMetadata();
        }

        public string GetNameOfStream(int index)
        {
            foreach (var named in NamedStreams)
            {
                if (named.Index == index)
                    return $"Named Stream \"{named.Name}\" ({index})";
            }

            return null;
        }


        private class NamedStream
        {
            public int Index;
            public string Name;
        }


        private int Version = 0;
        private int Signature = 0;
        private int Age = 0;

        private Guid PDBGuid;

        private List<NamedStream> NamedStreams = new List<NamedStream>();

        private int HashTableSize = 0;
        private int HashTableCapacity = 0;

        private List<int>[] BitVectors;

        private Dictionary<int, string> NameBuffer;


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var headergroup = lvw.Groups.Add("headers", "PDB Header Info");
            AddAnalysisItem(lvw, "Version", $"{Version} (0x{Version:X})", headergroup);
            AddAnalysisItem(lvw, "Signature", $"{Signature} (0x{Signature:X})", headergroup);
            AddAnalysisItem(lvw, "Age", $"{Age} (0x{Age:X})", headergroup);
            AddAnalysisItem(lvw, "GUID", $"{PDBGuid}", headergroup);

            var streamgroup = lvw.Groups.Add("streams", "Named Streams");
            foreach (var named in NamedStreams)
                AddAnalysisItem(lvw, named.Name, $"{named.Index}", streamgroup);

            var unknowngroup = lvw.Groups.Add("stuff", "Unknown Data");
            AddAnalysisItem(lvw, "Hash table size", $"{HashTableSize} (0x{HashTableSize:X})", unknowngroup);
            AddAnalysisItem(lvw, "Hash table capacity", $"{HashTableCapacity} (0x{HashTableCapacity:X})", unknowngroup);
            AddAnalysisItem(lvw, "Bit vector (Present) number of words", $"{BitVectors[0].Count} (0x{BitVectors[0].Count:X})", unknowngroup);
            AddAnalysisItem(lvw, "Bit vector (Deleted) number of words", $"{BitVectors[1].Count} (0x{BitVectors[1].Count:X})", unknowngroup);
        }


        private void ParsePDBHeader()
        {
            Version = ExtractInt32();
            Signature = ExtractInt32();
            Age = ExtractInt32();

            PDBGuid = ExtractGuid();
        }

        private void ParseNamedStreams()
        {
            int length = ExtractInt32();
            NameBuffer = ExtractTerminatedStrings(length);
        }

        private void ParseBitVectors()
        {
            HashTableSize = ExtractInt32();
            HashTableCapacity = ExtractInt32();

            BitVectors[0] = ExtractBitVector();
            BitVectors[1] = ExtractBitVector();
        }

        private void ParseStreamMetadata()
        {
            foreach (var name in NameBuffer)
            {
                int offsetofname = ExtractInt32();
                NamedStreams.Add(new NamedStream { Index = ExtractInt32(), Name = NameBuffer[offsetofname] });
            }
        }

        private List<int> ExtractBitVector()
        {
            var ret = new List<int>();
            int len = ExtractInt32();

            for (int i = 0; i < len; ++i)
                ret.Add(ExtractInt32());

            return ret;
        }
    }
}
