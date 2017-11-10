using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace rhubarb_for_spine {
	public class AnimationFileModel : ModelBase {
		private readonly string animationFilePath;
		private readonly SemaphoreSlim semaphore;
		private readonly JObject json;

		private string _mouthSlot;
		private MouthNaming _mouthNaming;
		private IReadOnlyCollection<MouthShape> _mouthShapes;
		private string _mouthShapesDisplayString;
		private BindingList<AudioFileModel> _audioFileModels;

		public AnimationFileModel(string animationFilePath, SemaphoreSlim semaphore) {
			this.animationFilePath = animationFilePath;
			this.semaphore = semaphore;
			json = SpineJson.ReadJson(animationFilePath);
			SpineJson.ValidateJson(json, AnimationFileDirectory);

			Slots = SpineJson.GetSlots(json);
			MouthSlot = SpineJson.GuessMouthSlot(Slots);
			var audioFileModels = SpineJson.GetAudioEvents(json)
				.Select(CreateAudioFileModel)
				.ToList();
			AudioFileModels = new BindingList<AudioFileModel>(audioFileModels);
		}

		public IReadOnlyCollection<string> Slots { get; }

		public string MouthSlot {
			get { return _mouthSlot; }
			set {
				_mouthSlot = value;
				MouthNaming = GetMouthNaming();
				MouthShapes = GetMouthShapes();
				OnPropertyChanged(nameof(MouthSlot));
			}
		}

		public MouthNaming MouthNaming {
			get { return _mouthNaming; }
			private set {
				_mouthNaming = value;
				OnPropertyChanged(nameof(MouthNaming));
			}
		}

		public IReadOnlyCollection<MouthShape> MouthShapes {
			get { return _mouthShapes; }
			private set {
				_mouthShapes = value;
				MouthShapesDisplayString = GetMouthShapesDisplayString();
				OnPropertyChanged(nameof(MouthShapes));
			}
		}

		public string MouthShapesDisplayString {
			get { return _mouthShapesDisplayString; }
			set {
				_mouthShapesDisplayString = value;
				SetError(nameof(MouthShapesDisplayString), GetMouthShapesDisplayStringError());
				OnPropertyChanged(nameof(MouthShapesDisplayString));
			}
		}

		public BindingList<AudioFileModel> AudioFileModels {
			get { return _audioFileModels; }
			set {
				_audioFileModels = value;
				if (!_audioFileModels.Any()) {
					SetError(nameof(AudioFileModels), "The JSON file doesn't contain any audio events.");
				}
				OnPropertyChanged(nameof(AudioFileModels));
			}
		}

		private string AnimationFileDirectory => Path.GetDirectoryName(animationFilePath);

		private MouthNaming GetMouthNaming() {
			IReadOnlyCollection<string> mouthNames = SpineJson.GetSlotAttachmentNames(json, _mouthSlot);
			return MouthNaming.Guess(mouthNames);
		}

		private List<MouthShape> GetMouthShapes() {
			IReadOnlyCollection<string> mouthNames = SpineJson.GetSlotAttachmentNames(json, _mouthSlot);
			return rhubarb_for_spine.MouthShapes.All
				.Where(shape => mouthNames.Contains(MouthNaming.GetName(shape)))
				.ToList();
		}

		private string GetMouthShapesDisplayString() {
			return MouthShapes
				.Select(shape => shape.ToString())
				.Join(", ");
		}

		private string GetMouthShapesDisplayStringError() {
			var basicMouthShapes = rhubarb_for_spine.MouthShapes.Basic;
			var missingBasicShapes = basicMouthShapes
				.Where(shape => !MouthShapes.Contains(shape))
				.ToList();
			if (!missingBasicShapes.Any()) return null;

			var result = new StringBuilder();
			string missingString = string.Join(", ", missingBasicShapes);
			result.AppendLine(missingBasicShapes.Count > 1
				? $"Mouth shapes {missingString} are missing."
				: $"Mouth shape {missingString} is missing.");
			MouthShape first = basicMouthShapes.First();
			MouthShape last = basicMouthShapes.Last();
			result.Append($"At least the basic mouth shapes {first}-{last} need corresponding image attachments.");
			return result.ToString();
		}

		private AudioFileModel CreateAudioFileModel(SpineJson.AudioEvent audioEvent) {
			string audioDirectory = SpineJson.GetAudioDirectory(json, AnimationFileDirectory);
			string filePath = Path.Combine(audioDirectory, audioEvent.RelativeAudioFilePath);
			bool animatedPreviously = SpineJson.HasAnimation(json, GetAnimationName(audioEvent.Name));
			var extendedMouthShapes = MouthShapes.Where(shape => !rhubarb_for_spine.MouthShapes.IsBasic(shape));
			return new AudioFileModel(
				audioEvent.Name, filePath, audioEvent.RelativeAudioFilePath, audioEvent.Dialog,
				new HashSet<MouthShape>(extendedMouthShapes), animatedPreviously,
				semaphore, cues => AcceptAnimationResult(cues, audioEvent));
		}

		private string GetAnimationName(string audioEventName) {
			return $"say_{audioEventName}";
		}

		private void AcceptAnimationResult(
			IReadOnlyCollection<MouthCue> mouthCues, SpineJson.AudioEvent audioEvent
		) {
			string animationName = GetAnimationName(audioEvent.Name);
			SpineJson.CreateOrUpdateAnimation(
				json, mouthCues, audioEvent.Name, animationName, MouthSlot, MouthNaming);
			File.WriteAllText(animationFilePath, json.ToString(Formatting.Indented));
		}
	}
}