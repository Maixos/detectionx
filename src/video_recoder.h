#pragma once

#include <string>
#include <mutex>

#include <opencv2/opencv.hpp>

class VideoRecorder {
public:
    VideoRecorder() = default;

    ~VideoRecorder() {
        stop();
    }

    bool start(
        const std::string &path, const int width, const int height, const int fps = 30,
        const int codec = cv::VideoWriter::fourcc('M','J','P','G')
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (writer_.isOpened()) return false;

        path_ = path;
        writer_.open(path_, codec, fps, cv::Size(width, height), true);
        if (!writer_.isOpened()) {
            return false;
        }
        recording_ = true;
        return true;
    }

    void write(const cv::Mat &frame) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (recording_ && !frame.empty()) {
            writer_.write(frame);
        }
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (recording_) {
            writer_.release();
            recording_ = false;
        }
    }

    bool isRecording() const {
        return recording_;
    }

    std::string path() const { return path_; }

private:
    cv::VideoWriter writer_;
    bool recording_ = false;
    std::string path_;
    mutable std::mutex mutex_;
};
