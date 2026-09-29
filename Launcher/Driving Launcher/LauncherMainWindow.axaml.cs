using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Styling;
using Sentry;
using Microsoft.Extensions.Configuration;
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Threading.Tasks;
using Amazon.Runtime;
using Amazon.S3;
using Amazon.S3.Model;
using Avalonia.Interactivity;
using System.Runtime.InteropServices;
using System.Xml;
using System.Net.Http;
using System.Net.WebSockets;
using System.Reactive.Subjects;
using System.Threading;
using System.Text.RegularExpressions;
using Avalonia.Media;

// Note: in linux, Path.GetDirectory(assemblyfile) does not return the directory of the exe
// but rather some temp directory that contains the runtime - use Directory.GetCurrentDirectory() instead.

// Note: in linux, Path.GetDirectory(assemblyfile) does not return the directory of the exe
// but rather some temp directory that contains the runtime - use Directory.GetCurrentDirectory() instead.

namespace Driving_Launcher
{
	public class MainWindow : Window, IStyleable
	{
		/// <summary>
		/// Checks whether local OBS service is up AND has websockets running
		/// </summary>
		/// <returns></returns>
		public static bool IsOBSRunning()
		{
			try
			{
				ClientWebSocket wsClient = new ClientWebSocket();
				Task connectTask =
					wsClient.ConnectAsync(new Uri("ws://localhost:4444"), new CancellationToken());
				connectTask.Wait();
				return true;
			}
			catch
			{
				return false;
			}
		}

		/// <summary>
		/// Non-blocking OBS check with optional wait
		/// </summary>
		/// <param name="sleepMilliseconds">time to wait for before checking</param>
		/// <returns></returns>
		public static async Task<bool> IsOBSRunningAsync(int sleepMilliseconds)
		{
			return await Task.Run(() =>
			{
				// Check OBS server is running
				Thread.Sleep(sleepMilliseconds);
				return IsOBSRunning();
			});
		}

		// == logic stuff ==
		private Configuration config;
		private IAmazonS3 s3DownloadClient;
		private string ConfigFileName = null;
		OSPlatform? OS = null;

		// Reactive UI for updating from tasks
		public Subject<double> DownloadProgress { get; private set; } = new Subject<double>();
		public Subject<string> StatusText { get; private set; } = new Subject<string>();
		public Subject<bool> isPlayButtonEnabled { get; private set; } = new Subject<bool>();

		string OSString
		{
			get
			{
				if (OS.HasValue)
				{
					if (OS == OSPlatform.Windows) return "Windows";
					if (OS == OSPlatform.Linux) return "Linux";
					return "Unsupported Platform";
				}

				return "Unsupported Platform";
			}
		}
		GameVersionInfo localVersion = null;
		GameVersionInfo cloudVersion = null;

		string GameExecutable
		{
			get
			{
				if (config.GameDirectory == null) return null;
				string exe = "CarlaUE4." + ((OS == OSPlatform.Windows) ? "exe" : "sh");
				return Path.Join(config.GameDirectory, OSString + "NoEditor", exe);
			}
		}

		string GameConfigFolder
		{
			get
			{
				if (config.GameDirectory == null) return null;
				try
				{
					DirectoryInfo configFolder = new DirectoryInfo(Path.Join(new string[]
						{config.GameDirectory, OSString + "NoEditor", "CarlaUE4", "Config"}));
					if (!Directory.Exists(configFolder.FullName))
						Directory.CreateDirectory(configFolder.FullName);
					DirectoryInfo[] subdirs = configFolder.GetDirectories();
					if (subdirs.Length < 1)
					{
						Directory.CreateDirectory(Path.Join(new string[]
						{config.GameDirectory, OSString + "NoEditor", "CarlaUE4", "Config", "Experiment Configs"}));
						subdirs = configFolder.GetDirectories();
					}

					return subdirs[0].FullName;
				}
				catch (DirectoryNotFoundException)
				{
					SetStatusMessage("Config directory not found. Game may not be installed.", Colors.OrangeRed);
					playButton.IsEnabled = false;
					return null;
				}
			}
		}

		private string launcherFileLocation;

		// === UI Elements ===
		// note: at some point the UI should be overhauled to MVVM

		// == Status bar ==
		private Panel statusBarPanel;
		private TextBlock statusBarTextBlock;

