using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace rhubarb_for_spine {
	public static class RhubarbCli {
		public static async Task<IReadOnlyCollection<MouthCue>> AnimateAsync(
			string audioFilePath, string dialog, ISet<MouthShape> extendedMouthShapes,
			IProgress<double> progress, CancellationToken cancellationToken
		) {
			if (cancellationToken.IsCancellationRequested) {
				throw new OperationCanceledException();
			}
			if (!File.Exists(audioFilePath)) {
				throw new ArgumentException($"File '{audioFilePath}' does not exist.");
			}

			using (var dialogFile = dialog != null ? new TemporaryTextFile(dialog) : null) {
				string rhubarbExePath = GetRhubarbExePath();
				string args = CreateArgs(audioFilePath, extendedMouthShapes, dialogFile?.FilePath);

				bool success = false;
				string errorMessage = null;
				string resultString = "";

				await ProcessTools.RunProcessAsync(
					rhubarbExePath, args,
					outString => resultString += outString,
					errString => {
						dynamic json = JObject.Parse(errString);
						switch ((string) json.type) {
							case "progress":
								progress.Report((double) json.value);
								break;
							case "success":
								success = true;
								break;
							case "failure":
								errorMessage = json.reason;
								break;
						}
					},
					cancellationToken);

				if (errorMessage != null) {
					throw new ApplicationException(errorMessage);
				}
				if (success) {
					progress.Report(1.0);
					return ParseRhubarbResult(resultString);
				}
				throw new ApplicationException("Rhubarb did not return a result.");
			}
		}

		private static string CreateArgs(
			string audioFilePath, ISet<MouthShape> extendedMouthShapes, string dialogFilePath
		) {
			string extendedShapesString =
				string.Join("", extendedMouthShapes.Select(shape => shape.ToString()));
			string args = "--machineReadable"
				+ " --exportFormat json"
				+ $" --extendedShapes \"{extendedShapesString}\""
				+ $" \"{audioFilePath}\"";
			if (dialogFilePath != null) {
				args = $"--dialogFile \"{dialogFilePath}\" " + args;
			}
			return args;
		}

		private static IReadOnlyCollection<MouthCue> ParseRhubarbResult(string jsonString) {
			dynamic json = JObject.Parse(jsonString);
			JArray mouthCues = json.mouthCues;
			return mouthCues
				.Cast<dynamic>()
				.Select(mouthCue => {
					TimeSpan time = TimeSpan.FromSeconds((double) mouthCue.start);
					MouthShape mouthShape = (MouthShape) Enum.Parse(typeof(MouthShape), (string) mouthCue.value);
					return new MouthCue(time, mouthShape);
				})
				.ToList();
		}

		private static string GetRhubarbExePath() {
			bool onUnix = Environment.OSVersion.Platform == PlatformID.Unix
						|| Environment.OSVersion.Platform == PlatformID.MacOSX;
			string exeName = "rhubarb" + (onUnix ? "" : ".exe");
			string guiExeDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
			string currentDirectory = guiExeDirectory;
			while (currentDirectory != Path.GetPathRoot(guiExeDirectory)) {
				string candidate = Path.Combine(currentDirectory, exeName);
				if (File.Exists(candidate)) {
					return candidate;
				}
				currentDirectory = Path.GetDirectoryName(currentDirectory);
			}
			throw new ApplicationException(
				$"Could not find Rhubarb Lip Sync executable '{exeName}'."
				+ $" Expected to find it in '{guiExeDirectory}' or any directory above.");
		}

		private class TemporaryTextFile : IDisposable {
			public string FilePath { get; }

			public TemporaryTextFile(string text) {
				FilePath = Path.GetTempFileName();
				File.WriteAllText(FilePath, text);
			}

			public void Dispose() {
				File.Delete(FilePath);
			}
		}
	}
}