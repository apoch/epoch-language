//------------------------------------------------------------------------------
// <copyright file="EpochClassifierProvider.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.Editor;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio;

namespace EpochVSIX
{
    /// <summary>
    /// Classifier provider. It adds the classifier to the set of classifiers.
    /// </summary>
    [Export(typeof(IClassifierProvider))]
    [ContentType("EpochFile")]
    internal class EpochClassifierProvider : IClassifierProvider
    {
        // Disable "Field is never assigned to..." compiler's warning. Justification: the field is assigned by MEF.
#pragma warning disable 649

        /// <summary>
        /// Classification registry to be used for getting a reference
        /// to the custom classification type later.
        /// </summary>
        [Import]
        private IClassificationTypeRegistryService classificationRegistry;

        [Import]
        private IVsEditorAdaptersFactoryService AdapterFactory;

        [Import]
        private SVsServiceProvider ServiceProvider;

#pragma warning restore 649

        public static EpochClassifierProvider Instance { get; private set; }

        public EpochClassifierProvider()
        {
            if (Instance == null)
                Instance = this;
        }


        #region IClassifierProvider

        /// <summary>
        /// Gets a classifier for the given text buffer.
        /// </summary>
        /// <param name="buffer">The <see cref="ITextBuffer"/> to classify.</param>
        /// <returns>A classifier for the text buffer, or null if the provider cannot do so in its current state.</returns>
        public IClassifier GetClassifier(ITextBuffer buffer)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            Parser.Project project = null;
            var bufferAdapter = AdapterFactory.GetBufferAdapter(buffer);
            var textManager = ServiceProvider.GetService(typeof(SVsTextManager)) as IVsTextManager;

            if (bufferAdapter != null)
            {
                IVsEnumTextViews enumerator;
                textManager.EnumViews(bufferAdapter, out enumerator);

                uint count = 0;
                enumerator.GetCount(ref count);
                if (count > 0)
                {
                    IVsTextView[] viewarray = new IVsTextView[count];
                    uint fetchCount = 0;
                    if (enumerator.Next(count, viewarray, ref fetchCount) == VSConstants.S_OK)
                    {
                        foreach (var view in viewarray)
                        {
                            var viewAdapter = AdapterFactory.GetWpfTextView(view);
                            viewAdapter.Properties.TryGetProperty<Parser.Project>(typeof(Parser.Project), out project);
                            if (project != null)
                                break;
                        }
                    }
                }
            }
            else
            {
                // This is done for highlighted tooltips
                buffer.Properties.TryGetProperty(typeof(Parser.Project), out project);
            }

            return buffer.Properties.GetOrCreateSingletonProperty<EpochClassifier>(creator: () => new EpochClassifier(this.classificationRegistry, project));
        }

        #endregion
    }
}