		// == Game tab ==
		private ComboBox configFileComboBox;
		private ToggleSwitch fullscreenSwitch;
		private ComboBox resolutionComboBox;
		private ComboBox graphicsComboBox;
		private Button playButton;

		private Button startOBSButton;
		private ToggleSwitch renderAllSwitch;
		private StackPanel renderAllStackPanel;
		private TextBox renderAllFPSTextBox;

		// == updates tab ==
		private TextBlock gameStatusTextBlock;
		private TextBlock gameVersionTextBlock;
		private TextBox gameDirectoryTextBox;
		private ProgressBar downloadProgressBar;
		private Button downloadButton;
		private TextBlock updatesTabTextBlock;
		private TextBox OBSPathTextBox;

		// == about tab ==
		private TextBlock versionTextBlock;
		private Button checkForUpdatesButton;

		public MainWindow()
		{
			InitializeComponent();
#if DEBUG
			this.AttachDevTools();
#endif
			//ExtendClientAreaToDecorationsHint = true;
			ExtendClientAreaTitleBarHeightHint = -1;

			TransparencyLevelHint = WindowTransparencyLevel.AcrylicBlur;

		}

		private void InitializeComponent()
		{
			AvaloniaXamlLoader.Load(this);

			// because in a dev environment we catch errors with the VS
#if (DEBUG == false)
			SentrySdk.Init("https://5ec6472093b64222b67391088e707fc5@o553633.ingest.sentry.io/5681217");
			SentrySdk.ConfigureScope(scope =>
			{
				scope.Contexts["Machine"] = new 
				{
					Name = Environment.MachineName
				};
			});
#endif

			// set static values
			TextBlock OSInfoTextBlock = this.FindControl<TextBlock>("OSInfoTextBlock");
			OperatingSystem os = Environment.OSVersion;
			if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
			{
				OS = OSPlatform.Windows;
				OSInfoTextBlock.Text = "Running on Windows";
			}

			else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
			{
				OS = OSPlatform.Linux;
				OSInfoTextBlock.Text = "Running on Linux";
			}

			else
				OSInfoTextBlock.Text = "Unsupported Platform";

			statusBarPanel = this.FindControl<Panel>("StatusBarPanel");
			statusBarTextBlock = this.FindControl<TextBlock>("StatusBarTextBlock");
			statusBarTextBlock.DataContext = this;
			statusBarTextBlock.Bind(TextBlock.TextProperty, StatusText);

			configFileComboBox = this.FindControl<ComboBox>("ConfigFileComboBox");
			fullscreenSwitch = this.FindControl<ToggleSwitch>("FullscreenSwitch");
			resolutionComboBox = this.FindControl<ComboBox>("ResolutionComboBox");
			graphicsComboBox = this.FindControl<ComboBox>("GraphicsComboBox");
			playButton = this.FindControl<Button>("PlayButton");
			playButton.DataContext = this;
			playButton.Bind(Button.IsEnabledProperty, isPlayButtonEnabled);
			startOBSButton = this.FindControl<Button>("StartOBSButton");
			renderAllStackPanel = this.FindControl<StackPanel>("RenderAllStackPanel");
			renderAllSwitch = this.FindControl<ToggleSwitch>("RenderAllSwitch");
			renderAllFPSTextBox = this.FindControl<TextBox>("RenderAllFPSTextBox");

			gameStatusTextBlock = this.FindControl<TextBlock>("GameStatusTextBlock");
			gameVersionTextBlock = this.FindControl<TextBlock>("GameVersionTextBlock");
			gameDirectoryTextBox = this.FindControl<TextBox>("GameDirectoryTextBox");
			downloadProgressBar = this.FindControl<ProgressBar>("DownloadProgressBar");
			downloadProgressBar.DataContext = this;
			downloadProgressBar.Bind(ProgressBar.ValueProperty, DownloadProgress);
			updatesTabTextBlock = this.FindControl<TextBlock>("UpdatesTabTextBlock");
			downloadButton = this.FindControl<Button>("DownloadButton");
			OBSPathTextBox = this.FindControl<TextBox>("OBSPathTextBox");

			versionTextBlock = this.FindControl<TextBlock>("VersionTextBlock");
			checkForUpdatesButton = this.FindControl<Button>("CheckForUpdatesButton");

			// get config for launcher
			ConfigFileName = Path.Join(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
										"Driving", "Launcher Config.xml");

			if (File.Exists(ConfigFileName))
			{
				try
				{
					ConfigurationBuilder configBuilder = new ConfigurationBuilder();
					configBuilder.AddXmlFile(ConfigFileName);
					IConfiguration savedConfig = configBuilder.Build();
					config = savedConfig.Get<Configuration>();

					if (config.Quality != null)
					{
						foreach (object o in graphicsComboBox.Items)
						{
							if (o is ComboBoxItem item)
							{
								if (item.Content.ToString() == config.Quality)
								{
									graphicsComboBox.SelectedItem = o;
									break;
								}
							}
						}
					}

					fullscreenSwitch.IsChecked = config.Fullscreen;

					if (config.Resolution != null)
					{
						foreach (object o in resolutionComboBox.Items)
						{
							if (o is ComboBoxItem item)
							{
								if (item.Content.ToString() == config.Resolution)
								{
									resolutionComboBox.SelectedItem = o;
									break;
								}
							}
						}
					}

					// == local files tab ==
					if (config.OBSPath != null)
					{
						OBSPathTextBox.Text = config.OBSPath;
						startOBSButton.IsEnabled = config.OBSPath.Length > 0;
					}
					else startOBSButton.IsEnabled = false;

					if (config.GameDirectory != null)
					{
						gameDirectoryTextBox.Text = config.GameDirectory;
						if (config.GameDirectory.Length > 0)
						{
							downloadButton.IsEnabled = true;
							playButton.IsEnabled = true;
						}
					}
				}
				catch
				{
					config = new Configuration();
					config.Save(ConfigFileName);
				}
			}
			else
			{
				config = new Configuration();
				config.Save(ConfigFileName);
			}

			// enumerate before binding because otherwise stuff are bound to default values
			EnumerateGameConfigFiles();

			// bind config to UI
			IObservable<object?> selectedConfigFile = configFileComboBox.GetObservable(ComboBox.SelectedItemProperty);
			selectedConfigFile.Subscribe(value =>
			{
				if (value is ComboBoxItem item)
					config.ConfigFile = item.Content.ToString();
				else if (value is string configString)
					config.ConfigFile = configString;
			});

			IObservable<object?> selectedResolution = resolutionComboBox.GetObservable(ComboBox.SelectedItemProperty);
			selectedResolution.Subscribe(value =>
			{
				if (value is ComboBoxItem item)
					config.Resolution = item.Content.ToString();
				else if (value is string resolutionString)
					config.Resolution = resolutionString;
			});

			IObservable<object?> gameDirectory = gameDirectoryTextBox.GetObservable(TextBox.TextProperty);
			gameDirectory.Subscribe(value =>
			{
				if (value is string directory)
					if (directory.Length > 0)
					{
						config.GameDirectory = directory;
						downloadButton.IsEnabled = true;
					}
					else { downloadButton.IsEnabled = false; }
			});

			IObservable<object?> OBSPath = this.FindControl<TextBox>("OBSPathTextBox").GetObservable(TextBox.TextProperty);
			OBSPath.Subscribe(value =>
			{
				if (value is string path)
					if (path.Length > 0)
					{
						config.OBSPath = path;
						startOBSButton.IsEnabled = true;
					}
					else
					{
						startOBSButton.IsEnabled = false;
					}
			});

			IObservable<object?> selectedQuality = graphicsComboBox.GetObservable(ComboBox.SelectedItemProperty);
			selectedQuality.Subscribe(value =>
			{
				if (value is ComboBoxItem item)
					config.Quality = item.Content.ToString();
				else if (value is string qualityString)
					config.Quality = qualityString;
			});

			// initialize download client
			s3DownloadClient = new AmazonS3Client(null, null,
				new AmazonS3Config()
				{
					ServiceURL = Configuration.ServiceURL,
					Timeout = TimeSpan.FromSeconds(10)
				});

			// Fill out version info
			Assembly asm = Assembly.GetExecutingAssembly();
			launcherFileLocation = asm.Location;
			AssemblyInformationalVersionAttribute asmVersion = asm.GetCustomAttribute<AssemblyInformationalVersionAttribute>();
			versionTextBlock.Text = "Version " + asmVersion.InformationalVersion.Split('+')[0];

			SetStatusMessage();

			CheckForUpdates();

			Task.Run(async () =>
			{
				if (await IsOBSRunningAsync(500))
					startOBSButton.Content = "✅ OBS Studio";
			});
		}

