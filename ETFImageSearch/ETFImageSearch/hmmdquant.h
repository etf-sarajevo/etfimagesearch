#ifndef HMMDQUANT_H
#define HMMDQUANT_H

#include "colorhistogram.h"


/**
 * This class implements a custom quantization table used for HMMD color model in MPEG-7
 * CSD descriptor. See
 *   Manjunath, Bangalore S and Ohm, J-R and Vasudevan, Vinod V and Yamada, Akio,
 *   "Color and texture descriptors" (2001).
 * Note that this class does NOT implement the CSD itself! Just the color quantization part.
 */

class HMMDquant : public ColorHistogram
{
public:
	HMMDquant();
	
	QString name() { return QString("HMMD quantization from CSD"); }
	static QString static_name() { return QString("HMMD quantization from CSD"); }
	int size() { return 184; }
	
	virtual void colorQuantize(Pixel &p);
	virtual void incrementHistogram(const Pixel& p);
};

#endif // HMMDQUANT_H
