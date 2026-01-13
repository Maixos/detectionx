#include "pipeline.h"

#include <opencv2/opencv.hpp>
#include <toolkitx/logx/logx.h>
#include <toolkitx/vision/visualization.h>

namespace detectionx {
    using namespace vcodecx;

    Pipeline::Pipeline(
        TaskConfig task_config,
        const std::shared_ptr<inferencex::InferenceX<inferencex::ImageX, inferencex::Detection2DResults> > &detector,
        const std::shared_ptr<vcodecx::Manager> &codec_manager,
        const std::shared_ptr<rtspx::MediaSession> &rtsp_session,
        const std::shared_ptr<mqttx::Client> &mqtt_client
    ) : task_config_(std::move(task_config)), detector_(detector), codec_manager_(codec_manager),
        rtsp_session_(rtsp_session), mqtt_client_(mqtt_client) {
    }

    Pipeline::~Pipeline() {
        release();
    }

    bool Pipeline::startup() {
        if (!stopped_.load(std::memory_order_acquire)) {
            LOG_WARN("pipeline", "pipeline already started");
            return true;
        }

        released_.store(false, std::memory_order_release);

        const auto &gcfg = GConfig::get_instance();
        video_width_ = gcfg.rtsp_config_.width;
        video_height_ = gcfg.rtsp_config_.height;

        if (!init_codec()) {
            release();
            return false;
        }

        init_region();

        detection_queue_ = std::make_shared<toolkitx::concurrent::BlockingQueue<DetectionTask> >(30);

        stopped_.store(false, std::memory_order_release);

        detect_thread_ = std::thread(&Pipeline::detect_thread, this);
        process_thread_ = std::thread(&Pipeline::process_thread, this);

        // recorder_.start("runs/" + task_config_.id + ".mp4", video_width_, video_height_, 30);

        LOG_INFO("pipeline", "pipeline %s started", task_config_.id.c_str());
        return true;
    }

    void Pipeline::detect_thread() {
        while (!stopped_.load(std::memory_order_acquire)) {
            std::shared_ptr<FrameX> framex{};

            const auto st = decoder_->read(framex, -1);
            if (st != IoStatus::Ok) {
                if (st == IoStatus::Timeout) {
                    continue;
                }

                // 终止：通知下游退出
                stopped_.store(true, std::memory_order_release);
                if (detection_queue_) detection_queue_->release();
                return;
            }

            auto imagex = inferencex::ImageX::from_device(
                framex->fd, framex->ptr, framex->width, framex->height, framex->width * 3, framex
            );

            const auto fut = detector_->commit(imagex);
            if (!detection_queue_ || !detection_queue_->push({framex, fut}, 10)) {
                // 队列已 release/close 或异常：直接退出
                stopped_.store(true, std::memory_order_release);
                if (detection_queue_) detection_queue_->release();
                return;
            }
        }
    }

    void Pipeline::process_thread() {
        DetectionTask task{};
        while (!stopped_.load(std::memory_order_acquire)) {
            if (!detection_queue_ || !detection_queue_->pop(task, -1)) break;

            if (task.handle.wait_for(std::chrono::milliseconds(50)) == std::future_status::ready) {
                cv::Mat image(cv::Size(task.framex->width, task.framex->height), CV_8UC3, task.framex->ptr);

                // region_.draw(image, 0.1, 1);

                auto results = task.handle.get();
                for (const auto &det: results) {
                    if (!region_.contains(det.bbox.rect)) continue;

                    auto color = vision::id_to_color(det.class_id);
                    cv::rectangle(image, det.bbox.rect, color, 2);
                    cv::circle(image, cv::Point2f(det.bbox.cx(), det.bbox.cy()), 3, color, cv::FILLED);
                    char text[64];
                    snprintf(text, sizeof(text), "%s %.2f", std::to_string(det.class_id).c_str(), det.score);
                    cv::putText(
                        image, text, cv::Point2f(det.bbox.x1(), det.bbox.y1() - 5.f),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6,
                        color, 1, cv::LINE_AA
                    );
                }
            }

            // recorder_.write(image);
            const auto st = encoder_->write(task.framex, 10);
            if (st != IoStatus::Ok && st != IoStatus::Timeout) {
                stopped_.store(true);
                if (detection_queue_) detection_queue_->release();
                return;
            }
        }
    }

    void Pipeline::release() {
        if (released_.exchange(true, std::memory_order_acq_rel)) return;

        stopped_.store(true, std::memory_order_release);

        // 先停下游：确保所有使用上游 FrameX 等资源的线程已退出，防止上游释放资源后下游继续访问导致内存破坏。
        if (detection_queue_) detection_queue_->release();
        if (process_thread_.joinable()) process_thread_.join();

        // 再停上游，唤醒 read(-1)
        if (decoder_) decoder_->release();
        if (detect_thread_.joinable()) detect_thread_.join();

        if (encoder_) encoder_->release();

        LOG_INFO("pipeline", "pipeline %s stopped", task_config_.id.c_str());
    }

