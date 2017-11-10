using System;

namespace rhubarb_for_spine {
	partial class MainForm {

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing) {
			if (disposing && (components != null)) {
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		protected override void OnLoad(EventArgs e) {
			base.OnLoad(e);

			// Some bindings cannot be added in InitializeComponent; see https://stackoverflow.com/a/47167781/52041.
			mouthSlotComboBox.DataBindings.Add(new System.Windows.Forms.Binding("DataSource", animationFileBindingSource, "Slots", true));
			mouthSlotComboBox.DataBindings.Add(new System.Windows.Forms.Binding("SelectedItem", animationFileBindingSource, "MouthSlot", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
			audioEventsDataGridView.AutoGenerateColumns = false;
			audioEventsDataGridView.DataBindings.Add(new System.Windows.Forms.Binding("DataSource", animationFileBindingSource, "AudioFileModels", true));
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent() {
			this.components = new System.ComponentModel.Container();
			System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
			System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
			this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
			this.filePathLabel = new System.Windows.Forms.Label();
			this.panel1 = new System.Windows.Forms.Panel();
			this.filePathTextBox = new System.Windows.Forms.TextBox();
			this.filePathBrowseButton = new System.Windows.Forms.Button();
			this.mouthSlotLabel = new System.Windows.Forms.Label();
			this.animationFileBindingSource = new System.Windows.Forms.BindingSource(this.components);
			this.mouthShapesLabel = new System.Windows.Forms.Label();
			this.audioEventsLabel = new System.Windows.Forms.Label();
			this.audioEventsDataGridView = new System.Windows.Forms.DataGridView();
			this.mouthShapesResultLabel = new System.Windows.Forms.Label();
			this.mouthNamingLabel = new System.Windows.Forms.Label();
			this.mouthNamingResultLabel = new System.Windows.Forms.Label();
			this.mainErrorProvider = new System.Windows.Forms.ErrorProvider(this.components);
			this.animationFileErrorProvider = new System.Windows.Forms.ErrorProvider(this.components);
			this.mainBindingSource = new System.Windows.Forms.BindingSource(this.components);
			this.mouthSlotComboBox = new rhubarb_for_spine.BindableComboBox();
			this.eventColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
			this.audioFileColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
			this.dialogColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
			this.statusColumn = new System.Windows.Forms.DataGridViewTextBoxColumn();
			this.actionColumn = new System.Windows.Forms.DataGridViewButtonColumn();
			this.tableLayoutPanel1.SuspendLayout();
			this.panel1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.animationFileBindingSource)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.audioEventsDataGridView)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.mainErrorProvider)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.animationFileErrorProvider)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.mainBindingSource)).BeginInit();
			this.SuspendLayout();
			// 
			// tableLayoutPanel1
			// 
			this.tableLayoutPanel1.AutoSize = true;
			this.tableLayoutPanel1.ColumnCount = 2;
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel1.Controls.Add(this.filePathLabel, 0, 0);
			this.tableLayoutPanel1.Controls.Add(this.panel1, 1, 0);
			this.tableLayoutPanel1.Controls.Add(this.mouthSlotLabel, 0, 1);
			this.tableLayoutPanel1.Controls.Add(this.mouthSlotComboBox, 1, 1);
			this.tableLayoutPanel1.Controls.Add(this.mouthShapesLabel, 0, 3);
			this.tableLayoutPanel1.Controls.Add(this.audioEventsLabel, 0, 4);
			this.tableLayoutPanel1.Controls.Add(this.audioEventsDataGridView, 1, 4);
			this.tableLayoutPanel1.Controls.Add(this.mouthShapesResultLabel, 1, 3);
			this.tableLayoutPanel1.Controls.Add(this.mouthNamingLabel, 0, 2);
			this.tableLayoutPanel1.Controls.Add(this.mouthNamingResultLabel, 1, 2);
			this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tableLayoutPanel1.Location = new System.Drawing.Point(5, 5);
			this.tableLayoutPanel1.Name = "tableLayoutPanel1";
			this.tableLayoutPanel1.Padding = new System.Windows.Forms.Padding(3);
			this.tableLayoutPanel1.RowCount = 5;
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.tableLayoutPanel1.Size = new System.Drawing.Size(903, 612);
			this.tableLayoutPanel1.TabIndex = 0;
			// 
			// filePathLabel
			// 
			this.filePathLabel.AutoSize = true;
			this.filePathLabel.Location = new System.Drawing.Point(6, 3);
			this.filePathLabel.Name = "filePathLabel";
			this.filePathLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
			this.filePathLabel.Size = new System.Drawing.Size(81, 19);
			this.filePathLabel.TabIndex = 0;
			this.filePathLabel.Text = "Spine JSON file";
			// 
			// panel1
			// 
			this.panel1.AutoSize = true;
			this.panel1.Controls.Add(this.filePathTextBox);
			this.panel1.Controls.Add(this.filePathBrowseButton);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
			this.panel1.Location = new System.Drawing.Point(93, 6);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(804, 23);
			this.panel1.TabIndex = 1;
			// 
			// filePathTextBox
			// 
			this.filePathTextBox.DataBindings.Add(new System.Windows.Forms.Binding("Text", this.mainBindingSource, "FilePath", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
			this.mainErrorProvider.SetIconPadding(this.filePathTextBox, -20);
			this.filePathTextBox.Location = new System.Drawing.Point(0, 0);
			this.filePathTextBox.Name = "filePathTextBox";
			this.filePathTextBox.Size = new System.Drawing.Size(769, 20);
			this.filePathTextBox.TabIndex = 2;
			// 
			// filePathBrowseButton
			// 
			this.filePathBrowseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.filePathBrowseButton.Location = new System.Drawing.Point(775, 0);
			this.filePathBrowseButton.Name = "filePathBrowseButton";
			this.filePathBrowseButton.Size = new System.Drawing.Size(30, 20);
			this.filePathBrowseButton.TabIndex = 3;
			this.filePathBrowseButton.Text = "...";
			this.filePathBrowseButton.UseVisualStyleBackColor = true;
			this.filePathBrowseButton.Click += new System.EventHandler(this.filePathBrowseButton_Click);
			// 
			// mouthSlotLabel
			// 
			this.mouthSlotLabel.AutoSize = true;
			this.mouthSlotLabel.Location = new System.Drawing.Point(6, 32);
			this.mouthSlotLabel.Name = "mouthSlotLabel";
			this.mouthSlotLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
			this.mouthSlotLabel.Size = new System.Drawing.Size(56, 19);
			this.mouthSlotLabel.TabIndex = 2;
			this.mouthSlotLabel.Text = "Mouth slot";
			// 
			// animationFileBindingSource
			// 
			this.animationFileBindingSource.DataMember = "AnimationFileModel";
			this.animationFileBindingSource.DataSource = this.mainBindingSource;
			// 
			// mouthShapesLabel
			// 
			this.mouthShapesLabel.AutoSize = true;
			this.mouthShapesLabel.Location = new System.Drawing.Point(6, 79);
			this.mouthShapesLabel.Name = "mouthShapesLabel";
			this.mouthShapesLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
			this.mouthShapesLabel.Size = new System.Drawing.Size(74, 19);
			this.mouthShapesLabel.TabIndex = 4;
			this.mouthShapesLabel.Text = "Mouth shapes";
			// 
			// audioEventsLabel
			// 
			this.audioEventsLabel.AutoSize = true;
			this.audioEventsLabel.Location = new System.Drawing.Point(6, 98);
			this.audioEventsLabel.Name = "audioEventsLabel";
			this.audioEventsLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
			this.audioEventsLabel.Size = new System.Drawing.Size(69, 19);
			this.audioEventsLabel.TabIndex = 6;
			this.audioEventsLabel.Text = "Audio events";
			// 
			// audioEventsDataGridView
			// 
			this.audioEventsDataGridView.AutoSizeRowsMode = System.Windows.Forms.DataGridViewAutoSizeRowsMode.AllCellsExceptHeaders;
			this.audioEventsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
			this.audioEventsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.eventColumn,
            this.audioFileColumn,
            this.dialogColumn,
            this.statusColumn,
            this.actionColumn});
			dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
			dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Window;
			dataGridViewCellStyle2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.ControlText;
			dataGridViewCellStyle2.SelectionBackColor = System.Drawing.SystemColors.Window;
			dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.ControlText;
			dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
			this.audioEventsDataGridView.DefaultCellStyle = dataGridViewCellStyle2;
			this.audioEventsDataGridView.Dock = System.Windows.Forms.DockStyle.Fill;
			this.animationFileErrorProvider.SetIconPadding(this.audioEventsDataGridView, -20);
			this.audioEventsDataGridView.Location = new System.Drawing.Point(93, 101);
			this.audioEventsDataGridView.Name = "audioEventsDataGridView";
			this.audioEventsDataGridView.ReadOnly = true;
			this.audioEventsDataGridView.RowHeadersVisible = false;
			this.audioEventsDataGridView.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
			this.audioEventsDataGridView.Size = new System.Drawing.Size(804, 505);
			this.audioEventsDataGridView.TabIndex = 7;
			this.audioEventsDataGridView.CellClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.audioEventsDataGridView_CellClick);
			this.audioEventsDataGridView.CellPainting += new System.Windows.Forms.DataGridViewCellPaintingEventHandler(this.audioEventsDataGridView_CellPainting);
			// 
			// mouthShapesResultLabel
			// 
			this.mouthShapesResultLabel.AutoSize = true;
			this.mouthShapesResultLabel.DataBindings.Add(new System.Windows.Forms.Binding("Text", this.animationFileBindingSource, "MouthShapesDisplayString", true));
			this.mouthShapesResultLabel.Location = new System.Drawing.Point(93, 79);
			this.mouthShapesResultLabel.Name = "mouthShapesResultLabel";
			this.mouthShapesResultLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
			this.mouthShapesResultLabel.Size = new System.Drawing.Size(16, 19);
			this.mouthShapesResultLabel.TabIndex = 8;
			this.mouthShapesResultLabel.Text = "   ";
			// 
			// mouthNamingLabel
			// 
			this.mouthNamingLabel.AutoSize = true;
			this.mouthNamingLabel.Location = new System.Drawing.Point(6, 59);
			this.mouthNamingLabel.Name = "mouthNamingLabel";
			this.mouthNamingLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
			this.mouthNamingLabel.Size = new System.Drawing.Size(74, 19);
			this.mouthNamingLabel.TabIndex = 9;
			this.mouthNamingLabel.Text = "Mouth naming";
			// 
			// mouthNamingResultLabel
			// 
			this.mouthNamingResultLabel.AutoSize = true;
			this.mouthNamingResultLabel.DataBindings.Add(new System.Windows.Forms.Binding("Text", this.animationFileBindingSource, "MouthNaming.DisplayString", true));
			this.mouthNamingResultLabel.Location = new System.Drawing.Point(93, 59);
			this.mouthNamingResultLabel.Name = "mouthNamingResultLabel";
			this.mouthNamingResultLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
			this.mouthNamingResultLabel.Size = new System.Drawing.Size(16, 19);
			this.mouthNamingResultLabel.TabIndex = 10;
			this.mouthNamingResultLabel.Text = "   ";
			// 
			// mainErrorProvider
			// 
			this.mainErrorProvider.BlinkStyle = System.Windows.Forms.ErrorBlinkStyle.NeverBlink;
			this.mainErrorProvider.ContainerControl = this;
			this.mainErrorProvider.DataSource = this.mainBindingSource;
			// 
			// animationFileErrorProvider
			// 
			this.animationFileErrorProvider.BlinkStyle = System.Windows.Forms.ErrorBlinkStyle.NeverBlink;
			this.animationFileErrorProvider.ContainerControl = this;
			this.animationFileErrorProvider.DataSource = this.animationFileBindingSource;
			// 
			// mainBindingSource
			// 
			this.mainBindingSource.DataSource = typeof(rhubarb_for_spine.MainModel);
			// 
			// mouthSlotComboBox
			// 
			this.mouthSlotComboBox.Dock = System.Windows.Forms.DockStyle.Top;
			this.mouthSlotComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.mouthSlotComboBox.Location = new System.Drawing.Point(93, 35);
			this.mouthSlotComboBox.Name = "mouthSlotComboBox";
			this.mouthSlotComboBox.Size = new System.Drawing.Size(804, 21);
			this.mouthSlotComboBox.TabIndex = 3;
			// 
			// eventColumn
			// 
			this.eventColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
			this.eventColumn.DataPropertyName = "Name";
			this.eventColumn.HeaderText = "Event";
			this.eventColumn.Name = "eventColumn";
			this.eventColumn.ReadOnly = true;
			// 
			// audioFileColumn
			// 
			this.audioFileColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
			this.audioFileColumn.DataPropertyName = "DisplayFilePath";
			this.audioFileColumn.HeaderText = "Audio file";
			this.audioFileColumn.Name = "audioFileColumn";
			this.audioFileColumn.ReadOnly = true;
			// 
			// dialogColumn
			// 
			this.dialogColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
			this.dialogColumn.DataPropertyName = "Dialog";
			dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
			this.dialogColumn.DefaultCellStyle = dataGridViewCellStyle1;
			this.dialogColumn.FillWeight = 300F;
			this.dialogColumn.HeaderText = "Dialog";
			this.dialogColumn.Name = "dialogColumn";
			this.dialogColumn.ReadOnly = true;
			// 
			// statusColumn
			// 
			this.statusColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
			this.statusColumn.DataPropertyName = "Status";
			this.statusColumn.HeaderText = "Status";
			this.statusColumn.Name = "statusColumn";
			this.statusColumn.ReadOnly = true;
			// 
			// actionColumn
			// 
			this.actionColumn.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.None;
			this.actionColumn.DataPropertyName = "ActionLabel";
			this.actionColumn.HeaderText = "";
			this.actionColumn.Name = "actionColumn";
			this.actionColumn.ReadOnly = true;
			this.actionColumn.Resizable = System.Windows.Forms.DataGridViewTriState.True;
			this.actionColumn.Text = "";
			this.actionColumn.Width = 80;
			// 
			// MainForm
			// 
			this.AllowDrop = true;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(913, 622);
			this.Controls.Add(this.tableLayoutPanel1);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "MainForm";
			this.Padding = new System.Windows.Forms.Padding(5);
			this.Text = "Rhubarb Lip Sync for Spine";
			this.DragDrop += new System.Windows.Forms.DragEventHandler(this.MainForm_DragDrop);
			this.DragEnter += new System.Windows.Forms.DragEventHandler(this.MainForm_DragEnter);
			this.tableLayoutPanel1.ResumeLayout(false);
			this.tableLayoutPanel1.PerformLayout();
			this.panel1.ResumeLayout(false);
			this.panel1.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.animationFileBindingSource)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.audioEventsDataGridView)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.mainErrorProvider)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.animationFileErrorProvider)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.mainBindingSource)).EndInit();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
		private System.Windows.Forms.Label filePathLabel;
		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.TextBox filePathTextBox;
		private System.Windows.Forms.Button filePathBrowseButton;
		private System.Windows.Forms.Label mouthSlotLabel;
		private BindableComboBox mouthSlotComboBox;
		private System.Windows.Forms.Label mouthShapesLabel;
		private System.Windows.Forms.Label audioEventsLabel;
		private System.Windows.Forms.DataGridView audioEventsDataGridView;
		private System.Windows.Forms.BindingSource mainBindingSource;
		private System.Windows.Forms.ErrorProvider mainErrorProvider;
		private System.Windows.Forms.Label mouthShapesResultLabel;
		private System.Windows.Forms.Label mouthNamingLabel;
		private System.Windows.Forms.Label mouthNamingResultLabel;
		private System.Windows.Forms.BindingSource animationFileBindingSource;
		private System.Windows.Forms.ErrorProvider animationFileErrorProvider;
		private System.Windows.Forms.DataGridViewTextBoxColumn eventColumn;
		private System.Windows.Forms.DataGridViewTextBoxColumn audioFileColumn;
		private System.Windows.Forms.DataGridViewTextBoxColumn dialogColumn;
		private System.Windows.Forms.DataGridViewTextBoxColumn statusColumn;
		private System.Windows.Forms.DataGridViewButtonColumn actionColumn;
	}
}

