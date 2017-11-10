using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Nito.AsyncEx;

namespace rhubarb_for_spine {
	public static class ProcessTools {

		public static async Task<int> RunProcessAsync(
			string processFilePath,
			string processArgs,
			Action<string> receiveStdout,
			Action<string> receiveStderr,
			CancellationToken cancellationToken
		) {
			var startInfo = new ProcessStartInfo {
				FileName = processFilePath,
				Arguments = processArgs,
				UseShellExecute = false, // Necessary to redirect streams
				CreateNoWindow = true,
				RedirectStandardOutput = true,
				RedirectStandardError = true
			};
			using (var process = new Process { StartInfo = startInfo, EnableRaisingEvents = true }) {
				// Process all events in the original call's context
				var pendingActions = new AsyncProducerConsumerQueue<Action>();
				int? exitCode = null;

				// ReSharper disable once AccessToDisposedClosure
				process.Exited += (sender, args) =>
					pendingActions.Enqueue(() => exitCode = process.ExitCode);

				process.OutputDataReceived += (sender, args) => {
					if (args.Data != null) {
						pendingActions.Enqueue(() => receiveStdout(args.Data));
					}
				};
				process.ErrorDataReceived += (sender, args) => {
					if (args.Data != null) {
						pendingActions.Enqueue(() => receiveStderr(args.Data));
					}
				};

				process.Start();
				process.BeginOutputReadLine();
				process.BeginErrorReadLine();

				cancellationToken.Register(() => pendingActions.Enqueue(() => {
					try {
						// ReSharper disable once AccessToDisposedClosure
						process.Kill();
					} catch (Exception e) {
						Debug.WriteLine($"Error terminating process: {e}");
					}
					// ReSharper disable once AccessToDisposedClosure
					process.WaitForExit();
					throw new OperationCanceledException();
				}));

				while (exitCode == null) {
					Action action = await pendingActions.DequeueAsync(cancellationToken);
					action();
				}
				return exitCode.Value;
			}
		}

	}
}