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

                foreach (var word in line)
                {
                    var classification = word.ClassificationType;

                    var label = new Label();
                    label.Width = double.NaN;
                    label.Height = double.NaN;
                    label.Content = word.Span.GetText();
                    label.Foreground = formatMap.GetTextProperties(classification).ForegroundBrush;

                    horizontalStack.Children.Add(label);
                }

                VerticalStack.Children.Add(horizontalStack);
            }

            VerticalStack.Background = CreateThemedBrush(EnvironmentColors.ToolTipColorKey);
        }
    }
}
