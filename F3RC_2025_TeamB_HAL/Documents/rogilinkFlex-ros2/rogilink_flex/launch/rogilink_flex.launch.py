from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    CONFIG_PATH = "src/rogilinkFlex-ros2/rogilink_flex/config/config.json"

    return LaunchDescription(
        [
            Node(
                package="rogilink_flex",
                executable="rogilink_flex",
                parameters=[{"config_path": CONFIG_PATH}],
            ),
            Node(
                package="rogilink_encoder_counts",
                executable="rogilink_encoder_counts",
            ),
        ]
    )
