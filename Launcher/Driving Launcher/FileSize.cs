using System;
using System.Collections.Generic;
using System.Text;

namespace Driving_Launcher
{
	class FileSize
	{
		public static string[] units = new string[] {"B", "KB", "MB", "GB"};

		public static string BytesToReadable(long bytes)
		{
			int unitIndex = 0;
			double value = (double)bytes;
			while ((value > 1024 * 0.95) && unitIndex < units.Length - 1)
			{
				value /= 1024.0;
				unitIndex++;
			}

			return string.Format("{0:#.##} {1}", value, units[unitIndex]);
		}

		public long bytes = 0;

		public double KB
		{
			get => (double)bytes / 1024.0;
			set { bytes = (long) (value * 1024); }
		}

		public double MB
		{
			get => KB / 1024.0;
			set { KB = value * 1024; }
		}

		public double GB
		{
			get => MB / 1024.0;
			set { MB = value * 1024; }
		}

		public FileSize()
		{
			bytes = 0;
		}

		public FileSize(long bytes)
		{
			this.bytes = bytes;
		}

		public override string ToString()
		{
			if (GB > 0.95) return string.Format("{0:#.##} GB", GB);
			if (MB > 0.95) return string.Format("{0:#.##} MB", MB);
			if (KB > 0.95) return string.Format("{0:#.##} KB", KB);
			return string.Format("{0} bytes", bytes);
		}

		public static FileSize operator +(FileSize lhs, long bytes)
		{
			return new FileSize(lhs.bytes + bytes);
		}
	}
}
