using Microsoft.VisualStudio.PlatformUI;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Text.Editor;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace EpochVSIX.Controls
{
    /// <summary>
    /// Interaction logic for EpochQuickInfoControl.xaml
    /// </summary>
    public partial class EpochQuickInfoControl : UserControl
    {
        public EpochQuickInfoControl()
        {
            InitializeComponent();
        }

        private Brush CreateThemedBrush(ThemeResourceKey key)
        {
            var color = VSColorTheme.GetThemedColor(key);
            var mediacolor = Color.FromArgb(color.A, color.R, color.G, color.B);
            return new SolidColorBrush(mediacolor);
        }

        internal void Attach(EpochQuickInfoContent content, IClassificationTypeRegistryService registry, IClassificationFormatMap formatMap)
        {
            ColoredBorder.BorderBrush = CreateThemedBrush(EnvironmentColors.ToolTipBorderColorKey);
            var fcbrush = CreateThemedBrush(EnvironmentColors.ToolTipTextColorKey);

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

            MainGrid.Background = CreateThemedBrush(EnvironmentColors.ToolTipColorKey);
        }
    }
}
