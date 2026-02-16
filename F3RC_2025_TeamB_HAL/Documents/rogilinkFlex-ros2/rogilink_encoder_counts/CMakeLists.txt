cmake_minimum_required(VERSION 3.8)
project(rogilink_encoder_counts)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# dependencies

## packages
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

## msgs
find_package(sensor_msgs REQUIRED)
find_package(rogilink_flex_interfaces REQUIRED)

# add executable
add_executable(rogilink_encoder_counts src/node.cpp)
ament_target_dependencies(
  rogilink_encoder_counts
  rclcpp
  sensor_msgs
  rogilink_flex_interfaces
)

# install
install(
  TARGETS rogilink_encoder_counts
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
