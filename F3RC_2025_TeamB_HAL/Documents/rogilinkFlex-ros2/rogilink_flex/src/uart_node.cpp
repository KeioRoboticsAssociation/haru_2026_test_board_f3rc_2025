#include <unistd.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rogilink_flex_interfaces/msg/frame.hpp"
#include "rogilink_flex_interfaces/srv/get_config.hpp"
#include "rogilink_flex_interfaces/srv/is_connected.hpp"
#include "uart.hpp"

using std::placeholders::_1;
using std::placeholders::_2;

using json = nlohmann::json;

namespace NodeName {
constexpr char UART_LINK[] = "uart_link";
}

namespace Parameters {
constexpr char CONFIG_PATH[] = "config_path";
constexpr char SELECT_DEVICE_INDEX[] = "select_device_index";
constexpr char PORT[] = "port";
}  // namespace Parameters

namespace TopicName {
constexpr char TRANSMISSION_PREFIX[] = "rogilink_transmission/";
constexpr char GENERAL_TRANSMISSION[] = "rogilink_general_transmission";
}  // namespace TopicName

namespace ServiceName {
constexpr char IS_CONNECTED[] = "is_connected";
constexpr char GET_CONFIG[] = "get_config";
}  // namespace ServiceName

const uint8_t SYSTEM_MESSAGE_FRAME_ID = 0x00;
enum SystemMessageID : uint8_t {
    CONNECTION_CHECK = 0x01,
    CONNECTION_CHECK_REPLY = 0x02,
};
const std::chrono::milliseconds CONNECTION_TIMEOUT{200};