		// === UI things because ain't nobody got the time for bindings ===

		private void LoadLocalVersion()
		{
			try
			{
				XmlDocument versionDoc = new XmlDocument();
				versionDoc.Load(Path.Join(config.GameDirectory, "version.xml"));

				localVersion = GameVersionInfo.FromXmlDoc(versionDoc);
			}
			catch
			{
				localVersion = new GameVersionInfo();
			}

			gameVersionTextBlock.Text = "Local version is " + localVersion.VersionName;
		}

		/// <summary>
		/// Checks against the object store in the cloud to see if there is a new version
		/// </summary>
		/// <returns></returns>
		private async void CheckForUpdates()
		{
			LoadLocalVersion();
			if (CheckInternetConnection())
			{
				// == Check Game Version ==
				if (await CheckObjectExists(OSString + " version.xml"))
				{
					GetObjectResponse download = await Download(OSString + " version.xml");
					Stream versionStream = download.ResponseStream;

					XmlDocument cloudVersionDoc = new XmlDocument();
					cloudVersionDoc.Load(versionStream);

					cloudVersion = GameVersionInfo.FromXmlDoc(cloudVersionDoc);
				}
				else cloudVersion = new GameVersionInfo();

				gameStatusTextBlock.Text = "Cloud version is " + cloudVersion.VersionName;

				bool gameUpdate = false;

				if (cloudVersion > localVersion)
				{
					gameStatusTextBlock.Text = "Update available to " + cloudVersion.VersionName;
					downloadButton.Content = "Update game";
					gameUpdate = true;
				}
				else
				{
					gameStatusTextBlock.Text = "Game files up-to-date";
					downloadButton.Content = "Download game";
					updatesTabTextBlock.Text = "Local files";
				}

				if (gameUpdate)
				{
					updatesTabTextBlock.Text = "Local files ⚠️";
					SetStatusMessage("Game update available", Colors.Goldenrod);
				}
			}

			checkForUpdatesButton.IsEnabled = true;
		}


