/**
 * @file optical_flow_uav_velocity.hpp
 * @brief Optical flow-based velocity estimation for UAV applications
 *
 * This file defines the OpticalFlowTracking class which implements Lucas-Kanade
 * optical flow tracking to estimate real-world velocity from camera motion.
 */

#ifndef OPTICAL_FLOW_UAV_VELOCITY_HPP
#define OPTICAL_FLOW_UAV_VELOCITY_HPP

#include <opencv2/video.hpp>
#include <opencv2/video/tracking.hpp>
#include <cmath>
#include <vector>
#include <tuple>

/**
 * @class OpticalFlowTracking
 * @brief Tracks optical flow features to estimate camera/UAV velocity
 *
 * This class implements optical flow tracking using the Lucas-Kanade method to
 * detect and track features across consecutive frames. It converts pixel-space
 * motion to real-world velocity estimates based on camera parameters and height
 * above ground.
 *
 * @note The class assumes a downward-facing camera configuration typical of UAVs.
 */
class OpticalFlowTracking {
public:
    /**
     * @brief Feature extraction performed dynamically at each frame
     */
    static constexpr int FEATURE_EXTRACTION_PROCEDURE_DYNAMIC = 1;

    /**
     * @brief Feature extraction performed once at initialization
     */
    static constexpr int FEATURE_EXTRACTION_PROCEDURE_ONCE = 2;

    /**
     * @brief Use Lucas-Kanade pyramidal optical flow method
     */
    static constexpr int OPTICAL_FLOW_LUCAS_KANADE = 100;

    /**
     * @brief Use OpenCV's simple feature extraction (goodFeaturesToTrack)
     */
    static constexpr int FEATURE_EXTRACTION_OPENCV_SIMPLE = 1000;

    /**
     * @brief Constructor for OpticalFlowTracking
     *
     * @param method Optical flow method to use (currently only OPTICAL_FLOW_LUCAS_KANADE supported)
     * @param delta_t Initial time step between frames in seconds
     * @param camera_focal_length Camera focal length in meters
     * @param cmos_width Physical width of the camera sensor in meters
     * @param cmos_height Physical height of the camera sensor in meters
     *
     * @note Field of view is automatically calculated from camera parameters
     */
    OpticalFlowTracking(int method, float delta_t, float camera_focal_length,
                       float cmos_width, float cmos_height);

    /**
     * @brief Update the time step between frames
     *
     * This should be called when the frame rate changes or to provide accurate
     * timing information for velocity estimation.
     *
     * @param delta_t_ Time step in seconds between consecutive frames
     */
    void _set_delta_t_(double delta_t_);

    /**
     * @brief Check if features are currently being tracked
     *
     * @return true if features have been extracted and are available, false otherwise
     */
    bool _has_features();

    /**
     * @brief Extract trackable features from an input image
     *
     * Uses Shi-Tomasi corner detection to identify up to 1000 good features
     * to track in the image.
     *
     * @param img Input image (grayscale or BGR)
     *
     * @note If the image has 3 channels, it will be converted to grayscale
     * @note Previous features are cleared when this method is called
     */
    void extractFeatures(const cv::Mat &img);

    /**
     * @brief Calculate real-world velocity from optical flow
     *
     * Tracks features from the previous frame to the current frame using
     * Lucas-Kanade optical flow, then converts pixel motion to real-world
     * velocity based on camera height and parameters.
     *
     * @param img Current grayscale image frame
     * @param height Height of the camera above the ground in meters
     *
     * @return Tuple containing:
     *         - v_est_x: Vector of estimated velocities in X direction (m/s)
     *         - v_est_y: Vector of estimated velocities in Y direction (m/s)
     *         - new_features: Tracked feature locations in current frame
     *         - old_features: Feature locations in previous frame
     *         - success: Boolean indicating successful processing
     *
     * @note Returns empty vectors on first call (no previous frame available)
     * @note Features are re-extracted after each calculation
     */
    std::tuple<std::vector<float>, std::vector<float>,
               std::vector<cv::Point2f>, std::vector<cv::Point2f>, bool>
    calculateRealVel(const cv::Mat &img, float height);

private:
    int _method;                    ///< Optical flow method identifier
    float _delta_t;                 ///< Time step between frames (seconds)
    float _focal_length;            ///< Camera focal length (meters)
    float _cmos_width;              ///< Sensor physical width (meters)
    float _cmos_height;             ///< Sensor physical height (meters)
    float _fov_h;                   ///< Horizontal field of view (radians)
    float _fov_v;                   ///< Vertical field of view (radians)
    int _img_width;                 ///< Image width in pixels
    int _img_height;                ///< Image height in pixels
    int _debug_count;               ///< Debug counter for frame tracking
    cv::Mat _last_im;               ///< Previous frame for optical flow
    std::vector<cv::Point2f> _features; ///< Currently tracked feature points

    /**
     * @brief Termination criteria for iterative optical flow algorithm
     *
     * Stops after 8 iterations or when change is less than 0.03 pixels
     */
    cv::TermCriteria _criteria = cv::TermCriteria(
        (cv::TermCriteria::COUNT) + (cv::TermCriteria::EPS), 8, 0.03);
};

#endif // OPTICAL_FLOW_UAV_VELOCITY_HPP