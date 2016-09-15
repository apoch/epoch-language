using System;
using System.ComponentModel.Composition;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.ProjectSystem.Build;
using Microsoft.VisualStudio.ProjectSystem.Utilities;
using Microsoft.VisualStudio.Threading;

namespace EpochVS
{
    [Export(typeof(IBuildUpToDateCheckProvider))]
    [AppliesTo(MyUnconfiguredProject.UniqueCapability)]
    internal class BuildUpToDateCheckProvider : IBuildUpToDateCheckProvider
    {
        public async Task<bool> IsUpToDateAsync(BuildAction buildAction, TextWriter logger, CancellationToken cancellationToken = default(CancellationToken))
        {
            // Since this method is likely to be I/O intensive, get off the UI thread explicitly first.
            await TaskScheduler.Default;

            // TODO: Return true when the project is up-to-date to skip invoking the build.

            // It's better to err on the side of caution by building without needing to than not building when we should have.
            return false;
        }

        public Task<bool> IsUpToDateCheckEnabledAsync(CancellationToken cancellationToken = default(CancellationToken))
        {
            // If you implement this method and you want to be async, add the async keyword to the method signature
            // and change the return statement to "return true;" instead of calling Task.FromResult.
            return Task.FromResult(true);
        }
    }
}
