using Microsoft.VisualStudio.ProjectSystem;
using System;
using System.ComponentModel.Composition;

namespace EpochVSIX
{
    [Export(typeof(IProjectTreePropertiesProvider))]
    [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
    internal class EpochProjectTreeModifier : IProjectTreePropertiesProvider
    {
        public void CalculatePropertyValues(IProjectTreeCustomizablePropertyContext propertyContext, IProjectTreeCustomizablePropertyValues propertyValues)
        {
            if (propertyValues.Flags.Contains(ProjectTreeFlags.Common.ProjectRoot))
                propertyValues.Icon = EpochImagesMonikers.ProjectIconImageMoniker.ToProjectSystemType();
            else if (propertyValues.Flags.Contains(ProjectTreeFlags.Common.FileOnDisk) && propertyContext.ItemName.EndsWith(".epoch", StringComparison.CurrentCultureIgnoreCase))
                propertyValues.Icon = EpochImagesMonikers.DocumentIconImageMoniker.ToProjectSystemType();
        }
    }
}