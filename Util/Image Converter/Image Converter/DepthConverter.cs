using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace Image_Converter
{
	class DepthConverter : IImageConverter
	{
		public void ConvertImage(Bitmap image)
		{
			unsafe
			{
				BitmapData bitmapData = image.LockBits(new Rectangle(0, 0, image.Width, image.Height),
					ImageLockMode.ReadWrite, image.PixelFormat);

				int offset = (image.PixelFormat == PixelFormat.Format32bppArgb) ? 1 : 0;

				int bytesPerPixel = Bitmap.GetPixelFormatSize(image.PixelFormat) / 8;
				int heightInPixels = bitmapData.Height;
				int widthInBytes = bitmapData.Width * bytesPerPixel;
				byte* ptrFirstPixel = (byte*)bitmapData.Scan0;

				for (int y = 0; y < heightInPixels; y++)
				{
					byte* currentLine = ptrFirstPixel + (y * bitmapData.Stride);
					for (int x = 0; x < widthInBytes; x += bytesPerPixel)
					{
						double b = currentLine[x];
						double g = currentLine[x + 1];
						double r = currentLine[x + 2];
						double depth = 1 + Math.Log((r + g * 256.0 + b * 256 * 256.0) / (256 * 256 * 256 - 1)) / 5.70378;
						if (depth < 0) depth = 0;
						if (depth > 1) depth = 1;

						// calculate new pixel value
						currentLine[x] = 
						currentLine[x + 1] = 
						currentLine[x + 2] = (byte) (255 * depth);
					}
				}
				image.UnlockBits(bitmapData);
            }
		}
	}
}