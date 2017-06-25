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


        private TypedByteSequence<int> Version;
        private TypedByteSequence<int> Signature;
        private TypedByteSequence<int> Age;

        private TypedByteSequence<Guid> PDBGuid;

        private List<NamedStream> NamedStreams = new List<NamedStream>();

        private TypedByteSequence<int> HashTableSize;
        private TypedByteSequence<int> HashTableCapacity;

        private List<int>[] BitVectors;

        private Dictionary<int, string> NameBuffer;


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var headergroup = lvw.Groups.Add("headers", "PDB Header Info");
            AddAnalysisItem(lvw, "Version", headergroup, Version);
            AddAnalysisItem(lvw, "Signature", headergroup, Signature);
            AddAnalysisItem(lvw, "Age", headergroup, Age);
            AddAnalysisItem(lvw, "GUID", headergroup, PDBGuid);

            var streamgroup = lvw.Groups.Add("streams", "Named Streams");
            foreach (var named in NamedStreams)
                AddAnalysisItem(lvw, named.Name, streamgroup, $"{named.Index}");

            var unknowngroup = lvw.Groups.Add("stuff", "Unknown Data");
            AddAnalysisItem(lvw, "Hash table size", unknowngroup, HashTableSize);
            AddAnalysisItem(lvw, "Hash table capacity", unknowngroup, HashTableCapacity);
            AddAnalysisItem(lvw, "Bit vector (Present) number of words", unknowngroup, $"{BitVectors[0].Count} (0x{BitVectors[0].Count:x})");
            AddAnalysisItem(lvw, "Bit vector (Deleted) number of words", unknowngroup, $"{BitVectors[1].Count} (0x{BitVectors[1].Count:x})");
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
            var length = ExtractInt32();
            NameBuffer = ExtractTerminatedStrings(length.ExtractedValue);
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
                var offsetofname = ExtractInt32();
                NamedStreams.Add(new NamedStream { Index = ExtractInt32().ExtractedValue, Name = NameBuffer[offsetofname.ExtractedValue] });
            }
        }

        private List<int> ExtractBitVector()
        {
            var ret = new List<int>();
            int len = ExtractInt32().ExtractedValue;

            for (int i = 0; i < len; ++i)
                ret.Add(ExtractInt32().ExtractedValue);

            return ret;
        }
    }
}
