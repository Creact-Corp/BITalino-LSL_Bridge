/*
    LSL_Bridge
    Copyright (C) 2020  Creact
    Copyright (C) 2020  Ullo
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bitalino.h"
#include "lsl_cpp.h"

#include "ButterworthFilter.h"
#include "SimpleFilter.h"
#include "circular_buffer.h"

#include <iostream>
#include <unistd.h>

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

// =============================================================================

class filter
{
public:
    filter(int samplingRate, int hf, int lf)
    {
        filter_highpass = new HighPassFilter<long>(samplingRate, hf);
        filter_lowpass = new ButterworthFilter<long>(samplingRate, lf);
    }
    
    long update(long rawdata)
    {
        bandpass_prev = bandpass;
        filter_highpass->step(rawdata);
        filter_lowpass->step(filter_highpass->getValue());
        bandpass = filter_lowpass->getValue();
        
        long derivative = bandpass - bandpass_prev;
        
        long power = derivative * derivative;
        
        smoothing.push_back(power);
        long smoothed = smoothing.mean();
        
        decimation_n++;
        if(decimation_n >= decimation)
        {
            decimation_n = 0;
            trend.push_back(smoothed);
        }
        
        return trend.max();
    }
    
private:
    long bandpass = 0;
    long bandpass_prev = 0;
    
    int decimation_n = 0;
    
    Circular_Buffer<long, 8> smoothing;
    
    const unsigned int decimation = 8;
    Circular_Buffer<long, 32> trend;
    
    HighPassFilter<long> *filter_highpass;
    ButterworthFilter<long> *filter_lowpass;
    
};

// =============================================================================

bool keypressed(void)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    
    timeval readtimeout;
    readtimeout.tv_sec = 0;
    readtimeout.tv_usec = 0;
    
    return (select (FD_SETSIZE, &readfds, NULL, NULL, &readtimeout) == 1);
}

void description(void)
{
    cout << "Usage: lsl_bridge [BITalino's MacAddress] [LSL name] [Sensors]" << endl;
    cout << "   [Sensors] Select the sensor to use." << endl;
    cout << "       -h  Use HeartRate.(Connect ECG Sensor to A1 of BITalino)" << endl;
    cout << "       -r  Use Respiration.(Connect PZT Sensor to A2 of BITalino)" << endl;
    cout << "       -e  Use EEG Alpha.(Connect EEG Sensor to A3 of BITalino)" << endl;
    cout << "Example: ./lsl_bridge 20:16:07:18:14:06 echopink -hr" << endl;
}


int main(int argc, char* argv[])
{

    string macAddress = "20:16:07:18:14:06";
    string lslname = "echopink";
    
    bool hr_enable, resp_enable, eeg_enable, ecg_enable;
    hr_enable = resp_enable = eeg_enable = ecg_enable = false;
    
    if (argc == 4)
    {
        cout << "MAC=" << argv[1] << endl;
        cout << "Name=" << argv[2] << endl;
        macAddress = argv[1];
        lslname = argv[2];
        
        int opt;
        opterr = 0;
        while ((opt = getopt(argc, argv, "hrec")) != -1)
        {
            switch (opt)
            {
                case 'h': hr_enable = true; break;      // HeartRate
                case 'r': resp_enable = true; break;    // Respiration
                case 'e': eeg_enable = true; break;     // EEG
                case 'c': ecg_enable = true; break;     // ECG
                default:
                    description();
                    return 0;
            }
        }
    }
    else
    {
        description();
        return 0;
    }
    
    try
    {
        // initialize bitalino
        BITalino dev(macAddress.c_str());
        string ver = dev.version();
        cout << ver.c_str() << endl;
        
        // make a new stream_info & outlet
        lsl::stream_info *info_hr, *info_resp, *info_eeg,*info_alpha, *info_ecg;
        lsl::stream_outlet *outlet_hr, *outlet_resp, *outlet_eeg, *outlet_alpha, *outlet_ecg;
        if (hr_enable)
        {
            info_hr = new lsl::stream_info(lslname.c_str(), "heartrate", 1, 0, lsl::cf_float32, "bitalinoHR_" + macAddress);
            outlet_hr = new lsl::stream_outlet(*info_hr);
        }

        if (resp_enable)
        {
            info_resp = new lsl::stream_info(lslname.c_str(), "breathingamp", 3, 10, lsl::cf_float32, "bitalinoResp_" + macAddress);
            outlet_resp = new lsl::stream_outlet(*info_resp);
        }
        if (eeg_enable)
        {
            //info_eeg = new lsl::stream_info(lslname.c_str(), "RAW_EEG", 1, 100, lsl::cf_float32, "bitalinoEEG_" + macAddress);
            //outlet_eeg = new lsl::stream_outlet(*info_eeg);
            info_alpha = new lsl::stream_info(lslname.c_str(), "NFB_alpha", 1, 100, lsl::cf_float32, "bitalinoAlpha_" + macAddress);
            outlet_alpha = new lsl::stream_outlet(*info_alpha);
        }
        if (ecg_enable)
        {
            info_ecg = new lsl::stream_info(lslname.c_str(), "RAW_ECG",1, 100, lsl::cf_float32, "bitalinoECG_" + macAddress);
            outlet_ecg = new lsl::stream_outlet(*info_ecg);
        }
        
        // 100Hz A1(ECG) A2(RESP) A3(EEG)
        dev.start(100, { 0, 1, 2 });
        
        BITalino::VFrame frames(1);
        float lslSample_hr[1];
        float lslSample_resp[3];
        float lslSample_ecg[1];
        float lslSample_eeg[1];
        float lslSample_alpha[1];
        
        cout << "Press Enter to exit." << endl;
        
        filter alpha(100, 8, 12);
            
        do
        {
            // count timing
            tick++;
            
            // get analog data and create 100Hz timing
            dev.read(frames);
            
            const BITalino::Frame& f = frames[0];
            int data_ecg = f.analog[0];     // ECG
            int data_resp = f.analog[1];    // RESP
            int data_eeg = f.analog[2];     // EEG
            
            // filter ECG, retrieve beat detection
            bool beat = updateECG(data_ecg);
            
            // new beat
            if (beat and !isECGing)
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
                if (hr_enable)
                {
                    lslSample_hr[0] = hr_insta;
                    outlet_hr->push_sample(lslSample_hr);
                }
            }
            
            // refractory time before new beat
            if (isECGing and tick - tick_ecg_start > ecg_time) 
            {
                isECGing = false;
            }
            
            // send LSL Resp 10Hz
            if (resp_enable)
            {
                if (tick % 10 == 0)
                {
                    lslSample_resp[0] = data_resp;
                    lslSample_resp[1] = (float)data_resp / 1023.0f;
                    lslSample_resp[2] = 0;
                    outlet_resp->push_sample(lslSample_resp);
                }
            }
            
            //long eeg_alpha = updateEEG(data_eeg);
            
            // send LSL EEG
            if (eeg_enable)
            {
                // RAW
                lslSample_eeg[0] = data_eeg;
                //outlet_eeg->push_sample(lslSample_eeg);
                
                // Alpha
                long eeg_alpha = alpha.update(data_eeg) / 10;
                if(eeg_alpha > 100) { eeg_alpha = 100; }
                lslSample_alpha[0] = (float)eeg_alpha * 0.01f;
                outlet_alpha->push_sample(lslSample_alpha);
                
            }
            
            // send LSL ECG
            if (ecg_enable)
            {
                lslSample_ecg[0] = data_ecg;
                outlet_ecg->push_sample(lslSample_ecg);
            }

            cout << " Time:" << to_string(tick) <<  
                    " HR:" << to_string(lslSample_hr[0]) << 
                    " ECG:" << to_string(data_ecg) << 
                    " RESP:" << to_string(data_resp)  << 
                    " EEG:" << to_string(data_eeg) << 
                    " Alpha:" << to_string(lslSample_alpha[0]) << endl;
            
        } while (!keypressed()); // Press Enter to exit.
        
        dev.stop();
        
        if (hr_enable)
        {
            delete outlet_hr;
            delete info_hr;
        }
        if (resp_enable)
        {
            delete outlet_resp;
            delete info_resp;
        }
        if (eeg_enable)
        {
            //delete outlet_eeg;
            //delete info_eeg;
            delete outlet_alpha;
            delete info_alpha;
        }
        if (ecg_enable)
        {
            delete outlet_ecg;
            delete info_ecg;
        }
        
        printf("exit.\n\n");
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