		private void SetStatusMessage(string? text = null, Color? color = null)
		{
			if (text == null)
			{
				statusBarTextBlock.IsVisible = false;
				statusBarPanel.IsVisible = false;
				playButton.Margin = this.FindControl<Button>("ExitButton").Margin = new Thickness(12, 24);
			}
			else
			{
				StatusText.OnNext(text);
				color ??= Colors.DodgerBlue;
				statusBarPanel.Background = new SolidColorBrush(color.Value);
				statusBarTextBlock.Foreground =
					new SolidColorBrush((color.Value == Colors.YellowGreen) || (color.Value == Colors.Gold) ? Colors.Black : Colors.White);
				statusBarTextBlock.IsVisible = true;
				statusBarPanel.IsVisible = true;
				playButton.Margin = this.FindControl<Button>("ExitButton").Margin = new Thickness(12, 36);
			}
		}

		private bool CheckInternetConnection()
		{
			try
			{
				new WebClient().OpenRead(new Uri("http://google.com/generate_204"));
				return true;
			}
			catch
			{
				SetStatusMessage("No internet connection; restart to re-check.", Colors.IndianRed);
				downloadButton.IsEnabled = false;
				return false;
			}
		}

		private void EnumerateGameConfigFiles()
		{
			if (GameConfigFolder != null)
			{
				if (!Directory.Exists(GameConfigFolder))
					Directory.CreateDirectory(GameConfigFolder);
				string[] configs = Directory.GetFiles(GameConfigFolder);
				// TODO: convert to comboboxitems for consistency
				List<string> comboBoxItems = new List<string>();
				foreach (string config in configs)
					comboBoxItems.Add(Path.GetFileNameWithoutExtension(config));
				comboBoxItems.Sort();
				comboBoxItems.Add("None");
				configFileComboBox.Items = comboBoxItems;

				// select previously selected config if available
				if ((config != null) && (config.ConfigFile != null))
				{
					string configName = null;
					foreach (object o in configFileComboBox.Items)
					{
						if (o is ComboBoxItem item)
							configName = item.Content.ToString();
						else if (o is string stringItem)
							configName = stringItem;

						if (configName == config.ConfigFile)
						{
							configFileComboBox.SelectedItem = o;
							break;
						}
					}
				}
				else
					configFileComboBox.SelectedItem = comboBoxItems[0];
			}
		}
		private async Task<MetadataCollection> GetMetadata(string key)
		{
			GetObjectMetadataRequest getRequest = new GetObjectMetadataRequest()
			{
				BucketName = Configuration.BucketName,
				Key = key,
			};
			GetObjectMetadataResponse getResponse = await s3DownloadClient.GetObjectMetadataAsync(getRequest);
			return getResponse.Metadata;
		}
		private async Task<bool> CheckObjectExists(string key)
		{
			GetObjectMetadataRequest getRequest = new GetObjectMetadataRequest()
			{
				BucketName = Configuration.BucketName,
				Key = key,
			};
			try
			{
				GetObjectMetadataResponse getResponse = await s3DownloadClient.GetObjectMetadataAsync(getRequest);
				return true;
			}
			catch
			{
				return false;
			}
		}

