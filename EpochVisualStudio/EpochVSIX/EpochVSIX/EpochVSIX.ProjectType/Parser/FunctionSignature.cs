using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class FunctionSignature
    {
        List<FunctionOverload> Overloads = new List<FunctionOverload>();
    }

    class FunctionOverload
    {
        public class Parameter
        {
            public string Name;
            public TypeSignature Type;
        }

        public TypeSignature ReturnType;
    }
}
