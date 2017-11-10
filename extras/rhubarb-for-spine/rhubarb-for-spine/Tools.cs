using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;

namespace rhubarb_for_spine {
	public static class Tools {
		public static string GetCommonPrefix(this IReadOnlyCollection<string> strings) {
			return strings.Any()
				? strings.First().Substring(0, GetCommonPrefixLength(strings))
				: string.Empty;
		}

		public static int GetCommonPrefixLength(this IReadOnlyCollection<string> strings) {
			if (!strings.Any()) return 0;

			string first = strings.First();
			int result = first.Length;
			foreach (string s in strings) {
				for (int i = 0; i < Math.Min(result, s.Length); i++) {
					if (s[i] != first[i]) {
						result = i;
						break;
					}
				}
			}
			return result;
		}

		public static string GetCommonSuffix(this IReadOnlyCollection<string> strings) {
			if (!strings.Any()) return string.Empty;

			int commonSuffixLength = GetCommonSuffixLength(strings);
			string first = strings.First();
			return first.Substring(first.Length - commonSuffixLength);
		}

		public static int GetCommonSuffixLength(this IReadOnlyCollection<string> strings) {
			if (!strings.Any()) return 0;

			string first = strings.First();
			int result = first.Length;
			foreach (string s in strings) {
				for (int i = 0; i < Math.Min(result, s.Length); i++) {
					if (s[s.Length - 1 - i] != first[first.Length - 1 - i]) {
						result = i;
						break;
					}
				}
			}
			return result;
		}

		public static bool Contains(this string s, string value, StringComparison stringComparison) {
			return s.IndexOf(value, stringComparison) >= 0;
		}

		public static string Join(this IEnumerable<string> values, string separator) {
			return string.Join(separator, values);
		}

		public static Color WithOpacity(this Color color, double opacity) {
			return Color.FromArgb((int) (opacity * 255), color);
		}
	}
	}