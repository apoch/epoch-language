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

            public ByteSequence OriginalSequence;
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

            public ByteSequence OriginalSequence;
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

        private int SectionContributionBeginOffset;
        private int ModulesBeginOffset;


        protected override void SubclassPopulateAnalysis(List<ListViewItem> lvw, ListView lvwcontrol, TreeView tvw)
        {
            var headergroup = AddAnalysisGroup(lvwcontrol, tvw, "headers", "DBI Header Info");
            AddAnalysisItem(lvw, tvw, "Signature", headergroup, Signature);
            AddAnalysisItem(lvw, tvw, "Version", headergroup, Version);
            AddAnalysisItem(lvw, tvw, "Age", headergroup, Age);

            var streamsgroup = AddAnalysisGroup(lvwcontrol, tvw, "streams", "Stream Indices");
            AddAnalysisItem(lvw, tvw, "Globals stream index", streamsgroup, GlobalsStreamIndex);
            AddAnalysisItem(lvw, tvw, "Publics stream index", streamsgroup, PublicsStreamIndex);
            AddAnalysisItem(lvw, tvw, "Symbols stream index", streamsgroup, SymbolsStreamIndex);

            var buildgroup = AddAnalysisGroup(lvwcontrol, tvw, "build", "Build Info");
            AddAnalysisItem(lvw, tvw, "Build number", buildgroup, BuildNumber);
            AddAnalysisItem(lvw, tvw, "PDB DLL version", buildgroup, PDBDLLVersion);
            AddAnalysisItem(lvw, tvw, "PDB DLL rebuild", buildgroup, PDBDLLBuild);

            var subsgroup = AddAnalysisGroup(lvwcontrol, tvw, "subs", "Substreams Info");
            AddAnalysisItem(lvw, tvw, "Module substream size", subsgroup, ModuleSubstreamSize);
            AddAnalysisItem(lvw, tvw, "Section contributions size", subsgroup, SectionContributionsSize);
            AddAnalysisItem(lvw, tvw, "Section map size", subsgroup, SectionMapSize);
            AddAnalysisItem(lvw, tvw, "File substream size", subsgroup, FileSubstreamSize);
            AddAnalysisItem(lvw, tvw, "Type-server map size", subsgroup, TypeServerMapSize);
            AddAnalysisItem(lvw, tvw, "Dbg header info size", subsgroup, DbgHeaderInfoSize);
            AddAnalysisItem(lvw, tvw, "EC substream size", subsgroup, ECSubstreamSize);

            var miscgroup = AddAnalysisGroup(lvwcontrol, tvw, "misc", "Miscellaneous Info");
            AddAnalysisItem(lvw, tvw, "Flags", miscgroup, Flags);
            AddAnalysisItem(lvw, tvw, "Machine type", miscgroup, MachineType);
            AddAnalysisItem(lvw, tvw, "MFC type server index", miscgroup, MFCTypeServerIndex);
            AddAnalysisItem(lvw, tvw, "Padding (1)", miscgroup, Padding1);
            AddAnalysisItem(lvw, tvw, "Section contribution version", miscgroup, SCVersion);


            var modnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("modsall", "Modules");
            modnode.Tag = new MaskedByteSequence(FlattenedBuffer, ModulesBeginOffset, ModuleSubstreamSize.ExtractedValue, "Modules");

            int i = 0;
            foreach (var mod in Mods)
            {
                var group = AddAnalysisGroup(lvwcontrol, tvw, $"mod{i}", $"Module {i} ({mod.SourceFileName})", modnode);
                AddAnalysisItem(lvw, tvw, "Mystery header", group, mod.UnusedModuleHeader);
                AddAnalysisItem(lvw, tvw, "Flags", group, mod.Flags);
                AddAnalysisItem(lvw, tvw, "Stream number", group, mod.StreamNumber);
                AddAnalysisItem(lvw, tvw, "Symbol size", group, mod.SymbolSize);
                AddAnalysisItem(lvw, tvw, "Bytes of line number data", group, mod.LineNumberBytes);
                AddAnalysisItem(lvw, tvw, "Bytes of C13 line number data", group, mod.C13LineNumberBytes);
                AddAnalysisItem(lvw, tvw, "Number of contributing files", group, mod.NumContributingFiles);
                AddAnalysisItem(lvw, tvw, "Padding", group, mod.Padding);
                AddAnalysisItem(lvw, tvw, "File name offset in string table", group, mod.FileNameOffset);
                AddAnalysisItem(lvw, tvw, "Source file name index", group, mod.SourceFileNameIndex);
                AddAnalysisItem(lvw, tvw, "PDB path index", group, mod.PDBPathIndex);
                AddAnalysisItem(lvw, tvw, "Source file name", group, mod.SourceFileName);
                AddAnalysisItem(lvw, tvw, "Object file name", group, mod.ObjectFileName);

                group.Node.Tag = mod.OriginalSequence;

                ++i;
            }

            var scnode = tvw.Nodes.Find("root", false)[0].Nodes.Add("scall", "Section Contributions");
            scnode.Tag = new MaskedByteSequence(FlattenedBuffer, SectionContributionBeginOffset, SectionContributionsSize.ExtractedValue, "Section Contributions");

            i = 0;
            foreach (var sc in Contributions)
            {
                var group = AddAnalysisGroup(lvwcontrol, tvw, $"sc{i}", $"Section Contribution {i}", scnode);
                AddAnalysisItem(lvw, tvw, "Section index", group, sc.SectionIndex);
                AddAnalysisItem(lvw, tvw, "Padding", group, sc.Padding1);
                AddAnalysisItem(lvw, tvw, "Offset", group, sc.Offset);
                AddAnalysisItem(lvw, tvw, "Size", group, sc.Size);
                AddAnalysisItem(lvw, tvw, "Characteristics", group, sc.Characteristics);
                AddAnalysisItem(lvw, tvw, "Module index", group, sc.ModuleIndex);
                AddAnalysisItem(lvw, tvw, "Padding", group, sc.Padding2);
                AddAnalysisItem(lvw, tvw, "Data CRC", group, sc.DataCRC);
                AddAnalysisItem(lvw, tvw, "Relocations CRC", group, sc.RelocationCRC);

                group.Node.Tag = sc.OriginalSequence;

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
            ModulesBeginOffset = begin;

            while (ReadOffset - begin < ModuleSubstreamSize.ExtractedValue)
            {
                var mod = ParseSingleModule();
                Mods.Add(mod);
            }
        }

        private void ParseSectionContributions()
        {
            int begin = ReadOffset;
            SectionContributionBeginOffset = begin;

            SCVersion = ExtractUInt32();

            while (ReadOffset - begin < SectionContributionsSize.ExtractedValue)
            {
                var sc = ParseSectionContribution();
                Contributions.Add(sc);
            }
        }

        private SectionContribution ParseSectionContribution()
        {
            var seq = new ByteSequence(FlattenedBuffer, ReadOffset, sizeof(ushort) * 4 + sizeof(uint) * 5);

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

            ret.OriginalSequence = seq;

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

            var seq = new ByteSequence(FlattenedBuffer, begin, ReadOffset - begin);
            ret.OriginalSequence = seq;

            return ret;
        }
    }
}
