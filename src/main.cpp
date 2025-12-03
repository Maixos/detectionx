#include <memory>
#include <thread>
#include <atomic>
#include <csignal>

#include "toolkitx/logx/logx.h"

#include "config.h"
#include "pipeline.h"

using namespace detectionx;

std::atomic<bool> running{true};

void handle_sigint(int) { running = false; }

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    /// ----------------------------------------
    ///                 LOGX
    /// ----------------------------------------
    logx::Logger::init("vcodec", "debug", false);

    /// ----------------------------------------
    ///                 CONFIG
    /// ----------------------------------------
    GConfig &g_config = GConfig::get_instance();
    if (!g_config.load("/home/firefly/coder/projs/Maixos/detectionx/release/config.yaml")) {
        LOG_ERROR("test", "load config.yaml failed");
        return -1;
    }

    /// ----------------------------------------
    ///                 RTSP
    /// ----------------------------------------
    const auto rtsp_server = rtspx::RtspServer::create();
    if (!rtsp_server->start("0.0.0.0", 8554)) {
        LOG_ERROR("test", "start rtsp server failed");
        return -1;
    }

    /// ----------------------------------------
    ///                 MQTT
    /// ----------------------------------------
    std::shared_ptr<mqttx::Client> mqttx_client;
    if (g_config.mqtt_config_.enable) {
        const auto ip = g_config.mqtt_config_.ip;
        const auto port = g_config.mqtt_config_.port;
        mqttx_client = mqttx::create_mqtt_client(ip + ":" + port, g_config.project_config_.name);
        if (!mqttx_client) {
            LOG_ERROR(
                "test", "MQTT connection failed. Please check your MQTT configuration: ip=%s, port=%s",
                ip.c_str(), port.c_str()
            );
            return -1;
        }
    }

    /// ----------------------------------------
    ///                 PIPELINES
    /// ----------------------------------------
    std::vector<std::unique_ptr<Pipeline> > task_pipelines;
    const auto codec_manager = vcodecx::Manager::create();
    for (const auto &task_config: g_config.task_configs_) {
        LOG_INFO("test", "loaded task config: %s", task_config.to_string().c_str());
        const std::string suffix = g_config.rtsp_config_.suffix + "/" + task_config.id;
        auto session = rtsp_server->add_session(suffix);
        session->add_source(rtspx::Video, rtspx::H265);
        task_pipelines.emplace_back(std::make_unique<Pipeline>(task_config, codec_manager, session, mqttx_client));
    }

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    task_pipelines.clear();

    rtsp_server->stop();

    return 0;
}
