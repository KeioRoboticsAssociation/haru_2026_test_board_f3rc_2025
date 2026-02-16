#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "rogilink_flex_interfaces/msg/frame.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

// 測定輪の個数
constexpr int N = 3;

namespace NodeName {
const std::string ROGILINK_ENCODER_COUNTS = "rogilink_encoder_counts";
}

namespace PubTopicName {
const std::string ENCODER_COUNTS = "/encoder_counts";
}

namespace SubTopicName {
const std::string ROGILINK_ENCODER_COUNTS =
    "/rogilink_reception/encoder_counts";
}

// 送られてくるエンコーダーカウント数の順番に合わせること
const std::vector<std::string> JOINT_STATE_NAMES = {
    "front_encoder",
    "rear_right_encoder",
    "rear_left_encoder",
};

class RogilinkEncoderCounts : public rclcpp::Node {
public:
    RogilinkEncoderCounts() : Node(NodeName::ROGILINK_ENCODER_COUNTS) {
        pub_ = this->create_publisher<sensor_msgs::msg::JointState>(
            PubTopicName::ENCODER_COUNTS, 10
        );

        sub_ = this->create_subscription<rogilink_flex_interfaces::msg::Frame>(
            SubTopicName::ROGILINK_ENCODER_COUNTS,
            10,
            [this](const rogilink_flex_interfaces::msg::Frame::SharedPtr msg) {
                this->callback(msg);
            }
        );

        RCLCPP_INFO(
            this->get_logger(), "Encoder Decoder Node has been started."
        );
    }

private:
    void callback(const rogilink_flex_interfaces::msg::Frame::SharedPtr msg) {
        // 送られてくるvector<uint8_t>をvector<int>として扱うため、
        // メモリの大きさが同じ、つまり要素数が4Nであることが求められる
        if (msg->data.size() != N * 4) {
            RCLCPP_WARN(
                this->get_logger(),
                "Received data size mismatch. Expected %d elements, got "
                "%zu",
                N * 4,
                msg->data.size()
            );
            return;
        }

        auto joint_state_msg = sensor_msgs::msg::JointState();
        joint_state_msg.header.stamp = this->now();
        joint_state_msg.header.frame_id = "encoder_base";
        joint_state_msg.name = JOINT_STATE_NAMES;

        int encoder_counts[N];
        // 送られてくるのはvector<uint8_t>だが、中身はvector<int>なので型変換せずにそのままコピー
        std::memcpy(encoder_counts, msg->data.data(), sizeof(encoder_counts));

        joint_state_msg.position.resize(N);
        for (int i = 0; i < N; i++) {
            joint_state_msg.position[i] =
                static_cast<double>(encoder_counts[i]);
        }

        pub_->publish(joint_state_msg);
    }

    rclcpp::Subscription<rogilink_flex_interfaces::msg::Frame>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr pub_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RogilinkEncoderCounts>());
    rclcpp::shutdown();
    return 0;
}
