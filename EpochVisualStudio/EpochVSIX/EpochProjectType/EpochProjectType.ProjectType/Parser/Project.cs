//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Wrapper for all data parsed from an .EPRJ project.
//
// This module stores the IntelliSense data produced by a
// ParseSession and its accompanying helper objects. Each
// construct that gets recognized during parsing is added
// to this container for use in syntax completion hinting
// and other tips (such as QuickInfo tooltips).
//
// Additionally this module is responsible for connecting
// to the Visual Studio UI and populating error lists and
// other elements.
//

using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.TextManager.Interop;
using System;
using System.Collections.Generic;
using System.Linq;

namespace EpochVSIX.Parser
{
    //
    // A Project is a logical collection of source code files.
    // It bears a one-to-one relationship with an EPRJ project
    // file in a Visual Studio solution.
    //
    class Project
    {
        //
        // Internal storage for parsed code constructs.
        //

        private Dictionary<string, SourceFile> Files;
        private Dictionary<string, List<LexicalScope>> Scopes;

        private LexicalScope GlobalScope;


        private Dictionary<string, FunctionSignature> FunctionSignatures;
        private Dictionary<string, Structure> StructureDefinitions;
        private Dictionary<string, SumType> SumTypes;
        private Dictionary<string, StrongAlias> StrongAliases;
        private Dictionary<string, WeakAlias> WeakAliases;


        //
        // Internal state for tracking the associated VS project
        // as well as debouncing back-to-back parsing requests.
        //

        private IVsHierarchy Hierarchy;
        private DateTime LastParseTime;


        //
        // Construct and initialize a Project wrapper object.
        //
        public Project(IVsHierarchy hierarchy)
        {
            ResetContents();

            LastParseTime = DateTime.MinValue;
            Hierarchy = hierarchy;
        }

        //
        // Issue a request to parse the project. Automatically
        // early-outs if parsing has occurred recently to help
        // avoid slamming the main thread with redundant work.
        //
        public void ParseIfOutdated()
        {
            if (DateTime.Now < LastParseTime.AddSeconds(15.0))
                return;

            LastParseTime = DateTime.Now;
            ResetContents();

            var filenames = new HashSet<string>();
            ParseHierarchy(Hierarchy, (uint)VSConstants.VSITEMID.Root, filenames);

            foreach (var filename in filenames)
            {
                string text = System.IO.File.ReadAllText(filename);
                SourceFile.AugmentProject(this, filename, text);
            }
        }

        //
        // Routine for storing a parsed function into the project.
        //
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

        //
        // Simple helper for stashing parsed global variables.
        //
        // Globals get special treatment since they can be visible
        // at any scope level in any module/file.
        //
        public void RegisterGlobalVariable(Variable variable)
        {
            GlobalScope.Variables.Add(variable);
        }

        //
        // Helper for registering a parsed source file.
        //
        // TODO - does registering files actually get us anything?
        //
        public void RegisterSourceFile(string path, SourceFile file)
        {
            if (Files.ContainsKey(path))
                Files[path] = file;
            else
                Files.Add(path, file);
        }

        //
        // Helper for registering a strong type alias.
        //
        public void RegisterStrongAlias(Token nametoken, StrongAlias alias)
        {
            if (!StrongAliases.ContainsKey(nametoken.Text))
                StrongAliases.Add(nametoken.Text, alias);
        }

        //
        // Helper for registering a structure type definition.
        //
        public void RegisterStructureType(Token nametoken, Structure structure)
        {
            if (!StructureDefinitions.ContainsKey(nametoken.Text))
                StructureDefinitions.Add(nametoken.Text, structure);
        }

        //
        // Helper for registering a sum type definition.
        //
        public void RegisterSumType(Token nametoken, SumType sumtype)
        {
            if (!SumTypes.ContainsKey(nametoken.Text))
                SumTypes.Add(nametoken.Text, sumtype);
        }

        //
        // Helper for registering a weak type alias.
        //
        public void RegisterWeakAlias(Token nametoken, WeakAlias alias)
        {
            if (!WeakAliases.ContainsKey(nametoken.Text))
                WeakAliases.Add(nametoken.Text, alias);
        }

