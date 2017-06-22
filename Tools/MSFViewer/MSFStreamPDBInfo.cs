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
            ParsePDBHeader();
            ParseNamedStreams();
            ParseBitVectors();
            ParseStreamMetadata();
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

        private int BitVector1 = 0;
        private int BitVector2 = 0;
        private int BitVector3 = 0;
        private int BitVector4 = 0;

        private int StreamIndex = 0;

        private int Signature2 = 0;


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var headergroup = lvw.Groups.Add("headers", "PDB Header Info");
            AddAnalysisItem(lvw, "Version", $"{Version}", headergroup);
            AddAnalysisItem(lvw, "Signature", $"{Signature}", headergroup);
            AddAnalysisItem(lvw, "Age", $"{Age}", headergroup);
            AddAnalysisItem(lvw, "GUID", $"{PDBGuid}", headergroup);

            var streamgroup = lvw.Groups.Add("streams", "Named Streams");
            foreach (var named in NamedStreams)
                AddAnalysisItem(lvw, named.Name, $"{named.Index}", streamgroup);

            var unknowngroup = lvw.Groups.Add("stuff", "Unknown Data");
            AddAnalysisItem(lvw, "Hash table size", $"{HashTableSize}", unknowngroup);
            AddAnalysisItem(lvw, "Hash table capacity", $"{HashTableCapacity}", unknowngroup);
            AddAnalysisItem(lvw, "Bit vector 1", $"{BitVector1}", unknowngroup);
            AddAnalysisItem(lvw, "Bit vector 2", $"{BitVector2}", unknowngroup);
            AddAnalysisItem(lvw, "Bit vector 3", $"{BitVector3}", unknowngroup);
            AddAnalysisItem(lvw, "Bit vector 4", $"{BitVector4}", unknowngroup);
            AddAnalysisItem(lvw, "Signature 2", $"{Signature2}", unknowngroup);

            var pdbstreamgroup = lvw.Groups.Add("pdbstream", "PDB Stream");
            AddAnalysisItem(lvw, "Stream index", $"{StreamIndex}", pdbstreamgroup);
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
            string name = ExtractStringWithLength(length);

            NamedStreams.Add(new NamedStream { Index = 0, Name = name });

            // TODO - associate name with stream?
        }

        private void ParseBitVectors()
        {
            HashTableSize = ExtractInt32();
            HashTableCapacity = ExtractInt32();

            BitVector1 = ExtractInt32();
            BitVector2 = ExtractInt32();
            BitVector3 = ExtractInt32();
            BitVector4 = ExtractInt32();
        }

        private void ParseStreamMetadata()
        {
            StreamIndex = ExtractInt32();

            Signature2 = ExtractInt32();
        }
    }
}
