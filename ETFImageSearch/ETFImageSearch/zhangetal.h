#ifndef ZHANGETAL_H
#define ZHANGETAL_H

#include "colorhistogram.h"


/**
 * This class implements a custom quantization table in HSV color model described in
 *   Zhang, Zhenhua and Li, Wenhui and Li, Bo, "An improving technique of color histogram
 *   in segmentation-based image retrieval" (2009).
 * This class does NOT implement the segmentation part of this paper.
 */

class ZhangEtAl : public ColorHistogram
{
public:
	ZhangEtAl();
	
	QString name() { return QString("Zhang et al."); }
	static QString static_name() { return QString("Zhang et al."); }
	
	virtual void colorQuantize(Pixel& p);
};

#endif // ZHANGETAL_H
