#include "authoring/qt4_widget_authoring_lcm_interface.h"

using namespace std;
using namespace lcm;
using namespace drc;
using namespace affordance;
using namespace state;
using namespace authoring;

Qt4_Widget_Authoring_LCM_Interface::
Qt4_Widget_Authoring_LCM_Interface( const string& xmlString, 
                                    QWidget * parent ) : QMainWindow( parent ),
                                                          _lcm( new LCM() ),
                                                          _lcm_timer( new QTimer( this ) ),
                                                          _qt4_widget_authoring( new Qt4_Widget_Authoring( xmlString, 128, this ) ) {
  setCentralWidget( _qt4_widget_authoring );

  connect( _lcm_timer, SIGNAL( timeout() ), this, SLOT( _handle_lcm_timer_timeout() ) ); 
  connect( this, SIGNAL( affordance_collection_update( std::vector< affordance::AffordanceState >& ) ),
            _qt4_widget_authoring, SLOT( update_affordance_collection( std::vector< affordance::AffordanceState >& ) ) );
  connect( this, SIGNAL( affordance_collection_update( std::vector< affordance::AffordanceState >& ) ),
            _qt4_widget_authoring->qt4_widget_opengl_authoring(), SLOT( update_opengl_object_affordance_collection_ghost( std::vector< affordance::AffordanceState >& ) ) );
  connect( this, SIGNAL( robot_plan_update( std::vector< state::State_GFE >& ) ),
            _qt4_widget_authoring, SLOT( update_robot_plan( std::vector< state::State_GFE >& ) ) );
  connect( this, SIGNAL( robot_plan_update( std::vector< state::State_GFE >& ) ),
            _qt4_widget_authoring->qt4_widget_opengl_authoring(), SLOT( update_opengl_object_robot_plan( std::vector< state::State_GFE >& ) ) );
  connect( this, SIGNAL( state_gfe_update( state::State_GFE& ) ), _qt4_widget_authoring, SLOT( update_state_gfe( state::State_GFE& ) ) );
  connect( this, SIGNAL( state_gfe_update( state::State_GFE& ) ), _qt4_widget_authoring->qt4_widget_opengl_authoring(), SLOT( update_opengl_object_gfe_ghost( state::State_GFE& ) ) ); 

  connect( _qt4_widget_authoring, SIGNAL( drc_action_sequence_t_publish( const drc::action_sequence_t& ) ), this, SLOT( publish_drc_action_sequence_t( const drc::action_sequence_t& ) ) );
  _lcm->subscribe( "EST_ROBOT_STATE", &Qt4_Widget_Authoring_LCM_Interface::_handle_est_robot_state_msg, this );
  _lcm->subscribe( "AFFORDANCE_COLLECTION", &Qt4_Widget_Authoring_LCM_Interface::_handle_affordance_collection_msg, this );
  _lcm->subscribe( "RESPONSE_MOTION_PLAN_FOR_ACTION_SEQUENCE", &Qt4_Widget_Authoring_LCM_Interface::_handle_candidate_robot_plan_msg, this );

  _lcm_timer->start( 10 );
}

Qt4_Widget_Authoring_LCM_Interface::
~Qt4_Widget_Authoring_LCM_Interface() {

}

Qt4_Widget_Authoring_LCM_Interface::
Qt4_Widget_Authoring_LCM_Interface( const Qt4_Widget_Authoring_LCM_Interface& other ) : QMainWindow(),
                                                                                        _lcm( other._lcm ) {

}

Qt4_Widget_Authoring_LCM_Interface&
Qt4_Widget_Authoring_LCM_Interface::
operator=( const Qt4_Widget_Authoring_LCM_Interface& other ) {

  return (*this);
}


void 
Qt4_Widget_Authoring_LCM_Interface::
publish_drc_action_sequence_t( const action_sequence_t& msg ){
  _lcm->publish( "REQUEST_MOTION_PLAN_FOR_ACTION_SEQUENCE", &msg );
  return;
}

void
Qt4_Widget_Authoring_LCM_Interface::
_handle_lcm_timer_timeout( void ){
  int lcm_fd = _lcm->getFileno();
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(lcm_fd, &fds);
  struct timeval timeout = { 0, 10 };
  if ( select( lcm_fd + 1, &fds, 0, 0, &timeout ) != 0 ){
    _lcm->handle();
  }  
  return;
}

void
Qt4_Widget_Authoring_LCM_Interface::
_handle_est_robot_state_msg( const ReceiveBuffer * rbuf,
                              const string& channel,
                              const robot_state_t* msg ){
  if( msg != NULL ){
    State_GFE state_gfe;  
    state_gfe.from_lcm( msg );
    emit state_gfe_update( state_gfe );
  }
  return;
}

void
Qt4_Widget_Authoring_LCM_Interface::
_handle_affordance_collection_msg( const ReceiveBuffer * rbuf,
                                    const string& channel,
                                    const affordance_collection_t* msg ){
  if( msg != NULL ){
    vector< AffordanceState > affordance_collection;
    for( unsigned int i = 0; i < msg->naffs; i++ )
      {
        AffordanceState affordance( &msg->affs[i] );
        
        if (affordance.getOTDFType() != AffordanceState::CAR)
          affordance_collection.push_back( affordance );
        else
          {
            cout << "\n\n\n ================ going to try and split car============\n\n\n" << endl;
            vector<AffPtr> split;
            if (!affordance.toBoxesCylindersSpheres(split))
              throw runtime_error("to boxes cylinders sphere failed for car");
            for(uint i = 0; i < split.size();i++)
              affordance_collection.push_back(*split[i]);
            cout << "\n spilt car into " << split.size() << " pieces " << endl; 
          }
    }
    emit affordance_collection_update( affordance_collection );
  }
  return;
}

void
Qt4_Widget_Authoring_LCM_Interface::
_handle_candidate_robot_plan_msg( const ReceiveBuffer * rbuf,
                                  const string& channel,
                                  const robot_plan_t* msg ){
  if( msg != NULL ){
    vector< State_GFE > robot_plan;
    for( unsigned int i = 0; i < msg->num_states; i++ ){
      State_GFE state;
      state.from_lcm( msg->plan[ i ] );
      robot_plan.push_back( state );
    }
    emit robot_plan_update( robot_plan );
  }
  return;
}

namespace authoring {
  ostream&
  operator<<( ostream& out,
              const Qt4_Widget_Authoring_LCM_Interface& other ) {
    return out;
  }

}
