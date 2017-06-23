using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MSFViewer
{
    class MSF
    {
        public MSF(string filename)
        {
            EntireFile = new MSFStreamEntireFile(File.ReadAllBytes(filename));
            EntireFile.Name = "(Entire file)";
        }

        public MSFStreamEntireFile EntireFile;
    }
}
