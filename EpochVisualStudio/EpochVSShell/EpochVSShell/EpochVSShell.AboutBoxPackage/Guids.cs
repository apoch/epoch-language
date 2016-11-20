// Guids.cs
// MUST match guids.h
using System;

namespace EpochVSShell.AboutBoxPackage
{
    static class GuidList
    {
        public const string guidAboutBoxPackagePkgString = "22133c87-8127-4e25-95ed-26812456ee81";
        public const string guidAboutBoxPackageCmdSetString = "0330f53e-dcc1-46e5-b1c7-64c1821d506e";

        public static readonly Guid guidAboutBoxPackageCmdSet = new Guid(guidAboutBoxPackageCmdSetString);
    };
}