    bool Pipeline::init_region() {
        region_ = vision::Region(
            cv::Rect(0, 0, video_width_, video_height_), video_width_, video_height_
        );

        if (task_config_.type == "xyxy") {
            if (task_config_.values.size() == 4) {
                const auto x1 = task_config_.values[0];
                const auto y1 = task_config_.values[1];
                const auto x2 = task_config_.values[2];
                const auto y2 = task_config_.values[3];
                region_ = vision::Region(cv::Rect2f(x1, y1, x2 - x1, y2 - y1), video_width_, video_height_);
            } else {
                LOG_WARN("pipeline", "xyxy region expects 4 values");
            }
        } else if (task_config_.type == "polygon") {
            if (!task_config_.values.empty() && task_config_.values.size() % 2 == 0) {
                std::vector<cv::Point2f> pts;
                for (size_t i = 0; i < task_config_.values.size(); i += 2) {
                    pts.emplace_back(
                        task_config_.values[i],
                        task_config_.values[i + 1]
                    );
                }
                region_ = vision::Region(pts, video_width_, video_height_);
            } else {
                LOG_WARN("pipeline", "invalid polygon region config");
            }
        } else if (task_config_.type == "ratio") {
            if (task_config_.values.size() == 4) {
                const float l = task_config_.values[0];
                const float t = task_config_.values[1];
                const float r = task_config_.values[2];
                const float b = task_config_.values[3];

                if (l < 0.f || t < 0.f || r < 0.f || b < 0.f ||
                    l + r >= 1.f || t + b >= 1.f) {
                    LOG_WARN(
                        "pipeline", "invalid ratio padding: [%.2f, %.2f, %.2f, %.2f]", l, t, r, b
                    );
                } else {
                    const auto x1 = l * video_width_;
                    const auto y1 = t * video_height_;
                    const auto x2 = (1.f - r) * video_width_;
                    const auto y2 = (1.f - b) * video_height_;

                    region_ = vision::Region(cv::Rect2f(x1, y1, x2 - x1, y2 - y1), video_width_, video_height_);
                }
            } else {
                LOG_WARN("pipeline", "ratio region expects 4 values");
            }
        }

        return true;
    }

    bool Pipeline::init_codec() {
        const auto &gcfg = GConfig::get_instance();
        const int width = gcfg.rtsp_config_.width;
        const int height = gcfg.rtsp_config_.height;
        constexpr int fps = 30;

        const StreamInfo stream_info{task_config_.id, task_config_.uri};

        const DecodeConfig decode_cfg{
            width, height, ImageFormat::BGR24, WorkerMode::Polling, fps, 10
        };
        decoder_ = codec_manager_->create_decoder(stream_info, decode_cfg);
        if (!decoder_) {
            LOG_ERROR("pipeline", "failed to create decoder");
            return false;
        }

        const EncodeConfig encode_cfg{
            width, height, WorkerMode::Callback, fps, 10, CodecType::H265
        };
        encoder_ = codec_manager_->create_encoder(encode_cfg);
        if (!encoder_) {
            LOG_ERROR("pipeline", "failed to create encoder");
            return false;
        }

        encoder_->subscribe([this](const auto &e) {
            on_encoded(e);
        });

        return true;
    }

    void Pipeline::on_encoded(const std::shared_ptr<EncodedX> &encodedx) const {
        if (!encodedx || encodedx->size == 0) return;
        if (stopped_.load(std::memory_order_acquire)) return;

        rtspx::EncodedShared packet{};
        packet.frame_type = encodedx->is_keyframe ? rtspx::VIDEO_FRAME_I : rtspx::VIDEO_FRAME_P;
        packet.size = encodedx->size;
        packet.pts = encodedx->pts;
        packet.data = encodedx->data;
        packet.holder = encodedx->holder;

        rtsp_session_->push_data(rtspx::MediaTrack::Video, packet);
    }

    std::shared_ptr<Pipeline> Pipeline::create(
        const TaskConfig &task_config,
        const std::shared_ptr<inferencex::InferenceX<inferencex::ImageX, inferencex::Detection2DResults> > &detector,
        const std::shared_ptr<Manager> &codec_manager,
        const std::shared_ptr<rtspx::MediaSession> &rtsp_session,
        const std::shared_ptr<mqttx::Client> &mqtt_client
    ) {
        auto task = std::make_shared<Pipeline>(task_config, detector, codec_manager, rtsp_session, mqtt_client);
        if (!task->startup()) {
            LOG_ERROR("pipeline", "failed to startup pipeline");
            return nullptr;
        }

        return task;
    }
};
