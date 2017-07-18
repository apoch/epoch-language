using System;
using System.ComponentModel.Composition;
using System.Drawing;
using System.Reflection;
using Microsoft.VisualStudio.Imaging;
using Microsoft.VisualStudio.ProjectSystem;
using Microsoft.VisualStudio.ProjectSystem.Utilities;
using Microsoft.VisualStudio.ProjectSystem.Designers;
using Microsoft.VisualStudio.ProjectSystem.Utilities.Designers;

namespace EpochVSIX
{
    [Export(typeof(IProjectTreeModifier))]
    [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
    internal class EpochProjectTreeModifier : IProjectTreeModifier
    {
        public IProjectTree ApplyModifications(IProjectTree tree, IProjectTreeProvider projectTreeProvider)
        {
            if (tree.Capabilities.Contains(ProjectTreeCapabilities.ProjectRoot))
            {
                tree = tree.SetIcon(EpochImagesMonikers.ProjectIconImageMoniker.ToProjectSystemType());
            }
            else if (tree.Capabilities.Contains(ProjectTreeCapabilities.FileOnDisk) && tree.FilePath.EndsWith(".epoch", StringComparison.CurrentCultureIgnoreCase))
            {
                tree = tree.SetIcon(EpochImagesMonikers.DocumentIconImageMoniker.ToProjectSystemType());
            }

            return tree;
        }
    }
}