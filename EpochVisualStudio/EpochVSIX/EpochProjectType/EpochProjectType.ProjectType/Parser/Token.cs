//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Basic wrapper for a tokenized chunk of text.
//

namespace EpochVSIX.Parser
{
    class Token
    {
        public string Text;
        public int Line;
        public int Column;
        public SourceFile File;

        public override string ToString()
        {
            return Text;
        }
    }
}
