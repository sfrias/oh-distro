#ifndef POINT_CONTACT_RELATION_H
#define POINT_CONTACT_RELATION_H

#include <boost/shared_ptr.hpp>
#include <action_authoring/RelationState.h>
#include <Eigen/Core>

namespace action_authoring
{

/**todo: add comment*/
class PointContactRelation : public RelationState
{
    //------------fields
 private:
    Eigen::Vector3f _point1;
    Eigen::Vector3f _point2;

    //------------Constructor--------
 public:
    PointContactRelation();

  //---------------Accessors
    Eigen::Vector3f getPoint1() { return _point1; }
    Eigen::Vector3f getPoint2() { return _point2; }
  //mutators
    void setPoint1(Eigen::Vector3f p1) { _point1 = p1; }
    void setPoint2(Eigen::Vector3f p2) { _point2 = p2; }
  
    virtual std::string getPrompt() const;
    virtual std::string getState() const;

}; //class PointContactRelation
 
 typedef boost::shared_ptr<PointContactRelation> PointContactRelationPtr;
 typedef boost::shared_ptr<const PointContactRelation> PointContactRelationConstPtr;

} //namespace action_authoring

#endif //POINT_CONTACT_RELATION_H
