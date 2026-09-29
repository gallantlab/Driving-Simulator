using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;

namespace Image_Converter
{
	class SemanticSegmentationConverter : IImageConverter
	{
		public static byte[,] Values =
		{
			{0, 0, 0}, // None         =   0u,
			{70, 70, 70}, // Buildings    =   1u,
			{190, 153, 153}, // Fences       =   2u,
			{250, 170, 160}, // Other        =   3u,
			{220, 20, 60}, // Pedestrians  =   4u,
			{153, 153, 153}, // Poles        =   5u,
			{153, 153, 153}, // RoadLines    =   6u,
			{128, 64, 128}, // Roads        =   7u,
			{244, 35, 232}, // Sidewalks    =   8u,
			{107, 142, 35}, // Vegetation   =   9u,
			{0, 0, 142}, // Vehicles     =  10u,
			{102, 102, 156}, // Walls        =  11u,
			{220, 220, 0}, // TrafficSigns =  12u,
			{80, 227, 0}, // Fields		=  13u,
			{30, 144, 255}, // Self			= 14
			{117, 86, 0}, // Ground		= 15
			{255, 183, 0}, // Target		= 16
		};

		public static readonly int MAX_VALUE_INDEX = 16;

		public void ConvertImage(Bitmap image)
		{
			unsafe
			{
				void SetPixel(byte* line, int x, byte r, byte g, byte b)
				{
					line[x] = b;
					line[x + 1] = g;
					line[x + 2] = r;
				}

				BitmapData bitmapData = image.LockBits(new Rectangle(0, 0, image.Width, image.Height),
					ImageLockMode.ReadWrite, image.PixelFormat);

				// note pixels are actually BGRA
				int bytesPerPixel = Bitmap.GetPixelFormatSize(image.PixelFormat) / 8;
				int heightInPixels = bitmapData.Height;
				int widthInBytes = bitmapData.Width * bytesPerPixel;
				byte* ptrFirstPixel = (byte*)bitmapData.Scan0;

				for (int y = 0; y < heightInPixels; y++)
				{
					byte* currentLine = ptrFirstPixel + (y * bitmapData.Stride);
					for (int x = 0; x < widthInBytes; x += bytesPerPixel)
					{
						byte r = currentLine[x + 2];
						if (r > MAX_VALUE_INDEX) return;	// probably not a semseg image
						SetPixel(currentLine, x, Values[r, 0], Values[r, 1], Values[r, 2]);
					}
				}

				image.UnlockBits(bitmapData);
			}
		}
	}
}
