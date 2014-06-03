//==============================================================================
// Copyright (c) 2014, Fadri Furrer <ffurrer@gmail.com>
// All rights reserved.
//
// TODO(ff): Enter some license
//==============================================================================
#include <mav_control/ros_controller_interface.h>

namespace mav_control {

RosControllerInterface::RosControllerInterface()
    : node_handle_(0),
      controller_created_(false) {

  std::string command_topic_attitude = "command/attitude";
  std::string command_topic_rate = "command/rate";
  std::string command_topic_motor = "command/motor";
  std::string ekf_topic = "msf_core/state_out";

  cmd_attitude_sub_ = node_handle_->subscribe(
      command_topic_attitude, 10,
      &RosControllerInterface::CommandAttitudeCallback, this);
  cmd_motor_sub_ = node_handle_->subscribe(
      command_topic_motor, 10, &RosControllerInterface::CommandMotorCallback,
      this);
  imu_sub_ = node_handle_->subscribe(imu_topic_, 10,
                                     &RosControllerInterface::ImuCallback,
                                     this);
  pose_sub_ = node_handle_->subscribe(pose_topic_, 10,
                                      &RosControllerInterface::PoseCallback,
                                      this);
  ekf_sub_ = node_handle_->subscribe(ekf_topic, 10, &RosControllerInterface::ExtEkfCallback,
                                      this);

  motor_cmd_pub_ = node_handle_->advertise<mav_msgs::MotorSpeed>(
      motor_velocity_topic_, 10);
}

RosControllerInterface::~RosControllerInterface() {
  if (node_handle_) {
    node_handle_->shutdown();
    delete node_handle_;
  }
}

void RosControllerInterface::InitializeParams() {
  //TODO(burrimi): Read parameters from yaml.
}
void RosControllerInterface::Publish() {
}

void RosControllerInterface::CommandAttitudeCallback(
    const mav_msgs::ControlAttitudeThrustPtr& input_reference_msg) {
  if (!controller_created_) {
// Get the controller and initialize its parameters.
    controller_ = mav_controller_factory::ControllerFactory::Instance()
        .CreateController("AttitudeController");
    controller_->InitializeParams();
    controller_created_ = true;
    ROS_INFO_STREAM("started AttitudeController" << std::endl);
  }
  Eigen::Vector4d input_reference(input_reference_msg->roll,
                                  input_reference_msg->pitch,
                                  input_reference_msg->yaw_rate,
                                  input_reference_msg->thrust);
  controller_->SetAttitudeThrustReference(input_reference);
}

void RosControllerInterface::ExtEkfCallback(
    const sensor_fusion_comm::ExtEkfConstPtr ekf_message) {
  if (!controller_created_)
    return;

}

void RosControllerInterface::CommandMotorCallback(
    const mav_msgs::ControlMotorSpeedPtr& input_reference_msg) {
  if (!controller_created_) {
// Get the controller and initialize its parameters.
    controller_ = mav_controller_factory::ControllerFactory::Instance()
        .CreateController("MotorController");
    controller_->InitializeParams();
    controller_created_ = true;
    ROS_INFO_STREAM("started AttitudeController" << std::endl);
  }

  Eigen::VectorXd input_reference;
  input_reference.resize(input_reference_msg->motor_speed.size());
  for (int i = 0; i < input_reference_msg->motor_speed.size(); ++i) {
    input_reference[i] = input_reference_msg->motor_speed[i];
  }
  controller_->SetMotorReference(input_reference);
}

void RosControllerInterface::ImuCallback(const sensor_msgs::ImuPtr& imu_msg) {
  if (!controller_created_)
    return;
  Eigen::Vector3d angular_rate(imu_msg->angular_velocity.x,
                               imu_msg->angular_velocity.y,
                               imu_msg->angular_velocity.z);
  controller_->SetAngularRate(angular_rate);
// imu->linear_acceleration;
}

void RosControllerInterface::PoseCallback(
    const geometry_msgs::PoseStampedPtr& pose_msg) {
  if (!controller_created_)
    return;
  Eigen::Quaternion<double> orientation(pose_msg->pose.orientation.w,
                                        pose_msg->pose.orientation.x,
                                        pose_msg->pose.orientation.y,
                                        pose_msg->pose.orientation.z);
  controller_->SetAttitude(orientation);
}

/*  void RosControllerInterface::Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
 {
 // Store the pointer to the model
 this->model_ = _model;

 // default params
 namespace_.clear();
 std::string command_topic_attitude = "command/attitude";
 std::string command_topic_rate = "command/rate";
 std::string command_topic_motor = "command/motor";

 if (_sdf->HasElement("robotNamespace"))
 namespace_ = _sdf->GetElement("robotNamespace")->Get<std::string>();
 else
 gzerr << "[gazebo_motor_model] Please specify a robotNamespace.\n";
 node_handle_ = new ros::NodeHandle(namespace_);

 if (_sdf->HasElement("commandTopicAttitude"))
 command_topic_attitude = _sdf->GetElement("commandTopicAttitude")->Get<std::string>();

 if (_sdf->HasElement("commandTopicMotor"))
 command_topic_motor = _sdf->GetElement("commandTopicMotor")->Get<std::string>();

 if (_sdf->HasElement("imuTopic"))
 imu_topic_ = _sdf->GetElement("imuTopic")->Get<std::string>();

 if (_sdf->HasElement("poseTopic"))
 pose_topic_ = _sdf->GetElement("poseTopic")->Get<std::string>();

 if (_sdf->HasElement("motorVelocityReferenceTopic")) {
 motor_velocity_topic_ = _sdf->GetElement(
 "motorVelocityReferenceTopic")->Get<std::string>();
 }


 // Listen to the update event. This event is broadcast every
 // simulation iteration.
 this->updateConnection_ = event::Events::ConnectWorldUpdateBegin(
 boost::bind(&RosControllerInterface::OnUpdate, this, _1));

 cmd_attitude_sub_ = node_handle_->subscribe(command_topic_attitude,
 10, &RosControllerInterface::CommandAttitudeCallback, this);
 cmd_motor_sub_ = node_handle_->subscribe(command_topic_motor,
 10, &RosControllerInterface::CommandMotorCallback, this);
 imu_sub_ = node_handle_->subscribe(imu_topic_,
 10, &RosControllerInterface::ImuCallback, this);
 pose_sub_ = node_handle_->subscribe(pose_topic_,
 10, &RosControllerInterface::PoseCallback, this);
 motor_cmd_pub_ = node_handle_->advertise<mav_msgs::MotorSpeed>(
 motor_velocity_topic_, 10);
 }

 // Called by the world update start event
 void RosControllerInterface::OnUpdate(const common::UpdateInfo&)
 {
 if(!controller_created_)
 return;
 Eigen::VectorXd ref_rotor_velocities;
 controller_->CalculateRotorVelocities(&ref_rotor_velocities);

 turning_velocities_msg_.motor_speed.clear();
 for (int i = 0; i < ref_rotor_velocities.size(); i++)
 turning_velocities_msg_.motor_speed.push_back(ref_rotor_velocities[i]);

 motor_cmd_pub_.publish(turning_velocities_msg_);
 }


 */
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "ros_controller_node");

  ros::spin();

  return 0;
}
