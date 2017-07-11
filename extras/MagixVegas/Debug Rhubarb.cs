using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing;
using System.Drawing.Design;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.UI.Design;
using System.Windows.Forms;
using System.Windows.Forms.Design;
using System.Xml;
using System.Xml.Serialization;
using ScriptPortal.Vegas; // For older versions, this should say Sony.Vegas
using Region = ScriptPortal.Vegas.Region; // For older versions, this should say Sony.Vegas.Region

public class EntryPoint {
	public void FromVegas(Vegas vegas) {
		Config config = Config.Load();
		ImportDialog importDialog = new ImportDialog(config, delegate { Import(config, vegas); });
		importDialog.ShowDialog();
		config.Save();
	}

	private void Import(Config config, Vegas vegas) {
		Project project = vegas.Project;

		// Clear markers and regions
		if (config.ClearMarkers) {
			project.Markers.Clear();
		}
		if (config.ClearRegions) {
			project.Regions.Clear();
		}

		// Load log file
		if (!File.Exists(config.LogFile)) {
			throw new Exception("Log file does not exist.");
		}
		Dictionary<EventType, List<TimedEvent>> timedEvents = ParseLogFile(config);

		// Add markers/regions
		foreach (EventType eventType in timedEvents.Keys) {
			foreach (Visualization visualization in config.Visualizations) {
				if (visualization.EventType != eventType) continue;

				List<TimedEvent> filteredEvents = FilterEvents(timedEvents[eventType], visualization.Regex);
				foreach (TimedEvent timedEvent in filteredEvents) {
					Timecode start = Timecode.FromSeconds(timedEvent.Start);
					Timecode end = Timecode.FromSeconds(timedEvent.End);
					Timecode length = end - start;
					if (config.LoopRegionOnly) {
						Timecode loopRegionStart = vegas.Transport.LoopRegionStart;
						Timecode loopRegionEnd = loopRegionStart + vegas.Transport.LoopRegionLength;
						if (start < loopRegionStart || start > loopRegionEnd || end < loopRegionStart || end > loopRegionEnd) {
							continue;
						}
					}
					switch (visualization.VisualizationType) {
						case VisualizationType.Marker:
							project.Markers.Add(new Marker(start, timedEvent.Value));
							break;
						case VisualizationType.Region:
							project.Regions.Add(new Region(start, length, timedEvent.Value));
							break;
					}
				}
			}
		}
	}

	private List<TimedEvent> FilterEvents(List<TimedEvent> timedEvents, Regex filterRegex) {
		if (filterRegex == null) return timedEvents;

		StringBuilder stringBuilder = new StringBuilder();
		Dictionary<int, TimedEvent> timedEventsByCharPosition = new Dictionary<int, TimedEvent>();
		foreach (TimedEvent timedEvent in timedEvents) {
			string inAngleBrackets = "<" + timedEvent.Value + ">";
			for (int charPosition = stringBuilder.Length;
				charPosition < stringBuilder.Length + inAngleBrackets.Length;
				charPosition++) {
				timedEventsByCharPosition[charPosition] = timedEvent;
			}
			stringBuilder.Append(inAngleBrackets);
		}

		MatchCollection matches = filterRegex.Matches(stringBuilder.ToString());
		List<TimedEvent> result = new List<TimedEvent>();
		foreach (Match match in matches) {
			if (match.Length == 0) continue;

			for (int charPosition = match.Index; charPosition < match.Index + match.Length; charPosition++) {
				TimedEvent matchedEvent = timedEventsByCharPosition[charPosition];
				if (!result.Contains(matchedEvent)) {
					result.Add(matchedEvent);
				}
			}
		}
		return result;
	}

	private static Dictionary<EventType, List<TimedEvent>> ParseLogFile(Config config) {
		string[] lines = File.ReadAllLines(config.LogFile);
		Regex structuredLogLine = new Regex(@"##(\w+)\[(\d*\.\d*)-(\d*\.\d*)\]: (.*)");
		Dictionary<EventType, List<TimedEvent>> timedEvents = new Dictionary<EventType, List<TimedEvent>>();
		foreach (string line in lines) {
			Match match = structuredLogLine.Match(line);
			if (!match.Success) continue;

			EventType eventType = (EventType) Enum.Parse(typeof(EventType), match.Groups[1].Value, true);
			double start = double.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture);
			double end = double.Parse(match.Groups[3].Value, CultureInfo.InvariantCulture);
			string value = match.Groups[4].Value;

			if (!timedEvents.ContainsKey(eventType)) {
				timedEvents[eventType] = new List<TimedEvent>();
			}
			timedEvents[eventType].Add(new TimedEvent(eventType, start, end, value));
		}
		return timedEvents;
	}
}

public class TimedEvent {
	private readonly EventType eventType;
	private readonly double start;
	private readonly double end;
	private readonly string value;

	public TimedEvent(EventType eventType, double start, double end, string value) {
		this.eventType = eventType;
		this.start = start;
		this.end = end;
		this.value = value;
	}

	public EventType EventType {
		get { return eventType; }
	}

	public double Start {
		get { return start; }
	}

	public double End {
		get { return end; }
	}

	public string Value {
		get { return value; }
	}
}

public class Config {
	private string logFile;
	private bool clearMarkers;
	private bool clearRegions;
	private bool loopRegionOnly;
	private List<Visualization> visualizations = new List<Visualization>();

