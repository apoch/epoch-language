using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System.Diagnostics;

namespace EpochVS
{
    public class BuildTask : Microsoft.Build.Utilities.Task
    {
        private string m_fileName;
        private string m_outputName;

        [Required]
        public string Filename
        {
            get { return m_fileName; }
            set { m_fileName = value; }
        }

        [Required]
        public string Output
        {
            get { return m_outputName; }
            set { m_outputName = value; }
        }

        public override bool Execute()
        {
            var process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = "d:\\epoch\\epoch-language\\bin\\debug\\EpochNativeBin.exe",      // TODO - remove hardcoded paths
                    Arguments = "/files " + m_fileName + " /output " + m_outputName,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true
                }
            };

            process.Start();
            Log.LogMessagesFromStream(process.StandardOutput, MessageImportance.High);

            return true;
        }
    }
}
