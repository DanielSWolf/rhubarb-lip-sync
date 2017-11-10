using System;
using System.Linq;
using System.Windows.Forms;

namespace rhubarb_for_spine {
	static class Program {
		[STAThread]
		static void Main(string[] args) {
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			string filePath = args.FirstOrDefault();
			MainModel mainModel = new MainModel(filePath);
			Application.Run(new MainForm(mainModel));
		}
	}
}
