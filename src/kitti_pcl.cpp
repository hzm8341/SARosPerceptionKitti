// ROS specific includes
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <geometry_msgs/Point.h>
#include <visualization_msgs/Marker.h>

// PCL specific includes
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/extract_indices.h>

// Detection
#include "../include/test_kitti/dbscan.h"

// Publisher
ros::Publisher pcl_pub;
ros::Publisher dbb_pub;

// Parameters
const float voxel_size = 0.2;
const float opening_angle = M_PI/4;
const float minimum_height = -1.3;
const float minimum_range = 3.0;
const float maximum_range = 20.0;

const bool filter_pointcloud = true;
const bool convert_to_voxelgrid = false;

void callback(const sensor_msgs::PointCloud2ConstPtr& input){

  // Convert the sensor_msgs/PointCloud2 data to pcl/PointCloud
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::fromROSMsg (*input, *cloud);

  // Filter the pointcloud
  if(filter_pointcloud){
    
    // Define inliers and indices
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices());
    pcl::ExtractIndices<pcl::PointXYZ> extract;

    // Loop through point cloud
    for(int i = 0; i < cloud->size(); ++i){

      // Read point
      pcl::PointXYZ point;
      point = cloud->at(i);

      // Determine angle of laser point
      float angle = std::abs( std::atan2(point.y, point.x) );

      // Check opening angle
      if(angle < opening_angle){
        // Check range
        float range = std::sqrt(point.x * point.x + point.y * point.y);
        if(minimum_range < range && range < maximum_range){
          // Check minimum height
          if(point.z > minimum_height && angle < opening_angle){
            inliers->indices.push_back(i);
          }
        }
      }
    }

    // Extract points
    extract.setInputCloud(cloud);
    extract.setIndices(inliers);
    extract.setNegative(false);
    extract.filter(*cloud);
  }

  // Convert to VoxelGrid
  if(convert_to_voxelgrid){

    // Define VoxelGrid
    pcl::VoxelGrid<pcl::PointXYZ> sor;
    sor.setInputCloud(cloud);
    sor.setLeafSize(voxel_size, voxel_size, voxel_size);
    sor.filter(*cloud);
  }

  // Detector
  Dbscan detector = Dbscan();
  // Detection step
  detector.run_dbscan(cloud);

  visualization_msgs::Marker marker;
  marker.header.frame_id = "base_link";
  marker.header.stamp = ros::Time();
  marker.ns = "my_namespace";
  marker.id = 0;
  marker.type = visualization_msgs::Marker::CUBE;
  marker.action = visualization_msgs::Marker::ADD;
  marker.pose.position.x = 10;
  marker.pose.position.y = 0;
  marker.pose.position.z = 1;
  marker.pose.orientation.x = 0.0;
  marker.pose.orientation.y = 0.0;
  marker.pose.orientation.z = 0.0;
  marker.pose.orientation.w = 1.0;
  marker.scale.x = 4;
  marker.scale.y = 2;
  marker.scale.z = 2;
  marker.color.a = 0.3; // Don't forget to set the alpha!
  marker.color.r = 0.0;
  marker.color.g = 1.0;
  marker.color.b = 0.0;
  //only if using a MESH_RESOURCE marker type:
  marker.mesh_resource = "package://pr2_description/meshes/base_v0/base.dae";
  dbb_pub.publish(marker);

  // Publish the data
  std::cout << cloud->size() << std::endl;
  pcl_pub.publish(cloud);
}

int main (int argc, char** argv){
  // Initialize ROS
  ros::init (argc, argv, "kitti_pcl");
  ros::NodeHandle nh;

  // Create a ROS subscriber for the input point cloud
  ros::Subscriber sub = nh.subscribe ("/kitti/velo/pointcloud", 1, callback);

  // Create a ROS publisher for the output point cloud
  pcl_pub = nh.advertise<sensor_msgs::PointCloud2> ("pointcloud", 1);

  // Create a ROS publisher for the detected bounding boxes
  dbb_pub = nh.advertise<visualization_msgs::Marker>( "detection", 0 );

  // Spin
  ros::spin ();
}