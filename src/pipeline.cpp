#include "pipeline.h"

#include "toolkitx/logx/logx.h"

namespace detectionx {
    Pipeline::Pipeline(
        TaskConfig task_config, const std::shared_ptr<vcodecx::Manager> &codec_manager,
        const std::shared_ptr<rtspx::MediaSession> &session, const std::shared_ptr<mqttx::Client> &mqtt_client
    ) : task_config_(std::move(task_config)), codec_manager_(codec_manager), mqtt_client_(mqtt_client),
        session_(session) {
        GConfig &g_config = GConfig::get_instance();

        int fps = 30;
        int width = g_config.rtsp_config_.width;
        int height = g_config.rtsp_config_.height;

        const vcodecx::StreamInfo stream_info{task_config_.id, task_config_.uri};
        const vcodecx::DecodeConfig decode_cfg{
            width, height, vcodecx::ImageFormat::NV12, vcodecx::WorkerMode::Callback, fps, 3
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

        decoder_->subscribe([this](const auto &f) { on_frame(f); });
        encoder_->subscribe([this](const auto &e) { on_encoded(e); });
    }

    Pipeline::~Pipeline() {
        stop();
    }

    void Pipeline::stop() {
        if (stopped_.exchange(true)) return;
        if (decoder_) decoder_->release();
        if (encoder_) encoder_->release();
        LOG_INFO("pipeline", "task pipeline %s stopped", task_config_.id.c_str());
    }

    void Pipeline::on_frame(const std::shared_ptr<vcodecx::FrameX> &framex) const {
        encoder_->write(framex, 5);
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
