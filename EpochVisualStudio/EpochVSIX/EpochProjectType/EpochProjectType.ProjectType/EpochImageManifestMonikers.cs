using System;
using Microsoft.VisualStudio.Imaging.Interop;

namespace EpochVSIX
{
    public static class EpochImageManifestMonikers
    {
        private static readonly Guid ManifestGuid = new Guid("e6b80c76-4e85-480e-9c1a-9418c5fc17d2");

        private const int ProjectIcon = 0;
        private const int DocumentIcon = 1;

        public static ImageMoniker ProjectIconImageMoniker
        {
            get
            {
                return new ImageMoniker { Guid = ManifestGuid, Id = ProjectIcon };
            }
        }

        public static ImageMoniker DocumentIconImageMoniker
        {
            get
            {
                return new ImageMoniker { Guid = ManifestGuid, Id = DocumentIcon };
            }
        }
    }
}
