# BITalino-LSL bridge

Convert ECG obtained from BITalino to HR, specify the name in LSL stream and output.

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
lsl_bridge [BITalino's MacAddress] [LSL name]