class UartLinkNode : public rclcpp::Node {
public:
    UartLinkNode() : Node(NodeName::UART_LINK) {
        uart_ = nullptr;
        device_list_ = UartLink::get_device_list();

        // --- parameters ---
        this->declare_parameter<std::string>(Parameters::CONFIG_PATH, "");
        // devices配列用
        this->declare_parameter<int>(Parameters::SELECT_DEVICE_INDEX, 0);
        // ポート
        this->declare_parameter<std::string>(Parameters::PORT, "");

        config_path_ = this->get_parameter(Parameters::CONFIG_PATH).as_string();
        fixed_port_ = this->get_parameter(Parameters::PORT).as_string();

        // --- load json ---
        std::ifstream f(config_path_);
        if (!f) {
            RCLCPP_FATAL(
                this->get_logger(),
                "Cannot open config file: %s",
                config_path_.c_str()
            );
            throw std::runtime_error("config open failed");
        }
        json config =
            json::parse(f, /*callback*/ nullptr, /*allow_exceptions*/ true);

        // devices配列にも単体にも対応
        const json* dev = &config;
        if (config.contains("devices") && config["devices"].is_array() &&
            !config["devices"].empty()) {
            int idx =
                this->get_parameter(Parameters::SELECT_DEVICE_INDEX).as_int();

            if (idx < 0 || idx >= static_cast<int>(config["devices"].size())) {
                idx = 0;
            }

            dev = &config["devices"][idx];

            RCLCPP_INFO(
                this->get_logger(),
                "Using devices[%d] from %s",
                idx,
                config_path_.c_str()
            );
        } else {
            RCLCPP_INFO(
                this->get_logger(),
                "Using single-device config %s",
                config_path_.c_str()
            );
        }

        // --- device_id ---
        if (!dev->contains("device_id") ||
            !(*dev)["device_id"].is_number_integer()) {
            RCLCPP_FATAL(
                this->get_logger(), "config error: device_id must be integer"
            );
            throw std::runtime_error("bad config: device_id");
        }
        device_id_ = static_cast<uint8_t>((*dev)["device_id"].get<int>());

        // --- baud_rate (optional) ---
        int baud_rate_int = 115200;
        if (dev->contains("baud_rate") &&
            (*dev)["baud_rate"].is_number_integer()) {
            baud_rate_int = (*dev)["baud_rate"].get<int>();
        }
        switch (baud_rate_int) {
            case 9600:
                baud_rate_ = BaudRate::B_9600;
                break;
            case 19200:
                baud_rate_ = BaudRate::B_19200;
                break;
            case 38400:
                baud_rate_ = BaudRate::B_38400;
                break;
            case 57600:
                baud_rate_ = BaudRate::B_57600;
                break;
            case 115200:
            default:
                baud_rate_ = BaudRate::B_115200;
                break;
        }

        // --- optional port from JSON ---
        if (dev->contains("port") && (*dev)["port"].is_string()) {
            // パラメータ優先。JSONにあり、paramが空なら採用。
            if (fixed_port_.empty())
                fixed_port_ = (*dev)["port"].get<std::string>();
        }

        // --- topics from config ---
        if (!dev->contains("reception_messages") ||
            !(*dev)["reception_messages"].is_array()) {
            RCLCPP_FATAL(
                this->get_logger(),
                "config error: reception_messages must be array"
            );
            throw std::runtime_error("bad config: reception_messages");
        }
        if (!dev->contains("transmission_messages") ||
            !(*dev)["transmission_messages"].is_array()) {
            RCLCPP_FATAL(
                this->get_logger(),
                "config error: transmission_messages must be array"
            );
            throw std::runtime_error("bad config: transmission_messages");
        }

        // reception -> publishers
        for (const auto& m : (*dev)["reception_messages"]) {
            if (!m.contains("id") || !m["id"].is_number_integer() ||
                !m.contains("name") || !m["name"].is_string()) {
                RCLCPP_FATAL(
                    this->get_logger(),
                    "config error: invalid entry in reception_messages"
                );
                throw std::runtime_error(
                    "bad config: reception_messages entry"
                );
            }
            uint8_t id = static_cast<uint8_t>(m["id"].get<int>());
            std::string name = m["name"].get<std::string>();
            reception_frame_map_[id] = name;
            pub_map_[id] =
                this->create_publisher<rogilink_flex_interfaces::msg::Frame>(
                    "rogilink_reception/" + name, 10
                );
        }

        // transmission -> subscribers
        for (const auto& m : (*dev)["transmission_messages"]) {
            if (!m.contains("id") || !m["id"].is_number_integer() ||
                !m.contains("name") || !m["name"].is_string()) {
                RCLCPP_FATAL(
                    this->get_logger(),
                    "config error: invalid entry in transmission_messages"
                );
                throw std::runtime_error(
                    "bad config: transmission_messages entry"
                );
            }
            uint8_t id = static_cast<uint8_t>(m["id"].get<int>());
            std::string name = m["name"].get<std::string>();
            transmission_frame_map_[id] = name;

            sub_map_[id] =
                this->create_subscription<rogilink_flex_interfaces::msg::Frame>(
                    TopicName::TRANSMISSION_PREFIX + name,
                    10,
                    [this,
                     id](rogilink_flex_interfaces::msg::Frame::SharedPtr msg) {
                        this->frame_callback_(msg, id);
                    }
                );
        }

        // general channel (frame_idで送れる経路)
        general_sub_ =
            this->create_subscription<rogilink_flex_interfaces::msg::Frame>(
                TopicName::GENERAL_TRANSMISSION,
                10,
                [this](rogilink_flex_interfaces::msg::Frame::SharedPtr msg) {
                    this->frame_callback_(msg, msg->frame_id);
                }
            );

        // services
        is_connected_service_ =
            this->create_service<rogilink_flex_interfaces::srv::IsConnected>(
                ServiceName::IS_CONNECTED,
                [this](
                    const std::shared_ptr<rogilink_flex_interfaces::srv::
                                              IsConnected::Request> /*req*/,
                    std::shared_ptr<
                        rogilink_flex_interfaces::srv::IsConnected::Response>
                        res
                ) {
                    res->connected = is_connected_;
                    res->device_id = device_id_;
                }
            );

        get_config_service_ = this->create_service<
            rogilink_flex_interfaces::srv::GetConfig>(
            ServiceName::GET_CONFIG,
            [this](
                const std::shared_ptr<
                    rogilink_flex_interfaces::srv::GetConfig::Request> /*req*/,
                std::shared_ptr<
                    rogilink_flex_interfaces::srv::GetConfig::Response> res
            ) { res->path = config_path_; }
        );

        RCLCPP_INFO(
            this->get_logger(),
            "uartlink ready (device_id=%u, baud=%d, port=%s)",
            static_cast<unsigned>(device_id_),
            baud_rate_int,
            fixed_port_.empty() ? "(auto scan)" : fixed_port_.c_str()
        );
    }

    // main loop
    void mainloop() {
        while (rclcpp::ok()) {
            try {
                loop_();
            } catch (const std::exception& e) {
                RCLCPP_WARN(this->get_logger(), "exception: %s", e.what());
                disconnect_();
            }
        }
    }

private:
    rclcpp::TimerBase::SharedPtr read_timer_;
    std::shared_ptr<UartLink> uart_;

    // ID <-> name
    std::map<uint8_t, std::string> reception_frame_map_;
    std::map<uint8_t, std::string> transmission_frame_map_;

    // ID <-> pub/sub
    std::map<
        uint8_t,
        rclcpp::Publisher<rogilink_flex_interfaces::msg::Frame>::SharedPtr>
        pub_map_;
    std::map<
        uint8_t,
        rclcpp::Subscription<rogilink_flex_interfaces::msg::Frame>::SharedPtr>
        sub_map_;
    rclcpp::Subscription<rogilink_flex_interfaces::msg::Frame>::SharedPtr
        general_sub_;

