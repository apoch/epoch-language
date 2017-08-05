using Microsoft.VisualStudio.PlatformUI;
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

        internal void Attach(string content)
        {
            var color = VSColorTheme.GetThemedColor(EnvironmentColors.ToolTipBorderColorKey);
            var mediacolor = Color.FromArgb(color.A, color.R, color.G, color.B);
            ColoredBorder.BorderBrush = new SolidColorBrush(mediacolor);

            var forecolor = VSColorTheme.GetThemedColor(EnvironmentColors.ToolTipTextColorKey);
            var mediaforecolor = Color.FromArgb(forecolor.A, forecolor.R, forecolor.G, forecolor.B);
            var fcbrush = new SolidColorBrush(mediaforecolor);

            var lines = content.Split(new string[] { "\r\n" }, StringSplitOptions.None);
            foreach (var line in lines)
            {
                var horizontalStack = new StackPanel();
                horizontalStack.Orientation = Orientation.Horizontal;
                horizontalStack.Width = double.NaN;
                horizontalStack.Height = double.NaN;

                var words = line.Split(' ');
                foreach (var word in words)
                {
                    var label = new Label();
                    label.Width = double.NaN;
                    label.Height = double.NaN;
                    label.Content = word;
                    label.Foreground = fcbrush;

                    horizontalStack.Children.Add(label);
                }

                VerticalStack.Children.Add(horizontalStack);
            }

            var bgcolor = VSColorTheme.GetThemedColor(EnvironmentColors.ToolTipColorKey);
            var mediabgcolor = Color.FromArgb(bgcolor.A, bgcolor.R, bgcolor.G, bgcolor.B);
            VerticalStack.Background = new SolidColorBrush(mediabgcolor);
        }
    }
}
