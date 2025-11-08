# MATLAB/Simulink S-Function Template

A comprehensive, production-ready template for creating custom S-functions that interface C++ code with Simulink. This enables seamless integration of third-party libraries and custom algorithms into Simulink models and supports automated code generation with Simulink Coder.

## Overview

S-functions (System-functions) provide a powerful mechanism to extend Simulink with custom C++ implementations. This repository demonstrates best practices for:

- Integrating external libraries (e.g., OpenCV) into Simulink
- Managing complex C++ objects within Simulink blocks
- Supporting both simulation and code generation workflows
- Handling memory safely with multiple memory management strategies
- Creating maintainable, well-documented embedded code

## Features

- **Professional code structure** with comprehensive Doxygen documentation
- **Dual memory management modes** for flexibility
- **Cross-platform support** (Windows, Linux, macOS)
- **Example implementation** with optical flow velocity estimation
- **CMake-based build system** with automatic MATLAB detection (FindMatlab.cmake)
- **Code generation support** with 4 auto-generated configuration files:
  - Include directories with full paths
  - Library directories for linker search paths
  - Library files with platform-specific extensions
  - Source files for compilation

## Project Structure

```
├── CMakeLists.txt                     # Build configuration with FindMatlab.cmake
├── cmake/
│   └── FindMatlab.cmake               # MATLAB auto-detection module
├── include/
│   └── optical_flow_velocity.hpp  # Custom class headers (Doxygen documented)
├── src/
│   ├── s_function.cpp                 # S-function implementation (fully documented)
│   └── optical_flow_velocity.cpp      # Custom class implementation
├── test_s_function.slx                # Example Simulink model
├── include_directories.txt            # Auto-generated (include paths)
├── link_directories.txt               # Auto-generated (library search paths)
├── link_libraries.txt                 # Auto-generated (library files with extensions)
├── source_files.txt                   # Auto-generated (source files)
├── LICENSE
└── README.md
```

## Prerequisites

- **MATLAB/Simulink** with Simulink Coder (R2018b or newer recommended)
- **CMake** 3.10 or higher
- **C++ compiler** compatible with your MATLAB version:
  - Windows: MSVC (Visual Studio 2017 or newer)
  - Linux: GCC 6.3 or newer
  - macOS: Clang (Xcode 9.0 or newer)
- **Third-party libraries**: OpenCV (or other libraries as needed)

## Quick Start

### 1. Clone and Build

```bash
# Clone the repository
git clone https://github.com/amaldevh/s-function.git
cd s-function

# Create build directory
mkdir build && cd build

# Configure and build (MATLAB auto-detected via FindMatlab.cmake)
cmake ..
```

The build process will:
- Auto-detect your MATLAB installation (or you can specify it manually)
- Compile the MEX S-function
- Generate 4 configuration files for Simulink Coder:
  - `include_directories.txt`
  - `link_directories.txt`
  - `link_libraries.txt`
  - `source_files.txt`

Or specify MATLAB location explicitly:

On Windows:
```powershell
cmake .. -DMATLAB_ROOT="C:/Program Files/MATLAB/R2023b"
```

On Linux/macOS:
```bash
cmake .. -DMATLAB_ROOT=/usr/local/MATLAB/R2023b
```

### 2. Use in Simulink

1. Copy the generated MEX file from `build/` to your Simulink model directory
2. Open your Simulink model
3. Add an **S-Function** block from the Simulink library
4. Set the **S-function name** to `s_function`
5. Configure the **S-function parameters**:
   - P1: Camera focal length (meters)
   - P2: Sensor height (meters)
   - P3: Sensor width (meters)
   - P4: Instance ID (unique integer)
   - P5: Image height (pixels)
   - P6: Image width (pixels)
6. Connect inputs and run the simulation

See `test_s_function.slx` for a complete example.

