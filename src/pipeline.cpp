#include "pipeline.h"

#include <opencv2/opencv.hpp>

#include <toolkitx/logx/logx.h>

namespace detectionx {
    Pipeline::Pipeline(
            TaskConfig task_config, const std::shared_ptr<vcodecx::Manager> &codec_manager,
            const std::shared_ptr<inferencex::detection::YOLO11Engine> &detector,
            const std::shared_ptr<rtspx::MediaSession> &session, const std::shared_ptr<mqttx::Client> &mqtt_client
    ) : task_config_(std::move(task_config)), codec_manager_(codec_manager), detector_(detector),
        mqtt_client_(mqtt_client),session_(session) {
        const GConfig &g_config = GConfig::get_instance();

        const int fps = 30;
        const int width = g_config.rtsp_config_.width;
        const int height = g_config.rtsp_config_.height;

        region_ = vision::Region(cv::Rect(0, 0, width, height), width, height);

        if (task_config_.type == "xyxy") {
            if (task_config_.values.size() == 4) {
                const int x1 = task_config_.values[0];
                const int y1 = task_config_.values[1];
                const int x2 = task_config_.values[2];
                const int y2 = task_config_.values[3];
                const cv::Rect rect(x1, y1, x2 - x1, y2 - y1);
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
                LOG_WARN("pipeline", "polygon region requires N pairs but got %zu", task_config.values.size());
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

        const vcodecx::EncodeConfig encode_cfg{
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
        const auto& class_names = detector_->get_metadata().class_names;

        while (!stopped_ && !decoder_->is_released()) {
            std::shared_ptr<vcodecx::FrameX> framex{};
            if (!decoder_->read(framex, 10)) {
                continue;
            }

            cv::Mat image(cv::Size(framex->width, framex->height), CV_8UC3, framex->ptr);
            auto fut = detector_->commit(image);

            if (fut.wait_for(std::chrono::milliseconds(30)) == std::future_status::ready) {
                auto results = fut.get();
                for (const auto& det : results) {
                    cv::rectangle(image, det.bbox.rect, {0, 255, 0}, 2);
                    char text[64];
                    snprintf(text, sizeof(text), "%s %.2f", class_names[det.class_id].c_str(), det.score);
                    cv::putText(
                        image, text, cv::Point2f(det.bbox.x1(), det.bbox.y1() - 5.),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6,
                        {0, 255, 0}, 1, cv::LINE_AA
                    );
                }
            }

            encoder_->write(framex, 3);
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
