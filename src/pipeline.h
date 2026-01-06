#pragma once

#include <rtspx/rtspx.h>
#include <mqttx/client.h>
#include <vcodecx/manager.h>
#include <toolkitx/vision/region.h>
#include <toolkitx/concurrent/queuex.h>
#include <inferencex/engines/detection/yolo11.h>

#include "config.h"

namespace detectionx {
    struct DetectionTask {
        std::shared_ptr<vcodecx::FrameX> framex;
        std::shared_future<inferencex::Detection2DResults> handle;
    };

    class Pipeline {
    public:
        explicit Pipeline(
            TaskConfig task_config,
            const std::shared_ptr<inferencex::InferenceX<cv::Mat, inferencex::Detection2DResults>>& detector,
            const std::shared_ptr<vcodecx::Manager>& codec_manager,
            const std::shared_ptr<rtspx::MediaSession>& rtsp_session,
            const std::shared_ptr<mqttx::Client>& mqtt_client
        );

        ~Pipeline();

        void release();

        static std::shared_ptr<Pipeline> create(
            const TaskConfig& task_config,
            const std::shared_ptr<inferencex::InferenceX<cv::Mat, inferencex::Detection2DResults>>& detector,
            const std::shared_ptr<vcodecx::Manager>& codec_manager,
            const std::shared_ptr<rtspx::MediaSession>& rtsp_session,
            const std::shared_ptr<mqttx::Client>& mqtt_client
        );

    private:
        bool startup();

        void detect_thread() const;

        void process_thread() const;

        bool init_region();

        bool init_codec();

        void shutdown();

        void on_encoded(const std::shared_ptr<vcodecx::EncodedX>& encodedx) const;

    private:
        TaskConfig task_config_{};
        std::shared_ptr<inferencex::InferenceX<cv::Mat, inferencex::Detection2DResults>> detector_{};
        std::shared_ptr<vcodecx::Manager> codec_manager_{};
        std::shared_ptr<rtspx::MediaSession> rtsp_session_{};
        std::shared_ptr<mqttx::Client> mqtt_client_{};

        vision::Region region_{};
        std::atomic<bool> stopped_{true};

        std::thread detect_thread_{};
        std::thread process_thread_{};
        std::shared_ptr<vcodecx::Decoder> decoder_{};
        std::shared_ptr<vcodecx::Encoder> encoder_{};

        std::shared_ptr<toolkitx::concurrent::BlockingQueue<DetectionTask>> detection_queue_{};
    };
}
