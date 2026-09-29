using Sentry;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace Driving_Launcher
{
	public class GameVersionInfo
	{
		public static GameVersionInfo FromXmlDoc(XmlDocument doc)
		{
			XmlNode nameNode, versionNode, machine, buildTime, branch;
			try
			{
				nameNode = doc.GetElementsByTagName("Name")[0];
				versionNode = doc.GetElementsByTagName("Version")[0];
			}
			catch
			{
#if (DEBUG == false)
				SentrySdk.CaptureMessage("No version info detected");
#endif
				return new GameVersionInfo();
			}

			try // account for old version docs
			{
				machine = doc.GetElementsByTagName("CompilingMachine")[0];
				buildTime = doc.GetElementsByTagName("BuildDate")[0];
				branch = doc.GetElementsByTagName("GitBranch")[0];

				return new GameVersionInfo(nameNode.InnerText,
											long.Parse(versionNode.InnerText),
											machine.InnerText,
											DateTime.FromBinary(long.Parse(buildTime.InnerText)),
											branch.InnerText);
			}
			catch
			{
#if (DEBUG == false)
				SentrySdk.CaptureMessage("Pre-1.4 release detected");
#endif
				return new GameVersionInfo(nameNode.InnerText, long.Parse(versionNode.InnerText));
			}

		}


		public string? VersionName { get; set; }

		public long? Version { get; set; }

		public string? BuildMachine { get; set; }

		public DateTime? BuildTime { get; set; }

		public string? GitBranch { get; set; }

		public GameVersionInfo()
		{
			VersionName = null;
			Version = 0;
			BuildMachine = null;
			BuildTime = DateTime.MinValue;
			GitBranch = null;
		}

		public GameVersionInfo(string name, long version)
		{
			VersionName = name;
			Version = version;

			BuildMachine = null;
			BuildTime = DateTime.MinValue;
			GitBranch = null;
		}

		public GameVersionInfo(string name, long version, string buildMachine, DateTime buildTime, string gitBranch)
		{
			VersionName = name;
			Version = version;

			BuildMachine = buildMachine;
			BuildTime = buildTime;
			GitBranch = gitBranch;
		}


		public static bool operator > (GameVersionInfo lhs, GameVersionInfo rhs)
		{
			if (lhs.BuildTime.HasValue && rhs.BuildTime.HasValue)
				return lhs.BuildTime > rhs.BuildTime;
			return lhs.Version > rhs.Version;
		}

		public static bool operator <(GameVersionInfo lhs, GameVersionInfo rhs)
		{
			if (lhs.BuildTime.HasValue && rhs.BuildTime.HasValue)
				return lhs.BuildTime < rhs.BuildTime;
			return lhs.Version < rhs.Version;
		}
	}
}
