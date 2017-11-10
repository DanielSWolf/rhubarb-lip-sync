using System;

namespace rhubarb_for_spine {
	public class MouthCue {
		public MouthCue(TimeSpan time, MouthShape mouthShape) {
			Time = time;
			MouthShape = mouthShape;
		}

		public TimeSpan Time { get; }
		public MouthShape MouthShape { get; }

		public override string ToString() {
			return $"{Time}: {MouthShape}";
		}
	}
}