        //
        // Retrieve a flattened list of functions defined in the project.
        //
        // Note that currently this may be a moderately expensive operation.
        //
        public List<FunctionSignature> GetAvailableFunctionSignatures()
        {
            return FunctionSignatures.Select(kvp => kvp.Value).ToList();
        }

        //
        // Retrieve a mapping of structure names to their definitions.
        //
        public Dictionary<string, Structure> GetAvailableStructureDefinitions()
        {
            return StructureDefinitions;
        }

        //
        // Retrieve a flattened list of type names defined in the project.
        //
        // May be moderately expensive, so call sparingly if possible.
        //
        public List<string> GetAvailableTypeNames()
        {
            var t = SumTypes.Select(kvp => kvp.Key).Union(
                    StrongAliases.Select(kvp => kvp.Key).Union(
                    WeakAliases.Select(kvp => kvp.Key)
                    ));

            return t.ToList();
        }

        //
        // Query the scope metadata for a list of variables visible
        // at a given position in a source code file.
        //
        // Generally somewhat expensive in large code files.
        //
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

        //
        // Query the scope metadata for a list of variables visible
        // at a given snapshot point inside a given text buffer.
        //
        public List<Variable> GetAvailableVariables(ITextBuffer buffer, SnapshotPoint pt)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            var line = pt.GetContainingLine().LineNumber;
            var column = pt.Position - pt.GetContainingLine().Start.Position;

            IVsTextBuffer bufferAdapter;
            buffer.Properties.TryGetProperty(typeof(IVsTextBuffer), out bufferAdapter);

            if (bufferAdapter != null)
            {
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

            return new List<Variable>();
        }

        //
        // Check if a given scope contains the requested position.
        //
        // If it does, add all variables defined in the scope to the
        // output list, and also recursively check any child scopes.
        //
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


        //
        // Retrieve the structure definition associated with the given type name.
        // May return null if the name does not map to a defined structure.
        //
        public Structure GetStructureDefinition(string name)
        {
            if (!StructureDefinitions.ContainsKey(name))
                return null;

            return StructureDefinitions[name];
        }

        //
        // Helper to check if a name corresponds to any functions.
        //
        public bool IsRecognizedFunction(string name)
        {
            return FunctionSignatures.ContainsKey(name);
        }

        //
        // Helper to check if a name corresponds to any structure definitions.
        //
        public bool IsRecognizedStructureType(string name)
        {
            return StructureDefinitions.ContainsKey(name);
        }

        //
        // Helper to check if a name is associated with a known type.
        //
        // Explicitly does *NOT* check for structures, so be sure to call
        // IsRecognizedStructureType if appropriate.
        //
        public bool IsRecognizedType(string name)
        {
            return SumTypes.ContainsKey(name) || StrongAliases.ContainsKey(name) || WeakAliases.ContainsKey(name);
        }


        //
        // Traverse a VS hierarchy of items, pulling out Epoch files to parse.
        //
        // Recursively explores the items in a project and looks for Epoch code
        // files that belong to the project. The set of canonical file names of
        // each matching item is accumulated in the "filenames" collection.
        //
        private void ParseHierarchy(IVsHierarchy hierarchy, uint itemId, HashSet<string> filenames)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            IntPtr nestedHiearchyValue = IntPtr.Zero;
            uint nestedItemIdValue = 0;

            Guid nestedHierarchyGuid = typeof(IVsHierarchy).GUID;
            int result = hierarchy.GetNestedHierarchy(itemId, ref nestedHierarchyGuid, out nestedHiearchyValue, out nestedItemIdValue);

            if (result == VSConstants.S_OK && nestedHiearchyValue != IntPtr.Zero && nestedItemIdValue == (uint)VSConstants.VSITEMID.Root)
            {
                IVsHierarchy nestedHierarchy = System.Runtime.InteropServices.Marshal.GetObjectForIUnknown(nestedHiearchyValue) as IVsHierarchy;
                System.Runtime.InteropServices.Marshal.Release(nestedHiearchyValue);

                if (nestedHierarchy != null)
                    ParseHierarchy(nestedHierarchy, (uint)VSConstants.VSITEMID.Root, filenames);
            }
            else
            {
                ParseHierarchyItem(hierarchy, itemId, filenames);

                object value = null;
                result = hierarchy.GetProperty(itemId, (int)__VSHPROPID.VSHPROPID_FirstChild, out value);

                while (result == VSConstants.S_OK && value != null)
                {
                    if (value is int && (VSConstants.VSITEMID)(int)value == VSConstants.VSITEMID.Nil)
                        break;

                    uint visibleChildNode = Convert.ToUInt32(value);
                    ParseHierarchy(hierarchy, visibleChildNode, filenames);

                    value = null;
                    result = hierarchy.GetProperty(visibleChildNode, (int)__VSHPROPID.VSHPROPID_NextSibling, out value);
                }
            }
        }


