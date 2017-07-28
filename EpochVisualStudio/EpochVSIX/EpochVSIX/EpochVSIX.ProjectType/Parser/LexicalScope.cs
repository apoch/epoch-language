using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class LexicalScope
    {
        public string Name;
        public int StartLine;
        public int StartColumn;
        public int EndLine;
        public int EndColumn;

        public List<Variable> Variables = new List<Variable>();

        public LexicalScope ParentScope = null;
    }
}
