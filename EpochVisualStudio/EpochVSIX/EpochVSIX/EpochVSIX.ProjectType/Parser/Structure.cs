using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class Structure
    {
        public TypeSignature Signature;
        public List<Member> Members;


        public class Member
        {
            public string Name;
            public TypeSignature Type;
        }
    }
}
