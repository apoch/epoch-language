//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Wrapper for an individual source code file in a project.
//
// Currently this is mostly plumbing but could potentially be expanded
// to do more things as features are added to the VS extension.
//

namespace EpochVSIX.Parser
{
    class SourceFile
    {
        private string FileFullPath;


        public string Path
        {
            get { return FileFullPath; }
        }


        //
        // Parse some code and add the data obtained to a Project.
        //
        private void AugmentProject(Project project, string filecontents)
        {
            project.RegisterSourceFile(FileFullPath, this);

            var lexer = new LexSession(this, filecontents);
            var parser = new ParseSession(lexer);

            parser.AugmentProject(project);
        }


        //
        // Helper routine to load a source code file into a Project object.
        //
        public static SourceFile AugmentProject(Project project, string filepath, string filecontents)
        {
            var file = new SourceFile();
            file.FileFullPath = filepath;

            file.AugmentProject(project, filecontents);

            return file;
        }
    }
}
