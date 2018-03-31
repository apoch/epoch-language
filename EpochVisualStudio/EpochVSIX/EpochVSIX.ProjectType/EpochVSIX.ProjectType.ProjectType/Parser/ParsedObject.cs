//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Basic generic wrapper for a named object.
//
// This wrapper is used to associate a name with a particular
// parsed piece of code, such as a Structure or type definition.
//

namespace EpochVSIX.Parser
{
    class ParsedObject<T>
    {
        public Token Name;
        public T Object;
    }
}
