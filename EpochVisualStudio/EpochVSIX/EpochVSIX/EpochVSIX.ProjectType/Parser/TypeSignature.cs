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

            public override string ToString()
            {
                return $"{ArgumentType} {Name}";
            }
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


        public override string ToString()
        {
            if (TypeName == null)
                return "nothing";

            string Params = "";
            if (TypeParameters != null && TypeParameters.Count > 0)
            {
                Params = "<" + string.Join(", ", TypeParameters) + ">";
            }

            string IsRef = IsReference ? " ref" : "";

            return $"{TypeName}{Params}{IsRef}";
        }


        public static TypeSignature Construct(ParseSession parser, int begintoken, int endtoken)
        {
            var signature = new TypeSignature();
            signature.TypeName = parser.PeekToken(begintoken).Text;

            int reftoken = endtoken - 1;
            if (reftoken > begintoken)
                signature.TypeIsReference = parser.CheckToken(reftoken, "ref");

            return signature;
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


        public static TypeSignatureInstantiated Construct(ParseSession parser, int begintoken, int endtoken)
        {
            var signature = new TypeSignatureInstantiated();
            signature.TypeName = parser.PeekToken(begintoken).Text;

            int reftoken = endtoken - 1;
            if (reftoken > begintoken)
                signature.TypeIsReference = parser.CheckToken(reftoken, "ref");

            return signature;
        }
    }
}
