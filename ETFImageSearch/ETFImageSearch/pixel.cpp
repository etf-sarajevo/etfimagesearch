#include "pixel.h"

#include <QColor>
#include <cmath>

#define NR_MODELS 12
static const char names[][NR_MODELS] = { "RGB", "YUV", "HSV", "HSL", "YIQ", "XYZ", "LAB", "LUV", "HMMD", "HY", "IHLS", "L1L2L3"};


Pixel::Pixel(ColorModel model) : model(model)
{
}



Pixel::ColorModel Pixel::fromString(QString string)
{
	for (int i(0); i<NR_MODELS; i++)
		if (string == names[i])
			return ColorModel(i);
	throw QString("Unknown color model %1").arg(string);
}

QString Pixel::toString(ColorModel model)
{
	return QString(names[int(model)]);
}

QStringList Pixel::allModels()
{
	QStringList result;
	for (int i(0); i<NR_MODELS; i++)
		result.append(QString(names[i]));
	return result;
}


int Pixel::toByte(int x) 
{
	if (x<0) x=0;
	if (x>255) x=255;
	return x;
}


void Pixel::convertColorModel(ColorModel model) 
{
	// When converting between models other then RGB, first convert to RGB then to other model
	if (this->model != RGB)
		toRgb();
	
	if (model == RGB) return; // Nothing to do
	
	int R = c[0], G = c[1], B = c[2];
	double X,Y,Z; // used in several conversions
	
	if (model == YUV) {
		// YUV formula
		/*double Y, U, V;
		Y = 0.299*R + 0.587*G + 0.114*B;
		U = 0.492*(B-Y);
		V = 0.877*(R-Y);
		
		// Ranges are now Ye[0,1], Ue[-0.436,0.436], Ve[-0.615,0.615]
		
		// Convert to byte:
		Y = toByte(Y*256);
		U = toByte((U+0.436)*256 / (0.436*2));
		V = toByte((V+0.615)*256 / (0.615*2));*/
		
		// The following is equivalent and comes from JPEG standard
		int Y = toByte ( 0.299 * R + 0.587 * G + 0.114 * B );
		int U = toByte ( 128 - 0.168736 * R - 0.331264 * G + 0.5 * B );
		int V = toByte ( 128 + 0.5 * R - 0.418688 * G - 0.081312 * B );
		
		c[0] = Y; c[1] = U; c[2] = V;
		return;
	}
	
	if (model == YIQ) {
		int Y = toByte ( 0.299 * R + 0.587 * G + 0.114 * B );
		int I = toByte ( (152 + 0.596 * R - 0.275 * G - 0.321 * B) / 304 * 255 );
		int Q = toByte ( (133 + 0.212 * R - 0.523 * G + 0.311 * B) / 266 * 255 );
		
		c[0] = Y; c[1] = I; c[2] = Q;
		
		return;
	}
	
	if (model == HSV) {
		int H, S, V;
		
		QColor color(R, G, B);
		color.getHsv(&H, &S, &V);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		c[0] = H; c[1] = S; c[2] = V;
		
		return;
	}
	
	if (model == HSL) {
		int H, S, L;
		
		QColor color(R, G, B);
		color.getHsl(&H, &S, &L);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		c[0] = H; c[1] = S; c[2] = L;
		
		return;
	}
	
	if (model == HSI) {
		int H, S, I;
		
		QColor color(R, G, B);
		color.getHsl(&H, &S, &I);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		I = (R+G+B)/765; // 765 = 3*255
		
		c[0] = H; c[1] = S; c[2] = I;
		
		return;
	}
	
	if (model == IHLS) {
		// More perceptually uniform variant of HSL color space
		// See: Hanbury and Serra "A 3D-polar coordinate colour representation suitable for image analysis" (2002)
		int H, S, L;
		
		QColor color(R, G, B);
		color.getHsl(&H, &S, &L);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		int M=qMax(R,qMax(G,B));
		int m=qMin(R,qMin(G,B));
		S = M-m;
		
		c[0] = H; c[1] = S; c[2] = L;
		
		return;
	}
	
	if (model == HMMD) { 
		// Color model used in MPEG-7 standard
		// See: Manjunath et al. "Color and texture descriptors" (2001)
		int H, S, V;
		
		QColor color(R, G, B);
		color.getHsv(&H, &S, &V);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		int M=qMax(R,qMax(G,B));
		int m=qMin(R,qMin(G,B));
		int D=M-m;
		
		c[0] = H; c[1] = M; c[2] = m; c[3] = D;
		
		return;
	}
	
	if (model == HY) {
		int H, S, V;
		
		QColor color(R, G, B);
		color.getHsv(&H, &S, &V);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		int Y = toByte ( 0.299 * R + 0.587 * G + 0.114 * B );
		
		c[0] = H; c[1] = Y;
		
		return;
	}
	
	if (model == XYZ || model == LAB || model == LUV) {
		// Convert RGB to range [0,1]
		double r(double(R)/255);
		double g(double(G)/255);
		double b(double(B)/255);
		
		// We assume that color is sRGB with decoding gamma of 2.2, as customary
		
		// Linearize sRGB
		if (r<0.04045) r=r/12.92; else r=pow((r+0.055)/1.055, 2.4);
		if (g<0.04045) g=g/12.92; else g=pow((g+0.055)/1.055, 2.4);
		if (b<0.04045) b=b/12.92; else b=pow((b+0.055)/1.055, 2.4);
		
		// Convert sRGB to XYZ using transformation matrix
		X = 0.4124564 * r + 0.3575761 * g + 0.1804375 * b;
		Y = 0.2126729 * r + 0.7151522 * g + 0.0721750 * b;
		Z = 0.0193339 * r + 0.1191920 * g + 0.9503041 * b;
	}

	if (model == XYZ) {
		c[0] = toByte(X*255);
		c[1] = toByte(Y*255);
		c[2] = toByte(Z*255);
		return;
	}
	
	if (model == LAB) {
		// Reference illuminant D65 white point
		double Xn = 0.95047;
		double Yn = 1;
		double Zn = 1.08883;
		
		double D = pow( 6.0 / 29.0, 3 );
		
		double Xfn( X / Xn ), Yfn( Y / Yn ), Zfn( Z / Zn );
		if ( Xfn > D ) Xfn = pow(Xfn,1.0/3.0); else Xfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Xfn + ( 4.0 / 29.0 );
		if ( Yfn > D ) Yfn = pow(Yfn,1.0/3.0); else Yfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Yfn + ( 4.0 / 29.0 );
		if ( Zfn > D ) Zfn = pow(Zfn,1.0/3.0); else Zfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Zfn + ( 4.0 / 29.0 );
		
		double L = 116 * Yfn - 16;
		double a = 500 * (Xfn - Yfn);
		double b = 200 * (Yfn - Zfn);
		
		// Convert to byte
		c[0] = toByte( L*255 / 100 );
		c[1] = toByte( a + 127 );
		c[2] = toByte( b + 127 );
		
		return;
	}
	
	if (model == LUV) {
		// Reference illuminant D65 white point
		double Xn = 0.95047;
		double Yn = 1;
		double Zn = 1.08883;
		
		double D = pow( 6.0 / 29.0, 3 );
		
		double Yfn( Y / Yn );
		if ( Yfn > D ) Yfn = pow(Yfn,1.0/3.0); else Yfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Yfn + ( 4.0 / 29.0 );
		
		double Xfn((4*X) / (X + 15*Y + 3*Z));
		double Zfn((9*Y) / (X + 15*Y + 3*Z));
		
		double Xref((4*Xn) / (Xn + 15*Yn + 3*Zn));
		double Zref((9*Yn) / (Xn + 15*Yn + 3*Zn));

		double L = 116 * Yfn - 16;
		double u = 13 * L * ( Xfn - Xref );
		double v = 13 * L * ( Zfn - Zref );
		
		// Convert to byte
		c[0] = toByte( L*255 / 100 );
		c[1] = toByte( u + 127 );
		c[2] = toByte( v + 127 );
		
		return;
	}
	
	if (model == L1L2L3) {
		// See: Gevers and Smeulders "Color-based object recognition" (1999)
		int d1 = (R-G)*(R-G);
		int d2 = (R-B)*(R-B);
		int d3 = (G-B)*(G-B);
		double l1 = double(d1) / (d1+d2+d3);
		double l2 = double(d2) / (d1+d2+d3);
		double l3 = double(d3) / (d1+d2+d3);
		c[0] = toByte(l1);
		c[1] = toByte(l2);
		c[2] = toByte(l3);
	}
}

void Pixel::toRgb() 
{
	if (this->model == RGB) return; // Nothing to do
	
	// TODO This function is unfinished
}
