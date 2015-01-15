#include <string>

#include "authoring/constraint_task_space_region.h"
#include "authoring/constraint_sequence.h"

using namespace std;
using namespace boost;
using namespace lcm;
using namespace urdf;
using namespace drc;
using namespace state;
using namespace affordance;
using namespace authoring;

Constraint_Sequence::
Constraint_Sequence( unsigned int numConstraints ) : _constraints( numConstraints ),
                                                      _q0() {

}

Constraint_Sequence::
~Constraint_Sequence() {

}

Constraint_Sequence::
Constraint_Sequence( const Constraint_Sequence& other ) : _constraints( other._constraints ),
                                                          _q0( other._q0 ) {

}

Constraint_Sequence&
Constraint_Sequence::
operator=( const Constraint_Sequence& other ) {
  _constraints = other._constraints;
  _q0 = other._q0;
  return (*this);
}

void
Constraint_Sequence::
from_xml( const string& filename ){
  xmlDoc * doc = NULL;
  doc = xmlReadFile( filename.c_str(), NULL, 0 );
  from_xml( xmlDocGetRootElement( doc ) );
  xmlFreeDoc( doc );
  return;
}

void
Constraint_Sequence::
from_xml( xmlNodePtr root ){
  // clean up the vector
  _constraints.erase(_constraints.begin(), _constraints.end());
  vector< Constraint_Task_Space_Region >::iterator it = _constraints.begin();
  if( root->type == XML_ELEMENT_NODE ){
    xmlNodePtr l1 = NULL;
    for( l1 = root->children; l1; l1 = l1->next ){
      if( l1->type == XML_ELEMENT_NODE ){
        if( xmlStrcmp( l1->name, ( const xmlChar* )( "constraint_task_space_region" ) ) == 0 ){
          cout << "found constraint task space region" << endl;
          Constraint_Task_Space_Region new_ctsr = Constraint_Task_Space_Region();
          new_ctsr.from_xml( l1 );
          _constraints.push_back(new_ctsr);
        }         
      }
    }
  }
  return;

  /*
  for( vector< Constraint_Task_Space_Region >::iterator it = _constraints.begin(); it != _constraints.end(); it++ ){
    it->active() = false;
  }

  vector< Constraint_Task_Space_Region >::iterator it = _constraints.begin();
  if( root->type == XML_ELEMENT_NODE ){
    xmlNodePtr l1 = NULL;
    for( l1 = root->children; l1; l1 = l1->next ){
      if( l1->type == XML_ELEMENT_NODE ){
        if( xmlStrcmp( l1->name, ( const xmlChar* )( "constraint_task_space_region" ) ) == 0 ){
          cout << "found constraint task space region" << endl;
          it->from_xml( l1 );
          it++;
          if( it == _constraints.end() ){
            return;
          }
        }         
      }
    }
  }
  return;
  */
}

void
Constraint_Sequence::
to_xml( const string& filename )const{
  ofstream out;
  out.open( filename.c_str() );
  to_xml( out );
  out.close();
  return;
}

void
Constraint_Sequence::
to_xml( ofstream& out,
        unsigned int indent )const{
  out << string( indent, ' ' ) << "<constraint_sequence>" << endl;
  for( vector< Constraint_Task_Space_Region >::const_iterator it = _constraints.begin(); it != _constraints.end(); it++ ){
    if( it->active() ){
      it->to_xml( out, indent + 2 );
    }   
  }
  out << string( indent, ' ' ) << "</constraint_sequence>" << endl;
  return;
}

void
Constraint_Sequence::
to_msg( action_sequence_t& msg, 
        vector< AffordanceState >& affordanceCollection,
        float ik_time_of_interest ){
  msg.utime = 0;
  msg.num_contact_goals = 0;
  msg.contact_goals.clear();
  msg.robot_name = "atlas";
  _q0.to_lcm( &msg.q0 );
  //msg.q0.robot_name = msg.robot_name;
  for( vector< Constraint_Task_Space_Region >::iterator it = _constraints.begin(); it != _constraints.end(); it++ ){
    cout << it->id() << ": " << it->active() << endl;
    if( it->active() && (ik_time_of_interest==-1 || (it->start()<=ik_time_of_interest&&it->end()>=ik_time_of_interest))) {
      it->add_to_drc_action_sequence_t( msg, affordanceCollection );
    }
  }
  return;
}

