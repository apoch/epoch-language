//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Module for customizing the "QuickInfo" IntelliSense tooltips in Visual Studio.
//
// When the mouse hovers over a code token, QuickInfo is responsible for popping up
// a tooltip that contains contextually relevant information about the token. As an
// example, hovering over a structure's type name will show the structure's members
// and their types.
//
// The default QuickInfo tips provided by VS integration are plain text only. While
// this is adequate, it is uninteresting. C# IntelliSense provides a nice glyph and
// colored text (matching syntax highlighting themes) and this looks nice and shiny
// so we want to replicate it.
//
// This module wires up the XAML UserControl (see EpochQuickInfoControl.xaml) to an
// actual IntelliSense "session," and acts as the bridge between our custom control
// and the editor UI itself.
//

using Microsoft.VisualStudio.Language.Intellisense;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Adornments;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Windows;
using System.Windows.Media;

namespace EpochVSIX
{
    //
    // This class succinctly captures the data necessary to render a QuickInfo
    // tooltip with our customizations. It contains classification spans and a
    // pair of optional "glyphs."
    //
    // The classification spans come directly from the syntax highlighter code
    // module, EpochClassifier. Each is a sequence of characters with a shared
    // coloration scheme. There is loosely speaking one span per token.
    //
    // The "primary" glyph is shown in the top left corner of the tooltip as a
    // visual indicator of what sort of content is being displayed. If left as
    // null there will be no glyph shown. A "sub" glyph is shown for sub-items
    // of the main content, such as overloaded functions or structure members.
    // If the sub glyph is *different* than the primary glyph, each line after
    // the first line of content will be indented slightly. This configuration
    // allows tooltips to show structure definitions in a nicer fashion, while
    // also handling overloads with a minimum of fuss.
    //
    // That said, it is a little magical, so it should probably be generalized
    // if the control is ever used elsewhere.
    //
    class EpochQuickInfoContent
    {
        public List<List<ClassificationSpan>> Lines;
        public ImageSource Glyph;
        public ImageSource SubGlyph;
    }


    //
    // The presenter class takes care of actually linking an IntelliSense request
    // with a piece of UI, specifically through the SurfaceElement property which
    // returns a XAML element. Visual Studio will take care of the positioning of
    // the element itself, but all aspects of rendering are defined by the XAML.
    //
    // This particular presenter is linked to a QuickInfo session but that is not
    // strictly required.
    //
    class EpochIntellisensePresenter : IPopupIntellisensePresenter
    {
        IQuickInfoSession QISession;
        Controls.EpochQuickInfoControl Control;         // Actual XAML control used to display the tooltip

        //
        // Construct and initialize a presenter object.
        //
        public EpochIntellisensePresenter(IQuickInfoSession qisession, IClassificationTypeRegistryService registry, IClassificationFormatMapService formatmap)
        {
            QISession = qisession;
            Control = new Controls.EpochQuickInfoControl();

            var content = qisession.QuickInfoContent[0] as EpochQuickInfoContent;
            Control.Attach(content, formatmap.GetClassificationFormatMap(qisession.TextView));

            PopupStyles = PopupStyles.DismissOnMouseLeaveText | PopupStyles.PositionClosest;

            // According to various implementations found on the web, this is a
            // necessity for ensuring our tip is placed and displayed correctly
            // by Visual Studio.
            SpaceReservationManagerName = "quickinfo";
        }

        //
        // Pipe opacity settings through to the XAML control.
        //
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
                return QISession.ApplicableToSpan;
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

        //
        // We don't use these events directly but VS expects us to provide them.
        //
#pragma warning disable 67
        public event EventHandler<ValueChangedEventArgs<PopupStyles>> PopupStylesChanged;
        public event EventHandler PresentationSpanChanged;
        public event EventHandler SurfaceElementChanged;
#pragma warning restore 67
    }

    //
    // Factory for creating IntelliSense presenters specialized for Epoch code.
    //
    // MEF detects this class and uses it on-demand to create presenter objects.
    // Since this is an MEF component we can import certain services that will
    // come in handy for rendering the tooltips we wish to customize.
    //
    [Export(typeof(IIntellisensePresenterProvider))]
    [Name("Epoch IntelliSense support")]
    [Order(Before = "Default Quick Info Presenter")]
    [ContentType("EpochFile")]
    class EpochIntellisensePresenterProvider : IIntellisensePresenterProvider
    {
        [Import]
        public IClassificationTypeRegistryService ClassificationTypeRegistry { get; set; }

        [Import]
        public IClassificationFormatMapService FormatMapService { get; set; }


        public IIntellisensePresenter TryCreateIntellisensePresenter(IIntellisenseSession session)
        {
            var qisession = session as IQuickInfoSession;
            if (qisession != null)
            {
                return new EpochIntellisensePresenter(qisession, ClassificationTypeRegistry, FormatMapService);
            }

            return null;
        }
    }
}
