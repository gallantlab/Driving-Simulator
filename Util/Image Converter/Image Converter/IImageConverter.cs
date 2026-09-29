using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Image_Converter
{
	public interface IImageConverter
	{
		public enum ConverterType
		{
			None,					// no conversion
			Depth,					// 24 bit depth values
			SemanticSegmentation	// semantic segmentation
		}

		/// <summary>
		/// Does something to the image pixel values in-place
		/// </summary>
		/// <param name="image"></param>
		public abstract void ConvertImage(Bitmap image);

		public  static IImageConverter GetImageConverter(ConverterType type)
		{
			switch (type)
			{
				case ConverterType.Depth:
					return new DepthConverter();
				case ConverterType.SemanticSegmentation:
					return new SemanticSegmentationConverter();
				default:
					return new NullImageConverter();
			}
		}

		public static IImageConverter GetImageConverter(string type)
		{
			if (type == "Depth")
				return GetImageConverter(ConverterType.Depth);
			if (type == "SemanticSegmentation" || type == "Semantic Segmentation")
				return GetImageConverter(ConverterType.SemanticSegmentation);
			return GetImageConverter(ConverterType.None);
		}
	}

	class NullImageConverter : IImageConverter
	{
		public void ConvertImage(Bitmap image)
		{
			return;
		}
	}
}
