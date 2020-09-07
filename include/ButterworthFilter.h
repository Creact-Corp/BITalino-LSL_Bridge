#include <math.h>

template<typename T>
class ButterworthFilter
{
  public:
    //  Default
    ButterworthFilter(double samplingRate, double cutOffFrequency) : yn0(0), yn1(0), yn2(0), xn1(0), xn2(0) 
    {
      const double omega0 = 2 * 3.14159265 * cutOffFrequency;
      const double tmp0 = 2 * samplingRate / omega0;
      a = tmp0*tmp0 + tmp0*sqrt(2) + 1;
      b = 2 - 2*tmp0*tmp0;
      c = tmp0*tmp0 - tmp0*sqrt(2) + 1;
    };
    //

    //  Public
    T step(T command)
    {
      yn0 = command + 2*xn1 + xn2 - b*yn1 - c*yn2;
      yn0 /= a;
  
      yn2 = yn1;
      yn1 = yn0;
      
      xn2 = xn1;
      xn1 = command;
      return yn0; 
    };
    //

    //  Set/get
    T getValue(int index = 0)
    {
      if(index == 0) return yn0;
      else if(index == 1) return yn1;
      else if(index == 2) return yn2;
      else return T(0);
    }
    T getCommand(int index)
    {
      if(index == 1) return xn1;
      else if(index == 2) return xn2;
      else return T(0);
    }
    //

  protected:
    //  Attributes
    T yn0, yn1, yn2;
    T xn1, xn2;
    double a, b, c;
    //
};
