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

        internal void Attach(IWpfTextView editor)
        {
            var color = VSColorTheme.GetThemedColor(EnvironmentColors.ToolTipBorderColorKey);
            var mediacolor = System.Windows.Media.Color.FromArgb(color.A, color.R, color.G, color.B);
            ColoredBorder.BorderBrush = new SolidColorBrush(mediacolor);

            MainGrid.Children.Add(editor.VisualElement);
        }
    }
}
