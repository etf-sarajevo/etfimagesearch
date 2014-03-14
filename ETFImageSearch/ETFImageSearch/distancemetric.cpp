#include "distancemetric.h"

#include <cmath>
#include <QDebug>
#include <sys/time.h>

#define NR_METRICS 22
static const char names[][NR_METRICS] = { "MATSUSHITA", "BRAY_CURTIS", "MANHATTAN", "EUCLIDEAN", "SOERGEL", "BHATTACHARYA", "CHI_SQUARE", "CANBERRA", "HIST_INT", "JSD", "ANGULAR", "CHORD", "WAVE_HEDGES", "WED", "K_S", "KUIPER", "MEAN", "TANIMOTO", "NED", "QUADRATIC", "MAHALANOBIS", "PCC"};

/*double getTime() 
{
	static timeval t;
	timeval newtime;
	double elapsedTime;
	
	gettimeofday(&newtime, NULL);
	elapsedTime = (newtime.tv_sec - t.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (newtime.tv_usec - t.tv_usec) / 1000.0;   // us to ms
	t = newtime;
	return elapsedTime;
}*/



double DistanceMetric::distance(Type distanceMetric, const FeatureVector& f1, const FeatureVector& f2)
{
	switch(distanceMetric) {

	case EUCLIDEAN:
	{
		double sum(0);
		uint size(f1.size());
		for (uint i(0); i<size; i++) {
			sum += (f2[i] - f1[i]) * (f2[i] - f1[i]);
		}
		//return sqrt(sum); // Not relevant for retrieval
		return sum;
	}
	
	case MATSUSHITA:
	{
		double sum(0);
		for (int i(0); i<f1.size(); i++) {
			double k = sqrt(f2[i]) - sqrt(f1[i]);
			sum += k * k;
		}
		//return sqrt(sum); // Not relevant for retrieval
		return sum;
	}
	
	case BRAY_CURTIS:
	{
		// Bray-Curtis distance
		double sum1(0), sum2(0);
		for (int i(0); i<f1.size(); i++) {
//			sum1 += double(abs(f2[i] - f1[i]));
			sum1 += fabs(f2[i] - f1[i]);
			sum2 += (f2[i] + f1[i]);
		}
		return sum1 / sum2;
	}
	
	case MANHATTAN:
	{
		// Manhattan distance
		/*int sum(0);
		for (int i(0); i<f1.size(); i++) {
			sum += abs(f2[i] - f1[i]);
		}
		return double(sum) / f1.size();*/
		double sum(0);
		int size(f1.size());
		for (int i(0); i<size; i++) {
			sum += fabs(f2[i] - f1[i]);
		}
		//return sum / f1.size(); // Not relevant for retrieval
		return sum;
	}
	
	case SOERGEL:
	{
		// Soergel distance
		double sum1(0), sum2(0);
		for (int i(0); i<f1.size(); i++) {
//			sum1 += double(abs(f2[i] - f1[i]));
			sum1 += fabs(f2[i] - f1[i]);
			sum2 += qMax(f2[i], f1[i]);
		}
		return sum1 / sum2;
	}
	
	case BHATTACHARYA:
	{
		double sum(0), sum1(0);
		for (int i(0); i<f1.size(); i++) {
			sum += sqrt(fabs(f2[i] - f1[i]));
			sum1 += f1[i];
		}
		return log10(sum/sum1);
	}
	
	case CHI_SQUARE:
	{
		double sum(0);
		for (int i(0); i<f1.size(); i++) {
			if (f2[i] + f1[i] > 0)
				sum += ((f2[i] - f1[i])*(f2[i] - f1[i])) / (f2[i] + f1[i]);
		}
		return sum;
	}
	
	case CANBERRA:
	{
		double sum(0);
		for (int i(0); i<f1.size(); i++) {
			if (f2[i] + f1[i] > 0)
				sum += fabs(f2[i] - f1[i]) / (f2[i] + f1[i]);
		}
		return sum;
	}
	
	case HIST_INT:
	{
		/*int sum(0), sum1(0), sum2(0);
		for (int i(0); i<f1.size(); i++) {
			sum  += qMin(f2[i], f1[i]);
			sum1 += f1[i];
			sum2 += f2[i];
		}
		return 1 - double(sum) / qMin (sum1, sum2);*/
		double sum(0), sum1(0), sum2(0);
		for (int i(0); i<f1.size(); i++) {
			sum  += qMin(f2[i], f1[i]);
			sum1 += f1[i];
			sum2 += f2[i];
		}
		return 1 - sum / qMin (sum1, sum2);
	}
	
	case JSD:
	{
		double sum(0);
		for (int i(0); i<f1.size(); i++) {
			double mi((f2[i] + f1[i]) / 2);
			if (f1[i] > 0)
				sum += f1[i] * log(double(f1[i]) / mi);
			if (f2[i] > 0)
				sum += f2[i] * log(double(f2[i]) / mi);
		}
		return sum;
	}
		
	case ANGULAR: // Angular separation
	{
		double sum1(0), sum2(0), sum3(0);
		for (int i(0); i<f1.size(); i++) {
			sum1 += f2[i] * f1[i];
			sum2 += f2[i] * f2[i];
			sum3 += f1[i] * f1[i];
		}
		return 1 - (sum1 / sqrt(sum2 * sum3) );
	}
		
	case CHORD:
	{
		double sum1(0), sum2(0), sum3(0);
		for (int i(0); i<f1.size(); i++) {
			sum1 += f2[i] * f1[i];
			sum2 += f2[i] * f2[i];
			sum3 += f1[i] * f1[i];
		}
		//return sqrt(2 - 2*(sum1 / sqrt(sum2 * sum3))); // Not relevant for retrieval
		return 2 - 2*(sum1 / sqrt(sum2 * sum3));
	}
		
	case WAVE_HEDGES:
	{
		double sum(0);
		for (int i(0); i<f1.size(); i++) {
			if (f1[i] > 0 && f2[i] > 0)
				sum += (1 - double(qMin(f2[i], f1[i])) / qMax(f2[i], f1[i]));
			else
				sum += 1;
		}
		return sum;
	}
	
	case WED: // Weighted Euclidean distance
	{
		double sum(0);
		for (int i(0); i<f1.size(); i++) {
			double k(f1[i]);
			if (k==0) k=1;
			sum += k * (f2[i] - f1[i]) * (f2[i] - f1[i]);
		}
		return sum;
	}
	
	case NED: // Normalized Euclidean distance
	{
		double sum(0);
		for (int i(0); i<f1.size(); i++) {
			double k(f1[i]);
			if (k==0) k=1;
			sum += k * (f2[i] - f1[i]) * (f2[i] - f1[i]);
		}
		return sum;
	}
		
	case K_S: // Kolmogorov-Smirnov
	{
		/*int max(0);
		for (int i(0); i<f1.size(); i++) {
			int d(abs(f2[i] - f1[i]));
			if (d>max) max=d;
		}
		return max;*/
		double max(0);
		for (int i(0); i<f1.size(); i++) {
			double d(fabs(f2[i] - f1[i]));
			if (d>max) max=d;
		}
		return max;
	}
		
	case KUIPER:
	{
		double max1(0), max2(0);
		for (int i(0); i<f1.size(); i++) {
			double d1(f2[i] - f1[i]);
			double d2(f1[i] - f2[i]);
			if (d1>max1) max1=d1;
			if (d2>max2) max2=d2;
		}
		return max1+max2;
	}
	
	case MEAN:
	{
		double sum1(0), sum2(0);
		for (int i(0); i<f1.size(); i++) {
			sum1 += i*f1[i];
			sum2 += i*f2[i];
		}
		return fabs(sum1 - sum2);
	}
		
	case TANIMOTO:
	{
		double sum1(0), sum2(0), sum3(0);
		for (int i(0); i<f1.size(); i++) {
			sum1 += f2[i] * f1[i];
			sum2 += f2[i] * f2[i];
			sum3 += f1[i] * f1[i];
		}
		if (sum2 == 0 && sum3 == 0) return 0;
		if (sum2 == 0 || sum3 == 0) return 1;
		return 1 - sum1 / (sum2 + sum3 - sum1);
	}
	
	// Quadratic and Mahalanobis differ only in the meaning of values in parameter vector
	case QUADRATIC:
	case MAHALANOBIS:
	{
		double sum(0);
		/*QString debugg;
		static double t1(0), t2(0), t3(0), t4(0), t5(0), t6(0);
		int cases(0);*/
		int size(f1.size());
		//getTime();
		
		for (int i(0); i<size; i++) {
			for (int j(0); j<size; j++) {
				//double t = getTime();
				//t1 += t;
				if (f1[i]==f2[i] || f1[j]==f2[j]) continue;
				//cases++;
				//t2 += t;
					//int val = abs(f1[i]-f2[i]) * abs(f1[j]-f2[j]);
				double val = fabs(f1[i]-f2[i]) * fabs(f1[j]-f2[j]);
				//t3 += getTime();
				//debugg = "";
				//debugg += QString("(%1,%2) %3 - ").arg(i).arg(j).arg(val);
				int idx = i*size + j;
				//t4 += getTime();
				//debugg += QString("%1 - ").arg(idx);
				double value = val*parameterVector[idx];
				//t5 += getTime();
				//debugg += QString("%1 - %2 ").arg(parameterVector[idx]).arg(value);
				sum += value;
				//qDebug() << debugg;
				//t6 += getTime();
			}
		}
		
		//qDebug() << "TOTAL: "<<getTime();
		//qDebug() << "TOTAL: "<<(t1+t3+t4+t5+t6)<<" t1"<<t1<<"t2cases"<<cases<<"t2"<<t2<<" t3"<<t3<<" t4"<<t4<<" t5"<<t5<<" t6"<<t6;
		
		return sum;
	}
		
	case PCC:
	{
		double sum1=0, sum2=0, sum3=0, avg1=0, avg2=0;
		int i;
		for (i=0; i<f1.size(); i++) {
			avg1 += f1[i];
			avg2 += f2[i];
		}
		avg1 /= f1.size();
		avg2 /= f1.size();
		
		for (i=0; i<f1.size(); i++) {
			sum1 += fabs(f1[i]-avg1)*fabs(f2[i]-avg2);
			sum2 += (f1[i]-avg1)*(f1[i]-avg1);
			sum3 += (f2[i]-avg2)*(f2[i]-avg2);
		}
		
		if (sum2 == 0 && sum3 == 0) return 0;
		if (sum2 == 0 || sum3 == 0) return 1;
		return 1 - sum1 / (sum2 * sum3); 
	}
		
	}
	
	return 0; // No distance metric found
}

DistanceMetric::Type DistanceMetric::fromString(QString string)
{
	for (int i(0); i<NR_METRICS; i++)
		if (string == names[i])
			return Type(i);
	throw QString("Unknown distance metric %1").arg(string);
}

QString DistanceMetric::toString(Type type)
{
	return QString(names[int(type)]);
}

QStringList DistanceMetric::allMetrics()
{
	QStringList result;
	for (int i(0); i<NR_METRICS; i++)
		result.append(QString(names[i]));
	return result;
}


