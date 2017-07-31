using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.TextManager.Interop;

namespace EpochVSIX.Parser
{
    class Project
    {
        private Dictionary<string, SourceFile> Files;
        private Dictionary<string, List<LexicalScope>> Scopes;

        private LexicalScope GlobalScope;


        private Dictionary<string, FunctionSignature> FunctionSignatures;
        private Dictionary<string, Structure> StructureDefinitions;
        private Dictionary<string, SumType> SumTypes;
        private Dictionary<string, StrongAlias> StrongAliases;
        private Dictionary<string, WeakAlias> WeakAliases;

        private IVsHierarchy Hierarchy;
        private DateTime LastParseTime;


        public Project(IVsHierarchy hierarchy)
        {
            ResetContents();

            LastParseTime = DateTime.MinValue;
            Hierarchy = hierarchy;
        }


        public void ParseIfOutdated()
        {
            if (DateTime.Now < LastParseTime.AddSeconds(15.0))
                return;

            LastParseTime = DateTime.Now;
            ResetContents();
            ParseHierarchy(Hierarchy, (uint)VSConstants.VSITEMID.Root);
        }


        public void RegisterFunction(FunctionSignature function)
        {
            if (FunctionSignatures.ContainsKey(function.Name.Text))
                FunctionSignatures[function.Name.Text].Overloads.AddRange(function.Overloads);
            else
                FunctionSignatures.Add(function.Name.Text, function);

            foreach (var overload in function.Overloads)
            {
                if (overload.Scope == null)
                    continue;

                string p = overload.Scope.File.Path.ToLower();
                if (!Scopes.ContainsKey(p))
                    Scopes.Add(p, new List<LexicalScope>());

                Scopes[p].Add(overload.Scope);
            }
        }

        public void RegisterGlobalVariable(Variable variable)
        {
            GlobalScope.Variables.Add(variable);
        }

        public void RegisterSourceFile(string path, SourceFile file)
        {
            if (Files.ContainsKey(path))
                Files[path] = file;
            else
                Files.Add(path, file);
        }

        public void RegisterStrongAlias(Token nametoken, StrongAlias alias)
        {
            if (!StrongAliases.ContainsKey(nametoken.Text))
                StrongAliases.Add(nametoken.Text, alias);
        }

        public void RegisterStructureType(Token nametoken, Structure structure)
        {
            if (!StructureDefinitions.ContainsKey(nametoken.Text))
                StructureDefinitions.Add(nametoken.Text, structure);
        }

        public void RegisterSumType(Token nametoken, SumType sumtype)
        {
            if (!SumTypes.ContainsKey(nametoken.Text))
                SumTypes.Add(nametoken.Text, sumtype);
        }

        public void RegisterWeakAlias(Token nametoken, WeakAlias alias)
        {
            if (!WeakAliases.ContainsKey(nametoken.Text))
                WeakAliases.Add(nametoken.Text, alias);
        }


        public List<FunctionSignature> GetAvailableFunctionSignatures()
        {
            return FunctionSignatures.Select(kvp => kvp.Value).ToList();
        }

        public Dictionary<string, Structure> GetAvailableStructureDefinitions()
        {
            return StructureDefinitions;
        }

        public List<string> GetAvailableTypeNames()
        {
            var t = SumTypes.Select(kvp => kvp.Key).Union(
                    StrongAliases.Select(kvp => kvp.Key).Union(
                    WeakAliases.Select(kvp => kvp.Key)
                    ));

            return t.ToList();
        }

        public List<Variable> GetAvailableVariables(string fileraw, int line, int column)
        {
            var ret = new List<Variable>();
            if (GlobalScope != null && GlobalScope.Variables.Count > 0)
                ret.AddRange(GlobalScope.Variables);

            var filelower = fileraw.ToLower();
            if (Scopes.ContainsKey(filelower))
            {
                foreach (var scope in Scopes[filelower])
                {
                    AddScopeTree(scope, ret, line, column);
                }
            }

            return ret;
        }

        public List<Variable> GetAvailableVariables(ITextBuffer buffer, SnapshotPoint pt)
        {
            var line = pt.GetContainingLine().LineNumber;
            var column = pt.Position - pt.GetContainingLine().Start.Position;

            IVsTextBuffer bufferAdapter;
            buffer.Properties.TryGetProperty(typeof(IVsTextBuffer), out bufferAdapter);

            if (bufferAdapter != null)
            {
                ThreadHelper.ThrowIfNotOnUIThread();
                var persistFileFormat = bufferAdapter as IPersistFileFormat;

                if (persistFileFormat != null)
                {
                    string filename = null;
                    uint formatIndex;
                    persistFileFormat.GetCurFile(out filename, out formatIndex);

                    if (!string.IsNullOrEmpty(filename))
                    {
                        return GetAvailableVariables(filename, line, column);
                    }
                }
            }

            // TODO - handle failure cases?

            return new List<Variable>();
        }

