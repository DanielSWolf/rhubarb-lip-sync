using System;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace rhubarb_for_spine {
	public partial class MainForm : Form {
		private MainModel MainModel { get; }

		public MainForm(MainModel mainModel) {
			InitializeComponent();
			MainModel = mainModel;
			mainBindingSource.DataSource = mainModel;
		}

		private void filePathBrowseButton_Click(object sender, EventArgs e) {
			var openFileDialog = new OpenFileDialog {
				Filter = "JSON files (*.json)|*.json",
				InitialDirectory = filePathTextBox.Text
			};
			if (openFileDialog.ShowDialog() == DialogResult.OK) {
				filePathTextBox.Text = openFileDialog.FileName;
			}
		}

		private void MainForm_DragEnter(object sender, DragEventArgs e) {
			if (e.Data.GetDataPresent(DataFormats.FileDrop)) {
				e.Effect = DragDropEffects.Copy;
			}
		}

		private void MainForm_DragDrop(object sender, DragEventArgs e) {
			var files = (string[]) e.Data.GetData(DataFormats.FileDrop);
			if (files.Any()) {
				filePathTextBox.Text = files.First();
			}
		}

		private void audioEventsDataGridView_CellClick(object sender, DataGridViewCellEventArgs e) {
			if (e.ColumnIndex != actionColumn.Index) return;

			AudioFileModel audioFileModel = GetAudioFileModel(e.RowIndex);
			audioFileModel?.PerformAction();
		}

		private void audioEventsDataGridView_CellPainting(object sender, DataGridViewCellPaintingEventArgs e) {
			if (e.ColumnIndex != statusColumn.Index || e.RowIndex == -1) return;

			e.PaintBackground(e.CellBounds, false);
			AudioFileModel audioFileModel = GetAudioFileModel(e.RowIndex);
			if (audioFileModel == null) return;

			string text = string.Empty;
			StringAlignment horizontalTextAlignment = StringAlignment.Near;
			Color backgroundColor = Color.Transparent;
			double? progress = null;
			switch (audioFileModel.Status) {
				case AudioFileStatus.NotAnimated:
					break;
				case AudioFileStatus.Scheduled:
					text = "Waiting";
					backgroundColor = SystemColors.Highlight.WithOpacity(0.2);
					break;
				case AudioFileStatus.Animating:
					progress = audioFileModel.AnimationProgress ?? 0.0;
					text = $"{(int) (progress * 100)}%";
					horizontalTextAlignment = StringAlignment.Center;
					break;
				case AudioFileStatus.Done:
					text = "Done";
					break;
				default:
					throw new ArgumentOutOfRangeException();
			}

			// Draw background
			var bounds = e.CellBounds;
			e.Graphics.FillRectangle(new SolidBrush(backgroundColor), bounds);

			// Draw progress bar
			if (progress.HasValue) {
				e.Graphics.FillRectangle(
					SystemBrushes.Highlight,
					bounds.X, bounds.Y, bounds.Width * (float) progress, bounds.Height);
			}

			// Draw text
			var stringFormat = new StringFormat {
				Alignment = horizontalTextAlignment,
				LineAlignment = StringAlignment.Center
			};
			e.Graphics.DrawString(
				text,
				e.CellStyle.Font, new SolidBrush(e.CellStyle.ForeColor),
				bounds, stringFormat);

			e.Handled = true;
		}

		private AudioFileModel GetAudioFileModel(int rowIndex) {
			return rowIndex < 0
				? null
				: MainModel.AnimationFileModel?.AudioFileModels[rowIndex];
		}
	}

}
