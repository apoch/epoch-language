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
        public MSFStreamDBI(int streamindex, byte[] entirefile, int streamsize, List<int> blocks, int blocksize, MSFStreamEntireFile ef)
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

            ef.RegisterDBIStreams(GlobalsStreamIndex.ExtractedValue, PublicsStreamIndex.ExtractedValue, SymbolsStreamIndex.ExtractedValue);
        }


        private class Mod
        {
            public TypedByteSequence<int> UnusedModuleHeader;
            public TypedByteSequence<short> Flags;
            public TypedByteSequence<short> StreamNumber;
            public TypedByteSequence<int> SymbolSize;
            public TypedByteSequence<int> LineNumberBytes;
            public TypedByteSequence<int> C13LineNumberBytes;
            public TypedByteSequence<short> NumContributingFiles;
            public TypedByteSequence<short> Padding;
            public TypedByteSequence<int> FileNameOffset;
            public TypedByteSequence<int> SourceFileNameIndex;
            public TypedByteSequence<int> PDBPathIndex;
            public TypedByteSequence<string> SourceFileName;
            public TypedByteSequence<string> ObjectFileName;
        }

        private class SectionContribution
        {
            public TypedByteSequence<short> SectionIndex;
            public TypedByteSequence<short> Padding1;
            public TypedByteSequence<int> Offset;
            public TypedByteSequence<int> Size;
            public TypedByteSequence<int> Characteristics;
            public TypedByteSequence<short> ModuleIndex;
            public TypedByteSequence<short> Padding2;
            public TypedByteSequence<int> DataCRC;
            public TypedByteSequence<int> RelocationCRC;
        }

        private List<Mod> Mods;
        private List<SectionContribution> Contributions;

        private TypedByteSequence<int> Signature;
        private TypedByteSequence<int> Version;
        private TypedByteSequence<int> Age;

        private TypedByteSequence<short> GlobalsStreamIndex;
        private TypedByteSequence<short> BuildNumber;
        private TypedByteSequence<short> PublicsStreamIndex;
        private TypedByteSequence<short> PDBDLLVersion;
        private TypedByteSequence<short> SymbolsStreamIndex;
        private TypedByteSequence<short> PDBDLLBuild;

        private TypedByteSequence<int> ModuleSubstreamSize;
        private TypedByteSequence<int> SectionContributionsSize;
        private TypedByteSequence<int> SectionMapSize;
        private TypedByteSequence<int> FileSubstreamSize;

        private TypedByteSequence<int> TypeServerMapSize;
        private TypedByteSequence<int> MFCTypeServerIndex;
        private TypedByteSequence<int> DbgHeaderInfoSize;
        private TypedByteSequence<int> ECSubstreamSize;

        private TypedByteSequence<short> Flags;
        private TypedByteSequence<short> MachineType;

        private TypedByteSequence<int> Padding1;

        private TypedByteSequence<uint> SCVersion;


        protected override void SubclassPopulateAnalysis(ListView lvw)
        {
            var headergroup = lvw.Groups.Add("headers", "DBI Header Info");
            AddAnalysisItem(lvw, "Signature", headergroup, Signature);
            AddAnalysisItem(lvw, "Version", headergroup, Version);
            AddAnalysisItem(lvw, "Age", headergroup, Age);

            var streamsgroup = lvw.Groups.Add("streams", "Stream Indices");
            AddAnalysisItem(lvw, "Globals stream index", streamsgroup, GlobalsStreamIndex);
            AddAnalysisItem(lvw, "Publics stream index", streamsgroup, PublicsStreamIndex);
            AddAnalysisItem(lvw, "Symbols stream index", streamsgroup, SymbolsStreamIndex);

            var buildgroup = lvw.Groups.Add("build", "Build Info");
            AddAnalysisItem(lvw, "Build number", buildgroup, BuildNumber);
            AddAnalysisItem(lvw, "PDB DLL version", buildgroup, PDBDLLVersion);
            AddAnalysisItem(lvw, "PDB DLL rebuild", buildgroup, PDBDLLBuild);

            var subsgroup = lvw.Groups.Add("subs", "Substreams Info");
            AddAnalysisItem(lvw, "Module substream size", subsgroup, ModuleSubstreamSize);
            AddAnalysisItem(lvw, "Section contributions size", subsgroup, SectionContributionsSize);
            AddAnalysisItem(lvw, "Section map size", subsgroup, SectionMapSize);
            AddAnalysisItem(lvw, "File substream size", subsgroup, FileSubstreamSize);
            AddAnalysisItem(lvw, "Type-server map size", subsgroup, TypeServerMapSize);
            AddAnalysisItem(lvw, "Dbg header info size", subsgroup, DbgHeaderInfoSize);
            AddAnalysisItem(lvw, "EC substream size", subsgroup, ECSubstreamSize);

            var miscgroup = lvw.Groups.Add("misc", "Miscellaneous Info");
            AddAnalysisItem(lvw, "Flags", miscgroup, Flags);
            AddAnalysisItem(lvw, "Machine type", miscgroup, MachineType);
            AddAnalysisItem(lvw, "MFC type server index", miscgroup, MFCTypeServerIndex);
            AddAnalysisItem(lvw, "Padding (1)", miscgroup, Padding1);
            AddAnalysisItem(lvw, "Section contribution version", miscgroup, SCVersion);

            int i = 0;
            foreach (var mod in Mods)
            {
                var group = lvw.Groups.Add($"mod{i}", $"Module {i} ({mod.SourceFileName})");
                AddAnalysisItem(lvw, "Mystery header", group, mod.UnusedModuleHeader);
                AddAnalysisItem(lvw, "Flags", group, mod.Flags);
                AddAnalysisItem(lvw, "Stream number", group, mod.StreamNumber);
                AddAnalysisItem(lvw, "Symbol size", group, mod.SymbolSize);
                AddAnalysisItem(lvw, "Bytes of line number data", group, mod.LineNumberBytes);
                AddAnalysisItem(lvw, "Bytes of C13 line number data", group, mod.C13LineNumberBytes);
                AddAnalysisItem(lvw, "Number of contributing files", group, mod.NumContributingFiles);
                AddAnalysisItem(lvw, "Padding", group, mod.Padding);
                AddAnalysisItem(lvw, "File name offset in string table", group, mod.FileNameOffset);
                AddAnalysisItem(lvw, "Source file name index", group, mod.SourceFileNameIndex);
                AddAnalysisItem(lvw, "PDB path index", group, mod.PDBPathIndex);
                AddAnalysisItem(lvw, "Source file name", group, mod.SourceFileName);
                AddAnalysisItem(lvw, "Object file name", group, mod.ObjectFileName);

                ++i;
            }

            i = 0;
            foreach (var sc in Contributions)
            {
                var group = lvw.Groups.Add($"sc{i}", $"Section Contribution {i}");
                AddAnalysisItem(lvw, "Section index", group, sc.SectionIndex);
                AddAnalysisItem(lvw, "Padding", group, sc.Padding1);
                AddAnalysisItem(lvw, "Offset", group, sc.Offset);
                AddAnalysisItem(lvw, "Size", group, sc.Size);
                AddAnalysisItem(lvw, "Characteristics", group, sc.Characteristics);
                AddAnalysisItem(lvw, "Module index", group, sc.ModuleIndex);
                AddAnalysisItem(lvw, "Padding", group, sc.Padding2);
                AddAnalysisItem(lvw, "Data CRC", group, sc.DataCRC);
                AddAnalysisItem(lvw, "Relocations CRC", group, sc.RelocationCRC);

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
            while (ReadOffset - begin < ModuleSubstreamSize.ExtractedValue)
            {
                var mod = ParseSingleModule();
                Mods.Add(mod);
            }
        }

        private void ParseSectionContributions()
        {
            int begin = ReadOffset;

            SCVersion = ExtractUInt32();

            while (ReadOffset - begin < SectionContributionsSize.ExtractedValue)
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

            if (ReadOffset - begin < ModuleSubstreamSize.ExtractedValue)
                Extract4ByteAlignment();

            return ret;
        }
    }
}
