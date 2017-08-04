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


        internal bool IsLiteralFunctionParam()
        {
            if (Text == "0")
                return true;

            if (Text == "0.0")
                return true;

            if (Text.Contains('.'))
            {
                float ignored;
                if (float.TryParse(Text, out ignored))
                    return true;
            }
            else
            {
                int ignored;
                if (int.TryParse(Text, out ignored))
                    return true;
            }

            return false;
        }
    }
}
