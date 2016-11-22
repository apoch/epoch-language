using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVS
{
    using System.ComponentModel.Composition;
    using Microsoft.VisualStudio.ProjectSystem;
    using Microsoft.VisualStudio.ProjectSystem.Build;
    using Microsoft.VisualStudio.ProjectSystem.Designers;
    using Microsoft.VisualStudio.ProjectSystem.Utilities;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.VisualStudio.Shell.Interop;
    using System.Collections.Immutable;
    using System.Threading;
    using Task = System.Threading.Tasks.Task;
    using System.IO;
    using System.Reflection;

    [Export(typeof(IProjectGlobalPropertiesProvider))]
    [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
    public class GlobalPropertiesProvider : StaticGlobalPropertiesProviderBase
    {
        [ImportingConstructor]
        public GlobalPropertiesProvider(IThreadHandling handling)
            : base(handling.JoinableTaskContext)
        {
            System.Windows.Forms.MessageBox.Show("GlobalPropertiesProvider constructed");
        }

        public override Task<IImmutableDictionary<string, string>> GetGlobalPropertiesAsync(CancellationToken cancellationToken)
        {
            string dllpath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

            System.Windows.Forms.MessageBox.Show(dllpath);

            IImmutableDictionary<string, string> properties = Empty.PropertiesMap
                .SetItem("EpochVSExtensionPath", dllpath);

            return Task.FromResult<IImmutableDictionary<string, string>>(properties);
        }
    }
}
