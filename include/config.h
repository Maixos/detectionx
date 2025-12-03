#pragma once

#include <utility>
#include <vector>
#include <string>

#include <yaml-cpp/yaml.h>

namespace detectionx {
    struct ProjectConfig {
        std::string mode;
        std::string name;
        std::string version;
        std::string describe;

        ProjectConfig() = default;

        explicit ProjectConfig(std::string mode, std::string name, std::string version, std::string describe)
            : mode(std::move(mode)), name(std::move(name)), version(std::move(version)), describe(std::move(describe)) {
        }

        [[nodiscard]] std::string to_string() const {
            std::ostringstream oss;
            oss << "ProjectConfig: { "
                    << "mode: " << mode
                    << ", name: " << name
                    << ", version: " << version
                    << ", describe: " << describe
                    << " }";
            return oss.str();
        }
    };

    struct DetectionConfig {
        float threshold{};
        std::vector<int> classes{};

        DetectionConfig() = default;

        explicit DetectionConfig(const float threshold, std::vector<int> classes)
            : threshold(threshold), classes(std::move(classes)) {
        }

        [[nodiscard]] std::string to_string() const {
            std::ostringstream oss;
            oss << "DetectionConfig { "
                    << "threshold: " << threshold
                    << ", classes: [";

            for (size_t i = 0; i < classes.size(); ++i) {
                oss << classes[i];
                if (i + 1 < classes.size())
                    oss << ", ";
            }

            oss << "] }";
            return oss.str();
        }
    };

    struct MQTTConfig {
        bool enable{};
        std::string ip{};
        std::string port{};
        int interval{};
        std::string notify_topic{};

        MQTTConfig() = default;

        MQTTConfig(std::string ip, std::string port, const int interval, std::string notify_topic)
            : ip(std::move(ip)), port(std::move(port)), interval(interval), notify_topic(std::move(notify_topic)) {
        }

        [[nodiscard]] std::string to_string() const {
            std::ostringstream oss;
            oss << "MQTTConfig: { "
                    << "enable: " << enable
                    << ", ip: " << ip
                    << ", port: " << port
                    << ", interval: " << interval
                    << ", notify_topic: " << notify_topic
                    << " }";
            return oss.str();
        }
    };

    struct RTSPConfig {
        int port{};
        int width{};
        int height{};
        std::string suffix;

        RTSPConfig() = default;

        explicit RTSPConfig(
            const bool enable, const int port, std::string suffix, const int width = 1280, const int height = 720
        ) : port(port), width(width), height(height), suffix(std::move(suffix)) {
        }

        [[nodiscard]] std::string to_string() const {
            std::ostringstream oss;
            oss << "RTSPConfig: { "
                    << "port: " << port
                    << ", width: " << width
                    << ", height: " << height
                    << ", suffix: " << suffix
                    << " }";
            return oss.str();
        }
    };

    struct TaskConfig {
        std::string id;
        std::string uri;
        std::vector<float> region;

        TaskConfig() = default;

        TaskConfig(std::string id, std::string uri, const std::vector<float> &region)
            : id(std::move(id)), uri(std::move(uri)), region(region) {
        }

        [[nodiscard]] std::string to_string() const {
            std::ostringstream oss;
            oss << "TaskConfig { "
                    << "id: " << id
                    << ", uri: " << uri
                    << ", region: [";

            for (size_t i = 0; i < region.size(); ++i) {
                oss << region[i];
                if (i + 1 < region.size()) oss << ", ";
            }

            oss << "] }";
            return oss.str();
        }
    };

    class GConfig {
    private:
        GConfig() = default;

    public:
        static GConfig &get_instance() {
            static GConfig instance;
            return instance;
        }

        bool load(const std::string &filename);

    public:
        ProjectConfig project_config_{};
        MQTTConfig mqtt_config_{};
        RTSPConfig rtsp_config_{};
        DetectionConfig detection_config_{};
        std::vector<TaskConfig> task_configs_{};

    private:
        bool loaded_ = false;
        std::string config_path_{};

        void parse_project(const YAML::Node &node);

        void parse_detection(const YAML::Node &node);

        void parse_mqtt(const YAML::Node &node);

        void parse_rtsp(const YAML::Node &node);

        void parse_tasks(const YAML::Node &node);
    };
}
