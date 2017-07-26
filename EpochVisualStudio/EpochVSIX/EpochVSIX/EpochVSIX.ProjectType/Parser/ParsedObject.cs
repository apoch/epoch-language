using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class ParsedObject<T>
    {
        public Token Name;
        public T Object;
    }
}
