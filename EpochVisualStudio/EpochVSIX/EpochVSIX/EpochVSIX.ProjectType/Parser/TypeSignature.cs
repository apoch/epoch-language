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
            if (TypeName == null || TypeName == "nothing")
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

            if (parser.CheckToken(begintoken + 1, "<"))
            {
                signature.TypeParameters = new List<TypeParameter>();
                int totaltokens = begintoken + 2;
                while (!parser.CheckToken(totaltokens, ">"))
                {
                    var tokentype = parser.PeekToken(totaltokens);
                    var tokenname = parser.PeekToken(totaltokens + 1);
                    var param = new TypeParameter { Name = tokenname.Text, ArgumentType = new TypeSignature { TypeName = tokentype.Text } };

                    signature.TypeParameters.Add(param);

                    totaltokens += 2;
                    if (parser.CheckToken(totaltokens, ","))
                        ++totaltokens;
                }
            }

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
            public TypeSignatureInstantiated SpecifiedType;

            public override string ToString()
            {
                return SpecifiedType.ToString();
            }
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


        public override string ToString()
        {
            if (TypeName == null || TypeName == "nothing")
                return "nothing";

            string args = "";
            if (TypeArguments != null && TypeArguments.Count > 0)
            {
                args = "<" + string.Join(", ", TypeArguments) + ">";
            }

            string IsRef = IsReference ? " ref" : "";

            return $"{TypeName}{args}{IsRef}";
        }


        public static TypeSignatureInstantiated Construct(ParseSession parser, int begintoken, int endtoken)
        {
            var signature = new TypeSignatureInstantiated();
            signature.TypeName = parser.PeekToken(begintoken).Text;


            if (parser.CheckToken(begintoken + 1, "<"))
            {
                signature.TypeArguments = new List<TypeArgument>();
                int totaltokens = begintoken + 2;
                while (!parser.CheckToken(totaltokens, ">"))
                {
                    var argtoken = parser.PeekToken(totaltokens);
                    signature.TypeArguments.Add(new TypeArgument { SpecifiedType = new TypeSignatureInstantiated { TypeName = argtoken.Text } });
                    ++totaltokens;

                    if (parser.CheckToken(totaltokens, ","))
                        ++totaltokens;
                }
            }


            int reftoken = endtoken - 1;
            if (reftoken > begintoken)
                signature.TypeIsReference = parser.CheckToken(reftoken, "ref");

            return signature;
        }
    }
}
