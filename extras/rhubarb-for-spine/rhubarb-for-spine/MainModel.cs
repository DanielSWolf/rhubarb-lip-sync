using System;
using System.IO;
using System.Threading;

namespace rhubarb_for_spine {
	public class MainModel : ModelBase {
		// For the time being, we're allowing only one file to be animated at a time.
		// Rhubarb tries to use all processor cores, so there isn't much to be gained by allowing
		// multiple parallel jobs. On the other hand, too many parallel Rhubarb instances may
		// seriously slow down the system.
		readonly SemaphoreSlim semaphore = new SemaphoreSlim(1);

		private string _filePath;
		private AnimationFileModel _animationFileModel;

		public MainModel(string filePath = null) {
			FilePath = filePath;
		}

		public string FilePath {
			get { return _filePath; }
			set {
				_filePath = value;
				AnimationFileModel = null;
				if (string.IsNullOrEmpty(_filePath)) {
					SetError(nameof(FilePath), "No input file specified.");
				} else if (!File.Exists(_filePath)) {
					SetError(nameof(FilePath), "File does not exist.");
				} else {
					try {
						AnimationFileModel = new AnimationFileModel(_filePath, semaphore);
						SetError(nameof(FilePath), null);
					} catch (Exception e) {
						SetError(nameof(FilePath), e.Message);
					}
				}
				OnPropertyChanged(nameof(FilePath));
			}
		}

		public AnimationFileModel AnimationFileModel {
			get { return _animationFileModel; }
			set {
				_animationFileModel = value;
				OnPropertyChanged(nameof(AnimationFileModel));
			}
		}

	}
}
