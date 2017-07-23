using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class Variable
    {
        public enum Origins
        {
            Local,
            Parameter,
            Return
        }

        public string Name;
        public TypeSignatureInstantiated Type;
        public Origins Origin;
    }
}
