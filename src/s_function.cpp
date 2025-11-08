/**
 * @file matlab_simulink_s_function.cpp
 * @brief MATLAB/Simulink S-Function for optical flow velocity estimation
 *
 * This S-Function integrates the OpticalFlowTracking class with Simulink,
 * enabling real-time velocity estimation from camera images within Simulink models.
 *
 * @section inputs Inputs
 * - Port 0: Image matrix (height x width, normalized 0-1)
 * - Port 1: Delta time (scalar, seconds between frames)
 *
 * @section outputs Outputs
 * - Port 0: Velocity estimates (2 x 1000 matrix, [vx; vy] for each feature)
 * - Port 1: Calculation time (scalar, seconds)
 * - Port 2: Number of valid samples (scalar)
 *
 * @section parameters Parameters
 * - P(0): Camera focal length (meters)
 * - P(1): CMOS sensor height (meters)
 * - P(2): CMOS sensor width (meters)
 * - P(3): Unique instance ID (for multi-instance support)
 * - P(4): Image height (pixels)
 * - P(5): Image width (pixels)
 */

#define S_FUNCTION_NAME s_function
#define S_FUNCTION_LEVEL 2
#define MDL_START
#define USE_PERSISTENT_MEMORY

#include "simstruc.h"
#include "optical_flow_velocity.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <algorithm>

/**
 * @brief Clamp a value between minimum and maximum bounds
 *
 * @tparam T Numeric type
 * @param value Value to clamp
 * @param minVal Minimum allowed value
 * @param maxVal Maximum allowed value
 * @return Clamped value
 */
template <typename T>
T clamp(T value, T minVal, T maxVal) {
    return (value < minVal) ? minVal : (value > maxVal) ? maxVal : value;
}

#ifndef USE_PERSISTENT_MEMORY
/**
 * @brief Static storage for multiple S-Function instances
 *
 * When not using persistent memory, these maps store tracker instances
 * and temporary image buffers indexed by unique instance IDs.
 */
static std::map<int, std::shared_ptr<OpticalFlowTracking>> obj_map;
static std::map<int, std::shared_ptr<cv::Mat>> img_map;
#endif

/**
 * @brief Initialize S-Function sizes and properties
 *
 * Configures the number and dimensions of input/output ports, parameters,
 * and sample times. This is called once during model compilation.
 *
 * @param S SimStruct pointer containing S-Function state
 */
static void mdlInitializeSizes(SimStruct* S) {
    // Verify that exactly 6 parameters are provided to the S-function
    ssSetNumSFcnParams(S, 6);
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
        return;
    }

    // Extract image dimensions and instance ID from parameters
    const int height = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 4)));
    const int width = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 5)));
    const int instance_id = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 3)));

    // Configure input ports
    // Port 0: Image matrix (height × width), direct feedthrough required
    // Port 1: Delta time scalar, direct feedthrough required
    if (!ssSetNumInputPorts(S, 2)) {
        return;
    }
    ssSetInputPortMatrixDimensions(S, 0, height, width);
    ssSetInputPortDirectFeedThrough(S, 0, 1);

    ssSetInputPortWidth(S, 1, 1);
    ssSetInputPortDirectFeedThrough(S, 1, 1);

    // Configure output ports
    // Port 0: Velocity estimates (2 × 1000 for vx, vy pairs)
    // Port 1: Computation time (scalar)
    // Port 2: Number of valid features (scalar)
    if (!ssSetNumOutputPorts(S, 3)) {
        return;
    }
    ssSetOutputPortMatrixDimensions(S, 0, 2, 1000);
    ssSetOutputPortWidth(S, 1, 1);
    ssSetOutputPortWidth(S, 2, 1);

    ssSetNumSampleTimes(S, 1);

#ifdef USE_PERSISTENT_MEMORY
    // Reserve 2 persistent work pointers: one for tracker, one for image buffer
    ssSetNumPWork(S, 2);
