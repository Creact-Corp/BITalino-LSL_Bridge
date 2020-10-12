# BITalino-LSL bridge

The biological signal acquired by BITalino is output by the LSL stream.  
You can use BITalino as a sensor for your Echo device.  
The ECG and Respiratoin(PZT) sensor are supported.(EEG is not yet implemented.)  
This program runs on raspberry pi.  

## Build
This program requires Lab Streaming Layer.  
Clone the repository and build.  
```
git clone --recurse-submodules https://github.com/labstreaminglayer/labstreaminglayer.git
cd labstreaminglayer/
cmake .
cmake --build . --config Release --target install
cd ..

git clone https://github.com/Creact-Corp/BITalino-LSL_Bridge.git
cd BITalino-LSL_Bridge/
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage
lsl_bridge [BITalino's MacAddress] [LSL name] [sensors]  
	[sensors] Select the sensor to use.  
		-h  Use HeartRate.(Connect ECG Sensor to A1 of BITalino)  
		-r  Use Respiration.(Connect PZT Sensor to A2 of BITalino)  
