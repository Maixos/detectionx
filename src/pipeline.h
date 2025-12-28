#pragma once

#include <rtspx/rtspx.h>
#include <mqttx/client.h>
#include <vcodecx/manager.h>
#include <toolkitx/vision/region.h>

#include "config.h"

namespace detectionx {
    class Pipeline {
    public:
        Pipeline() = default;

        Pipeline(
                TaskConfig task_config, const std::shared_ptr<vcodecx::Manager> &codec_manager,
                const std::shared_ptr<rtspx::MediaSession> &session, const std::shared_ptr<mqttx::Client> &mqtt_client
        );

        ~Pipeline();

        void stop();

    private:
        void process() const;

        void on_encoded(const std::shared_ptr<vcodecx::EncodedX> &encodedx) const;

    private:
        TaskConfig task_config_{};

        vision::Region region_{};
        std::atomic<bool> stopped_{false};

        std::thread processor_{};
        std::shared_ptr<vcodecx::Decoder> decoder_;
        std::shared_ptr<vcodecx::Encoder> encoder_;
        std::shared_ptr<vcodecx::Manager> codec_manager_;

        std::shared_ptr<mqttx::Client> mqtt_client_;
        std::shared_ptr<rtspx::MediaSession> session_;
    };
}
