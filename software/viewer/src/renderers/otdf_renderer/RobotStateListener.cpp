#include <iostream>

#include <lcm/lcm-cpp.hpp>
#include "lcmtypes/drc_lcmtypes.hpp"

#include "RobotStateListener.hpp"

using namespace std;
using namespace boost;

namespace otdf_renderer 
{
  //==================constructor / destructor
  
  /**Subscribes to Robot URDF Model and to EST_ROBOT_STATE.*/
  RobotStateListener::RobotStateListener(boost::shared_ptr<lcm::LCM> &lcm, BotViewer *viewer):
    _lcm(lcm),
    _viewer(viewer)

  {
    //lcm ok?
    if(!lcm->good())
      {
	cerr << "\nLCM Not Good: Robot State Handler" << endl;
	return;
      }
    T_body_world = KDL::Frame::Identity();
    // Subscribe to Robot_state. 
    lcm->subscribe("EST_ROBOT_STATE", &RobotStateListener::handleRobotStateMsg, this); 
  }
  
  RobotStateListener::~RobotStateListener() {}


  //=============message callbacks

void RobotStateListener::handleRobotStateMsg(const lcm::ReceiveBuffer* rbuf,
						 const string& chan, 
						 const drc::robot_state_t* msg)						 
  { 
  	  KDL::Frame  T_world_body;
  	    
      T_world_body.p[0]= msg->origin_position.translation.x;
	    T_world_body.p[1]= msg->origin_position.translation.y;
	    T_world_body.p[2]= msg->origin_position.translation.z;		    
	    T_world_body.M =  KDL::Rotation::Quaternion(msg->origin_position.rotation.x, msg->origin_position.rotation.y, msg->origin_position.rotation.z, msg->origin_position.rotation.w);

      T_body_world=T_world_body.Inverse();   

    
  } // end handleMessage


} //end namespace


