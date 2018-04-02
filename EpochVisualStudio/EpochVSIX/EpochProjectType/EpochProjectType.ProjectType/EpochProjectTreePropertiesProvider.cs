using System;
using System.ComponentModel.Composition;
using System.Drawing;
using System.Reflection;
using Microsoft.VisualStudio.Imaging;
using Microsoft.VisualStudio.ProjectSystem;

namespace EpochVSIX
{
    /// <summary>
    /// Updates nodes in the project tree by overriding property values calcuated so far by lower priority providers.
    /// </summary>
    [Export(typeof(IProjectTreePropertiesProvider))]
    [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
    // TODO: Consider removing the Order attribute as it typically should not be needed when creating a new project type. It may be needed when customizing an existing project type to override the default behavior (e.g. the default C# implementation).
    [Order(1000)]
    internal class EpochProjectTreePropertiesProvider : IProjectTreePropertiesProvider
    {
        /// <summary>
        /// Calculates new property values for each node in the project tree.
        /// </summary>
        /// <param name="propertyContext">Context information that can be used for the calculation.</param>
        /// <param name="propertyValues">Values calculated so far for the current node by lower priority tree properties providers.</param>
        public void CalculatePropertyValues(
            IProjectTreeCustomizablePropertyContext propertyContext,
            IProjectTreeCustomizablePropertyValues propertyValues)
        {
            if (propertyValues.Flags.Contains(ProjectTreeFlags.Common.ProjectRoot))
                propertyValues.Icon = EpochImageManifestMonikers.ProjectIconImageMoniker.ToProjectSystemType();
            else if (propertyValues.Flags.Contains(ProjectTreeFlags.Common.FileOnDisk) && propertyContext.ItemName.EndsWith(".epoch", StringComparison.CurrentCultureIgnoreCase))
                propertyValues.Icon = EpochImageManifestMonikers.DocumentIconImageMoniker.ToProjectSystemType();
        }
    }
}