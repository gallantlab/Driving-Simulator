using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Media.Imaging;
using Image = Avalonia.Controls.Image;

namespace Image_Converter
{
	public class MainWindow : Window, INotifyPropertyChanged
	{
		public string SourceFolder { get; set; } = "";

		public IBitmap? DisplayImage
		{
			get
			{
				if (frame == null) return null;

				avaloniaImageConvertStream.SetLength(0);
				avaloniaImageConvertStream.Position = 0;
				frame.Save(avaloniaImageConvertStream, System.Drawing.Imaging.ImageFormat.Bmp);
				avaloniaImageConvertStream.Position = 0;
				return new Avalonia.Media.Imaging.Bitmap(avaloniaImageConvertStream);
			}
		}

		private System.Drawing.Bitmap? frame = null;
		private MemoryStream avaloniaImageConvertStream = new MemoryStream();

		private IImageConverter? imageConverter = null;

		private ComboBox converterTypeComboBox;
		private Slider frameSlider;

		private List<string> frameFiles = new List<string>();

		private Image imageDisplay;

		public MainWindow()
		{
			InitializeComponent();
			converterTypeComboBox = this.FindControl<ComboBox>("ConverterComboBox");
			imageDisplay = this.FindControl<Image>("ImageDisplay");
			frameSlider = this.FindControl<Slider>("FrameSlider");

			IObservable<double> sliderValue = frameSlider.GetObservable(Slider.ValueProperty);
			sliderValue.Subscribe(value =>
			{
				if (frameFiles.Count < 1) return;
				int frameIndex = (int)(value / 100 * frameFiles.Count);
				if (frameIndex >= frameFiles.Count)
					frameIndex = frameFiles.Count - 1;
				ReadAndDisplayImage(frameFiles[frameIndex]);
			});

#if DEBUG
			this.AttachDevTools();
#endif
		}

		private void InitializeComponent()
		{
			AvaloniaXamlLoader.Load(this);
		}

		private async void SelectFolderButton_OnClick(object? sender, RoutedEventArgs e)
		{
			OpenFolderDialog openFolderDialog = new OpenFolderDialog()
			{
				Title = "Choose frames directory..."
			};
			if (SourceFolder.Length > 0)
				openFolderDialog.Directory = SourceFolder;

			string selected = await openFolderDialog.ShowAsync(this);

			if (String.IsNullOrEmpty(selected))
				return;

			SourceFolder = selected;
			this.Find<TextBox>("SourceFolderTextBox").Text = SourceFolder;
			this.Find<MenuItem>("SaveConvertedMenuItem").IsEnabled = true;
			EnumerateFrames();
			if (imageConverter == null)
				ConverterComboBox_OnSelectionChanged(null, null);

			ReadAndDisplayImage(frameFiles[0]);
		}

		private void EnumerateFrames()
		{
			string[] files = Directory.GetFiles(SourceFolder, "*.png");
			frameFiles.Clear();
			foreach (string file in files)
				frameFiles.Add(file);
			frameFiles.Sort();
		}

		private void ReadAndDisplayImage(string file)
		{
			frame = new System.Drawing.Bitmap(file);
			imageConverter.ConvertImage(frame);
			imageDisplay.Source = DisplayImage;
		}

		private void ConverterComboBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
		{
			if (converterTypeComboBox != null)
			{
				imageConverter =
					IImageConverter.GetImageConverter(((ComboBoxItem) converterTypeComboBox.SelectedItem).Content
						.ToString());
				if (frameFiles.Count < 1) return;
				int frameIndex = (int)(frameSlider.Value / 100 * frameFiles.Count);
				if (frameIndex >= frameFiles.Count)
					frameIndex = frameFiles.Count - 1;
				ReadAndDisplayImage(frameFiles[frameIndex]);
			}
		}

		private async void SaveConvertedMenuItem_OnClick(object? sender, RoutedEventArgs e)
		{
			OpenFolderDialog openFolderDialog = new OpenFolderDialog()
			{
				Title = "Choose output directory..."
			};
			string selected = await openFolderDialog.ShowAsync(this);

			if (String.IsNullOrEmpty(selected))
				return;

			Parallel.For(0, frameFiles.Count, (i) =>
			{
				System.Drawing.Bitmap image = new System.Drawing.Bitmap(frameFiles[i]);
				imageConverter.ConvertImage(image);
				image.Save(Path.Join(selected, Path.GetFileName(frameFiles[i])));
			});
		}
	}
}
