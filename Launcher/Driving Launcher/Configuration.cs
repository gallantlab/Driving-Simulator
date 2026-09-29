using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using Sentry;

namespace Driving_Launcher
{
	/// <summary>
	/// Config class
	/// </summary>
	public class Configuration
	{
		public String Resolution { get; set; } = "1280x768";
		public bool Fullscreen { get; set; } = false;
		public String? GameDirectory { get; set; } = null;
		public String? ConfigFile { get; set; } = null;
		public String? OBSPath { get; set; } = null;
		public String? Quality { get; set; } = null;

		public static String BucketName = "glab-tzhang-driving";
		public static String ServiceURL = "http://s3.us-west-1.wasabisys.com";

		public Configuration()
		{
		}

		public Configuration(string resolution, bool fullscreen, string gameDirectory, string configFile,
			string OBSPath, string quality)
		{
			Resolution = resolution;
			Fullscreen = fullscreen;
			GameDirectory = gameDirectory;
			ConfigFile = configFile;
			this.OBSPath = OBSPath;
			Quality = quality;
		}

		public bool Save(string fileName)
		{
			try
			{
				FileInfo fileInfo = new FileInfo(fileName);
				if (!Directory.Exists(fileInfo.Directory.FullName))
					Directory.CreateDirectory(fileInfo.Directory.FullName);
				XmlWriterSettings xmlSettings = new XmlWriterSettings()
				{
					Indent = true,
				};
				XmlWriter xmlWriter = XmlWriter.Create(fileName, xmlSettings);
				xmlWriter.WriteStartDocument();
				xmlWriter.WriteStartElement("Driving-Launcher");

				xmlWriter.WriteStartElement("Resolution");
				xmlWriter.WriteString(Resolution);
				xmlWriter.WriteEndElement();

				xmlWriter.WriteStartElement("Fullscreen");
				xmlWriter.WriteValue(Fullscreen);
				xmlWriter.WriteEndElement();

				if (GameDirectory != null)
				{
					xmlWriter.WriteStartElement("GameDirectory");
					xmlWriter.WriteString(GameDirectory);
					xmlWriter.WriteEndElement();
				}

				if (ConfigFile != null)
				{
					xmlWriter.WriteStartElement("ConfigFile");
					xmlWriter.WriteString(ConfigFile);
					xmlWriter.WriteEndElement();
				}
				if (OBSPath != null)
				{
					xmlWriter.WriteStartElement("OBSPath");
					xmlWriter.WriteString(OBSPath);
					xmlWriter.WriteEndElement();
				}
				if (Quality != null)
					xmlWriter.WriteElementString("Quality", Quality);

				xmlWriter.WriteEndElement();
				xmlWriter.WriteEndDocument();
				xmlWriter.Close();
			}
			catch (Exception ex)
			{
				SentrySdk.CaptureException(ex);
				return false;
			}

			return true;
		}
	}
}