    // services
    rclcpp::Service<rogilink_flex_interfaces::srv::IsConnected>::SharedPtr
        is_connected_service_;
    rclcpp::Service<rogilink_flex_interfaces::srv::GetConfig>::SharedPtr
        get_config_service_;

    // state
    std::vector<std::string> device_list_;
    std::chrono::system_clock::time_point connected_time_;
    bool is_connected_ = false;
    uint8_t device_id_ = 0;

    std::string config_path_;
    std::string fixed_port_;  // JSON/paramで与えられた場合に優先

    rclcpp::TimerBase::SharedPtr connection_timer_;

    BaudRate baud_rate_{BaudRate::B_115200};

    void loop_() {
        if (uart_ == nullptr) {
            try_connect_();
        }

        if (!is_connected_) {
            if (std::chrono::system_clock::now() - connected_time_ >
                CONNECTION_TIMEOUT) {
                if (uart_ != nullptr) {
                    (void)uart_->reset_device();
                }
                disconnect_();
            }
        }

        if (uart_ != nullptr) {
            uart_->loop();
        }
        rclcpp::spin_some(this->shared_from_this());
    }

    void init_(const std::string& device) {
        uart_ = std::make_shared<UartLink>(device, baud_rate_);
        uart_->set_callback(
            SYSTEM_MESSAGE_FRAME_ID,
            std::bind(&UartLinkNode::system_message_callback_, this, _1)
        );
        uart_->set_general_callback(
            std::bind(&UartLinkNode::general_message_callback_, this, _1, _2)
        );
        uart_->set_disconnect_callback(
            std::bind(&UartLinkNode::disconnect_, this)
        );
    }

    void try_connect_() {
        // 明示ポートがあればそれを最優先で使う
        if (!fixed_port_.empty()) {
            init_(fixed_port_);
            uart_->send({CONNECTION_CHECK}, SYSTEM_MESSAGE_FRAME_ID);
            connected_time_ = std::chrono::system_clock::now();
            return;
        }

        if (device_list_.empty()) {
            device_list_ = UartLink::get_device_list();
            if (device_list_.empty()) return;
        }

        // 先頭を試す
        auto device = device_list_.at(0);
        device_list_.erase(device_list_.begin());
        init_(device);
        uart_->send({CONNECTION_CHECK}, SYSTEM_MESSAGE_FRAME_ID);
        connected_time_ = std::chrono::system_clock::now();
    }

    void disconnect_() {
        uart_ = nullptr;
        is_connected_ = false;
    }

    // --- callbacks ---
    void system_message_callback_(std::vector<uint8_t> data) {
        RCLCPP_INFO(this->get_logger(), "System message received");
        if (data.empty()) return;

        uint8_t id = data.at(0);
        std::vector<uint8_t> payload(data.begin() + 1, data.end());

        switch (id) {
            case CONNECTION_CHECK_REPLY:
                connected_time_ = std::chrono::system_clock::now();
                if (!payload.empty()) {
                    connection_reply_callback_(payload.at(0));
                }
                break;
            default:
                break;
        }
    }

    void connection_reply_callback_(uint8_t id) {
        if (id == device_id_) {
            is_connected_ = true;
            RCLCPP_INFO(
                this->get_logger(),
                "Connected to device %u successfully",
                static_cast<unsigned>(id)
            );
        } else {
            RCLCPP_INFO(
                this->get_logger(),
                "Connected to device %u failed",
                static_cast<unsigned>(id)
            );
            disconnect_();
            try_connect_();
        }
    }

    void general_message_callback_(uint8_t id, std::vector<uint8_t> data) {
        if (id == SYSTEM_MESSAGE_FRAME_ID) return;
        if (!is_connected_) return;

        rogilink_flex_interfaces::msg::Frame frame;
        frame.frame_id = id;
        frame.device_id = device_id_;
        frame.data.clear();
        for (auto b : data) frame.data.push_back(b);

        auto it = pub_map_.find(id);
        if (it != pub_map_.end()) {
            it->second->publish(frame);
        } else {
            // 未定義IDは数値トピックで出す（後方互換）
            auto pub =
                this->create_publisher<rogilink_flex_interfaces::msg::Frame>(
                    TopicName::TRANSMISSION_PREFIX + std::to_string(id), 10
                );
            pub_map_[id] = pub;
            pub->publish(frame);
        }
    }

    void frame_callback_(
        const rogilink_flex_interfaces::msg::Frame::SharedPtr msg, uint8_t id
    ) {
        if (!is_connected_) return;
        if (msg->device_id != device_id_) return;

        std::vector<uint8_t> data;
        data.reserve(msg->data.size());
        for (auto b : msg->data) data.push_back(b);

        uart_->send(data, id);
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<UartLinkNode>();
    node->mainloop();
    rclcpp::shutdown();
    return 0;
}
