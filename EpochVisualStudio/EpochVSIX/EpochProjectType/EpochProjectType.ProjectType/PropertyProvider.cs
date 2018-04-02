using Microsoft.VisualStudio.ProjectSystem;
using Microsoft.VisualStudio.ProjectSystem.Build;
using System.Collections.Immutable;
using System.ComponentModel.Composition;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;

namespace EpochVSIX
{
    [Export(typeof(IProjectGlobalPropertiesProvider))]
    [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
    public class PropertyProvider : StaticGlobalPropertiesProviderBase
    {
        [ImportingConstructor]
        public PropertyProvider(IProjectCommonServices services)
            : base(services)
        {
        }

        public override Task<IImmutableDictionary<string, string>> GetGlobalPropertiesAsync(CancellationToken cancellationToken)
        {
            string dllpath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

            IImmutableDictionary<string, string> properties = Empty.PropertiesMap
                .SetItem("EpochVSExtensionPath", dllpath);

            return Task.FromResult<IImmutableDictionary<string, string>>(properties);
        }
    }
}
