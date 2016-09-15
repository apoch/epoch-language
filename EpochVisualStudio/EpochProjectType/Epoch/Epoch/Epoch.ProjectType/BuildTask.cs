using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace EpochVS
{
    public class BuildTask : Microsoft.Build.Utilities.Task
    {
        private string m_fileName;

        [Required]
        public string Filename
        {
            get { return m_fileName; }
            set { m_fileName = value; }
        }

        public override bool Execute()
        {
            Log.LogMessage(MessageImportance.High, "Compiling file {0}...", m_fileName);

            return true;
        }
    }
}
