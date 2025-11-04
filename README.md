# matlab-simulink-s-function

A comprehensive guide for creating custom S-functions that interface C++ or C code with Simulink Coder. This enables compilation of Simulink models with third-party libraries that aren’t natively supported.

## Overview

S-functions (System-functions) provide a powerful mechanism to extend Simulink with custom C++ implementations. This repository demonstrates best practices for integrating external libraries and custom algorithms into your Simulink workflows.

## Project Structure

```
├── CMakeLists.txt                      # Build configuration
├── include/
│   └── optical_flow_uav_velocity.hpp  # Custom class headers
├── src/
│   ├── matlab_simulink_s_function.cpp # S-function implementation
│   └── optical_flow_uav_velocity.cpp  # Custom class implementation
├── test_s_function.slx                 # Example Simulink model
├── include_directories.txt
├── link_libraries.txt
├── source_files.txt
├── LICENSE
└── README.md
```

## Getting Started

### Prerequisites

- MATLAB/Simulink with Simulink Coder
- CMake (3.10 or higher recommended)
- C++ compiler compatible with your MATLAB version
- Any third-party libraries your code depends on (e.g., OpenCV)

### Implementation Steps

#### 1. Prepare Your Custom Code

Place your source files in `src/` and headers in `include/`:

```cpp
// include/your_class.hpp
class YourClass {
public:
    void yourMethod();
};
```

#### 2. Configure the S-Function

Edit `src/matlab_simulink_s_function.cpp` and include your headers:

```cpp
#include "your_class.hpp"
```

#### 3. Choose Memory Management Strategy

Select one of two approaches by defining at the top of your S-function file:

**Option A: Persistent Memory** (Recommended for complex objects)

```cpp
#define USE_PERSISTENT_MEMORY
```

**Option B: Static Objects** (Simpler, but less flexible)

```cpp
// Leave USE_PERSISTENT_MEMORY undefined
```

#### 4. Implement Required Functions

##### `mdlInitializeSizes(SimStruct *S)`

Configure the S-function’s interface:

- Set number of input/output ports
- Define signal dimensions
- Specify discrete sample times
- Allocate persistent memory (if using)

##### `mdlInitializeSampleTimes(SimStruct *S)`

Define when the S-function executes. Typically:

```cpp
ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
ssSetOffsetTime(S, 0, 0.0);
```

##### `mdlStart(SimStruct *S)`

Initialize your custom objects (called once at simulation start):

```cpp
#ifdef MDL_START
void mdlStart(SimStruct *S) {
    // Create instances using new
    // Store in persistent memory or static containers
}
#endif
```

##### `mdlOutputs(SimStruct *S, int_T tid)`

Main computation loop (called every simulation step):

```cpp
void mdlOutputs(SimStruct *S, int_T tid) {
    // Access input signals
    // Perform computations
    // Write to output signals
}
```

##### `mdlTerminate(SimStruct *S)` (Optional)

Cleanup dynamically allocated resources:

```cpp
#ifdef MDL_TERMINATE
void mdlTerminate(SimStruct *S) {
    // Delete objects created with new
}
#endif
```

#### 5. Working with SimStruct

The `SimStruct *S` pointer provides access to Simulink’s execution context:

- Input signals: `ssGetInputPortSignal(S, portIndex)`
- Output signals: `ssGetOutputPortSignal(S, portIndex)`
- Parameters: `ssGetSFcnParam(S, paramIndex)`

For comprehensive examples, see the [MATLAB SimStruct documentation](https://www.mathworks.com/help/simulink/sfg/simstruct_introduction.html).

#### 6. Configure CMake Build

Edit `CMakeLists.txt` to specify your dependencies:

```cmake
# Add required packages
set(CUSTOM_PACKAGES 
    OpenCV
    YourPackage1
    YourPackage2
)

# Specify source files
set(SRCS 
    ${CMAKE_SOURCE_DIR}/src/matlab_simulink_s_function.cpp
    ${CMAKE_SOURCE_DIR}/src/your_implementation.cpp
)

# Include directories
set(INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
```

#### 7. Build the S-Function

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake (specify your MATLAB installation path)
cmake .. -DMATLAB_BIN_DIR=/path/to/matlab/bin

# Build
make
```

The compiled S-function (`.mexa64`, `.mexw64`, or `.mexmaci64`) will be generated in the `build/` directory.

#### 8. Use in Simulink

1. Open `test_s_function.slx` as a reference
1. Add an S-Function block to your model
1. Set the S-function name to your compiled MEX file (without extension)
1. Configure block parameters as needed
1. Run your simulation

## Example Use Case

The included example demonstrates optical flow-based UAV velocity estimation, showing how to:

- Interface with OpenCV library
- Process image data in real-time
- Manage state across simulation steps
- Handle complex C++ objects within Simulink

## Troubleshooting

- **Compilation errors**: Verify MATLAB version compatibility with your compiler
- **Missing symbols**: Check that all required libraries are linked in CMakeLists.txt
- **Runtime errors**: Ensure proper memory management in mdlStart and mdlTerminate
- **Sample time issues**: Verify sample time configuration matches your model requirements

## Support

If you find this repository helpful, please ⭐ star it to show your support!

[![GitHub stars](https://img.shields.io/github/stars/AmalDevHaridevan/matlab-simulink-s-function.svg?style=social&label=Star)](https://github.com/AmalDevHaridevan/matlab-simulink-s-function/stargazers)

## Resources

- [MATLAB S-Function Documentation](https://www.mathworks.com/help/simulink/sfg/what-is-an-s-function.html)
- [SimStruct Reference](https://www.mathworks.com/help/simulink/sfg/simstruct_introduction.html)
- [Writing C MEX S-Functions](https://www.mathworks.com/help/simulink/sfg/writing-c-mex-s-functions.html)

## License

See <LICENSE> file for details.​​​​​​​​​​​​​​​​