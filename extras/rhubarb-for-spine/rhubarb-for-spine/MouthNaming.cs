using System.Collections.Generic;
using System.Linq;

namespace rhubarb_for_spine {
	public class MouthNaming {
		public MouthNaming(string prefix, string suffix, MouthShapeCasing mouthShapeCasing) {
			Prefix = prefix;
			Suffix = suffix;
			MouthShapeCasing = mouthShapeCasing;
		}

		public string Prefix { get; }
		public string Suffix { get; }
		public MouthShapeCasing MouthShapeCasing { get; }

		public static MouthNaming Guess(IReadOnlyCollection<string> mouthNames) {
			string firstMouthName = mouthNames.First();
			if (mouthNames.Count == 1) {
				return firstMouthName == string.Empty
					? new MouthNaming(string.Empty, string.Empty, MouthShapeCasing.Lower)
					: new MouthNaming(
						firstMouthName.Substring(0, firstMouthName.Length - 1),
						string.Empty,
						GuessMouthShapeCasing(firstMouthName.Last()));
			}

			string commonPrefix = mouthNames.GetCommonPrefix();
			string commonSuffix = mouthNames.GetCommonSuffix();
			var mouthShapeCasing = firstMouthName.Length > commonPrefix.Length
				? GuessMouthShapeCasing(firstMouthName[commonPrefix.Length])
				: MouthShapeCasing.Lower;
			return new MouthNaming(commonPrefix, commonSuffix, mouthShapeCasing);
		}

		public string DisplayString {
			get {
				string casing = MouthShapeCasing == MouthShapeCasing.Upper
					? "<UPPER-CASE SHAPE NAME>"
					: "<lower-case shape name>";
				return $"\"{Prefix}{casing}{Suffix}\"";
			}
		}

		public string GetName(MouthShape mouthShape) {
			string name = MouthShapeCasing == MouthShapeCasing.Upper
				? mouthShape.ToString()
				: mouthShape.ToString().ToLowerInvariant();
			return $"{Prefix}{name}{Suffix}";
		}

		private static MouthShapeCasing GuessMouthShapeCasing(char mouthShape) {
			return char.IsUpper(mouthShape) ? MouthShapeCasing.Upper : MouthShapeCasing.Lower;
		}
	}

	public enum MouthShapeCasing {
		Upper,
		Lower
	}
}