/**
 * @file /cost_map_demos/src/lib/from_ros_costmaps.cpp
 */
/*****************************************************************************
** Includes
*****************************************************************************/

#include "../../include/cost_map_demos/from_ros_costmaps.hpp"

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace cost_map_demos {

/*****************************************************************************
** TransformBroadcaster
*****************************************************************************/

TransformBroadcaster::~TransformBroadcaster() {
  broadcasting_thread.join();
}

void TransformBroadcaster::add(const std::string& name, tf::Vector3 origin, const tf::Quaternion& orientation) {
  tf::Transform transform;
  transform.setOrigin(origin);
  transform.setRotation(orientation);
  transforms.insert(std::pair<std::string, tf::Transform>(name, transform));
}

void TransformBroadcaster::startBroadCastingThread() {
  broadcasting_thread = std::thread(&TransformBroadcaster::broadcast, this);
}

void TransformBroadcaster::broadcast() {
  tf::TransformBroadcaster tf_broadcaster;
  while(ros::ok())
  {
    for (std::pair<std::string, tf::Transform> p: transforms) {
      tf::StampedTransform stamped_transform(p.second, ros::Time::now(), "map", p.first);
      tf_broadcaster.sendTransform(stamped_transform);
    }
    ros::Duration(0.1).sleep();
  }
}

/*****************************************************************************
** ROS Costmap Server
*****************************************************************************/

ROSCostmapServer::ROSCostmapServer(const std::string& name,
                                   const std::string& base_link_transform_name,
                                   const cost_map::Position& origin,
                                   const double& width,
                                   const double& height
)
: transform_listener(ros::Duration(1.0))
{
  ros::NodeHandle private_node_handle("~");
  // lots of parameters here affect the construction ( e.g. rolling window)
  // if you don't have any parameters set, then this
  //  - alot of defaults which get dumped on the ros param server
  //  - fires up an obstacle layer and an inflation layer
  //  - creates a publisher for an occupancy grid
  private_node_handle.setParam(name + "/robot_base_frame", base_link_transform_name);
  private_node_handle.setParam(name + "/origin_x", origin.x());
  private_node_handle.setParam(name + "/origin_y", origin.y());
  private_node_handle.setParam(name + "/width", width);
  private_node_handle.setParam(name + "/height", height);
  private_node_handle.setParam(name + "/resolution", 1.0);
  private_node_handle.setParam(name + "/robot_radius", 0.03); // clears 1 cell if inside, up to 4 cells on a vertex
  costmap = std::make_shared<ROSCostmap>(name, transform_listener);

  for ( unsigned int index = 0; index < costmap->getCostmap()->getSizeInCellsY(); ++index ) {
    unsigned int dimension = costmap->getCostmap()->getSizeInCellsX();
    // @todo assert dimension > 1
    // set the first row to costmap_2d::FREE_SPACE? but it shows up invisibly in rviz, so don't bother
    for ( unsigned int fill_index = 0; fill_index < dimension - 2; ++fill_index )
    {
      double fraction = static_cast<double>(fill_index + 1) / static_cast<double>(costmap->getCostmap()->getSizeInCellsX());
      costmap->getCostmap()->setCost(fill_index, index, fraction*costmap_2d::INSCRIBED_INFLATED_OBSTACLE );
    }
    costmap->getCostmap()->setCost(dimension - 2, index, costmap_2d::LETHAL_OBSTACLE);
    costmap->getCostmap()->setCost(dimension - 1, index, costmap_2d::NO_INFORMATION);
  }
}

/*****************************************************************************
 ** Trailers
 *****************************************************************************/

} // namespace cost_map_demos
