using Microsoft.VisualStudio.Package;
using Microsoft.VisualStudio.Shell;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX
{
    [Guid(VsPackage.EditorGuid)]
    public class EpochEditorFactory : EditorFactory
    {
        public EpochEditorFactory(Package package) : base(package)
        {
        }

        public override Guid GetLanguageServiceGuid()
        {
            return typeof(EpochLanguageService).GUID;
        }
    }
}