		private async Task<GetObjectResponse> Download(string key)
		{

			GetObjectRequest getRequest = new GetObjectRequest()
			{
				BucketName = Configuration.BucketName,
				Key = key,
			};
			GetObjectResponse getResponse = await s3DownloadClient.GetObjectAsync(getRequest);
			return getResponse;
		}

		private async Task Download(string key, string localFileName)
		{
			GetObjectResponse getResponse = await Download(key);
			getResponse.WriteObjectProgressEvent += (object? sender, WriteObjectProgressArgs eventArgs) =>
			{
				StatusText.OnNext("Downloaded " + FileSize.BytesToReadable(eventArgs.TransferredBytes) + " of " +
								  FileSize.BytesToReadable(eventArgs.TotalBytes));
				DownloadProgress.OnNext(eventArgs.PercentDone);
			};
			await getResponse.WriteResponseStreamToFileAsync(localFileName, false, new System.Threading.CancellationToken());
		}

		// === Event Handlers ===
		private void Window_OnClosing(object? sender, CancelEventArgs e)
		{
			config.Save(ConfigFileName);
		}

		private async void DownloadButton_OnClick(object? sender, RoutedEventArgs e)
		{
			string localPath = Path.Join(config.GameDirectory, OSString + ".zip");
			gameStatusTextBlock.Text = "Updating to " + cloudVersion.VersionName;
			downloadButton.IsEnabled = false;
			playButton.IsEnabled = false;
			SetStatusMessage("Downloading");

			await Download(OSString + ".zip", localPath);

			downloadProgressBar.IsIndeterminate = true;

			SetStatusMessage("Extracting files...");
			await Task.Run(() => ZipFile.ExtractToDirectory(localPath, config.GameDirectory, true));

			File.Delete(localPath);

			await Download(OSString + " version.xml", Path.Join(config.GameDirectory, "version.xml"));

			downloadProgressBar.IsIndeterminate = false;
			DownloadProgress.OnNext(0);
			downloadButton.IsEnabled = true;
			playButton.IsEnabled = true;

			if (OS == OSPlatform.Linux)
			{
				ProcessStartInfo addExePermissions = new ProcessStartInfo()
				{
					FileName = "chmod",
					UseShellExecute = true,
				};
				addExePermissions.ArgumentList.Add("+x");
				addExePermissions.ArgumentList.Add(GameExecutable);

				Process chmod = Process.Start(addExePermissions);
			}

			SetStatusMessage("Version " + cloudVersion.VersionName + " finished downloading", Colors.YellowGreen);

			CheckForUpdates();
		}