        private void AddScopeTree(LexicalScope scope, List<Variable> ret, int line, int column)
        {
            if (scope.StartLine > line)
                return;

            if (scope.EndLine < line)
                return;

            if ((scope.StartLine == line) && (scope.StartColumn > column))
                return;

            if ((scope.EndLine == line) && (scope.EndColumn < column))
                return;

            ret.AddRange(scope.Variables);

            if (scope.ChildScopes == null)
                return;

            foreach (var child in scope.ChildScopes)
                AddScopeTree(child, ret, line, column);
        }


        public Structure GetStructureDefinition(string name)
        {
            if (!StructureDefinitions.ContainsKey(name))
                return null;

            return StructureDefinitions[name];
        }


        public bool IsRecognizedFunction(string name)
        {
            return FunctionSignatures.ContainsKey(name);
        }

        public bool IsRecognizedStructureType(string name)
        {
            return StructureDefinitions.ContainsKey(name);
        }

        public bool IsRecognizedType(string name)
        {
            return SumTypes.ContainsKey(name) || StrongAliases.ContainsKey(name) || WeakAliases.ContainsKey(name);
        }


        private void ParseHierarchy(IVsHierarchy hierarchy, uint itemId)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            int result;
            IntPtr nestedHiearchyValue = IntPtr.Zero;
            uint nestedItemIdValue = 0;
            object value = null;
            uint visibleChildNode;
            Guid nestedHierarchyGuid;
            IVsHierarchy nestedHierarchy;

            nestedHierarchyGuid = typeof(IVsHierarchy).GUID;
            result = hierarchy.GetNestedHierarchy(itemId, ref nestedHierarchyGuid, out nestedHiearchyValue, out nestedItemIdValue);

            if (result == VSConstants.S_OK && nestedHiearchyValue != IntPtr.Zero && nestedItemIdValue == (uint)VSConstants.VSITEMID.Root)
            {
                nestedHierarchy = System.Runtime.InteropServices.Marshal.GetObjectForIUnknown(nestedHiearchyValue) as IVsHierarchy;
                System.Runtime.InteropServices.Marshal.Release(nestedHiearchyValue);

                if (nestedHierarchy != null)
                    ParseHierarchy(nestedHierarchy, (uint)VSConstants.VSITEMID.Root);
            }
            else
            {
                ParseHierarchyItem(hierarchy, itemId);

                result = hierarchy.GetProperty(itemId, (int)__VSHPROPID.VSHPROPID_FirstChild, out value);

                while (result == VSConstants.S_OK && value != null)
                {
                    if (value is int && (VSConstants.VSITEMID)(int)value == VSConstants.VSITEMID.Nil)
                        break;

                    visibleChildNode = Convert.ToUInt32(value);
                    ParseHierarchy(hierarchy, visibleChildNode);

                    value = null;
                    result = hierarchy.GetProperty(visibleChildNode, (int)__VSHPROPID.VSHPROPID_NextSibling, out value);
                }
            }
        }


        private void ParseHierarchyItem(IVsHierarchy hierarchy, uint itemId)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            string canonicalName = null;
            int result = hierarchy.GetCanonicalName(itemId, out canonicalName);

            if (!string.IsNullOrEmpty(canonicalName))
            {
                if (canonicalName.EndsWith(".epoch", StringComparison.CurrentCultureIgnoreCase))
                    ParseFile(canonicalName);
            }
        }

        private void ParseFile(string filename)
        {
            string text = System.IO.File.ReadAllText(filename);
            var file = SourceFile.AugmentProject(this, filename, text);
        }

        private void ResetContents()
        {
            Files = new Dictionary<string, SourceFile>();
            Scopes = new Dictionary<string, List<LexicalScope>>();
            GlobalScope = new LexicalScope();

            FunctionSignatures = new Dictionary<string, FunctionSignature>();
            StructureDefinitions = new Dictionary<string, Structure>();
            SumTypes = new Dictionary<string, SumType>();
            StrongAliases = new Dictionary<string, StrongAlias>();
            WeakAliases = new Dictionary<string, WeakAlias>();

            var helper = new ParseSession.ErrorListHelper();
            var ErrorProvider = new ErrorListProvider(helper);
            ErrorProvider.ProviderName = "Epoch Language";
            ErrorProvider.ProviderGuid = new Guid(VsPackage.PackageGuid);
            ErrorProvider.Tasks.Clear();        // TODO - this is probably too brute force
        }
    }
}
