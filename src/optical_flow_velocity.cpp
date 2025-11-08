/**
 * @file optical_flow_velocity.cpp
 * @brief Implementation of optical flow-based velocity estimation
 *
 * This file implements the OpticalFlowTracking class methods for tracking
 * features and estimating real-world velocity from optical flow.
 */

#include "optical_flow_velocity.hpp"
#include <opencv2/video.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <vector>
#include <tuple>

OpticalFlowTracking::OpticalFlowTracking(int method, float delta_t,
                                         float camera_focal_length,
                                         float cmos_width, float cmos_height)
    : _method(method),
      _delta_t(delta_t),
      _focal_length(camera_focal_length),
      _cmos_width(cmos_width),
      _cmos_height(cmos_height),
      _fov_h(0.0f),
      _fov_v(0.0f),
      _img_width(0),
      _img_height(0),
      _debug_count(0),
      _last_im(),
      _features() {

    // Calculate horizontal and vertical field of view from camera sensor dimensions
    // FOV = 2 * arctan(sensor_size / (2 * focal_length))
    // This allows converting pixel velocities to angular velocities
    _fov_h = 2.0f * std::atan(_cmos_width / (2.0f * _focal_length));
    _fov_v = 2.0f * std::atan(_cmos_height / (2.0f * _focal_length));
}

void OpticalFlowTracking::_set_delta_t_(double delta_t_) {
    // Update the time step used for velocity calculations
    // Convert from double to float for internal storage
    _delta_t = static_cast<float>(delta_t_);
}

bool OpticalFlowTracking::_has_features() {
    // Check if there are any tracked features available
    return !_features.empty();
}

void OpticalFlowTracking::extractFeatures(const cv::Mat &img) {
    cv::Mat gray;

    // Convert to grayscale if necessary (optical flow works on single-channel images)
    if (img.channels() == 3) {
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = img.clone();
    }

    // Clear previous features and detect new ones using Shi-Tomasi corner detection
    // Parameters: max 1000 features, quality level 0.1, min distance 8 pixels, block size 2
    _features.clear();
    cv::goodFeaturesToTrack(gray, _features, 1000, 0.1, 8.0, cv::Mat(), 2);

    // If no features found, exit early
    if (!_has_features()) {
        return;
    }

    // Store the current frame for next iteration's optical flow calculation
    _last_im = gray;
    _img_width = gray.cols;
    _img_height = gray.rows;
}

std::tuple<std::vector<float>, std::vector<float>,
           std::vector<cv::Point2f>, std::vector<cv::Point2f>, bool>
OpticalFlowTracking::calculateRealVel(const cv::Mat &img, float height) {

    // On first call, no previous frame exists - initialize and return empty
    if (_last_im.empty()) {
        extractFeatures(img);
        return {{}, {}, {}, {}, true};
    }

    // Convert current image to grayscale for optical flow processing
    cv::Mat current_gray;
    if (img.channels() == 3) {
        cv::cvtColor(img, current_gray, cv::COLOR_BGR2GRAY);
    } else {
        current_gray = img.clone();
    }

    _debug_count++;

    // Track features from previous frame to current frame using Lucas-Kanade method
    // Pyramidal implementation with window size 16x16, max pyramid level 2
    std::vector<uchar> status;
    std::vector<float> error;
    std::vector<cv::Point2f> new_features;

    cv::calcOpticalFlowPyrLK(_last_im, current_gray, _features, new_features,
                            status, error, cv::Size(16, 16), 2, _criteria);

    // Calculate pixel velocities for successfully tracked features
    std::vector<float> vels_x, vels_y;
    std::vector<cv::Point2f> old_features;

    for (size_t i = 0; i < new_features.size(); ++i) {
        if (status[i]) {
            // Compute pixel displacement
            float dx = new_features[i].x - _features[i].x;
            float dy = new_features[i].y - _features[i].y;

            // Convert to pixel velocities
            // Note: dy maps to X (forward) and -dx maps to Y (left) for UAV coordinate system
            vels_x.push_back(dy / _delta_t);
            vels_y.push_back(-dx / _delta_t);
            old_features.push_back(_features[i]);
        }
    }

    // Extract fresh features for next iteration and update stored frame
    _features.clear();
    cv::goodFeaturesToTrack(current_gray, _features, 1000, 0.1, 8.0, cv::Mat(), 2);
    _last_im = current_gray;

    // Convert pixel velocities to real-world velocities using camera height and FOV
    std::vector<float> v_est_x, v_est_y;
    for (size_t i = 0; i < vels_x.size(); ++i) {
        // Convert pixel velocity to angular velocity using field of view
        float angular_vel_x = vels_x[i] * _fov_v / static_cast<float>(_img_height);
        float angular_vel_y = vels_y[i] * _fov_h / static_cast<float>(_img_width);

        // Convert angular velocity to linear velocity: v = h * tan(θ) / Δt
        // where θ is the angular displacement and h is the height above ground
        v_est_x.push_back(height * std::tan(angular_vel_x * _delta_t) / _delta_t);
        v_est_y.push_back(height * std::tan(angular_vel_y * _delta_t) / _delta_t);
    }

    return {v_est_x, v_est_y, new_features, old_features, true};
}
