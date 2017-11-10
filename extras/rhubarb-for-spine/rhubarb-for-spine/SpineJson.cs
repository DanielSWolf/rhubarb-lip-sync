using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Newtonsoft.Json.Linq;

namespace rhubarb_for_spine {
	public static class SpineJson {
		public static JObject ReadJson(string filePath) {
			string jsonString = File.ReadAllText(filePath);
			try {
				return JObject.Parse(jsonString);
			} catch (Exception) {
				throw new ApplicationException("Wrong file format. This is not a valid JSON file.");
			}
		}

		public static void ValidateJson(JObject json, string animationFileDirectory) {
			// This method doesn't validate the entire JSON.
			// It merely checks that there are no obvious problems.

			dynamic skeleton = ((dynamic) json).skeleton;
			if (skeleton == null) {
				throw new ApplicationException("JSON file is corrupted.");
			}
			if (skeleton.images == null) {
				throw new ApplicationException(
					"JSON file is incomplete. Make sure you checked 'Nonessential data' when exporting.");
			}
			if (((dynamic) json).skins["default"] == null) {
				throw new ApplicationException("JSON file has no default skin.");
			}
			GetImagesDirectory(json, animationFileDirectory);
			GetAudioDirectory(json, animationFileDirectory);
		}

		public static string GetImagesDirectory(JObject json, string animationFileDirectory) {
			string result = Path.GetFullPath(Path.Combine(animationFileDirectory, (string) ((dynamic) json).skeleton.images));
			if (!Directory.Exists(result)) {
				throw new ApplicationException(
					"Could not find images directory relative to the JSON file."
					+ " Make sure the JSON file is in the same directory as the original Spine file.");
			}
			return result;
		}

		public static string GetAudioDirectory(JObject json, string animationFileDirectory) {
			string result = Path.GetFullPath(Path.Combine(animationFileDirectory, (string) ((dynamic) json).skeleton.audio));
			if (!Directory.Exists(result)) {
				throw new ApplicationException(
					"Could not find audio directory relative to the JSON file."
					+ " Make sure the JSON file is in the same directory as the original Spine file.");
			}
			return result;
		}

		public static double GetFrameRate(JObject json) {
			return (double?) ((dynamic) json).skeleton.fps ?? 30.0;
		}

		public static IReadOnlyCollection<string> GetSlots(JObject json) {
			return ((JArray) ((dynamic) json).slots)
				.Cast<dynamic>()
				.Select(slot => (string) slot.name)
				.ToList();
		}

		public static string GuessMouthSlot(IReadOnlyCollection<string> slots) {
			return slots.FirstOrDefault(slot => slot.Contains("mouth", StringComparison.InvariantCultureIgnoreCase))
				?? slots.FirstOrDefault();
		}

		public class AudioEvent {
			public AudioEvent(string name, string relativeAudioFilePath, string dialog) {
				Name = name;
				RelativeAudioFilePath = relativeAudioFilePath;
				Dialog = dialog;
			}

			public string Name { get; }
			public string RelativeAudioFilePath { get; }
			public string Dialog { get; }
		}

		public static IReadOnlyCollection<AudioEvent> GetAudioEvents(JObject json) {
			return ((IEnumerable<KeyValuePair<string, JToken>>) ((dynamic) json).events)
				.Select(pair => {
					string name = pair.Key;
					dynamic value = pair.Value;
					string relativeAudioFilePath = value.audio;
					string dialog = value["string"];
					return new AudioEvent(name, relativeAudioFilePath, dialog);
				})
				.Where(audioEvent => audioEvent.RelativeAudioFilePath != null)
				.ToList();
		}

		public static IReadOnlyCollection<string> GetSlotAttachmentNames(JObject json, string slotName) {
			return ((JObject) ((dynamic) json).skins["default"][slotName])
				.Properties()
				.Select(property => property.Name)
				.ToList();
		}

		public static bool HasAnimation(JObject json, string animationName) {
			JObject animations = ((dynamic) json).animations;
			if (animations == null) return false;

			return animations.Properties().Any(property => property.Name == animationName);
		}

		public static void CreateOrUpdateAnimation(
			JObject json, IReadOnlyCollection<MouthCue> animationResult,
			string eventName, string animationName, string mouthSlot, MouthNaming mouthNaming
		) {
			dynamic dynamicJson = json;
			dynamic animations = dynamicJson.animations;
			if (animations == null) {
				animations = dynamicJson.animations = new JObject();
			}

			// Round times to full frame. Always round down.
			// If events coincide, prefer the later one.
			double frameRate = GetFrameRate(json);
			var keyframes = new Dictionary<int, MouthShape>();
			foreach (MouthCue mouthCue in animationResult) {
				int frameNumber = (int) (mouthCue.Time.TotalSeconds * frameRate);
				keyframes[frameNumber] = mouthCue.MouthShape;
			}

			animations[animationName] = new JObject {
				["slots"] = new JObject {
					[mouthSlot] = new JObject {
						["attachment"] = new JArray(
							keyframes
								.OrderBy(pair => pair.Key)
								.Select(pair => new JObject {
									["time"] = pair.Key / frameRate,
									["name"] = mouthNaming.GetName(pair.Value)
								})
								.Cast<object>()
								.ToArray()
						)
					}
				},
				["events"] = new JArray {
					new JObject {
						["time"] = 0.0,
						["name"] = eventName,
						["string"] = ""
					}
				}
			};
		}
	}
}