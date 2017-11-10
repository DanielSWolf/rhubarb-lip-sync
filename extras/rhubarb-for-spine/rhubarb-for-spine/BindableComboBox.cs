using System;
using System.Linq;
using System.Windows.Forms;

namespace rhubarb_for_spine {
	/// <summary>
	/// A modification of the standard <see cref="ComboBox"/> in which a data binding
	/// on the SelectedItem property with the update mode set to DataSourceUpdateMode.OnPropertyChanged
	/// actually updates when a selection is made in the combobox.
	/// Code taken from https://stackoverflow.com/a/8392100/52041
	/// </summary>
	public class BindableComboBox : ComboBox {
		/// <inheritdoc />
		protected override void OnSelectionChangeCommitted(EventArgs e) {
			base.OnSelectionChangeCommitted(e);

			var bindings = DataBindings
				.Cast<Binding>()
				.Where(binding => binding.PropertyName == nameof(SelectedItem)
					&& binding.DataSourceUpdateMode == DataSourceUpdateMode.OnPropertyChanged);
			foreach (Binding binding in bindings) {
				// Force the binding to update from the new SelectedItem
				binding.WriteValue();

				// Force the Textbox to update from the binding
				binding.ReadValue();
			}
		}
	}
}