#endif

    ssSetOperatingPointCompliance(S, USE_DEFAULT_OPERATING_POINT);
    ssSetRuntimeThreadSafetyCompliance(S, RUNTIME_THREAD_SAFETY_COMPLIANCE_TRUE);
    ssSetOptions(S,
                 SS_OPTION_WORKS_WITH_CODE_REUSE |
                 SS_OPTION_EXCEPTION_FREE_CODE |
                 SS_OPTION_USE_TLC_WITH_ACCELERATOR);
}
/**
 * @brief Initialize sample times for the S-Function
 *
 * Configures the S-Function to inherit its sample time from the Simulink model.
 *
 * @param S SimStruct pointer containing S-Function state
 */
static void mdlInitializeSampleTimes(SimStruct* S) {
    // Inherit sample time from the Simulink model
    // This allows the S-function to run at whatever rate the model specifies
    ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0.0);
    ssSetModelReferenceSampleTimeDefaultInheritance(S);
}

/**
 * @brief Initialize the optical flow tracker
 *
 * Called once at the start of simulation to create and configure
 * the OpticalFlowTracking instance with camera parameters.
 *
 * @param S SimStruct pointer containing S-Function state
 */
static void mdlStart(SimStruct* S) {
    // Extract camera parameters from S-function block parameters
    const double focal_length = mxGetScalar(ssGetSFcnParam(S, 0));
    const double cmos_width = mxGetScalar(ssGetSFcnParam(S, 2));
    const double cmos_height = mxGetScalar(ssGetSFcnParam(S, 1));
    const int instance_id = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 3)));
    const int height = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 4)));
    const int width = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 5)));

#ifndef USE_PERSISTENT_MEMORY
    // Static memory mode: Store tracker and image buffer in global maps indexed by instance ID
    // This allows multiple S-function blocks to coexist in the same model
    obj_map[instance_id] = std::make_shared<OpticalFlowTracking>(
        100, 1.0f, static_cast<float>(focal_length),
        static_cast<float>(cmos_width), static_cast<float>(cmos_height));
    img_map[instance_id] = std::make_shared<cv::Mat>(height, width, CV_8UC1, cv::Scalar(0));
#else
    // Persistent memory mode: Allocate tracker and image buffer on heap
    // Method 100 = Lucas-Kanade optical flow, initial delta_t = 1.0 second
    OpticalFlowTracking* tracker = new OpticalFlowTracking(
        100, 1.0f, static_cast<float>(focal_length),
        static_cast<float>(cmos_width), static_cast<float>(cmos_height));

    if (tracker == nullptr) {
        ssSetErrorStatus(S, "Failed to instantiate OpticalFlowTracking object.");
        return;
    }

    // Allocate image buffer for converting Simulink data to OpenCV format
    cv::Mat* image = new cv::Mat(height, width, CV_8UC1);

    // Store pointers in persistent work vector for access in mdlOutputs
    ssSetPWorkValue(S, 0, static_cast<void*>(tracker));
    ssSetPWorkValue(S, 1, static_cast<void*>(image));
#endif
}

/**
 * @brief Main computation function called at each simulation step
 *
 * Receives image data from Simulink, converts it to OpenCV format,
 * performs optical flow tracking, and outputs velocity estimates.
 *
 * @param S SimStruct pointer containing S-Function state
 * @param tid Task ID (unused for single-tasking)
 */
static void mdlOutputs(SimStruct* S, int_T tid) {
    // Retrieve image dimensions and instance ID
    const int height = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 4)));
    const int width = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 5)));
    const int instance_id = static_cast<int>(mxGetScalar(ssGetSFcnParam(S, 3)));

#ifdef USE_PERSISTENT_MEMORY
    // Persistent memory mode: Retrieve pointers from work vector
    OpticalFlowTracking* tracker = static_cast<OpticalFlowTracking*>(ssGetPWorkValue(S, 0));
    cv::Mat* image = static_cast<cv::Mat*>(ssGetPWorkValue(S, 1));
#else
    // Static memory mode: Retrieve from global maps
    std::shared_ptr<cv::Mat> image = img_map[instance_id];
    std::shared_ptr<OpticalFlowTracking> tracker = obj_map[instance_id];
