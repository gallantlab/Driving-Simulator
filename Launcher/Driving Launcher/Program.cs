using System;
using Avalonia;
using Sentry;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;

namespace Driving_Launcher
{
	class Program
	{
		// Initialization code. Don't use any Avalonia, third-party APIs or any
		// SynchronizationContext-reliant code before AppMain is called: things aren't initialized
		// yet and stuff might break.
		public static void Main(string[] args)
		{
			//using (SentrySdk.Init("https://5ec6472093b64222b67391088e707fc5@o553633.ingest.sentry.io/5681217"))
			//{
				BuildAvaloniaApp()
			  .StartWithClassicDesktopLifetime(args);
			//}
		}

		// Avalonia configuration, don't remove; also used by visual designer.
		public static AppBuilder BuildAvaloniaApp()
			=> AppBuilder.Configure<App>()
				.UsePlatformDetect()
				.LogToTrace();
	}
}
