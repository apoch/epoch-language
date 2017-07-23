using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class TypeSignature
    {
        public class TypeParameter
        {
            public string Name;
            public TypeSignature ArgumentType;
        }


        private string TypeName;
        private bool TypeIsReference;
        private List<TypeParameter> TypeParameters;

        public string Name
        {
            get { return TypeName; }
        }

        public bool IsReference
        {
            get { return TypeIsReference; }
        }

        public List<TypeParameter> Parameters
        {
            get { return TypeParameters; }
        }



        public static TypeSignature Parse(IEnumerator<Token> tokens)
        {
            throw new NotImplementedException();
        }
    }

    class TypeSignatureInstantiated
    {
        public class TypeArgument
        {
            public TypeSignature SpecifiedType;
        }


        private string TypeName;
        private bool TypeIsReference;
        private List<TypeArgument> TypeArguments;

        public string Name
        {
            get { return TypeName; }
        }

        public bool IsReference
        {
            get { return TypeIsReference; }
        }

        public List<TypeArgument> Arguments
        {
            get { return TypeArguments; }
        }


        public static TypeSignatureInstantiated Parse(IEnumerator<Token> tokens)
        {
            throw new NotImplementedException();
        }
    }
}
