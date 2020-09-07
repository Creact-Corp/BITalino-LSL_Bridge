#include "bitalino.h"
#include "lsl_cpp.h"

#include "ButterworthFilter.h"
#include "SimpleFilter.h"
#include "circular_buffer.h"

#include <iostream>

using namespace std;

// counter for animation and duration of output signal
double tick_beat_start = 0;
// how long a beat should last
double beat_time = 250000;

/** filtering **/
// sampling rate of the ECG sensor (in Hz)
static int samplingRate = 100;

// buffer for processing
// WARNING: with current library, buffer size limited to powers of 2
Circular_Buffer<long, 8> ecg_smoothing;
// trend buffer will be filled every now and then
const unsigned int ecg_decimation = 8;
Circular_Buffer<long, 32> ecg_trend; // approx. 0.250s * decimation

// filters for processing ECG
HighPassFilter<long> filter_ecg_highpass(samplingRate, 1);
ButterworthFilter<long> filter_ecg_lowpass(samplingRate, 20);

// counter to prevent double beats
uint32_t tick = 0;
uint32_t tick_ecg_start = 0;
double isECGing = false;
// double ecg_time = 400000; // 0.4s, no more than 150BPM
double ecg_time = 40; // 0.4s, no more than 150BPM

// compute BPM with each new beat
float hr_insta = 60;

// filtering ECG return true if a beat is detected
// ecg_raw: raw value from ECG
bool updateECG(long ecg_raw) {
  // intermediate values for variable of interest, keep previous to compute first derivative
  static long ecg_bandpass = 0;
  static long ecg_bandpass_prev = 0;
  // tracking when we should update trend buffer
  static int ecg_decimation_n = 0;

  bool beating = false;
  // band-pass filter to clean signal
  ecg_bandpass_prev = ecg_bandpass;
  filter_ecg_highpass.step(ecg_raw);
  filter_ecg_lowpass.step(filter_ecg_highpass.getValue());
  ecg_bandpass = filter_ecg_lowpass.getValue();
  // derivative to increase difference
  long ecg_derivative = ecg_bandpass - ecg_bandpass_prev;
  // power, even more
  long ecg_power = ecg_derivative * ecg_derivative;
  // smoothing time window
  ecg_smoothing.push_back(ecg_power);
  long ecg_smoothed = ecg_smoothing.mean(); // staying integer, loos resolution but gain speed
  // add values to compute trend
  ecg_decimation_n++;
  if (ecg_decimation_n >= ecg_decimation) {
    ecg_decimation_n = 0;
    ecg_trend.push_back(ecg_smoothed);
  }

  // detect beat by comparing short window to long window
  long ecg_trig = 0;
  if (ecg_smoothed > ecg_trend.max() / 2 && ecg_smoothed > ecg_trend.mean() * 2) {
    ecg_trig = ecg_trend.max();
    beating = true;
  }

  return beating;
}

int main(int argc, char* argv[])
{

    string macAddress = "20:16:07:18:14:06";
    string lslname = "echopink";

    if (argc == 3)
    {
        cout << "MAC=" << argv[1] << endl;
        cout << "Name=" << argv[2] << endl;
        macAddress = argv[1];
        lslname = argv[2];
    }
    else
    {
        cout << "Usage: lsl_bridge [BITalino's MacAddress] [LSL name]" << endl;
        cout << "Example: ./lsl_bridge 20:16:07:18:14:06 echopink" << endl;
        return 0;

    }

    try
    {
        // initialize bitalino
        BITalino dev(macAddress.c_str());
        string ver = dev.version();
        cout << ver.c_str() << endl;

        // make a new stream_info
        lsl::stream_info info(lslname.c_str(), "heartrate", 1, 100, lsl::cf_float32, "bitalinoHR_" + macAddress);
        lsl::stream_info info_rawECG("test", "rawECG",1, 100, lsl::cf_float32, "bitalinoECG_" + macAddress);

        // make a new outlet
        lsl::stream_outlet outlet(info);
        lsl::stream_outlet outlet_rawECG(info_rawECG);

        // 100Hz A1
        dev.start(100, { 5 });

        BITalino::VFrame frames(1);
        float sample[1];
        float rawData[1];
        
        while (1)
        {
            // count timing
            tick++;
            
            // get analog data and create 100Hz timing
            dev.read(frames);
            
            const BITalino::Frame& f = frames[0];
            int data = f.analog[0];
            
            // filter ECG, retrieve beat detection
            bool beat = updateECG(data);
            
            // new beat
            if(beat and !isECGing)
            {
                // start of new QRS complex
                isECGing = true;
                
                // computing intantaneous heart-rate
                // WARNING: the very first HR value will be off, you should implement a mecanism to detect when a new user touch the sensor, and only take into account HR value starting from the second QRS complex.
                hr_insta = 6000 / (tick - tick_ecg_start);
                
                tick_ecg_start = tick;
                
                // INSERT HERE CODE YOU WOULD LIKE TO TRIGGER WITH EACH NEW BEAT
                
                printf("Beat!\n");
                
                // send LSL HRdata
                sample[0] = hr_insta;
                outlet.push_sample(sample);
                
            }
            
            // refractory time before new beat
            if(isECGing and tick - tick_ecg_start > ecg_time) 
            {
                isECGing = false;
            }
            
            
            // send LSL rawECG
            rawData[0] = data;
            outlet_rawECG.push_sample(rawData);
            
            cout << " Time:" << to_string(tick) <<  " lsl:" << to_string(sample[0]) << " bitalino:" << to_string(data) << endl;
            
        }
    }
    catch (BITalino::Exception& e)
    {
        cerr << e.getDescription() << endl;
    }
    catch (std::exception& e)
    {
        cerr << "Got an exception: " << e.what() << endl;
    }

    return 0;
}
