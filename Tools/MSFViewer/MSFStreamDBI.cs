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
            Mods = new List<Mod>();
            Contributions = new List<SectionContribution>();

            ParseHeaders();
            ParseModules();
            ParseSectionContributions();

            // TODO
            //ParseSectionMap();
            //ParseFiles();
        }


        private class Mod
        {
            public int UnusedModuleHeader;
            public short Flags;
            public short StreamNumber;
            public int SymbolSize;
            public int LineNumberBytes;
            public int C13LineNumberBytes;
            public short NumContributingFiles;
            public short Padding;
            public int FileNameOffset;
            public int SourceFileNameIndex;
            public int PDBPathIndex;
            public string SourceFileName;
            public string ObjectFileName;
        }

        private class SectionContribution
        {
            public short SectionIndex;
            public short Padding1;
            public int Offset;
            public int Size;
            public int Characteristics;
            public short ModuleIndex;
            public short Padding2;
            public int DataCRC;
            public int RelocationCRC;
        }

        private List<Mod> Mods;
        private List<SectionContribution> Contributions;

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

        private uint SCVersion;


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
            AddAnalysisItem(lvw, "Padding (1)", $"{Padding1} (0x{Padding1:X})", miscgroup);
            AddAnalysisItem(lvw, "Section contribution version", $"{SCVersion} ({SCVersion - 0xeffe0000})", miscgroup);

            int i = 0;
            foreach (var mod in Mods)
            {
                var group = lvw.Groups.Add($"mod{i}", $"Module {i} ({mod.SourceFileName})");
                AddAnalysisItem(lvw, "Mystery header", $"{mod.UnusedModuleHeader} (0x{mod.UnusedModuleHeader:X})", group);
                AddAnalysisItem(lvw, "Flags", $"{mod.Flags} (0x{mod.Flags:X})", group);
                AddAnalysisItem(lvw, "Stream number", $"{mod.StreamNumber}", group);
                AddAnalysisItem(lvw, "Symbol size", $"{mod.SymbolSize} (0x{mod.SymbolSize:X})", group);
                AddAnalysisItem(lvw, "Bytes of line number data", $"{mod.LineNumberBytes} (0x{mod.LineNumberBytes:X})", group);
                AddAnalysisItem(lvw, "Bytes of C13 line number data", $"{mod.C13LineNumberBytes} (0x{mod.C13LineNumberBytes:X})", group);
                AddAnalysisItem(lvw, "Number of contributing files", $"{mod.NumContributingFiles} (0x{mod.NumContributingFiles:X})", group);
                AddAnalysisItem(lvw, "Padding", $"{mod.Padding} (0x{mod.Padding:X})", group);
                AddAnalysisItem(lvw, "File name offset in string table", $"{mod.FileNameOffset} (0x{mod.FileNameOffset:X})", group);
                AddAnalysisItem(lvw, "Source file name index", $"{mod.SourceFileNameIndex} (0x{mod.SourceFileNameIndex:X})", group);
                AddAnalysisItem(lvw, "PDB path index", $"{mod.PDBPathIndex} (0x{mod.PDBPathIndex:X})", group);
                AddAnalysisItem(lvw, "Source file name", $"{mod.SourceFileName}", group);
                AddAnalysisItem(lvw, "Object file name", $"{mod.ObjectFileName}", group);

                ++i;
            }

            i = 0;
            foreach (var sc in Contributions)
            {
                var group = lvw.Groups.Add($"sc{i}", $"Section Contribution {i}");
                AddAnalysisItem(lvw, "Section index", $"{sc.SectionIndex}", group);
                AddAnalysisItem(lvw, "Padding", $"{sc.Padding1} (0x{sc.Padding1:X})", group);
                AddAnalysisItem(lvw, "Offset", $"{sc.Offset} (0x{sc.Offset:X})", group);
                AddAnalysisItem(lvw, "Size", $"{sc.Size} (0x{sc.Size:X})", group);
                AddAnalysisItem(lvw, "Characteristics", $"0x{sc.Characteristics:X}", group);
                AddAnalysisItem(lvw, "Module index", $"{sc.ModuleIndex}", group);
                AddAnalysisItem(lvw, "Padding", $"{sc.Padding2} (0x{sc.Padding2:X})", group);
                AddAnalysisItem(lvw, "Data CRC", $"{sc.DataCRC} (0x{sc.DataCRC:X})", group);
                AddAnalysisItem(lvw, "Relocations CRC", $"{sc.RelocationCRC} (0x{sc.RelocationCRC:X})", group);

                ++i;
            }
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

        private void ParseModules()
        {
            int begin = ReadOffset;
            while (ReadOffset - begin < ModuleSubstreamSize)
            {
                var mod = ParseSingleModule();
                Mods.Add(mod);
            }
        }

        private void ParseSectionContributions()
        {
            int begin = ReadOffset;

            SCVersion = ExtractUInt32();

            while (ReadOffset - begin < SectionContributionsSize)
            {
                var sc = ParseSectionContribution();
                Contributions.Add(sc);
            }
        }

        private SectionContribution ParseSectionContribution()
        {
            var ret = new SectionContribution();
            ret.SectionIndex = ExtractInt16();
            ret.Padding1 = ExtractInt16();
            ret.Offset = ExtractInt32();
            ret.Size = ExtractInt32();
            ret.Characteristics = ExtractInt32();
            ret.ModuleIndex = ExtractInt16();
            ret.Padding2 = ExtractInt16();
            ret.DataCRC = ExtractInt32();
            ret.RelocationCRC = ExtractInt32();

            return ret;
        }

        private Mod ParseSingleModule()
        {
            int begin = ReadOffset;
            var ret = new Mod();

            ret.UnusedModuleHeader = ExtractInt32();
            ParseSectionContribution();

            ret.Flags = ExtractInt16();
            ret.StreamNumber = ExtractInt16();
            ret.SymbolSize = ExtractInt32();
            ret.LineNumberBytes = ExtractInt32();
            ret.C13LineNumberBytes = ExtractInt32();
            ret.NumContributingFiles = ExtractInt16();
            ret.Padding = ExtractInt16();
            ret.FileNameOffset = ExtractInt32();
            ret.SourceFileNameIndex = ExtractInt32();
            ret.PDBPathIndex = ExtractInt32();
            ret.SourceFileName = ExtractTerminatedString();
            ret.ObjectFileName = ExtractTerminatedString();

            if (ReadOffset - begin < ModuleSubstreamSize)
                Extract4ByteAlignment();

            return ret;
        }
    }
}
