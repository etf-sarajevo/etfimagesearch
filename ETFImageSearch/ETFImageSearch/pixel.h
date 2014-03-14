#ifndef PIXEL_H
#define PIXEL_H

#include <QString>
#include <QStringList>

class Pixel
{
public:
	/**
	 * Supported color models
	 */
	enum ColorModel { RGB, YUV, HSV, HSL, YIQ, XYZ, LAB, LUV, HMMD, HY, IHLS, L1L2L3, HSI };

	Pixel(ColorModel model=Pixel::RGB);
	
	/**
	 * Convert one pixel from RGB into current color model.
	 */
	void convertColorModel(ColorModel model);
	
	/**
	 * Get model from string name.
	 */
	static ColorModel fromString(QString string);
	
	/**
	 * Get string name from model.
	 */
	static QString toString(ColorModel model);
	
	/**
	 * List all available models as strings.
	 */
	static QStringList allModels();

	/**
	 * Commonly used function, clip integer to range [0,255]
	 */
	static int toByte(int x);
	
	/**
	 * Convert pixel back to RGB color model.
	 */
	void toRgb();
	
	/**
	 * Color components
	 */
	int c[4];
	ColorModel model;
};

#endif // PIXEL_H
