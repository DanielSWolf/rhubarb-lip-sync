using System;
using System.Collections.Generic;
using System.Threading;

namespace rhubarb_for_spine {
	public class AudioFileModel : ModelBase {
		private readonly string filePath;
		private readonly ISet<MouthShape> extendedMouthShapes;
		private readonly SemaphoreSlim semaphore;
		private readonly Action<IReadOnlyCollection<MouthCue>> reportResult;
		private bool animatedPreviously;
		private CancellationTokenSource cancellationTokenSource;

		private AudioFileStatus _status;
		private string _actionLabel;
		private double? _animationProgress;

		public AudioFileModel(
			string name,
			string filePath,
			string displayFilePath,
			string dialog,
			ISet<MouthShape> extendedMouthShapes,
			bool animatedPreviously,
			SemaphoreSlim semaphore,
			Action<IReadOnlyCollection<MouthCue>> reportResult
		) {
			this.reportResult = reportResult;
			Name = name;
			this.filePath = filePath;
			this.extendedMouthShapes = extendedMouthShapes;
			this.animatedPreviously = animatedPreviously;
			this.semaphore = semaphore;
			DisplayFilePath = displayFilePath;
			Dialog = dialog;
			Status = animatedPreviously ? AudioFileStatus.Done : AudioFileStatus.NotAnimated;
		}

		public string Name { get; }
		public string DisplayFilePath { get; }
		public string Dialog { get; }

		public double? AnimationProgress {
			get { return _animationProgress; }
			private set {
				_animationProgress = value;
				OnPropertyChanged(nameof(AnimationProgress));

				// Hack, so that a binding to Status will refresh
				OnPropertyChanged(nameof(Status));
			}
		}

		public AudioFileStatus Status {
			get { return _status; }
			private set {
				_status = value;
				ActionLabel = _status == AudioFileStatus.Scheduled || _status == AudioFileStatus.Animating
					? "Cancel"
					: "Animate";
				OnPropertyChanged(nameof(Status));
			}
		}

		public string ActionLabel {
			get { return _actionLabel; }
			private set {
				_actionLabel = value;
				OnPropertyChanged(nameof(ActionLabel));
			}
		}

		public void PerformAction() {
			switch (Status) {
				case AudioFileStatus.NotAnimated:
				case AudioFileStatus.Done:
					StartAnimation();
					break;
				case AudioFileStatus.Scheduled:
				case AudioFileStatus.Animating:
					CancelAnimation();
					break;
				default:
					throw new ArgumentOutOfRangeException();
			}
		}

		private async void StartAnimation() {
			cancellationTokenSource = new CancellationTokenSource();
			Status = AudioFileStatus.Scheduled;
			try {
				await semaphore.WaitAsync(cancellationTokenSource.Token);
				Status = AudioFileStatus.Animating;
				try {
					var progress = new Progress(value => AnimationProgress = value);
					IReadOnlyCollection<MouthCue> mouthCues = await RhubarbCli.AnimateAsync(
						filePath, Dialog, extendedMouthShapes, progress, cancellationTokenSource.Token);
					animatedPreviously = true;
					Status = AudioFileStatus.Done;
					reportResult(mouthCues);
				} finally {
					AnimationProgress = null;
					semaphore.Release();
					cancellationTokenSource = null;
				}
			} catch (OperationCanceledException) {
				Status = animatedPreviously ? AudioFileStatus.Done : AudioFileStatus.NotAnimated;
			}
		}

		private void CancelAnimation() {
			cancellationTokenSource.Cancel();
		}

		private class Progress : IProgress<double> {
			private readonly Action<double> report;
			private double value;

			public Progress(Action<double> report) {
				this.report = report;
			}

			public void Report(double progress) {
				value = progress;
				report(progress);
			}
		}
	}

	public enum AudioFileStatus {
		NotAnimated,
		Scheduled,
		Animating,
		Done
	}
}