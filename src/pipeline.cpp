#include "pipeline.h"

#include <opencv2/opencv.hpp>

#include <toolkitx/logx/logx.h>

namespace detectionx {
    Pipeline::Pipeline(
            TaskConfig cfg, const std::shared_ptr<vcodecx::Manager> &codec_manager,
            const std::shared_ptr<rtspx::MediaSession> &session, const std::shared_ptr<mqttx::Client> &mqtt_client
    ) : task_config_(std::move(cfg)), codec_manager_(codec_manager), mqtt_client_(mqtt_client),
        session_(session) {
        GConfig &g_config = GConfig::get_instance();

        int fps = 30;
        int width = g_config.rtsp_config_.width;
        int height = g_config.rtsp_config_.height;

        region_ = vision::Region(cv::Rect(0, 0, width, height), width, height);

        if (task_config_.type == "xyxy") {
            if (task_config_.values.size() == 4) {
                int x1 = task_config_.values[0];
                int y1 = task_config_.values[1];
                int x2 = task_config_.values[2];
                int y2 = task_config_.values[3];
                cv::Rect rect(x1, y1, x2 - x1, y2 - y1);
                region_ = vision::Region(rect, width, height);
            } else {
                LOG_WARN("pipeline", "xyxy region requires 4 floats but got %zu", task_config_.values.size());
            }
        } else if (task_config_.type == "polygon") {
            if (task_config_.values.size() % 2 == 0 && !task_config_.values.empty()) {
                std::vector<cv::Point2f> pts;
                pts.reserve(task_config_.values.size() / 2);

                for (size_t i = 0; i < task_config_.values.size(); i += 2) {
                    pts.emplace_back(task_config_.values[i], task_config_.values[i + 1]);
                }

                region_ = vision::Region(pts, width, height);
            } else {
                LOG_WARN("pipeline", "polygon region requires N pairs but got %zu", cfg.values.size());
            }
        }

        const vcodecx::StreamInfo stream_info{task_config_.id, task_config_.uri};
        const vcodecx::DecodeConfig decode_cfg{
                width, height, vcodecx::ImageFormat::BGR24, vcodecx::WorkerMode::Polling, fps, 3
        };
        decoder_ = codec_manager->create_decoder(stream_info, decode_cfg);
        if (!decoder_) {
            LOG_ERROR("pipeline", "failed to create decoder");
            return;
        }

        vcodecx::EncodeConfig encode_cfg{
                width, height, vcodecx::WorkerMode::Callback, fps, 3, vcodecx::CodecType::H265
        };
        encoder_ = codec_manager->create_encoder(encode_cfg);
        if (!decoder_) {
            LOG_ERROR("pipeline", "failed to create encoder");
            return;
        }

        encoder_->subscribe([this](const auto &e) { on_encoded(e); });

        processor_ = std::thread(&Pipeline::process, this);
    }

    Pipeline::~Pipeline() {
        stop();
    }

    void Pipeline::stop() {
        if (stopped_.exchange(true)) return;

        if (processor_.joinable()) {
            processor_.join();
        }

        if (decoder_) decoder_->release();
        if (encoder_) encoder_->release();
        LOG_INFO("pipeline", "task pipeline %s stopped", task_config_.id.c_str());
    }

    void Pipeline::process() const {
        while (!stopped_ && !decoder_->is_released()) {
            std::shared_ptr<vcodecx::FrameX> framex;
            if (decoder_->read(framex, 10)) {
                encoder_->write(framex, 3);
            }
        }
    }

    void Pipeline::on_encoded(const std::shared_ptr<vcodecx::EncodedX> &encodedx) const {
        if (encodedx->size == 0) return;

        rtspx::EncodedShared packet{};
        packet.frame_type = encodedx->is_keyframe ? rtspx::VIDEO_FRAME_I : rtspx::VIDEO_FRAME_P;
        packet.size = encodedx->size;
        packet.pts = encodedx->pts;
        packet.data = encodedx->data;
        packet.holder = encodedx->holder;

        session_->push_data(rtspx::MediaTrack::Video, packet);
    }
};
