using System.Collections.Generic;
using System.ComponentModel;

namespace rhubarb_for_spine {
	public class ModelBase : INotifyPropertyChanged, IDataErrorInfo {
		private readonly Dictionary<string, string> errors = new Dictionary<string, string>();

		public event PropertyChangedEventHandler PropertyChanged;

		protected void OnPropertyChanged(string propertyName) {
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
		}

		protected void SetError(string propertyName, string error) {
			errors[propertyName] = error;
		}

		string IDataErrorInfo.this[string propertyName] =>
			errors.ContainsKey(propertyName) ? errors[propertyName] : null;

		string IDataErrorInfo.Error => null;

	}
}