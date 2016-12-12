using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System.Diagnostics;
using Microsoft.Win32;

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
            string compilerPath = "";

            var regvalue = Registry.GetValue("HKEY_LOCAL_MACHINE\\Software\\Epoch\\CurrentInstall", "InstallPath", "");
            if (regvalue == null)
            {
                LogMissingCompiler();
                return false;
            }

            compilerPath = regvalue as string;
            if (compilerPath.Length <= 0)
            {
                LogMissingCompiler();
                return false;
            }

            string compilerFileName = compilerPath + "\\EpochNativeBin.exe";
            try
            {
                var process = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = compilerFileName,
                        Arguments = "/files " + m_fileName + " /output " + m_outputName,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        CreateNoWindow = true
                    }
                };

                process.Start();
                Log.LogMessagesFromStream(process.StandardOutput, MessageImportance.High);

                process.WaitForExit();

                return (process.ExitCode == 0);
            }
            catch
            {
                Log.LogError("Error invoking compiler at \"{0}\". Please ensure Epoch compiler is properly installed.", compilerFileName);
                return false;
            }
        }

        private void LogMissingCompiler()
        {
            Log.LogError("Compiler not found. Please ensure Epoch compiler is properly installed.");
        }
    }


    public class BuildTask32 : Microsoft.Build.Utilities.Task
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
            string compilerPath = "";

            var regvalue = Registry.GetValue("HKEY_LOCAL_MACHINE\\Software\\Epoch\\CurrentInstall", "InstallPath", "");
            if (regvalue == null)
            {
                LogMissingCompiler();
                return false;
            }

            compilerPath = regvalue as string;
            if (compilerPath.Length <= 0)
            {
                LogMissingCompiler();
                return false;
            }

            string compilerFileName = compilerPath + "\\Epoch32\\EpochCompiler.exe";
            try
            {
                var process = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = compilerFileName,
                        Arguments = "/files " + m_fileName + " /output " + m_outputName,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        CreateNoWindow = true
                    }
                };

                process.Start();
                Log.LogMessagesFromStream(process.StandardOutput, MessageImportance.High);

                process.WaitForExit();

                return (process.ExitCode == 0);
            }
            catch
            {
                Log.LogError("Error invoking compiler at \"{0}\". Please ensure Epoch compiler is properly installed.", compilerFileName);
                return false;
            }
        }

        private void LogMissingCompiler()
        {
            Log.LogError("Compiler not found. Please ensure Epoch compiler is properly installed.");
        }
    }
}