#endif

    // Validate that tracker was properly initialized
    if (tracker == nullptr) {
        ssSetErrorStatus(S, "Optical flow tracker not initialized.");
        return;
    }
    if (image == nullptr) {
        ssSetErrorStatus(S, "Image buffer not initialized.");
        return;
    }

    // Start timing for performance measurement
    auto start_time = std::chrono::high_resolution_clock::now();

    // Get pointers to input signals from Simulink
    InputRealPtrsType input_image = ssGetInputPortRealSignalPtrs(S, 0);
    InputRealPtrsType delta_t_ptr = ssGetInputPortRealSignalPtrs(S, 1);

    // Update time step for velocity calculation
    tracker->_set_delta_t_(delta_t_ptr[0][0]);

    // Convert Simulink image data (normalized 0-1) to OpenCV format (0-255)
    // Simulink stores matrices in column-major order, so we iterate accordingly
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            image->at<uchar>(r, c) = static_cast<uchar>(
                clamp(input_image[r + c * height][0] * 255.0, 0.0, 255.0));
        }
    }

    // Perform optical flow velocity estimation
    std::vector<float> vel_x, vel_y;

    try {
        // Calculate real-world velocities assuming 1.0m height above ground
        // (height can be made a parameter if needed)
        auto result = tracker->calculateRealVel(*image, 1.0f);
        vel_x = std::get<0>(result);
        vel_y = std::get<1>(result);
    } catch (const cv::Exception& e) {
        // Catch OpenCV exceptions and issue warning instead of crashing simulation
        ssWarning(S, e.what());
        vel_x.clear();
        vel_y.clear();
    }

    // Get output port dimensions
    const int_T num_rows = ssGetOutputPortDimensions(S, 0)[0];
    const int_T num_cols = ssGetOutputPortDimensions(S, 0)[1];

    // Get pointers to output signals
    real_T* output_velocities = ssGetOutputPortRealSignal(S, 0);
    real_T* computation_time = ssGetOutputPortRealSignal(S, 1);
    real_T* num_features = ssGetOutputPortRealSignal(S, 2);

    // Determine how many valid features were tracked
    const int valid_features = std::min(static_cast<int>(vel_x.size()), static_cast<int>(num_cols));

    // Write velocity estimates to output (column-major order)
    for (int i = 0; i < valid_features; i++) {
        output_velocities[0 + i * num_rows] = vel_x[i];
        output_velocities[1 + i * num_rows] = vel_y[i];
    }

    // Zero out unused columns to avoid undefined output values
    for (int i = valid_features; i < num_cols; i++) {
        output_velocities[0 + i * num_rows] = 0.0;
        output_velocities[1 + i * num_rows] = 0.0;
    }

    // Calculate and report computation time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double>(end_time - start_time).count();

    *computation_time = elapsed;
    *num_features = static_cast<double>(valid_features);
}

/**
 * @brief Clean up resources when simulation ends
 *
 * Deallocates the OpticalFlowTracking instance when using persistent memory.
 * Static memory instances are automatically cleaned up.
 *
 * @param S SimStruct pointer containing S-Function state
 */
static void mdlTerminate(SimStruct* S) {
#ifdef USE_PERSISTENT_MEMORY
    // Clean up heap-allocated tracker object
    OpticalFlowTracking* tracker = static_cast<OpticalFlowTracking*>(ssGetPWorkValue(S, 0));
    if (tracker != nullptr) {
        delete tracker;
        ssSetPWorkValue(S, 0, nullptr);
    }

    // Clean up heap-allocated image buffer
    cv::Mat* image = static_cast<cv::Mat*>(ssGetPWorkValue(S, 1));
    if (image != nullptr) {
        delete image;
        ssSetPWorkValue(S, 1, nullptr);
    }
#else
    // In static memory mode, shared_ptr automatically handles cleanup
    // No explicit action needed here
#endif
}

#ifdef MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
