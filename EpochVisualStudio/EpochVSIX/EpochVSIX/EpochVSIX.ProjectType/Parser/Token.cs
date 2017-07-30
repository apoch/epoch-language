using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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
