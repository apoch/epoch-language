using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class Structure
    {
        public Token Name;
        public List<Member> Members;

        public class Member
        {
            public Token Name;
            public TypeSignatureInstantiated Type;
        }


        public override string ToString()
        {
            string memberlist = string.Join(",\r\n", Members.Select(x => $"    {x.Type.ToString()} {x.Name.Text}").ToArray());
            return $"structure {Name.Text} :\r\n{memberlist}";
        }


        internal static ParsedObject<Structure> Parse(ParseSession parser)
        {
            int totaltokens = 0;

            if (!parser.CheckToken(0, "structure"))
                return null;

            var nametoken = parser.PeekToken(1);
            if (nametoken == null)
                return null;

            if (parser.CheckToken(2, "<"))
            {
                if (!parser.ParseTemplateParameters(3, nametoken, out totaltokens))
                    return null;
            }
            else
                totaltokens = 2;
            
            
            if (!parser.CheckToken(totaltokens, ":"))
                return null;

            ++totaltokens;

            var structure = new Structure { Name = nametoken };
            structure.Members = new List<Member>();
            var parsed = new ParsedObject<Structure> { Name = nametoken, Object = structure };

            bool moremembers = true;
            while (moremembers)
            {
                if (parser.CheckToken(totaltokens, "("))
                {
                    ++totaltokens;

                    var membername = parser.PeekToken(totaltokens);

                    ++totaltokens;

                    if (!parser.CheckToken(totaltokens, ":"))
                        return null;

                    ++totaltokens;

                    bool moreparams = true;
                    while (moreparams)
                    {
                        ++totaltokens;
                        if (!parser.CheckToken(totaltokens, ","))
                            moreparams = false;
                        else
                            ++totaltokens;
                    }

                    if (parser.CheckToken(totaltokens, "->"))
                        totaltokens += 2;

                    if (!parser.CheckToken(totaltokens, ")"))
                        return null;

                    ++totaltokens;

                    // TODO - register function-typed structure members
                }
                else
                {
                    int typestarttoken = totaltokens;
                    var membertype = parser.PeekToken(totaltokens);

                    ++totaltokens;
                    int typeendtoken = totaltokens;

                    var membername = parser.PeekToken(totaltokens);

                    ++totaltokens;

                    if (membername.Text.Equals("<"))
                    {
                        int starttotal = totaltokens;
                        if (!parser.ParseTemplateArguments(totaltokens, membertype, out totaltokens))
                            return null;

                        if (totaltokens <= starttotal)
                            return null;

                        typeendtoken = totaltokens;

                        membername = parser.PeekToken(totaltokens);
                        ++totaltokens;
                    }

                    if (membername.Text.Equals("ref"))
                    {
                        typeendtoken = totaltokens;
                        membername = parser.PeekToken(totaltokens);
                        ++totaltokens;
                    }

                    parsed.Object.Members.Add(new Member { Name = membername, Type = TypeSignatureInstantiated.Construct(parser, typestarttoken, typeendtoken) });
                }

                if (!parser.CheckToken(totaltokens, ","))
                    moremembers = false;
                else
                    ++totaltokens;
            }

            parser.ConsumeTokens(totaltokens);
            return parsed;
        }
    }
}
