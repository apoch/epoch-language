using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class SourceFile
    {
        private string FileFullPath;


        public string Path
        {
            get { return FileFullPath; }
        }


        private void AugmentProject(Project project, string filecontents)
        {
            project.RegisterSourceFile(FileFullPath, this);

            var lexer = new LexSession(this, filecontents);
            var parser = new ParseSession(lexer);

            parser.AugmentProject(project);
        }


        public static SourceFile AugmentProject(Project project, string filepath, string filecontents)
        {
            var file = new SourceFile();
            file.FileFullPath = filepath;

            file.AugmentProject(project, filecontents);

            return file;
        }
    }
}
