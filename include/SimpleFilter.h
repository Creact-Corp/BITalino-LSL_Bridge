
template<typename T>
class LowPassFilter
{
  public:
    //  Default
    LowPassFilter(double samplingRate, double cutOffFrequency, T initial = T(0)) : yn0(0), yn1(initial)
    {
      const double omega0 = 2 * 3.14159265 * cutOffFrequency;
      alpha = omega0 / samplingRate / (1 + omega0 / samplingRate);
    };
    //

    //  Public
    T step(T command)
    {
      yn0 = alpha*command + (1. - alpha)*yn1;  
      yn1 = yn0;
      return yn0; 
    };
    //

    //  Set/get
    T getValue(int index = 0)
    {
      if(index == 0) return yn0;
      else if(index == 1) return yn1;
      else return T(0);
    }

  protected:
    //  Attributes
    T yn0, yn1;
    double alpha;
    //
};


template<typename T>
class HighPassFilter
{
  public:
    //  Default
    HighPassFilter(double samplingRate, double cutOffFrequency) : yn0(0), yn1(0), xn1(0)
    {
      const double omega0 = 2 * 3.14159265 * cutOffFrequency;
      alpha = 1 / (1 + omega0 / samplingRate);
    };
    //

    //  Public
    T step(T command)
    {
      yn0 = alpha*yn1 + alpha*(command - xn1);
      yn1 = yn0;
      xn1 = command;
      return yn0; 
    };
    //

    //  Set/get
    T getValue(int index = 0)
    {
      if(index == 0) return yn0;
      else if(index == 1) return yn1;
      else return T(0);
    }

  protected:
    //  Attributes
    T yn0, yn1;
    T xn1;
    double alpha;
    //
};
