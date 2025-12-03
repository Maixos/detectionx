#include "config.h"

#include <fstream>

#include "toolkitx/logx/logx.h"
#include "toolkitx/file/path.h"

namespace detectionx {
    bool GConfig::load(const std::string &filename) {
        if (loaded_) {
            LOG_ERROR("config", "config already loaded, skipping reload");
            return false;
        }

        try {
            YAML::Node root = YAML::LoadFile(filename);
            if (!root) return false;

            config_path_ = filename;

            if (root["project"]) parse_project(root["project"]);
            if (root["detection"]) parse_detection(root["detection"]);
            if (root["mqtt"]) parse_mqtt(root["mqtt"]);
            if (root["rtsp"]) parse_rtsp(root["rtsp"]);
            if (root["tasks"]) parse_tasks(root["tasks"]);

            loaded_ = true;

            return true;
        } catch (const std::exception &e) {
            LOG_ERROR("config", "failed to load configuration: %s", e.what());
            return false;
        }
    }

    void GConfig::parse_project(const YAML::Node &node) {
        project_config_.mode = node["mode"].as<std::string>();
        project_config_.name = node["name"].as<std::string>();
        project_config_.version = node["version"].as<std::string>();
        project_config_.describe = node["describe"].as<std::string>();
    }

    void GConfig::parse_detection(const YAML::Node &node) {
        detection_config_.threshold = node["threshold"].as<float>();
        detection_config_.classes = node["classes"].as<std::vector<int> >(std::vector<int>{});
    }

    void GConfig::parse_rtsp(const YAML::Node &node) {
        rtsp_config_.port = node["port"].as<int>();
        rtsp_config_.width = node["width"].as<int>();
        rtsp_config_.height = node["height"].as<int>();
        rtsp_config_.suffix = node["suffix"].as<std::string>();
    }

    void GConfig::parse_mqtt(const YAML::Node &node) {
        mqtt_config_.enable = node["enable"].as<bool>();
        mqtt_config_.ip = node["ip"].as<std::string>();
        mqtt_config_.port = node["port"].as<std::string>();
        mqtt_config_.notify_topic = node["notify_topic"].as<std::string>();
    }

    void GConfig::parse_tasks(const YAML::Node &node) {
        task_configs_.reserve(node.size());
        for (const auto &item: node) {
            auto id = item["id"].as<std::string>();
            auto uri = item["uri"].as<std::string>();
            auto region = item["region"].as<std::vector<float> >(std::vector<float>{});
            task_configs_.emplace_back(id, uri, region);
        }
    }
}