	[DisplayName("Log File")]
	[Description("A log file generated by Rhubarb Lip Sync.")]
	[Editor(typeof(FileNameEditor), typeof(UITypeEditor))]
	public string LogFile {
		get { return logFile; }
		set { logFile = value; }
	}

	[DisplayName("Clear Markers")]
	[Description("Clear all markers in the current project.")]
	public bool ClearMarkers {
		get { return clearMarkers; }
		set { clearMarkers = value; }
	}

	[DisplayName("Clear Regions")]
	[Description("Clear all regions in the current project.")]
	public bool ClearRegions {
		get { return clearRegions; }
		set { clearRegions = value; }
	}

	[DisplayName("Loop region only")]
	[Description("Adds regions or markers to the loop region only.")]
	public bool LoopRegionOnly {
		get { return loopRegionOnly; }
		set { loopRegionOnly = value; }
	}

	[DisplayName("Visualization rules")]
	[Description("Specify how to visualize various log events.")]
	[Editor(typeof(CollectionEditor), typeof(UITypeEditor))]
	[XmlIgnore]
	public List<Visualization> Visualizations {
		get { return visualizations; }
		set { visualizations = value; }
	}

	[Browsable(false)]
	public Visualization[] VisualizationArray {
		get { return visualizations.ToArray(); }
		set { visualizations = new List<Visualization>(value); }
	}

	private static string ConfigFileName {
		get {
			string folder = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
			return Path.Combine(folder, "DebugRhubarbSettings.xml");
		}
	}

	public static Config Load() {
		try {
			XmlSerializer serializer = new XmlSerializer(typeof(Config));
			using (FileStream file = File.OpenRead(ConfigFileName)) {
				return (Config) serializer.Deserialize(file);
			}
		} catch (Exception) {
			return new Config();
		}
	}

	public void Save() {
		XmlSerializer serializer = new XmlSerializer(typeof(Config));
		using (StreamWriter file = File.CreateText(ConfigFileName)) {
			XmlWriterSettings settings = new XmlWriterSettings();
			settings.Indent = true;
			settings.IndentChars = "\t";
			using (XmlWriter writer = XmlWriter.Create(file, settings)) {
				serializer.Serialize(writer, this);
			}
		}
	}
}

public class Visualization {
	private EventType eventType;
	private string regexString;
	private VisualizationType visualizationType = VisualizationType.Marker;

	[DisplayName("Event Type")]
	[Description("The type of event to visualize.")]
	public EventType EventType {
		get { return eventType; }
		set { eventType = value; }
	}

	[DisplayName("Regular Expression")]
	[Description("A regular expression used to filter events. Leave empty to disable filtering.\nInput is a string of events in angle brackets. Example: '<AO>(?=<T>)' finds every AO phone followed by a T phone.")]
	public string RegexString {
		get { return regexString; }
		set { regexString = value; }
	}

	[Browsable(false)]
	public Regex Regex {
		get { return string.IsNullOrEmpty(RegexString) ? null : new Regex(RegexString); }
	}

	[DisplayName("Visualization Type")]
	[Description("Specify how to visualize events.")]
	public VisualizationType VisualizationType {
		get { return visualizationType; }
		set { visualizationType = value; }
	}

	public override string ToString() {
		return string.Format("{0} -> {1}", EventType, VisualizationType);
	}
}

public enum EventType {
	Utterance,
	Word,
	RawPhone,
	Phone,
	Shape,
	Segment
}

public enum VisualizationType {
	None,
	Marker,
	Region
}

public delegate void ImportAction();

public class ImportDialog : Form {
	private readonly Config config;
	private readonly ImportAction import;

	public ImportDialog(Config config, ImportAction import) {
		this.config = config;
		this.import = import;
		SuspendLayout();
		InitializeComponent();
		ResumeLayout(false);
	}

	private void InitializeComponent() {
		// Configure dialog
		Text = "Debug Rhubarb";
		Size = new Size(600, 400);
		Font = new Font(Font.FontFamily, 10);

		// Add property grid
		PropertyGrid propertyGrid1 = new PropertyGrid();
		propertyGrid1.SelectedObject = config;
		Controls.Add(propertyGrid1);
		propertyGrid1.Dock = DockStyle.Fill;

		// Add button panel
		FlowLayoutPanel buttonPanel = new FlowLayoutPanel();
		buttonPanel.FlowDirection = FlowDirection.RightToLeft;
		buttonPanel.AutoSize = true;
		buttonPanel.Dock = DockStyle.Bottom;
		Controls.Add(buttonPanel);

		// Add Cancel button
		Button cancelButton1 = new Button();
		cancelButton1.Text = "Cancel";
		cancelButton1.DialogResult = DialogResult.Cancel;
		buttonPanel.Controls.Add(cancelButton1);
		CancelButton = cancelButton1;

		// Add OK button
		Button okButton1 = new Button();
		okButton1.Text = "OK";
		okButton1.Click += OkButtonClickedHandler;
		buttonPanel.Controls.Add(okButton1);
		AcceptButton = okButton1;
	}

	private void OkButtonClickedHandler(object sender, EventArgs e) {
		try {
			import();
			DialogResult = DialogResult.OK;
		} catch (Exception exception) {
			MessageBox.Show(exception.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}
	}
}