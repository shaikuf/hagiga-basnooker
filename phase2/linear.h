
#ifndef MATHS_REGRESSION_LINEAR_H
#define MATHS_REGRESSION_LINEAR_H

#include <assert.h>
#include <math.h>
#include <cv.h>
#include <vector>
using namespace std;

class Linear
{
public:
Linear(vector<CvPoint2D32f> points);

~Linear();

//! Class constructor
void do_fit(int n, double* x, double*y);
/*
Linear(int n, double *x, double *y)
{

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
*/
//! Evaluates the linear regression function at the given abscissa.

double getValue(double x)
{
    return m_a + m_b * x;
}

//! Returns the slope of the regression line
double getSlope()
{
   return m_b;
}

//! Returns the intercept on the Y axis of the regression line
double getIntercept()
{
  return m_a;
}

//! Returns the linear regression coefficient

double getCoefficient()
{
  return m_coeff;
}

int* getFlags(){return flags;}

private:

double m_a, m_b, m_coeff;
int* flags;
};

#endif

