namespace MSFViewer
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.MainFormMenuStrip = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.OFDialog = new System.Windows.Forms.OpenFileDialog();
            this.StreamListBox = new System.Windows.Forms.ListBox();
            this.TabStrip = new System.Windows.Forms.TabControl();
            this.TabAnalysis = new System.Windows.Forms.TabPage();
            this.AnalysisListView = new System.Windows.Forms.ListView();
            this.ColumnHeaderDescription = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.ColumnHeaderValue = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.TabBytes = new System.Windows.Forms.TabPage();
            this.ByteEditorPanel = new System.Windows.Forms.Panel();
            this.MainFormMenuStrip.SuspendLayout();
            this.TabStrip.SuspendLayout();
            this.TabAnalysis.SuspendLayout();
            this.TabBytes.SuspendLayout();
            this.SuspendLayout();
            // 
            // MainFormMenuStrip
            // 
            this.MainFormMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem});
            this.MainFormMenuStrip.Location = new System.Drawing.Point(0, 0);
            this.MainFormMenuStrip.Name = "MainFormMenuStrip";
            this.MainFormMenuStrip.Size = new System.Drawing.Size(1036, 24);
            this.MainFormMenuStrip.TabIndex = 1;
            this.MainFormMenuStrip.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.toolStripMenuItem1,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.Size = new System.Drawing.Size(112, 22);
            this.openToolStripMenuItem.Text = "&Open...";
            this.openToolStripMenuItem.Click += new System.EventHandler(this.openToolStripMenuItem_Click);
            // 
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size(109, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(112, 22);
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // OFDialog
            // 
            this.OFDialog.Filter = "PDB Files (*.pdb)|*.pdb|All Files (*.*)|*.*";
            // 
            // StreamListBox
            // 
            this.StreamListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.StreamListBox.FormattingEnabled = true;
            this.StreamListBox.Location = new System.Drawing.Point(12, 27);
            this.StreamListBox.Name = "StreamListBox";
            this.StreamListBox.Size = new System.Drawing.Size(183, 589);
            this.StreamListBox.TabIndex = 2;
            this.StreamListBox.SelectedIndexChanged += new System.EventHandler(this.StreamListBox_SelectedIndexChanged);
            // 
            // TabStrip
            // 
            this.TabStrip.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TabStrip.Controls.Add(this.TabAnalysis);
            this.TabStrip.Controls.Add(this.TabBytes);
            this.TabStrip.Location = new System.Drawing.Point(201, 27);
            this.TabStrip.Name = "TabStrip";
            this.TabStrip.SelectedIndex = 0;
            this.TabStrip.Size = new System.Drawing.Size(823, 596);
            this.TabStrip.TabIndex = 3;
            // 
            // TabAnalysis
            // 
            this.TabAnalysis.Controls.Add(this.AnalysisListView);
            this.TabAnalysis.Location = new System.Drawing.Point(4, 22);
            this.TabAnalysis.Name = "TabAnalysis";
            this.TabAnalysis.Padding = new System.Windows.Forms.Padding(3);
            this.TabAnalysis.Size = new System.Drawing.Size(815, 570);
            this.TabAnalysis.TabIndex = 0;
            this.TabAnalysis.Text = "Analysis";
            this.TabAnalysis.UseVisualStyleBackColor = true;
            // 
            // AnalysisListView
            // 
            this.AnalysisListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.AnalysisListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.ColumnHeaderDescription,
            this.ColumnHeaderValue});
            this.AnalysisListView.FullRowSelect = true;
            this.AnalysisListView.Location = new System.Drawing.Point(6, 6);
            this.AnalysisListView.Name = "AnalysisListView";
            this.AnalysisListView.Size = new System.Drawing.Size(803, 558);
            this.AnalysisListView.TabIndex = 0;
            this.AnalysisListView.UseCompatibleStateImageBehavior = false;
            this.AnalysisListView.View = System.Windows.Forms.View.Details;
            // 
            // ColumnHeaderDescription
            // 
            this.ColumnHeaderDescription.Text = "Description";
            this.ColumnHeaderDescription.Width = 200;
            // 
            // ColumnHeaderValue
            // 
            this.ColumnHeaderValue.Text = "Value";
            this.ColumnHeaderValue.Width = 250;
            // 
            // TabBytes
            // 
            this.TabBytes.Controls.Add(this.ByteEditorPanel);
            this.TabBytes.Location = new System.Drawing.Point(4, 22);
            this.TabBytes.Name = "TabBytes";
            this.TabBytes.Padding = new System.Windows.Forms.Padding(3);
            this.TabBytes.Size = new System.Drawing.Size(815, 570);
            this.TabBytes.TabIndex = 1;
            this.TabBytes.Text = "Hex Viewer";
            this.TabBytes.UseVisualStyleBackColor = true;
            // 
            // ByteEditorPanel
            // 
            this.ByteEditorPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.ByteEditorPanel.Location = new System.Drawing.Point(6, 6);
            this.ByteEditorPanel.Name = "ByteEditorPanel";
            this.ByteEditorPanel.Size = new System.Drawing.Size(806, 558);
            this.ByteEditorPanel.TabIndex = 1;
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1036, 635);
            this.Controls.Add(this.TabStrip);
            this.Controls.Add(this.StreamListBox);
            this.Controls.Add(this.MainFormMenuStrip);
            this.MainMenuStrip = this.MainFormMenuStrip;
            this.Name = "MainForm";
            this.Text = "MSF Viewer";
            this.MainFormMenuStrip.ResumeLayout(false);
            this.MainFormMenuStrip.PerformLayout();
            this.TabStrip.ResumeLayout(false);
            this.TabAnalysis.ResumeLayout(false);
            this.TabBytes.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.MenuStrip MainFormMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.OpenFileDialog OFDialog;
        private System.Windows.Forms.ListBox StreamListBox;
        private System.Windows.Forms.TabControl TabStrip;
        private System.Windows.Forms.TabPage TabAnalysis;
        private System.Windows.Forms.TabPage TabBytes;
        private System.Windows.Forms.Panel ByteEditorPanel;
        private System.Windows.Forms.ListView AnalysisListView;
        private System.Windows.Forms.ColumnHeader ColumnHeaderDescription;
        private System.Windows.Forms.ColumnHeader ColumnHeaderValue;
    }
}

