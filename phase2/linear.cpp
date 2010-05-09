#include "linear.h"
#include <cv.h>
#define coef_inc  0.05
using namespace std;

Linear::Linear(vector<CvPoint2D32f> points){
	int size=points.size();
	double* x_arr = new double[size];
	double* y_arr = new double[size];
	flags = new int[size];
	for(int i=0; i<size; i++){
		CvPoint2D32f current= points[i];
		x_arr[i] = current.x;
		y_arr[i] = current.y;
	}
	do_fit(size-1,x_arr,y_arr);
	double first_m = m_a;
	double first_coef = m_coeff;

	//testing the increment in the coefficient without everyone of the points
	for(int i=0; i<size; i++){
		double* x_arr = new double[size];
		double* y_arr = new double[size];
		//create a copy of the array without the i'th point
		for(int k=0; k<size-1; k++){
			CvPoint2D32f current;
			if(k<i)
				current = points[i];
			else
				current = points[i+1];
			x_arr[k] = current.x;
			y_arr[k] = current.y;
		}
		do_fit(size-1,x_arr,y_arr);
		if( (m_coeff) > (first_coef+coef_inc) )
			flags[i]=1;
		else
			flags[i]=0;
	}
	int count = 0;
	for(int i=0; i<size; i++){
		if(flags[i]==1){
			CvPoint2D32f current = points[i];
			x_arr[count]=current.x;
			y_arr[count]=current.y;
			count++;
		}
	}
	do_fit(count,x_arr,y_arr);
	delete[] x_arr;
	delete[] y_arr;
}

void Linear::do_fit(int n, double*x, double*y){

            // calculate the averages of arrays x and y
            double xa = 0, ya = 0;
            for (int i = 0; i < n; i++) {
                xa += x[i];
                ya += y[i];
            }
            xa /= n;
            ya /= n;

            // calculate auxiliary sums
            double xx = 0, yy = 0, xy = 0;
            for (int i = 0; i < n; i++) {
                double tmpx = x[i] - xa, tmpy = y[i] - ya;
                xx += tmpx * tmpx;
                yy += tmpy * tmpy;
                xy += tmpx * tmpy;
            }

            // calculate regression line parameters

            // make sure slope is not infinite
            assert(fabs(xx) != 0);

                m_b = xy / xx;
                m_a = ya - m_b * xa;
            m_coeff = (fabs(yy) == 0) ? 1 : xy / sqrt(xx * yy);
}

Linear::~Linear(){
	delete[] flags;
}