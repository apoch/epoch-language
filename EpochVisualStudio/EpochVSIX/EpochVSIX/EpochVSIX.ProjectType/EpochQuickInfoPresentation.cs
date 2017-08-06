using Microsoft.VisualStudio.Language.Intellisense;
using Microsoft.VisualStudio.Utilities;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Adornments;
using System.Windows;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Text.Classification;

namespace EpochVSIX
{
    class EpochIntellisensePresenter : IPopupIntellisensePresenter
    {
        IQuickInfoSession QISession;
        ITrackingSpan TrackingSpan;
        Controls.EpochQuickInfoControl Control;

        public EpochIntellisensePresenter(IQuickInfoSession qisession, ITextBufferFactoryService bufferfactory, ITextEditorFactoryService editorfactory, IContentTypeRegistryService ctregistry, IClassificationTypeRegistryService registry, IClassificationFormatMapService formatmap)
        {
            QISession = qisession;
            Control = new Controls.EpochQuickInfoControl();

            var content = qisession.QuickInfoContent[0] as string;

            //var epochContentType = ctregistry.GetContentType("EpochFile");
            //var declbuffer = bufferfactory.CreateTextBuffer(content, epochContentType);

            //declbuffer.Properties.AddProperty(typeof(Parser.Project), qisession.TextView.TextBuffer.Properties.GetProperty(typeof(Parser.Project)) as Parser.Project);

            //var editor = editorfactory.CreateTextView(declbuffer);

            Control.Attach(content, registry, formatmap.GetClassificationFormatMap(qisession.TextView));

            PopupStyles = PopupStyles.DismissOnMouseLeaveText | PopupStyles.PositionClosest;
            SpaceReservationManagerName = "quickinfo";
        }

        public double Opacity
        {
            get { return Control.Opacity; }
            set { Control.Opacity = value; }
        }

        public PopupStyles PopupStyles
        {
            get;
            private set;
        }

        public ITrackingSpan PresentationSpan
        {
            get
            {
                if (TrackingSpan == null)
                    TrackingSpan = QISession.ApplicableToSpan;

                return TrackingSpan;
            }
        }

        public IIntellisenseSession Session
        {
            get
            {
                return QISession;
            }
        }

        public string SpaceReservationManagerName
        {
            get;
            private set;
        }

        public UIElement SurfaceElement
        {
            get
            {
                return Control;
            }
        }

#pragma warning disable 67
        public event EventHandler<ValueChangedEventArgs<PopupStyles>> PopupStylesChanged;
        public event EventHandler PresentationSpanChanged;
        public event EventHandler SurfaceElementChanged;
#pragma warning restore 67
    }

    [Export(typeof(IIntellisensePresenterProvider))]
    [Name("Epoch IntelliSense support")]
    [Order(Before = "Default Quick Info Presenter")]
    [ContentType("EpochFile")]
    class EpochIntellisensePresenterProvider : IIntellisensePresenterProvider
    {
        [Import]
        public ITextEditorFactoryService EditorFactory { get; set; }

        [Import]
        public ITextBufferFactoryService BufferFactory { get; set; }

        [Import]
        public IContentTypeRegistryService ContentTypeRegistry { get; set; }

        [Import]
        public IClassificationTypeRegistryService ClassificationTypeRegistry { get; set; }

        [Import]
        public IClassificationFormatMapService FormatMapService { get; set; }


        public IIntellisensePresenter TryCreateIntellisensePresenter(IIntellisenseSession session)
        {
            var qisession = session as IQuickInfoSession;
            if (qisession != null)
            {
                return new EpochIntellisensePresenter(qisession, BufferFactory, EditorFactory, ContentTypeRegistry, ClassificationTypeRegistry, FormatMapService);
            }

            return null;
        }
    }
}