void
Constraint_Sequence::
from_msg( const action_sequence_t& msg ){
  _q0.from_lcm( &msg.q0 );
  for( vector< Constraint_Task_Space_Region >::iterator it = _constraints.begin(); it != _constraints.end(); it++ ){
    it->active() = false;
    it->parents().clear();
  }
  cout << "found " << msg.num_contact_goals << " contact_goals" << endl;
  vector< Constraint_Task_Space_Region >::iterator it = _constraints.begin();
  for( unsigned int i = 0; i < msg.num_contact_goals; i++ ){
    it->active() = true;
    it->metadata() = "NA";
    it->start() = msg.contact_goals[i].lower_bound_completion_time;
    it->end() = msg.contact_goals[i].upper_bound_completion_time;
    it->relation_type() = CONSTRAINT_TASK_SPACE_REGION_AFFORDANCE_RELATION_TYPE;
    if( msg.contact_goals[i].contact_type == contact_goal_t::WITHIN_REGION ){
      it->contact_type() = CONSTRAINT_TASK_SPACE_REGION_WITHIN_REGION_CONTACT_TYPE;
    } else {
      it->contact_type() = CONSTRAINT_TASK_SPACE_REGION_SUPPORTED_WITHIN_REGION_CONTACT_TYPE;
    }
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_X_MIN_RANGE ].first = ( msg.contact_goals[i].x_offset_lower > -1000.0 );
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_X_MAX_RANGE ].first = ( msg.contact_goals[i].x_offset_upper < 1000.0 );
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Y_MIN_RANGE ].first = ( msg.contact_goals[i].y_offset_lower > -1000.0 );
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Y_MAX_RANGE ].first = ( msg.contact_goals[i].y_offset_upper < 1000.0 );
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Z_MIN_RANGE ].first = ( msg.contact_goals[i].z_offset_lower > -1000.0 );
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Z_MAX_RANGE ].first = ( msg.contact_goals[i].z_offset_upper < 1000.0 );
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_X_MIN_RANGE ].second = msg.contact_goals[i].x_offset_lower;
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_X_MAX_RANGE ].second = msg.contact_goals[i].x_offset_upper;
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Y_MIN_RANGE ].second = msg.contact_goals[i].y_offset_lower;
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Y_MAX_RANGE ].second = msg.contact_goals[i].y_offset_upper;
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Z_MIN_RANGE ].second = msg.contact_goals[i].z_offset_lower;
    it->ranges()[ CONSTRAINT_TASK_SPACE_REGION_Z_MAX_RANGE ].second = msg.contact_goals[i].z_offset_upper;
    it->parents().push_back( msg.contact_goals[i].object_1_name + "-" + msg.contact_goals[i].object_1_contact_grp );
    it->child() = ( msg.contact_goals[i].object_2_name + "/" + msg.contact_goals[i].object_2_contact_grp );
    it++;
  }
  

  return;
}

void
Constraint_Sequence::
print_msg( const action_sequence_t& msg ){
  cout << "utime:{" << msg.utime << "} ";
  cout << "robot_name:{" << msg.robot_name << "} ";
  cout << "contact_goals[" << msg.num_contact_goals << "]:{";
  for( vector< contact_goal_t >::const_iterator it = msg.contact_goals.begin(); it != msg.contact_goals.end(); it++ ){
    cout << "{(" << it->lower_bound_completion_time << "," << it->upper_bound_completion_time << "),";
    cout << "(" << it->object_1_name << "," << it->object_1_contact_grp << "),";
    cout << "(" << it->object_2_name << "," << it->object_2_contact_grp << "),";
    if( it->contact_type == contact_goal_t::WITHIN_REGION ){
      cout << "(WITHIN_REGION),";
    } else if ( it->contact_type == contact_goal_t::SUPPORTED_WITHIN_REGION ){
      cout << "(SUPPORTED_WITHIN_REGION),";
    } else {
      cout << "(UNKNOWN),";
    }
    if( it->x_relation_lower == contact_goal_t::REL_EQUAL ){
      cout << "(xl=" << it->x_offset_lower << "),";
    } else if( it->x_relation_lower == contact_goal_t::REL_LESS_THAN ){
      cout << "(xl<" << it->x_offset_lower << "),";
    } else if ( it->x_relation_lower == contact_goal_t::REL_GREATER_THAN ){
      cout << "(xl>" << it->x_offset_lower << "),";
    }
    if( it->y_relation_lower == contact_goal_t::REL_EQUAL ){
      cout << "(yl=" << it->y_offset_lower << "),";
    } else if( it->y_relation_lower == contact_goal_t::REL_LESS_THAN ){
      cout << "(yl<" << it->y_offset_lower << "),";
    } else if ( it->y_relation_lower == contact_goal_t::REL_GREATER_THAN ){
      cout << "(yl>" << it->y_offset_lower << "),";
    }
    if( it->z_relation_lower == contact_goal_t::REL_EQUAL ){
      cout << "(zl=" << it->z_offset_lower << ")";
    } else if( it->z_relation_lower == contact_goal_t::REL_LESS_THAN ){
      cout << "(zl<" << it->z_offset_lower << ")";
    } else if ( it->z_relation_lower == contact_goal_t::REL_GREATER_THAN ){
      cout << "(zl>" << it->z_offset_lower << ")";
    }
    if( it->x_relation_upper == contact_goal_t::REL_EQUAL ){
      cout << "(xu=" << it->x_offset_upper << "),";
    } else if( it->x_relation_upper == contact_goal_t::REL_LESS_THAN ){
      cout << "(xu<" << it->x_offset_upper << "),";
    } else if ( it->x_relation_upper == contact_goal_t::REL_GREATER_THAN ){
      cout << "(xu>" << it->x_offset_upper << "),";
    }
    if( it->y_relation_upper == contact_goal_t::REL_EQUAL ){
      cout << "(yu=" << it->y_offset_upper << "),";
    } else if( it->y_relation_upper == contact_goal_t::REL_LESS_THAN ){
      cout << "(yu<" << it->y_offset_upper << "),";
    } else if ( it->y_relation_upper == contact_goal_t::REL_GREATER_THAN ){
      cout << "(yu>" << it->y_offset_upper << "),";
    }
    if( it->z_relation_upper == contact_goal_t::REL_EQUAL ){
      cout << "(zu=" << it->z_offset_upper << ")";
    } else if( it->z_relation_upper == contact_goal_t::REL_LESS_THAN ){
      cout << "(zu<" << it->z_offset_upper << ")";
    } else if ( it->z_relation_upper == contact_goal_t::REL_GREATER_THAN ){
      cout << "(zu>" << it->z_offset_upper << ")";
    }
    cout << "}";
  }
  cout << "}" << endl;
  return;
}

namespace authoring {
  ostream&
  operator<<( ostream& out,
              const Constraint_Sequence& other ) {
    return out;
  }

}