using System;
using System.Collections.Generic;
using System.Linq;

namespace rhubarb_for_spine {
	public enum MouthShape {
		A, B, C, D, E, F, G, H, X
	}

	public static class MouthShapes {
		public static IReadOnlyCollection<MouthShape> All =>
			Enum.GetValues(typeof(MouthShape))
				.Cast<MouthShape>()
				.ToList();

		public const int BasicShapesCount = 6;

		public static bool IsBasic(MouthShape mouthShape) =>
			(int) mouthShape < BasicShapesCount;

		public static IReadOnlyCollection<MouthShape> Basic =>
			All.Take(BasicShapesCount).ToList();
	}
}