		private async void PlayButton_OnClick(object? sender, RoutedEventArgs e)
		{
			ProcessStartInfo startInfo = new ProcessStartInfo()
			{
				FileName = GameExecutable
			};

			// note there is this weird try-catch pattern with reading strings form the comboboxes
			// because the avalonia combo box items can change type. The avalonia combobox items
			// can be any iterable, and the objects in the list are not converted to ComboBoxItems

			string resolutionText;
			try
			{
				resolutionText = (string)resolutionComboBox.SelectedItem;
			}
			catch (InvalidCastException)
			{
				resolutionText = ((ComboBoxItem)resolutionComboBox.SelectedItem).Content.ToString();
			}

			if (!fullscreenSwitch.IsChecked.Value) // only set resolution if not fullscreen
			{
				startInfo.Arguments += " -windowed";
				string[] tokens = resolutionText.Split("x");
				int x = int.Parse(tokens[0]);
				int y = int.Parse(tokens[1]);
				startInfo.Arguments += string.Format(" -ResX={0} -ResY={1}", x, y);
			}

			startInfo.Arguments += string.Format(" -quality={0}",
				((string)((ComboBoxItem)graphicsComboBox.SelectedItem).Content) == "Discrete" ? "Epic" : "Low");
			if (localVersion.BuildMachine != null)
			{
				startInfo.Arguments += " -compilingMachine=" + localVersion.BuildMachine;
				startInfo.Arguments += " -branch=" + localVersion.GitBranch;
				startInfo.Arguments += " -version=\"" + localVersion.VersionName + "\"";
				startInfo.Arguments += string.Format(" -date=\"{0}\"", localVersion.BuildTime.Value.ToLongDateString());
			}

			string configText;
			try
			{
				configText = (string)configFileComboBox.SelectedItem;
			}
			catch (InvalidCastException)
			{
				configText = ((ComboBoxItem)configFileComboBox.SelectedItem).Content.ToString();
			}

			if (configText != "None")
				startInfo.Arguments += string.Format(" -carla-settings={0}.ini", configText);

			SetStatusMessage();
			// auto rendering mode
			await Task.Run(() =>
			{
				isPlayButtonEnabled.OnNext(false);
				if (renderAllSwitch.IsChecked.Value)
				{
					startInfo.Arguments += String.Format(" -RenderAll={0}", renderAllFPSTextBox.Text);
					while (IsAnyDemosNotRendered())
						LaunchGame(startInfo);
				}
				// regular mode
				else
				{
					// check if OBS is running and attempt to start it if not
					try
					{
						if (!IsOBSRunning()) StartOBS();
					}
					catch { /* catch all OBS errors because this isn't a guaranteed action */ }
					finally
					{
						LaunchGame(startInfo);
					}
				}

				isPlayButtonEnabled.OnNext(true);
			});

			if (renderAllSwitch.IsChecked.Value)
				SetStatusMessage("All demos rendered", Colors.YellowGreen);

			EnumerateGameConfigFiles();
		}

		/// <summary>
		/// Called by the playbutton_onlick method to actually launch the game
		/// Separated out because this may be called multiple times if we're auto rendering
		/// </summary>
		/// <param name="startInfo"></param>
		private void LaunchGame(ProcessStartInfo startInfo)
		{
			try
			{
				Process game = Process.Start(startInfo);
				game.WaitForExit();
			}
			catch (Win32Exception ex)   // case linux file permission error and core dumping
			{
				SentrySdk.CaptureException(ex);

				ProcessStartInfo addExePermissions = new ProcessStartInfo()
				{
					FileName = "chmod",
					UseShellExecute = true,
				};
				addExePermissions.ArgumentList.Add("+x");
				addExePermissions.ArgumentList.Add(GameExecutable);

				Process chmod = Process.Start(addExePermissions);
				chmod.WaitForExit();

				Process game = Process.Start(startInfo);
				game.WaitForExit();
			}
		}

		private void ExitButton_OnClick(object? sender, RoutedEventArgs e)
		{
			Close();
		}

		private async void StartOBSButton_OnClick(object? sender, RoutedEventArgs e)
		{
			try
			{
				StartOBS();
				if (await IsOBSRunningAsync(500))
				{
					startOBSButton.Content = "✅ OBS Studio";
				}
				else throw new Exception("OBS start failed");
			}
			catch (Exception ex)
			{
				SetStatusMessage("Could not start OBS", Colors.OrangeRed);
				return;
			}

		}

		private void StartOBS()
		{
			ProcessStartInfo OBSInfo = new ProcessStartInfo()
			{
				FileName = config.OBSPath,
				UseShellExecute = true,
			};

			Process.Start(OBSInfo);
		}

