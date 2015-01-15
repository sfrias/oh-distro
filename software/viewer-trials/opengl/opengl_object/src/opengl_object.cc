#include <cstring>
#include <GL/gl.h>

#include "opengl/opengl_object.h"

using namespace std;
using namespace KDL;
using namespace Eigen;
using namespace opengl;

/**
 * OpenGL_Object
 * class constructor
 */
OpenGL_Object::
OpenGL_Object(const string &id,
                const Frame& transform,
                const Frame& offset, 
                bool isHighlighted, 
                Vector3f highlightColor)
  :
  _id( id ),
  _visible( true ),
  _color( 1.0, 1.0, 1.0 ),
  _transparency( 1.0 ),
  _transform( transform ),
  _offset( offset ),
  _highlightColor(highlightColor), 
  _isHighlighted(isHighlighted)
{
}

/**
 * ~OpenGL_Object
 * class destructor 
 */
OpenGL_Object::
~OpenGL_Object(){

}

/**
 * OpenGL_Object
 * copy constructor
 */
OpenGL_Object::
OpenGL_Object( const OpenGL_Object& other ) : _id( other._id ),
                                              _visible( other._visible ),
                                              _color( other._color ),
                                              _transparency( other._transparency ),
                                              _transform( other._transform ),
                                              _offset( other._offset ),
                                              _highlightColor( other._highlightColor ),
                                              _isHighlighted( other._isHighlighted ){    
}

/**
 * operator=
 * assignment operator
 */
OpenGL_Object&
OpenGL_Object::
operator=( const OpenGL_Object& other ){
  _id = other._id;
  _visible = other._visible;
  _color = other._color;
  _transparency = other._transparency;
  _transform = other._transform;
  _offset = other._offset;
  return (*this);
}

/**
 * apply_transform
 * convers the KDL::Frame into a GL and calls glMultMatrixd to execute the transform
 */
void
OpenGL_Object::
apply_transform( void ){
  Frame origin = _transform * _offset;

  GLdouble m[] = { origin( 0, 0 ), origin( 1, 0 ), origin( 2, 0 ), origin( 3, 0 ), 
                        origin( 0, 1 ), origin( 1, 1 ), origin( 2, 1 ), origin( 3, 1 ), 
                        origin( 0, 2 ), origin( 1, 2 ), origin( 2, 2 ), origin( 3, 2 ), 
                        origin( 0, 3 ), origin( 1, 3 ), origin( 2, 3 ), origin( 3, 3 ) };
  glMultMatrixd( m ); 
  return;
}

/**
 * draw
 * a virtual function that performs the opengl drawing operation
 */
void
OpenGL_Object::
draw( void ){
  return;
}

/**
 * draw
 * a virtual function that performs the opengl drawing operation with a specific color
 */
void
OpenGL_Object::
draw( Vector3f color ){
  return;
}

/**
 * set_id
 * sets the id for the opengl object
 */
void
OpenGL_Object::
set_id( string id ){
  _id = id;
  return;
}

/**
 * set_visible
 * sets whether the opengl object is visible
 */
void
OpenGL_Object::
set_visible( bool visible ){
  _visible = visible;
  return;
} 

/**
 * set_color
 * sets the color of the opengl object (Vector3f representing rgb values that range between 0.0 and 1.0)
 */
void
OpenGL_Object::
set_color( Vector3f color ){
  _color = color;
  return;
}

void
OpenGL_Object::
set_color( Vector4f color ){
  _color(0) = color(0);
  _color(1) = color(1);
  _color(2) = color(2);
  _transparency = color(3);
  return;
}

void OpenGL_Object::setHighlighted(bool highlight)
{
    _isHighlighted = highlight;
}


/**
 * set_transparency
 * sets the transparency of the object (double value that ranges between 0.0 (clear) and 1.0 (opaque))
 */ 
void
OpenGL_Object::
set_transparency( double transparency ){
  _transparency = transparency;
  return;
}


/**
 * set_transform
 * sets the transform of the opengl object
 */
void
OpenGL_Object::
set_transform( Frame transform ){
  _transform = transform;
  return;
}

/**
 * set_offset
 * sets the offset of the opengl object
 */
void
OpenGL_Object::
set_offset( Frame offset ){
  _offset = offset;
  return;
}

string
OpenGL_Object::
id( void )const{
  return _id;
}

/**
 * visible
 * returns the visibility of the opengl object
 */
bool
OpenGL_Object::
visible( void )const{
  return _visible;
}

/**
 * color
 * returns the color of the opengl object
 */
Vector3f
OpenGL_Object::
color( void )const{
  return _color;
}

/**
 * transparency
 * returns the transparency of the opengl object 
 */
double
OpenGL_Object::
transparency( void )const{
  return _transparency;
}

/**
 * transform
 * returns the transform of the opengl object
 */
Frame
OpenGL_Object::
transform( void )const{
  return _transform;
}

/**
 * offset
 * returns the offset of the opengl object
 */
Frame
OpenGL_Object::
offset( void )const{
  return _offset;
}

/**
 * operator<<
 * ostream operator
 */
namespace opengl {
  ostream&
  operator<<( ostream& out,
              const OpenGL_Object& other ){
    return out;
  }
}