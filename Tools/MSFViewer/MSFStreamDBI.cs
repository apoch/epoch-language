using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MSFViewer
{
    class MSFStreamDBI : MSFStream
    {
        public MSFStreamDBI(int streamindex, byte[] entirefile, int streamsize, List<int> blocks, int blocksize)
            : base(streamindex, entirefile, streamsize, blocks, blocksize)
        {
            ParseHeaders();

            // TODO
            //ParseModules();
            //ParseSectionContributions();
            //ParseSectionMap();
            //ParseFiles();
        }


        private int Signature;
        private int Version;
        private int Age;

        private short GlobalsStreamIndex;
        private short BuildNumber;
        private short PublicsStreamIndex;
        private short PDBDLLVersion;
        private short SymbolsStreamIndex;
        private short PDBDLLBuild;

        private int ModuleSubstreamSize;
        private int SectionContributionsSize;
        private int SectionMapSize;
        private int FileSubstreamSize;

        private int TypeServerMapSize;
        private int MFCTypeServerIndex;
        private int DbgHeaderInfoSize;
        private int ECSubstreamSize;

        private short Flags;
        private short MachineType;

        private int Padding1;


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var headergroup = lvw.Groups.Add("headers", "DBI Header Info");
            AddAnalysisItem(lvw, "Signature", $"{Signature} (0x{Signature:X})", headergroup);
            AddAnalysisItem(lvw, "Version", $"{Version} (0x{Version:X})", headergroup);
            AddAnalysisItem(lvw, "Age", $"{Age} (0x{Age:X})", headergroup);

            var streamsgroup = lvw.Groups.Add("streams", "Stream Indices");
            AddAnalysisItem(lvw, "Globals stream index", $"{GlobalsStreamIndex}", streamsgroup);
            AddAnalysisItem(lvw, "Publics stream index", $"{PublicsStreamIndex}", streamsgroup);
            AddAnalysisItem(lvw, "Symbols stream index", $"{SymbolsStreamIndex}", streamsgroup);

            var buildgroup = lvw.Groups.Add("build", "Build Info");
            AddAnalysisItem(lvw, "Build number", $"{BuildNumber}", buildgroup);
            AddAnalysisItem(lvw, "PDB DLL version", $"{PDBDLLVersion}", buildgroup);
            AddAnalysisItem(lvw, "PDB DLL rebuild", $"{PDBDLLBuild}", buildgroup);

            var subsgroup = lvw.Groups.Add("subs", "Substreams Info");
            AddAnalysisItem(lvw, "Module substream size", $"{ModuleSubstreamSize} (0x{ModuleSubstreamSize:X})", subsgroup);
            AddAnalysisItem(lvw, "Section contributions size", $"{SectionContributionsSize} (0x{SectionContributionsSize:X})", subsgroup);
            AddAnalysisItem(lvw, "Section map size", $"{SectionMapSize} (0x{SectionMapSize:X})", subsgroup);
            AddAnalysisItem(lvw, "File substream size", $"{FileSubstreamSize} (0x{FileSubstreamSize:X})", subsgroup);
            AddAnalysisItem(lvw, "Type-server map size", $"{TypeServerMapSize} (0x{TypeServerMapSize:X})", subsgroup);
            AddAnalysisItem(lvw, "Dbg header info size", $"{DbgHeaderInfoSize} (0x{DbgHeaderInfoSize:X})", subsgroup);
            AddAnalysisItem(lvw, "EC substream size", $"{ECSubstreamSize} (0x{ECSubstreamSize:X})", subsgroup);

            var miscgroup = lvw.Groups.Add("misc", "Miscellaneous Info");
            AddAnalysisItem(lvw, "Flags", $"{Flags} (0x{Flags:X})", miscgroup);
            AddAnalysisItem(lvw, "Machine type", $"{MachineType} (0x{MachineType:X})", miscgroup);
            AddAnalysisItem(lvw, "MFC type server index", $"{MFCTypeServerIndex}", miscgroup);
            AddAnalysisItem(lvw, "Padding (1)", $"{Padding1} (0x{Padding1})", miscgroup);
        }


        private void ParseHeaders()
        {
            Signature = ExtractInt32();
            Version = ExtractInt32();
            Age = ExtractInt32();

            GlobalsStreamIndex = ExtractInt16();
            BuildNumber = ExtractInt16();
            PublicsStreamIndex = ExtractInt16();
            PDBDLLVersion = ExtractInt16();
            SymbolsStreamIndex = ExtractInt16();
            PDBDLLBuild = ExtractInt16();

            ModuleSubstreamSize = ExtractInt32();
            SectionContributionsSize = ExtractInt32();
            SectionMapSize = ExtractInt32();
            FileSubstreamSize = ExtractInt32();

            TypeServerMapSize = ExtractInt32();
            MFCTypeServerIndex = ExtractInt32();
            DbgHeaderInfoSize = ExtractInt32();
            ECSubstreamSize = ExtractInt32();

            Flags = ExtractInt16();
            MachineType = ExtractInt16();

            Padding1 = ExtractInt32();
        }
    }
}