### 3. Building Example in Windows
1. Install OpenCV, for example the precompiled binaries from sourceforge at
[OpenCV Compiled library Sourceforge](https://sourceforge.net/projects/opencvlibrary/files/4.11.0)

2. Install the MinGW-64 C/C++ toolbox from Matlab file exchange at
 [MingW-64](https://www.mathworks.com/matlabcentral/fileexchange/52848-matlab-support-for-mingw-w64-c-c-compile)

3. Find Mingw-64 path by executing following command in Matlab window
   ```matlab
   %% Find the value of MW_MINGW64_LOC set by matlab
   mingw_path = getenv('MW_MINGW64_LOC')
   ```
5. Open Powershell and execute 
```powershell
# Extract the OpenCV installation

# Set the MW_MINGW64_LOC env var
$env:MW_MINGW64_LOC = "mingw_path"

# Clone the repository
git clone https://github.com/amaldevh/s-function.git
cd s-function

# Create build directory
mkdir build && cd build

# Configure and build (MATLAB auto-detected via FindMatlab.cmake)
cmake .. -DCMAKE_PREFIX_PATH="\path\to\opencv\build\dir"
```

## Customization Guide

### Step 1: Prepare Your Custom Code

Place your C++ source files in `src/` and headers in `include/`:

```cpp
// include/your_algorithm.hpp
/**
 * @brief Your custom algorithm class
 */
class YourAlgorithm {
public:
    YourAlgorithm(double param1, double param2);
    void processData(const std::vector<double>& input);
    std::vector<double> getResults() const;
};
```

### Step 2: Configure Memory Management

Choose one of two memory management strategies in `s_function.cpp`:

**Option A: Persistent Memory** (Recommended)
- Better for complex objects
- Explicit lifetime management
- Used by default in this template

```cpp
#define USE_PERSISTENT_MEMORY
```

**Option B: Static Objects**
- Simpler implementation
- Automatic cleanup
- Better for multiple instances

```cpp
// Comment out or remove USE_PERSISTENT_MEMORY definition
// #define USE_PERSISTENT_MEMORY
```

### Step 3: Implement S-Function Callbacks

All callbacks are documented in `src/s_function.cpp`. Key functions:

#### `mdlInitializeSizes()`
Configure ports and parameters:
```cpp
ssSetNumSFcnParams(S, num_parameters);
ssSetNumInputPorts(S, num_inputs);
ssSetNumOutputPorts(S, num_outputs);
```

#### `mdlStart()`
Initialize your objects:
```cpp
YourAlgorithm* algo = new YourAlgorithm(param1, param2);
ssSetPWorkValue(S, 0, static_cast<void*>(algo));
```

#### `mdlOutputs()`
Process data each timestep:
```cpp
YourAlgorithm* algo = static_cast<YourAlgorithm*>(ssGetPWorkValue(S, 0));
InputRealPtrsType inputs = ssGetInputPortRealSignalPtrs(S, 0);
real_T* outputs = ssGetOutputPortRealSignal(S, 0);
// Process inputs and write to outputs
```

#### `mdlTerminate()`
Clean up resources:
```cpp
YourAlgorithm* algo = static_cast<YourAlgorithm*>(ssGetPWorkValue(S, 0));
delete algo;
```

### Step 4: Configure Build Dependencies

Edit `CMakeLists.txt` to add your libraries:

```cmake
set(CUSTOM_PACKAGES
    OpenCV
    Eigen3
    YourLibrary
)

set(SRCS
    ${CMAKE_SOURCE_DIR}/src/s_function.cpp
    ${CMAKE_SOURCE_DIR}/src/your_implementation.cpp
)
```

### Step 5: Build and Test

```bash
cd build
cmake ..  # FindMatlab.cmake will auto-detect MATLAB

# Or specify MATLAB location:
# cmake .. -DMATLAB_ROOT=/path/to/matlab

# View build output
# Generated files:
#   - matlab_simulink_s_function.mex* (your S-function)
#   - include_directories.txt (include paths)
#   - link_directories.txt (library search paths)
#   - link_libraries.txt (full library paths with extensions)
#   - source_files.txt (source files)
```

### Step 6: Code Generation Setup

For Simulink Coder code generation:

1. Open **Model Configuration Parameters** in Simulink
2. Navigate to **Code Generation → Custom Code**
3. Add the contents of the generated files to the appropriate fields:
   - **Include directories**: Contents of `include_directories.txt` and `link_directories.txt`
   - **Libraries**: Contents of `link_libraries.txt`
   - **Source files**: Contents of `source_files.txt`
4. Generate code as usual

**Note:** The CMake build automatically generates these files with:
- Full library paths with extensions (`.so`, `.a`, `.lib`, `.dylib`)
- Platform-specific library directories extracted from package configurations
- All necessary include paths for headers

## Example: Optical Flow Velocity Estimation

The included example demonstrates real-time optical flow tracking for UAV applications:

**Features:**
- OpenCV integration for computer vision algorithms
- Lucas-Kanade pyramidal optical flow tracking
- Real-world velocity estimation from pixel motion
- State management across simulation timesteps
- Dual memory management support

**Inputs:**
- Port 0: Grayscale or RGB image (normalized 0-1)
- Port 1: Time delta between frames (seconds)

**Outputs:**
- Port 0: Velocity estimates (2×1000 matrix with vx, vy pairs)
- Port 1: Computation time (seconds)
- Port 2: Number of tracked features

**Algorithm:**
1. Extract Shi-Tomasi corner features
2. Track features using Lucas-Kanade optical flow
3. Convert pixel velocities to real-world velocities using camera parameters
4. Output velocity estimates for each tracked feature

## Generated Configuration Files

The CMake build process automatically generates 4 text files for Simulink Coder integration:

### 1. `include_directories.txt`
Contains full paths to include directories for header files.

**Example content:**
```
/usr/include/opencv4
/home/user/project/include
```

**Simulink field:** Code Generation → Custom Code → Include directories

### 2. `link_directories.txt`
Contains library search paths where linker should look for libraries.

**Example content:**
```
/usr/lib/x86_64-linux-gnu
/usr/local/lib
```

**Simulink field:** Code Generation → Custom Code → Library search path

### 3. `link_libraries.txt`
Contains full library file paths with platform-specific extensions.

**Example content (Linux):**
```
/usr/lib/x86_64-linux-gnu/libopencv_core.so
/usr/lib/x86_64-linux-gnu/libopencv_imgproc.so
/usr/lib/x86_64-linux-gnu/libopencv_video.so
```

**Example content (Windows):**
```
C:/opencv/lib/opencv_core454.lib
C:/opencv/lib/opencv_imgproc454.lib
```

**Simulink field:** Code Generation → Custom Code → Libraries

### 4. `source_files.txt`
Contains paths to all source files that need to be compiled.

**Example content:**
```
/home/user/project/src/s_function.cpp
/home/user/project/src/optical_flow_velocity.cpp
```

**Simulink field:** Code Generation → Custom Code → Source files

## Architecture and Design

### Memory Management Modes

This template supports two memory management strategies:

| Feature | Persistent Memory | Static Objects |
|---------|------------------|----------------|
| **Complexity** | More explicit | Simpler |
| **Cleanup** | Manual (mdlTerminate) | Automatic |
| **Multi-instance** | Requires unique IDs | Native support via map |
| **Use case** | Single complex object | Multiple lightweight instances |

### Code Quality Features

- **Comprehensive documentation**: All functions documented with Doxygen
- **Memory safety**: No memory leaks, proper RAII where applicable
- **Error handling**: Try-catch blocks for OpenCV exceptions
- **Type safety**: Explicit type conversions, const-correctness
- **Cross-platform**: Tested on Windows, Linux, and macOS

## Troubleshooting

### Build Issues

**Problem**: `MATLAB_BIN_DIR not set`
```
Solution: Specify MATLAB path explicitly:
  cmake .. -DMATLAB_BIN_DIR=/path/to/matlab/bin
```

**Problem**: `Could not find OpenCV`
```
Solution: Install OpenCV or specify OpenCV_DIR:
  cmake .. -DOpenCV_DIR=/path/to/opencv/build
```

**Problem**: Compiler not compatible with MATLAB
```
Solution: Check MATLAB documentation for supported compilers:
  mex -setup C++
```

### Runtime Issues

**Problem**: `Optical flow tracker not initialized`
```
Solution: Ensure mdlStart() completes successfully. Check MATLAB console for errors.
```

**Problem**: Incorrect output dimensions
```
Solution: Verify output port dimensions match your algorithm's output size.
```

**Problem**: Memory leaks in simulation
```
Solution: Ensure mdlTerminate() properly deletes all allocated objects.
```

### Simulink Integration

**Problem**: S-function not found in Simulink
```
Solution:
  1. Copy MEX file to model directory
  2. Add build directory to MATLAB path: addpath('build')
```

**Problem**: Parameter mismatch error
```
Solution: Verify number of S-function parameters matches ssSetNumSFcnParams() call.
```

## Best Practices

1. **Always initialize all member variables** in constructors
2. **Use const-correctness** for parameters that shouldn't be modified
3. **Handle exceptions** from third-party libraries (e.g., OpenCV)
4. **Document all parameters** with units and ranges
5. **Zero-initialize output buffers** to avoid undefined behavior
6. **Test with multiple instances** if using static memory mode
7. **Validate input dimensions** before processing

## Performance Tips

- Use `-O3` optimization in MEX compilation for production code
- Pre-allocate buffers in `mdlStart()` to avoid repeated allocations
- Profile using MATLAB Profiler to identify bottlenecks
- Consider using MEX function caching for frequently called operations
- Use appropriate data types (prefer `float` over `double` when precision allows)

## Support

If you find this repository helpful, please ⭐ star it!

[![GitHub stars](https://img.shields.io/github/stars/AmalDevHaridevan/matlab-simulink-s-function.svg?style=social&label=Star)](https://github.com/AmalDevHaridevan/matlab-simulink-s-function/stargazers)

## Additional Resources

### Official Documentation
- [MATLAB S-Function Documentation](https://www.mathworks.com/help/simulink/sfg/what-is-an-s-function.html)
- [Writing C MEX S-Functions](https://www.mathworks.com/help/simulink/sfg/writing-c-mex-s-functions.html)
- [Simulink Coder User's Guide](https://www.mathworks.com/help/rtw/)

### Code Generation
- [Custom Code Integration](https://www.mathworks.com/help/rtw/ug/custom-code-integration.html)
- [S-Function Code Generation](https://www.mathworks.com/help/rtw/ug/s-functions-and-code-generation.html)

### Community
- [MATLAB Central File Exchange](https://www.mathworks.com/matlabcentral/fileexchange/)
- [Simulink Answers](https://www.mathworks.com/matlabcentral/answers/)

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Follow the existing code style and documentation standards
4. Test your changes thoroughly
5. Submit a pull request with a clear description

## License

See the LICENSE file for details.
