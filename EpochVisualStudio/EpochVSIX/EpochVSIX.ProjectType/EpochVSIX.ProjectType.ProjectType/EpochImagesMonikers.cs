using System;
using Microsoft.VisualStudio.Imaging.Interop;

namespace EpochVSIX
{
    public static class EpochImagesMonikers
    {
        private static readonly Guid ManifestGuid = new Guid("0d1a8232-322a-48e2-98db-20bdda93fce0");

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
