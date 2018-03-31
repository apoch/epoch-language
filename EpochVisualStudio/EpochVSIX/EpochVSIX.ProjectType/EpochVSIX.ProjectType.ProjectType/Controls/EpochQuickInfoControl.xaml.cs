//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// XAML implementation/logic for custom QuickInfo tooltips.
//
// This module is primarily responsible for dynamically generating
// a tooltip based on syntax highlighting and some optional glyphs
// provided by an EpochQuickInfoContent object.
//
// Everything interesting happens in the Attach() method.
//

using Microsoft.VisualStudio.PlatformUI;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Text.Classification;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace EpochVSIX.Controls
{
    //
    // A XAML user control which represents a QuickInfo tooltip.
    //
    // The tooltip can mimic syntax highlighting of actual code windows,
    // using the provided classification format map. It can also display
    // glyphs in various configurations, to provide supplemental visuals
    // for the text content.
    //
    // The actual text and glyphs are stored in an EpochQuickInfoContent
    // object which is given to the Attach() method. See the comments on
    // that class for more details on its semantics.
    //
    public partial class EpochQuickInfoControl : UserControl
    {
        //
        // Construct and initialize a tooltip control.
        //
        public EpochQuickInfoControl()
        {
            InitializeComponent();
        }

        //
        // Internal helper for translating Visual Studio themed colors
        // over to WPF brushes for use in the displayed text.
        //
        private Brush CreateThemedBrush(ThemeResourceKey key)
        {
            var color = VSColorTheme.GetThemedColor(key);
            var mediacolor = Color.FromArgb(color.A, color.R, color.G, color.B);
            return new SolidColorBrush(mediacolor);
        }

        //
        // Attach some display content to a control instance.
        //
        // This is the real work-horse of the control. It generates children
        // for each line of content, including optional glyphs. Each span of
        // text is assumed to be a different classification, so they all get
        // a label control in a horizontally-oriented stack container.
        //
        // Once assembled, lines are stacked vertically in another container
        // control. Glyphs are added according to the semantics specified in
        // the EpochQuickInfoContent class documentation.
        //
        internal void Attach(EpochQuickInfoContent content, IClassificationFormatMap formatMap)
        {
            //
            // Set the border and background colors of the tooltip according to
            // the appropriate settings from the current Visual Studio theme.
            //
            ColoredBorder.BorderBrush = CreateThemedBrush(EnvironmentColors.ToolTipBorderColorKey);
            MainGrid.Background = CreateThemedBrush(EnvironmentColors.ToolTipColorKey);


            //
            // Set up child controls for each line of content, including glyphs.
            //
            foreach (var line in content.Lines)
            {
                var horizontalStack = new StackPanel();
                horizontalStack.Orientation = Orientation.Horizontal;
                horizontalStack.Width = double.NaN;
                horizontalStack.Height = double.NaN;

                if (content.Glyph != null)
                {
                    var source = content.Glyph;
                    double leftMargin = 0.0;
                    if (VerticalStack.Children.Count > 0)
                    {
                        source = content.SubGlyph;
                        if (content.Glyph != content.SubGlyph)
                            leftMargin = 10.0;
                    }

                    var glyph = new Image();
                    glyph.Source = source;
                    glyph.Margin = new Thickness(leftMargin, 0.0, 3.0, 0.0);
                    horizontalStack.Children.Add(glyph);
                }

                //
                // Given a classification span ("word"), create an appropriately
                // colored label control to display that word, based on whatever
                // classification was provided for the span.
                //
                // This gets a bit funky because of the magic margins, so a word
                // of explanation is in order. When the default margins are used
                // in the horizontal stack, words appear vastly spaced out, with
                // some tokens (such as punctuation) adding to the stretchiness,
                // since they each get a label control to themselves.
                //
                // This effect is undesirable, and the simplest way to eliminate
                // it (that I know of as of right now) is to use negative margin
                // settings. The bottom line is that the margins used here serve
                // to visually "pull" the tokens together horizontally.
                //
                // The result is a much more aesthetically pleasing line of text
                // that doesn't have a bunch of egregious horizontal whitespace.
                // Alternative solutions (including a RichTextBox) were rejected
                // due to being overly heavy-weight and cumbersome; but if there
                // is a genuinely better way to get the visual effect, I'm eager
                // to learn of it.
                //
                foreach (var word in line)
                {
                    var trimmed = word.Span.GetText().Trim();
                    if (string.IsNullOrEmpty(trimmed))
                        continue;

                    double leftMargin = -3.0;
                    double rightMargin = -3.0;

                    if (trimmed[0] == ',' || trimmed[0] == ')' || trimmed[0] == ']' || trimmed[0] == '>')
                        leftMargin = -6.0;
                    else if (trimmed[0] == '[')
                        rightMargin = -6.0;
                    else if (trimmed[0] == '(' || trimmed[0] == '<')
                        leftMargin = rightMargin = -6.0;

                    var label = new Label();
                    label.Width = double.NaN;
                    label.Height = double.NaN;
                    label.Content = trimmed;
                    label.Foreground = formatMap.GetTextProperties(word.ClassificationType).ForegroundBrush;
                    label.Margin = new Thickness(leftMargin, label.Margin.Top, rightMargin, label.Margin.Bottom);

                    horizontalStack.Children.Add(label);
                }

                VerticalStack.Children.Add(horizontalStack);
            }
        }
    }
}
