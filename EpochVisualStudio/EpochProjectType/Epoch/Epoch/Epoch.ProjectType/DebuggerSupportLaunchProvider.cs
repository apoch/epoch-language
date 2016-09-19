using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Threading.Tasks;
using Microsoft.VisualStudio.ProjectSystem;
using Microsoft.VisualStudio.ProjectSystem.Debuggers;
using Microsoft.VisualStudio.ProjectSystem.Utilities;
using Microsoft.VisualStudio.ProjectSystem.Utilities.DebuggerProviders;
using Microsoft.VisualStudio.ProjectSystem.VS.Debuggers;

namespace EpochVS
{
    [ExportDebugger(DebuggerSupport.SchemaName)]
    [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
    public class DebuggerSupportLaunchProvider : DebugLaunchProviderBase
    {
        [ExportPropertyXamlRuleDefinition("EpochVS, Version=1.0.0.0, Culture=neutral, PublicKeyToken=9be6e469bc4921f1", "XamlRuleToCode:DebuggerSupport.xaml", "Project")]
        [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
        private object DebuggerXaml { get { throw new NotImplementedException(); } }

        [ImportingConstructor]
        public DebuggerSupportLaunchProvider(ConfiguredProject configuredProject)
            : base(configuredProject)
        {
        }

        /// <summary>
        /// Gets project properties that the debugger needs to launch.
        /// </summary>
        [Import]
        private ProjectProperties DebuggerProperties { get; set; }

        public override async Task<bool> CanLaunchAsync(DebugLaunchOptions launchOptions)
        {
            var properties = await this.DebuggerProperties.GetDebuggerSupportPropertiesAsync();
            string commandValue = await properties.DebuggerSupportCommand.GetEvaluatedValueAtEndAsync();
            return !string.IsNullOrEmpty(commandValue);
        }

        public override async Task<IReadOnlyList<IDebugLaunchSettings>> QueryDebugTargetsAsync(DebugLaunchOptions launchOptions)
        {
            var settings = new DebugLaunchSettings(launchOptions);

            // The properties that are available via DebuggerProperties are determined by the property XAML files in your project.
            var debuggerProperties = await this.DebuggerProperties.GetDebuggerSupportPropertiesAsync();
            settings.CurrentDirectory = await debuggerProperties.DebuggerSupportWorkingDirectory.GetEvaluatedValueAtEndAsync();
            settings.Executable = await debuggerProperties.DebuggerSupportCommand.GetEvaluatedValueAtEndAsync();
            settings.Arguments = await debuggerProperties.DebuggerSupportCommandArguments.GetEvaluatedValueAtEndAsync();
            settings.LaunchOperation = DebugLaunchOperation.CreateProcess;

            settings.LaunchDebugEngineGuid = DebuggerEngines.NativeOnlyEngine;

            return new IDebugLaunchSettings[] { settings };
        }
    }
}
