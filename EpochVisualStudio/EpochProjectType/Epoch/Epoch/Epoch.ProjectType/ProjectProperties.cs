namespace EpochVS
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.Composition;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.ProjectSystem;
    using Microsoft.VisualStudio.ProjectSystem.Properties;
    using Microsoft.VisualStudio.ProjectSystem.Utilities;

    [Export]
    internal partial class ProjectProperties : StronglyTypedPropertyAccess
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectProperties"/> class.
        /// </summary>
        [ImportingConstructor]
        public ProjectProperties(ConfiguredProject configuredProject)
            : base(configuredProject)
        {
            System.Windows.Forms.MessageBox.Show("A");
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectProperties"/> class.
        /// </summary>
        public ProjectProperties(ConfiguredProject configuredProject, string file, string itemType, string itemName)
            : base(configuredProject, file, itemType, itemName)
        {
            System.Windows.Forms.MessageBox.Show("B");
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectProperties"/> class.
        /// </summary>
        public ProjectProperties(ConfiguredProject configuredProject, IProjectPropertiesContext projectPropertiesContext)
            : base(configuredProject, projectPropertiesContext)
        {
            System.Windows.Forms.MessageBox.Show("C");
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectProperties"/> class.
        /// </summary>
        public ProjectProperties(ConfiguredProject configuredProject, UnconfiguredProject unconfiguredProject)
            : base(configuredProject, unconfiguredProject)
        {
            System.Windows.Forms.MessageBox.Show("D");
        }
    }
}