        //
        // Explore a single item in a hierarchy (project) and flag any associated Epoch code.
        //
        // Does not directly parse the code; instead the filename is added to a collection for
        // later consumption by the parser driver itself (see ParseIfOutdated). This permits a
        // caller to deduplicate parsing requests.
        //
        private void ParseHierarchyItem(IVsHierarchy hierarchy, uint itemId, HashSet<string> filenames)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            string canonicalName = null;
            int result = hierarchy.GetCanonicalName(itemId, out canonicalName);

            if (!string.IsNullOrEmpty(canonicalName))
            {
                if (canonicalName.EndsWith(".epoch", StringComparison.CurrentCultureIgnoreCase))
                    filenames.Add(canonicalName);
            }
        }

        //
        // Reset the contents of the project to a pristine state.
        //
        // This method is designed to return the Project object to a blank
        // configuration as if no code had been parsed. This should be done
        // prior to any large-scale parsing operation to ensure that the
        // data extracted from the parsed code does not contain duplicated
        // or stale information.
        //
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
            ErrorProvider.ProviderGuid = new Guid("b95d8222-cdfc-44b4-9635-585db8a10b9f");          // VsPackage.cs - PackageGuid
            ErrorProvider.Tasks.Clear();        // TODO - this is probably too brute force
        }
        
        //
        // Plumbing - an event handler for when a Task is double-clicked, i.e.
        // to navigate to the code causing a parser error.
        //
        internal async void NavigationHandler(object sender, EventArgs args)
        {
            var task = sender as Microsoft.VisualStudio.Shell.Task;
            if (string.IsNullOrEmpty(task.Document))
                return;

            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            var serviceProvider = new ParseSession.ErrorListHelper();
            var openDoc = serviceProvider.GetService(typeof(IVsUIShellOpenDocument)) as IVsUIShellOpenDocument;
            if (openDoc == null)
                return;

            IVsWindowFrame frame;
            Microsoft.VisualStudio.OLE.Interop.IServiceProvider sp;
            IVsUIHierarchy hier;
            uint itemid;
            Guid logicalView = VSConstants.LOGVIEWID_Code;

            if (ErrorHandler.Failed(openDoc.OpenDocumentViaProject(task.Document, ref logicalView, out sp, out hier, out itemid, out frame)) || frame == null)
                return;

            object docData;
            frame.GetProperty((int)__VSFPROPID.VSFPROPID_DocData, out docData);

            VsTextBuffer buffer = docData as VsTextBuffer;
            if (buffer == null)
            {
                IVsTextBufferProvider bufferProvider = docData as IVsTextBufferProvider;
                if (bufferProvider != null)
                {
                    IVsTextLines lines;
                    ErrorHandler.ThrowOnFailure(bufferProvider.GetTextBuffer(out lines));
                    buffer = lines as VsTextBuffer;

                    if (buffer == null)
                        return;
                }
            }

            IVsTextManager mgr = serviceProvider.GetService(typeof(VsTextManagerClass)) as IVsTextManager;
            if (mgr == null)
                return;

            // This whole mess could arguably be a lot simpler as a call to ErrorProvider.Navigate().
            // Unfortunately that API assumes 1-based column/line indices, whereas our task (in order
            // to display correctly in the task list) assumes 0-based. This can be worked around with
            // a trivial addition/substraction, but the kicker is that the column is not used by that
            // particular API. Therefore to preserve the column we do all this crazy stuff instead.
            mgr.NavigateToLineAndColumn(buffer, ref logicalView, task.Line, task.Column, task.Line, task.Column);
        }

    }
}
