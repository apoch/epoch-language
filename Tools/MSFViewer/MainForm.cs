﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel.Design;
using System.Windows.Forms;

namespace MSFViewer
{
    public partial class MainForm : Form
    {
        private ByteViewer ByteEditorControl = null;
        private MSF EditingMSF = null;

        private int PreviousStreamSelectionIndex = -1;


        public MainForm()
        {
            InitializeComponent();

            ByteEditorControl = new ByteViewer();
            ByteEditorPanel.Controls.Add(ByteEditorControl);
            ByteEditorControl.Dock = DockStyle.Fill;
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (OFDialog.ShowDialog() == DialogResult.OK)
            {
                EditingMSF = new MSF(OFDialog.FileName);

                StreamListBox.Items.Clear();
                StreamListBox.Items.Add(EditingMSF.EntireFile);
                foreach (var stream in EditingMSF.EntireFile.Streams)
                {
                    StreamListBox.Items.Add(stream);
                }

                StreamListBox.SelectedIndex = 0;
            }
        }

        private void StreamListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (StreamListBox.SelectedIndex >= 0 && (StreamListBox.SelectedIndex != PreviousStreamSelectionIndex))
            {
                Application.UseWaitCursor = true;
                Application.DoEvents();

                var stream = (StreamListBox.SelectedItem as MSFStream);
                var bytes = stream.GetFlattenedBuffer();
                if (bytes != null)
                    ByteEditorControl.SetBytes(bytes);
                else
                    ByteEditorControl.SetBytes(new byte[0]);

                stream.PopulateAnalysis(AnalysisListView);

                Application.UseWaitCursor = false;
            }

            PreviousStreamSelectionIndex = StreamListBox.SelectedIndex;
        }
    }
}