		private async void ChooseGameDirectoryButton_OnClick(object? sender, RoutedEventArgs e)
		{
			OpenFolderDialog openFolderDialog = new OpenFolderDialog()
			{
				Title = "Choose game storage directory..."
			};
			if (config.GameDirectory != null)
				openFolderDialog.Directory = config.GameDirectory;

			string selected = await openFolderDialog.ShowAsync(this);

			if (String.IsNullOrEmpty(selected))
				return;
			gameDirectoryTextBox.Text = selected;
		}

		private async void ChooseOBSButton_OnClick(object? sender, RoutedEventArgs e)
		{
			OpenFileDialog openFileDialog = new OpenFileDialog()
			{
				Title = "Choose OBS exe...",
				AllowMultiple = false
			};
			if (config.OBSPath != null)
				openFileDialog.InitialFileName = config.OBSPath;

			string[] selected = await openFileDialog.ShowAsync(this);

			if (selected == null || selected.Length == 0)
				return;
			OBSPathTextBox.Text = selected[0];
		}

		// hidden switch that can only be activated if you know how!
		private void StatusBarPanel_OnDoubleTapped(object? sender, RoutedEventArgs e)
		{
			renderAllStackPanel.IsVisible = !renderAllStackPanel.IsVisible;
		}

		/// <summary>
		/// Lists all demo files, and lists all xml files. The existence of an xml file for the demo
		/// indicates that the demo has been rendered.
		/// </summary>
		/// <returns>whether all demos have a xml</returns>
		private bool IsAnyDemosNotRendered()
		{
			// because we don't know whether it's shipping or dev build, just check both possible locations
			string[] saveFolders = new string[]
			{
				Path.Join(config.GameDirectory, OSString + "NoEditor", "CarlaUE4", "Saved"),									// dev save folder
				Path.Join(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "CarlaUE4", "Saved"),		// windows shipping save folder
				Path.Join(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Epic", "CarlaUE4", "Saved")	// linux shipping save folder that does not make sense
			};
			foreach (string saveFolder in saveFolders)
			{
				string demoFolder = Path.Join(saveFolder, "Demos");
				if (Directory.Exists(demoFolder)) // demos actually exist
				{
					string[] demos = Directory.GetDirectories(demoFolder);
					string[] logFiles = Directory.GetFiles(saveFolder, "*.xml");

					// convert the array that includes full paths to just file names
					List<string> logFileNames = logFiles.Select(logFile => Path.GetFileNameWithoutExtension(logFile)).ToList();
					List<string> demoNames = demos.Select(demo => Path.GetFileNameWithoutExtension(demo)).ToList();
					if (demoNames.Any(demo => !logFileNames.Contains(demo + "-positions")))
					{
						return true;
					}
				}
			}

			return false;
		}

		private void OpenConfigFileButton_OnClick(object? sender, RoutedEventArgs e)
		{
			if (GameConfigFolder != null)
			{
				ProcessStartInfo fileInfo = new ProcessStartInfo()
				{
					FileName = (OS == OSPlatform.Windows) ? "explorer.exe" : "xdg-open",
					UseShellExecute = true,
				};
				if (OS == OSPlatform.Windows)
				{
					fileInfo.FileName = Path.Join(GameConfigFolder, String.Format("{0}.ini", (string)configFileComboBox.SelectedItem));
				}
				else
				{
					fileInfo.FileName = "xdg-open";
					fileInfo.ArgumentList.Add(Path.Join(GameConfigFolder, String.Format("{0}.ini", (string)configFileComboBox.SelectedItem)));
				}

				Process.Start(fileInfo);
			}
			else SetStatusMessage("Game config folder not found", Colors.OrangeRed);
		}

		private void OpenConfigFolderButton_OnClick(object? sender, RoutedEventArgs e)
		{
			if (GameConfigFolder != null)
			{
				ProcessStartInfo folderInfo = new ProcessStartInfo()
				{
					FileName = (OS == OSPlatform.Windows) ? "explorer.exe" : "xdg-open",
					UseShellExecute = true,
				};

				folderInfo.ArgumentList.Add(GameConfigFolder);

				Process.Start(folderInfo);
			}
			else SetStatusMessage("Game config folder not found", Colors.OrangeRed);
		}

		private void CheckForUpdatesButton_OnClick(object? sender, RoutedEventArgs e)
		{
			CheckForUpdates();
			checkForUpdatesButton.IsEnabled = false;
		}
	}
}
