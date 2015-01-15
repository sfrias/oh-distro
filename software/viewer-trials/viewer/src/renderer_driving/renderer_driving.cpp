// Renderer for a point-and-click message publisher
// used to send a message to relocalize a robot
// this was orginally part of envoy/renderers
// mfallon aug2011
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <bot_vis/bot_vis.h>
#include <bot_core/bot_core.h>

#include <bot_param/param_client.h>
#include <bot_param/param_util.h>
#include <bot_frames/bot_frames.h>
#include <bot_frames_cpp/bot_frames_cpp.hpp>

#include <Eigen/Dense>
#include <Eigen/StdVector>
//#include <visualization/renderer_localize.h>

#include <lcmtypes/drc_lcmtypes.h>
#include <lcmtypes/drc_lcmtypes.hpp>
#include <lcmtypes/bot_core.h>
#include <lcmtypes/perception_pointing_vector_t.h>
//#include <lcmtypes/drc_driving_cmd_t.h>


#define RENDERER_NAME "Driving"
#define PARAM_DRIVE_TIME "Drive Duration (s)"
#define PARAM_SEND_DRIVE_COMMAND "Send Drive Command"
#define PARAM_GOAL_SEND "Seek Relative"
#define PARAM_GOAL_TIMEOUT "Goal Lifespan"
#define PARAM_VISUAL_GOAL_TYPE "VisGoalType"
#define PARAM_VISUAL_GOAL      "Seek Visual"
#define PARAM_GEAR_POSITION "Gear Position"
#define PARAM_ENABLE "Enable"

#define PARAM_START_SEQUENCE "Startup Car"
#define PARAM_TURN_IGNITION "Ignition"
#define PARAM_THROTTLE "Throttle"

#define PARAM_HANDBRAKE "Handbrake"
#define PARAM_BRAKE "Brake"
#define PARAM_STOP "E_Stop"

#define PARAM_STOP_CONTROLLER "Stop Controller"

#define PARAM_MAX_STEERING "Max Steering"
#define PARAM_MIN_STEERING "Min Steering"

#define PARAM_MAX_GAS "Max Gas"
#define PARAM_MIN_GAS "Min Gas"

#define PARAM_UPDATE_MANIP_MAP "Update Manip Map"

#define PARAM_ENABLE_STEERING_ANGLE_OFFSET "Enable Steering Offset"
#define PARAM_STEERING_ANGLE_OFFSET "Steering Offset"

#define PARAM_STEERING_ANGLE "Steering Angle"
#define PARAM_THROTTLE_RATIO "Gas ratio"

#define PARAM_FULL_ACCELERATION "Vehicle Full Acceleration"

#define PARAM_DO_BRAKING "Do Active Braking"

#define PARAM_THROTTLE_DURATION "Acceleration duration (s)"
#define PARAM_GOAL_TYPE "Goal Type"

#define PARAM_UPDATE_CAR_MODEL_REDUCED "Use reduced car model"
#define PARAM_UPDATE_CAR_MODEL_FULL "Use full car model"

#define PARAM_LOOKAHEAD "Lookahead Distance"
#define PARAM_P_GAIN "P Gain"
#define PARAM_D_GAIN "D Gain"
#define PARAM_I_GAIN "I Gain"

#define PARAM_STEERING_DELTA 1.0
#define PARAM_THROTTLE_DELTA 0.01
#define PARAM_BRAKE_DELTA 0.05

#define STEERING_RATIO 0.125//0.063670

#define MAX_ACCELERATION_TIME 6
#define DEFAULT_ACCELERATION_TIME 2

#define MAX_DRIVE_TIME 40
#define DEFAULT_DRIVE_TIME 10

#define H_WIDTH 0.6
#define H_LENGTH 0.9

#define PARAM_UPDATE "Update Params"

typedef enum _visual_goal_type_t {
    VISUAL_GOAL_GLOBAL, VISUAL_GOAL_RELATIVE
} visual_goal_type_t;

typedef enum _gear_type_t {
    GEAR_FORWARD, GEAR_NEUTRAL, GEAR_REVERSE
} gear_type_t;

typedef enum _goal_type_t {
    USE_ROAD_CARROT, USE_ROAD_ARC, USE_TLD_WITH_ROAD, USE_TLD_IGNORE_ROAD, USE_USER_GOAL
} goal_type_t;


#define DRAW_PERSIST_SEC 4
#define VARIANCE_THETA (30.0 * 180.0 / M_PI);
//Not sure why abe has this high a variance for the angle //((2*M_PI)*(2*M_PI))
#define MIN_STD 0.3
#define MAX_STD INFINITY

////////////////// THE FOLLOWING CODE WAS COPIED IN HERE TO AVOID
////////////////// DEPENDENCY WITH THE COMMON_UTILS/GEOM_UTILS POD [MFALLON]
#define GEOM_EPSILON 1e-9

// ===== 2 dimensional structure =====
#ifndef _point2d_t_h
typedef struct _point2d {
    double x;
    double y;
} point2d_t;
#endif

// ===== 3 dimensional strucutres =====
// double 
typedef struct _point3d {
    double x;
    double y;
    double z;
} point3d_t;

#define point3d_as_array(p) ((double*)p)

/* The magic below allows you to use the POINT3D() macro to convert a
 * double[3] to a point3d_t.  gcc is smart -- if you try to cast anything
 * other than a point3d_t or a double[] with this macro, gcc will emit
 * a warning. */
union _point3d_any_t {
    point3d_t point;
    double array[3];
};

typedef point3d_t vec3d_t;

#define POINT3D(p) (&(((union _point3d_any_t *)(p))->point))

using namespace std;


int geom_ray_plane_intersect_3d (const point3d_t *ray_point, const vec3d_t *ray_dir,
                                 const point3d_t *plane_point, const vec3d_t *plane_normal,
                                 point3d_t *result, double *u)
{
    double lambda1 = ray_dir->x * plane_normal->x +
        ray_dir->y * plane_normal->y +
        ray_dir->z * plane_normal->z;

    // check for degenerate case where ray is (more or less) parallel to plane
    if (fabs (lambda1) < GEOM_EPSILON) return 0;

    double lambda2 = (plane_point->x - ray_point->x) * plane_normal->x +
        (plane_point->y - ray_point->y) * plane_normal->y +
        (plane_point->z - ray_point->z) * plane_normal->z;
    double v = lambda2 / lambda1;
    result->x = ray_point->x + v * ray_dir->x;
    result->y = ray_point->y + v * ray_dir->y;
    result->z = ray_point->z + v * ray_dir->z;
    if (u) *u = v;
    return 1;
}

int geom_ray_z_plane_intersect_3d(const point3d_t *ray_point, 
                                  const point3d_t *ray_dir, double plane_z, point2d_t *result_xy)
{
    point3d_t plane_pt = { 0, 0, plane_z};
    point3d_t plane_normal = { 0, 0, 1};
    point3d_t plane_isect_point;
    double plane_point_dist;
    if (!geom_ray_plane_intersect_3d (ray_point, ray_dir, &plane_pt,
                                      &plane_normal, &plane_isect_point, &plane_point_dist) ||
        plane_point_dist <= 0) {
        return -1;
    }
    result_xy->x = plane_isect_point.x;
    result_xy->y = plane_isect_point.y;
    return 0;
}

Eigen::Quaterniond euler_to_quat(double yaw, double pitch, double roll) {
    double sy = sin(yaw*0.5);
    double cy = cos(yaw*0.5);
    double sp = sin(pitch*0.5);
    double cp = cos(pitch*0.5);
    double sr = sin(roll*0.5);
    double cr = cos(roll*0.5);
    double w = cr*cp*cy + sr*sp*sy;
    double x = sr*cp*cy - cr*sp*sy;
    double y = cr*sp*cy + sr*cp*sy;
    double z = cr*cp*sy - sr*sp*cy;
    return Eigen::Quaterniond(w,x,y,z);
}

void quat_to_euler(Eigen::Quaterniond q, double& yaw, double& pitch, double& roll) {
    const double q0 = q.w();
    const double q1 = q.x();
    const double q2 = q.y();
    const double q3 = q.z();
    roll = atan2(2*(q0*q1+q2*q3), 1-2*(q1*q1+q2*q2));
    pitch = asin(2*(q0*q2-q3*q1));
    yaw = atan2(2*(q0*q3+q1*q2), 1-2*(q2*q2+q3*q3));
}


// modulu - similar to matlab's mod()
// result is always possitive. not similar to fmod()
// Mod(-3,4)= 1   fmod(-3,4)= -3
double Mod(double x, double y)
{
    if (0 == y)
        return x;

    return x - y * floor(x/y);
}

// wrap [rad] angle to [-PI..PI)
double WrapPosNegPI(double fAng)
{
    return Mod(fAng + M_PI, M_PI*2) - M_PI;
}




////////////////////////////// END OF CODE COPIED IN FROM COMMON_UTILS

typedef struct _RendererDriving {
    BotRenderer renderer;
    BotEventHandler ehandler;

    BotParam *bot_param;
    BotFrames *bot_frames;
    bot::frames* bot_frames_cpp;    
    BotViewer *viewer;
    lcm_t *lc;

    BotGtkParamWidget *pw;

    int dragging;
    int active; //1 = relocalize, 2 = set person location
    int last_active; //1 = relocalize, 2 = set person location
    point2d_t drag_start_local;
    point2d_t drag_finish_local;

    point2d_t click_pos;
    double theta;
    double goal_std;
    double goal_timeout; // no. seconds before this goal expires

    double last_tld_tracker_update; 
    double last_road_detection_update;

    int64_t max_draw_utime;
    double circle_color[3];
  
    // Most recent robot position, rotation and utime
    int64_t robot_utime;
    Eigen::Isometry3d robot_pose;
    double robot_yaw;

    double min_turn_radius;
    double wheel_separation; // left/right distance between the wheels
    Eigen::Isometry3d center_arc_left;
    Eigen::Isometry3d center_arc_right;
    double max_velocity;
  
    drc_driving_controller_values_t *controller_values;
    drc_driving_controller_status_t *controller_status;
    drc_driving_status_t *ground_truth_status;

    // Actuation limits imposed by manipulation
    int steer_have_manip_map;
    double steer_min_rad;
    double steer_max_rad;
    int gas_pedal_have_manip_map;
    double gas_pedal_min;
    double gas_pedal_max;
    double brake_pedal_have_manip_map;
    double brake_pedal_min;
    double brake_pedal_max;

    drc_affordance_t *car_affordance;
  
    visual_goal_type_t visual_goal_type;
    // Local [use when visual navigation works]
    Eigen::Vector4d local_pointpose_from;
    Eigen::Vector4d local_pointpose_to;
    // Body [use when visual navigation fails]
    Eigen::Vector4d body_pointpose_from;
    Eigen::Vector4d body_pointpose_to;
  
}RendererDriving;

static void draw_arc(double radius){
    glColor3f(0,1,1);

    double R = fabs(radius);

    
        
    double x = 0;
    double y = 0;
            
    //draw car footprints for given angle
    double swept_angle = 20 / R;
        
    int no_segments = 20;
    double angle_d = swept_angle/ no_segments;
    for(int i=0; i < no_segments; i++){
        double theta = angle_d * i;
        double s, c;
        bot_fasttrig_sincos(theta, &s, &c);
        x = R * s;
                

        double theta_act = theta;
        y = R*(1-c);
        if(radius <0){
            y = -y;
            theta = -theta;
        }
                
        //this is the position 
        glPushMatrix();
        glTranslatef(x , y , 0);
        glRotatef(theta*180/M_PI , 0, 0, 1.0);

        glBegin(GL_QUADS);
        glVertex2f(H_LENGTH, -H_WIDTH);
        glVertex2f(H_LENGTH, H_WIDTH);
        glVertex2f(-H_LENGTH, H_WIDTH);
        glVertex2f(-H_LENGTH, -H_WIDTH);
        glEnd();
        glPopMatrix();
    }
}

static void
_draw (BotViewer *viewer, BotRenderer *renderer)
{
    RendererDriving *self = (RendererDriving*) renderer;

    if(self->controller_values){
        BotTrans car_to_local;
        
        bot_frames_get_trans(self->bot_frames, "car", "local", 
                             &car_to_local);
    
        //this part could be done upon msg also 
        //get the wheel arcs from the steering angle
        double wheel_angle = (bot_to_radians(self->controller_values->hand_steer)) * STEERING_RATIO ;
                
        BotTrans pt_to_car;
        double rpy[3];
        bot_roll_pitch_yaw_to_quat(rpy, pt_to_car.rot_quat);

        double rpy_car[3];
        
        bot_quat_to_roll_pitch_yaw(car_to_local.rot_quat, rpy_car);

        double l = 1.8;
        double w = 1.2; 
        int draw_line  = 0;

        double R;
        
        if(fabs(wheel_angle) < 0.002){
            R = 1000;
        }
        else{
            R = pow( pow(l/ tan(wheel_angle),2) + pow(l,2), 0.5);
        }
               
        glPushMatrix();
        glTranslatef(car_to_local.trans_vec[0] , car_to_local.trans_vec[1] , 0.05);
        glRotatef(rpy_car[2]*180/M_PI , 0, 0, 1.0);

        double goal_heading = bot_to_radians(self->controller_values->goal_heading_angle);
        double goal_distance = 10 * self->controller_values->goal_distance;

        double gs, gc;
        bot_fasttrig_sincos(goal_heading, &gs, &gc);
        double gx = goal_distance * gc;
        double gy = goal_distance * gs;
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor3f(1,0,0);

        glPushMatrix();
        glTranslatef(gx , gy , 0.05);
        glLineWidth(5.0);
        bot_gl_draw_circle(0.3);
        glPopMatrix();
        
        
        /* glColor3f(0,1,1);

        
        double x = 0;
        double y = 0;
            
        //draw car footprints for given angle
        double swept_angle = 20 / R;
        
        int no_segments = 20;;
        double angle_d = swept_angle/ no_segments;
        for(int i=0; i < no_segments; i++){
            double theta = angle_d * i;
            double s, c;
            bot_fasttrig_sincos(theta, &s, &c);
            x = R * s;
                

            double theta_act = theta;
            y = R*(1-c);
            if(wheel_angle <0){
                y = -y;
                theta = -theta;
            }
                
            //this is the position 
            glPushMatrix();
            glTranslatef(x , y , 0);
            glRotatef(theta*180/M_PI , 0, 0, 1.0);

            glBegin(GL_QUADS);
            glVertex2f(H_LENGTH, -H_WIDTH);
            glVertex2f(H_LENGTH, H_WIDTH);
            glVertex2f(-H_LENGTH, H_WIDTH);
            glVertex2f(-H_LENGTH, -H_WIDTH);
            glEnd();
            glPopMatrix();
            }*/

        if(wheel_angle < 0){
            draw_arc(-R);
        }
        else{
            draw_arc(R);
        }

        //draw a set of arcs         
        if(0){
            glColor3f(0,0.3,0.4);
            double max_steering_angle = bot_to_radians(90);
            
            int no_arcs = 10;
            
            double delta = max_steering_angle / no_arcs;
            
            for(int i=-10; i <= 10; i++){
                double angle = delta * i * STEERING_RATIO;
                double rad = 0;
                if(fabs(angle) < 0.01){
                    rad = 1000;
                }
                rad = pow( pow(l/ tan(angle),2) + pow(l,2), 0.5);
                if(i < 0)
                    rad = -rad;
                
                draw_arc(rad);
            }
        }

            
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);              
        glPopMatrix();
    }


    goal_type_t goal_type = (goal_type_t) bot_gtk_param_widget_get_enum(self->pw, PARAM_GOAL_TYPE);
    if(goal_type == USE_USER_GOAL){
        //draw arc based on the use steering angle 
        BotTrans car_to_local;
        
        bot_frames_get_trans(self->bot_frames, "car", "local", 
                             &car_to_local);
    
        //this part could be done upon msg also 
        //get the wheel arcs from the steering angle
        double steering_angle = bot_to_radians(bot_gtk_param_widget_get_double(self->pw, PARAM_STEERING_ANGLE) + bot_gtk_param_widget_get_double(self->pw, PARAM_STEERING_ANGLE_OFFSET));

        double wheel_angle = -steering_angle * STEERING_RATIO ;
                
        BotTrans pt_to_car;
        double rpy[3];
        bot_roll_pitch_yaw_to_quat(rpy, pt_to_car.rot_quat);

        double rpy_car[3];
        
        bot_quat_to_roll_pitch_yaw(car_to_local.rot_quat, rpy_car);

        double l = 1.88;
        double w = 1.2; 
        int draw_line  = 0;

        double R;

        if(fabs(wheel_angle) < 0.01){
            R = 1000;
        }
        else{
            R = pow( pow(l/ tan(wheel_angle),2) + pow(l,2), 0.5);
        }

        glPushMatrix();
        glTranslatef(car_to_local.trans_vec[0] , car_to_local.trans_vec[1] , 0.1);
        glRotatef(rpy_car[2]*180/M_PI , 0, 0, 1.0);

        glColor3f(1.0,0,0);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        double x = 0;
        double y = 0;
        
        //draw car footprints for given angle
        double swept_angle = 20 / R;
        
        int no_segments = 20;;
        double angle_d = swept_angle/ no_segments;
        for(int i=0; i < no_segments; i++){
            double theta = angle_d * i;
            double s, c;
            bot_fasttrig_sincos(theta, &s, &c);
            x = R * s;
            
            
            double theta_act = theta;
            y = R*(1-c);
            if(wheel_angle <0){
                y = -y;
                theta = -theta;
            }
            
            //this is the position 
            glPushMatrix();
            glTranslatef(x , y , 0);
            glRotatef(theta*180/M_PI , 0, 0, 1.0);
            //glVertex3f(x,y,0);
            
            
            glBegin(GL_QUADS);
            glVertex2f(H_LENGTH, -H_WIDTH);
            glVertex2f(H_LENGTH, H_WIDTH);
            glVertex2f(-H_LENGTH, H_WIDTH);
            glVertex2f(-H_LENGTH, -H_WIDTH);
            glEnd();
            glPopMatrix();
        }
        
        
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glPopMatrix();
    }
  
    // Alternatively this could be controlled using the input dropdown..
    //if (!bot_gtk_param_widget_get_bool (self->pw, PARAM_ENABLE)) {
    //    return; 
    //}

    // Render text
    glPushAttrib (GL_ENABLE_BIT);
    glEnable (GL_BLEND);
    glDisable (GL_DEPTH_TEST);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int gl_width = (GTK_WIDGET(viewer->gl_area)->allocation.width);
    int gl_height = (GTK_WIDGET(viewer->gl_area)->allocation.height);

    // transform into window coordinates, where <0, 0> is the top left corner
    // of the window and <viewport[2], viewport[3]> is the bottom right corner
    // of the window
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, gl_width, 0, gl_height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0, gl_height, 0);
    glScalef(1, -1, 1);

    //void *font = GLUT_BITMAP_8_BY_13;
    void *font = GLUT_BITMAP_9_BY_15;
    int line_height = 14;
    int line_length = 200;

    float colors[6][3] = {
        { 1.0, 0.0, 0.0 }, // red
        { 0.7, 0.7, 0.0 }, // yellow
        { 0.0, 1.0, 0.5 }, // green
        { 0.0, 0.7, 0.7 }, // ??
        { 0.0, 0.0, 1.0 }, // blue
        { 0.0, 0.4, 0.3 },
    };
    //{ 0.6, 0.6, 0.6 }, // gray        

    char line1[80], line2[80], line3[80], line4[80], line5[80], line6[80], line7[90], line8[90], line9[90];

    if (self->controller_values) {
        sprintf (line1, "Time rem: %.1f\n", ((double) self->controller_values->time_to_drive)/10.0);
        if (self->ground_truth_status) {
            sprintf (line2, "Throttle: %.2f [%.2f]\n", (self->controller_values->throttle_value) / 100.0, 
                     self->ground_truth_status->gas_pedal);
            sprintf (line3, "   Brake: %.2f [%.2f]\n", (self->controller_values->brake_value) / 100.0,
                     self->ground_truth_status->brake_pedal);
            sprintf (line4, "Steer(d): %3.0f [%3.0f]\n", (double) self->controller_values->hand_steer,
                     bot_to_degrees(self->ground_truth_status->hand_wheel));
        }
        else {
            sprintf (line2, "Throttle: %.2f\n", self->controller_values->throttle_value/ 100.0);
            sprintf (line3, "   Brake: %.2f\n", self->controller_values->brake_value/100.0);
            sprintf (line4, "Steer(d): %3.0f\n", (double) self->controller_values->hand_steer);
        }
    }
    else {
        sprintf (line1, "Time rem: ???\n");
       if (self->ground_truth_status) {
            sprintf (line2, "Throttle: ??? [%.2f]\n", self->ground_truth_status->gas_pedal);
            sprintf (line3, "   Brake: ??? [%.2f]\n", self->ground_truth_status->brake_pedal);
            sprintf (line4, "Steer(d): ??? [%3.0f]\n", bot_to_degrees(self->ground_truth_status->hand_wheel));
        }
        else {
            sprintf (line2, "Throttle: ??? [???]\n");
            sprintf (line3, "   Brake: ??? [???]\n");
            sprintf (line4, "   Steer: ??? [???]\n");
        }
    }

    if (self->ground_truth_status) {
        sprintf (line5, "     Dir: %d\n", self->ground_truth_status->direction);
        sprintf (line6, "     Key: %d\n", self->ground_truth_status->key);
        sprintf (line7, "      HB: %.2f\n", self->ground_truth_status->hand_brake);
    }
    else {
        sprintf (line5, "    Dir.: ???\n");
        sprintf (line6, "     Key: ???\n");
        sprintf (line7, "      HB: ???\n");
    }
    
    double x = 0; //gl_width - line_length; // hind * 150 + 120;
    double y = 0; //gl_height - 8 * line_height;
    
    int x_pos = 200;//189; // text lines x_position
    if (1) {//self->shading){
      glColor4f(0, 0, 0, 0.7);
      glBegin(GL_QUADS);
      glVertex2f(x, y);
      glVertex2f(x + line_length, y); // 21 is the number of chars in the box

      glVertex2f(x + line_length, y + 8 * line_height); // 21 is the number of chars in the box
      glVertex2f(x       , y + 8 * line_height);
      glEnd();
    }

    glColor3fv(colors[0]);
    glRasterPos2f(x, y + 1 * line_height);
    glutBitmapString(font, (unsigned char*) line1);

    glColor3fv(colors[0]);
    glRasterPos2f(x, y + 2 * line_height);
    glutBitmapString(font, (unsigned char*) line2);

    glColor3fv(colors[0]);
    glRasterPos2f(x, y + 3 * line_height);
    glutBitmapString(font, (unsigned char*) line3);

    glColor3fv(colors[0]);
    glRasterPos2f(x, y + 4 * line_height);
    glutBitmapString(font, (unsigned char*) line4);

    glColor3fv(colors[2]);
    glRasterPos2f(x, y + 5 * line_height);
    glutBitmapString(font, (unsigned char*) line5);

    glColor3fv(colors[2]);
    glRasterPos2f(x, y + 6 * line_height);
    glutBitmapString(font, (unsigned char*) line6);

    glColor3fv(colors[2]);
    glRasterPos2f(x, y + 7 * line_height);
    glutBitmapString(font, (unsigned char*) line7);
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib ();

    if(0){
  
        glLineWidth(2.0);
        glPushMatrix();
        if (self->visual_goal_type == VISUAL_GOAL_GLOBAL){
            glColor3f(0,1,1); //cyan
        }else{
            glColor3f(0.65,0.16,0.16); //brown
        }
        glBegin(GL_LINE_STRIP);
        glVertex3f(self->local_pointpose_from[0], self->local_pointpose_from[1],
                   self->local_pointpose_from[2] );
        glVertex3f(self->local_pointpose_to[0], self->local_pointpose_to[1],
                   self->local_pointpose_to[2] );

        glEnd();
        glPopMatrix();
  
  
    

  
  
        // select xy (theta) point
        // convert to local space
        // determine if within feasable region
        // draw
  
  
  
        // Determine the left and right max turning circles:
        Eigen::Isometry3d offset;
        offset.setIdentity();
        offset.translation()  << 0, self->min_turn_radius,0;
        self->center_arc_left  = self->robot_pose * offset;
        offset.translation()  << 0,- (self->min_turn_radius) ,0;
        self->center_arc_right = self->robot_pose * offset;  
  
        // Circles representing the left and right wheels at the limits:
        glLineWidth(2.0);
        glPushMatrix();
        glTranslatef(self->center_arc_left.translation().x() , self->center_arc_left.translation().y() , self->center_arc_left.translation().z());
        glColor3f(0,0.5,1);
        bot_gl_draw_circle( self->min_turn_radius - self->wheel_separation/2.0 ); // inner radius
        glColor3f(1.0,0.1,0);
        bot_gl_draw_circle( self->min_turn_radius );
        glPopMatrix();
        glPushMatrix();
        glTranslatef(self->center_arc_right.translation().x() , self->center_arc_right.translation().y() , self->center_arc_right.translation().z());
        glColor3f(0,0.5,1);
        bot_gl_draw_circle( self->min_turn_radius - self->wheel_separation/2.0 ); // inner radius
        glColor3f(1.0,0.1,0);
        bot_gl_draw_circle( self->min_turn_radius );
        glPopMatrix();

        // Maximum permissable distance arc:
        double max_permissable_travel_distance =  self->goal_timeout * self->max_velocity;
        //cout << "max_permissable_travel_distance: " << max_permissable_travel_distance << "\n";
        glColor3f(1.0,0.1,0);
        glPushMatrix();
        glTranslatef(self->robot_pose.translation().x() , self->robot_pose.translation().y() , self->robot_pose.translation().z());
        glRotatef(self->robot_yaw*180/M_PI , 0, 0, 1.0); // rotate on z-axis
        // Line straight out in front:
        glBegin(GL_LINE_STRIP);
        glVertex2f(0.0,0.0);
        glVertex2f(max_permissable_travel_distance, 0);
        glEnd();
        for (int i=-1; i<=1 ; i=i+2){
            glBegin(GL_LINE_STRIP);
            glVertex2f(max_permissable_travel_distance, 0);
            for (double count = 0.001; count < 1 ; count =count+0.01){
                double r = 1/count;
                double circ = 2*M_PI*r;  
                double fraction =  max_permissable_travel_distance/circ;
                double boundary_x = r*sin( fraction *2*M_PI);
                double boundary_y = r*cos( fraction *2*M_PI);

                if (sqrt(boundary_x*boundary_x +  (boundary_y)*(boundary_y))  <  self->min_turn_radius ){
                    // cout << r << " \n";
                    break; 
                }
      
                if (i > 0){
                    glVertex2f(boundary_x, boundary_y - r);
                }else{
                    glVertex2f(boundary_x, r -boundary_y );
                }
            }
  
            double r = self->min_turn_radius;
            double circ = 2*M_PI*r;  
            double fraction =  max_permissable_travel_distance/circ;
            double boundary_x = r*sin( fraction *2*M_PI);
            double boundary_y = r*cos( fraction *2*M_PI);

            if (i > 0){
                glVertex2f(boundary_x, boundary_y - r);
            }else{
                glVertex2f(boundary_x, r -boundary_y );
            }
  
            glEnd();
        }
        glPopMatrix();
  

        // Determine if end goal is within the permissable region:
        // NB: this code mixes the relative openGL axes and the relative northings|eastings  axes
        // 1. Convert the point into a relative angle and range
        double dx = self->click_pos.x - self->robot_pose.translation().x(); 
        double dy = self->click_pos.y - self->robot_pose.translation().y();
        //cout << dx << " and " << dy <<" top\n";
        double click_rel_heading =  atan2(dy, dx)   - self->robot_yaw;
        click_rel_heading  = WrapPosNegPI (click_rel_heading );   // click rel heading: + to left | - to right. forward 0 behind +/ pi
        double click_pos_rel_range = sqrt(dx*dx + dy*dy);
        // Only used below for renderering:
        double gl_click_pos_rel_x =  click_pos_rel_range * cos(click_rel_heading);
        double gl_click_pos_rel_y =  click_pos_rel_range * sin(click_rel_heading);
        ////////////////////////////////////
        click_rel_heading = click_rel_heading + M_PI/2; // convert to theta=0 at eastings convention
  
        // 2. Determine the radius and path length
        double xyframe_rel_x = click_pos_rel_range * cos(click_rel_heading);
        double xyframe_rel_y = click_pos_rel_range * sin(click_rel_heading);  
        double path_R =100;
        double path_MaxTheta = 0.8;
        double path_MinTheta =0;
        double path_center=0;
        double path_Angle = 0;
        if (click_rel_heading >=  M_PI/2){ //LHS of robot
            double alpha = M_PI/2 - click_rel_heading;
            double h = xyframe_rel_y/(2* sin(alpha));
            path_R = abs( h/sin( click_rel_heading) );
            path_center = path_R;
            path_Angle = abs(alpha*2);
            path_MaxTheta = path_Angle;
            path_MinTheta =0;
        }else{ // RHS of robot
            double alpha = M_PI/2 - click_rel_heading;
            double h = xyframe_rel_y/(2* sin(alpha));
            path_R = abs( h/sin( click_rel_heading) );
            path_center = -path_R;
            path_Angle = abs(alpha*2);
            path_MaxTheta =M_PI ;
            path_MinTheta =M_PI  - path_Angle;
        }
        double frac = path_Angle / (2*M_PI);
        double proposed_distance = frac *2*M_PI * path_R;
        //cout << "frac: " << frac << "\n";
        //cout << "proposed_distance: " << proposed_distance << "\n";
  
        // the end goal in openGL coordinates:
        glColor3f(0, 1,0.5 );
        glPushMatrix();
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glVertex2f(self->click_pos.x, self->click_pos.y);
        glEnd();
        glPopMatrix();  
        // the end goal in the robot frame:
        glColor3f(0.0, 0.4,0.05 );
        glPushMatrix();
        glTranslatef(self->robot_pose.translation().x() , self->robot_pose.translation().y() , self->robot_pose.translation().z());
        glRotatef(self->robot_yaw*180/M_PI , 0, 0, 1.0); // rotate on z-axis
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glVertex2f(gl_click_pos_rel_x, gl_click_pos_rel_y);
        glEnd();
        glPopMatrix();      

        // if in permisable region - draw it.
        if ((proposed_distance < max_permissable_travel_distance) && 
            (path_R > self->min_turn_radius )) {
  
            glPushMatrix();
            glTranslatef(self->robot_pose.translation().x() , self->robot_pose.translation().y() , self->robot_pose.translation().z());
            glRotatef(self->robot_yaw*180/M_PI , 0, 0, 1.0); // rotate on z-axis
            // OpenGL is x forward, y left
            double path_x, path_y;
            glBegin(GL_LINE_STRIP);
            for (double i= path_MinTheta;i< path_MaxTheta ; i=i+0.025) {
                path_x = path_R*sin(i);
                path_y = - path_R*cos(i);
                glVertex2f(path_x, path_y + path_center); 
            }
            path_x = path_R*sin(path_MaxTheta);
            path_y = - path_R*cos(path_MaxTheta);
            glVertex2f(path_x, path_y + path_center);
            glEnd();
            glPopMatrix();
            bot_viewer_set_status_bar_message(self->viewer, "Valid Goal, click \"Seek Relative\" to send it");
        }else{
            bot_viewer_set_status_bar_message(self->viewer, "Invalid Goal: too far or too tight");
        }
    }
}

static void
recompute_2d_goal_pose(RendererDriving *self)
{

    self->click_pos = self->drag_start_local;
    double dx = self->drag_finish_local.x - self->drag_start_local.x;
    double dy = self->drag_finish_local.y - self->drag_start_local.y;

    double theta = atan2(dy,dx);
    self->theta = theta;

    self->goal_std = sqrt(dx*dx + dy*dy);
    if(self->goal_std < MIN_STD)
        self->goal_std = MIN_STD;
    if(self->goal_std > MAX_STD)
        self->goal_std = MAX_STD;
    self->max_draw_utime = bot_timestamp_now() + DRAW_PERSIST_SEC * 1000000;

}

static int 
mouse_press (BotViewer *viewer, BotEventHandler *ehandler, const double ray_start[3], 
             const double ray_dir[3], const GdkEventButton *event)
{
    RendererDriving *self = (RendererDriving*) ehandler->user;

    //fprintf(stderr, "Active: %d | Mouse Press : %f,%f\n",self->active, ray_start[0], ray_start[1]);

    self->dragging = 0;

    if(self->active==0){
        // fprintf(stderr, "Not Active\n");
        return 0;
    }

    if(event->button != 1){
        //fprintf(stderr,"Wrong Button\n");
        return 0;
    }

    point2d_t click_pt_local;
    if (0 != geom_ray_z_plane_intersect_3d(POINT3D(ray_start),
                                           POINT3D(ray_dir), 0, &click_pt_local)) {
        bot_viewer_request_redraw(self->viewer);
        self->active = 0;
        return 0;
    }

    self->dragging = 1;

    self->drag_start_local = click_pt_local;
    self->drag_finish_local = click_pt_local;

    recompute_2d_goal_pose(self);

    bot_viewer_request_redraw(self->viewer);
    return 1;
}



static int mouse_release(BotViewer *viewer, BotEventHandler *ehandler,
                         const double ray_start[3], const double ray_dir[3],
                         const GdkEventButton *event)
{
    RendererDriving *self = (RendererDriving*) ehandler->user;

    if (self->dragging) {
        self->dragging = 0;
    }
    if (self->active != 0) {
        // check drag points and publish

        printf("x,y,t: %f %f %f.    std: %f\n",self->click_pos.x
               ,self->click_pos.y,self->theta,self->goal_std);

        fprintf(stderr,"Localizer Button Released => Activate Value : %d\n", self->active);
        if(self->active == 1){
            drc_localize_reinitialize_cmd_t msg;
            msg.utime = self->robot_utime; //bot_timestamp_now();
            msg.mean[0] = self->click_pos.x;
            msg.mean[1] = self->click_pos.y;
            msg.mean[2] = self->theta;

            double v = self->goal_std * self->goal_std;
            msg.variance[0] = v;
            msg.variance[1] = v;
            msg.variance[2] = VARIANCE_THETA;
            fprintf(stderr, "Sending LOCALIZE_REINITIALIZE\n");
            drc_localize_reinitialize_cmd_t_publish(self->lc, "LOCALIZE_REINITIALIZE", &msg);
            bot_viewer_set_status_bar_message(self->viewer, "Sent LOCALIZE_REINITIALIZE");

            self->circle_color[0] = 1;
            self->circle_color[1] = 0;
            self->circle_color[2] = 0;
        }else if (self->active ==2){
            drc_nav_goal_timed_t msg;
            msg.utime = self->robot_utime; //bot_timestamp_now();
            msg.timeout = (int64_t) 1E6*self->goal_timeout;

            msg.goal_pos.translation.x = self->click_pos.x;
            msg.goal_pos.translation.y = self->click_pos.y;
            msg.goal_pos.translation.z = 0;
            double rpy[] = {0,0,self->theta};
            double quat_out[4];
            bot_roll_pitch_yaw_to_quat(rpy, quat_out); // its in w,x,y,z format
            msg.goal_pos.rotation.w = quat_out[0];
            msg.goal_pos.rotation.x = quat_out[1];
            msg.goal_pos.rotation.y = quat_out[2];
            msg.goal_pos.rotation.z = quat_out[3];
            fprintf(stderr, "Sending NAV_GOAL_TIMED\n");
            drc_nav_goal_timed_t_publish(self->lc, "NAV_GOAL_TIMED", &msg);
            bot_viewer_set_status_bar_message(self->viewer, "Sent NAV_GOAL_TIMED");
        }else if (self->active ==3){
            drc_nav_goal_t msg;
            msg.utime = self->robot_utime; // bot_timestamp_now();

            msg.goal_pos.translation.x = self->click_pos.x;
            msg.goal_pos.translation.y = self->click_pos.y;
            msg.goal_pos.translation.z = 2; // hard coded for now
            double rpy[] = {0,0,self->theta};
            double quat_out[4];
            bot_roll_pitch_yaw_to_quat(rpy, quat_out); // its in w,x,y,z format
            msg.goal_pos.rotation.w = quat_out[0];
            msg.goal_pos.rotation.x = quat_out[1];
            msg.goal_pos.rotation.y = quat_out[2];
            msg.goal_pos.rotation.z = quat_out[3];
            fprintf(stderr, "Sending NAV_GOAL_LEFT_HAND\n");
            drc_nav_goal_t_publish(self->lc, "NAV_GOAL_LEFT_HAND", &msg);
            bot_viewer_set_status_bar_message(self->viewer, "Sent NAV_GOAL_LEFT_HAND");
        }
        self->last_active = self->active;
        self->active = 0;

        //ehandler->picking = 0;
        return 0;
    }

    return 0;
}

static int mouse_motion (BotViewer *viewer, BotEventHandler *ehandler,
                         const double ray_start[3], const double ray_dir[3],
                         const GdkEventMotion *event)
{
    RendererDriving *self = (RendererDriving*) ehandler->user;

    if(!self->dragging || self->active==0)
        return 0;

    point2d_t drag_pt_local;
    if (0 != geom_ray_z_plane_intersect_3d(POINT3D(ray_start),
                                           POINT3D(ray_dir), 0, &drag_pt_local)) {
        return 0;
    }
    self->drag_finish_local = drag_pt_local;
    recompute_2d_goal_pose(self);

    bot_viewer_request_redraw(self->viewer);
    return 1;
}

gboolean heartbeat_cb (gpointer data)
{
    RendererDriving *self = (RendererDriving *) data;

    if (!self) return false;
    if (!self->last_tld_tracker_update) return false; 
    drc_system_status_t status_msg;
    if(self->last_tld_tracker_update) { 
        bool good = bot_timestamp_now() - self->last_tld_tracker_update <= 2 * 1e6;
        std::stringstream ss; ss << "DRIVING HEADING:" << ( good ? "GOOD" : "BAD");  
        status_msg.importance = ( good ? DRC_SYSTEM_STATUS_T_VERY_IMPORTANT : DRC_SYSTEM_STATUS_T_VERY_IMPORTANT );// use enums!!
        status_msg.utime =  self->last_tld_tracker_update;
        status_msg.value = (char*)ss.str().c_str();
    } 

    if (self->last_road_detection_update) {
        bool good = (bot_timestamp_now() - self->last_road_detection_update <= 2 * 1e6);
        std::stringstream ss; ss << "ROAD DETECT:" << ( good ? "GOOD" : "BAD");  
        status_msg.importance = ( good ? DRC_SYSTEM_STATUS_T_VERY_IMPORTANT : DRC_SYSTEM_STATUS_T_VERY_IMPORTANT );// use enums!!
        status_msg.utime =  self->last_road_detection_update;
        status_msg.value = (char*)ss.str().c_str();
    }
    status_msg.system = DRC_SYSTEM_STATUS_T_TRACKING;// use enums!!
    status_msg.frequency = DRC_SYSTEM_STATUS_T_MEDIUM_FREQUENCY;// use enums!!
    drc_system_status_t_publish(self->lc, "SYSTEM_STATUS", &status_msg);
    // printf("Heartbeat tracker update %s\n", ss.str().c_str());
    
    return true;
}

void activate(RendererDriving *self, int type)
{
    self->active = type;

    self->goal_std=0;
    if (self->active ==1){
        self->circle_color[0] = 1;
        self->circle_color[1] = 0;
        self->circle_color[2] = 0;
    }else if (self->active ==2){
        self->circle_color[0] = 0;
        self->circle_color[1] = 1;
        self->circle_color[2] = 0;
    }else if (self->active ==3){
        self->circle_color[0] = 0;
        self->circle_color[1] = 0;
        self->circle_color[2] = 1;
    }
}

static int key_press (BotViewer *viewer, BotEventHandler *ehandler, 
                      const GdkEventKey *event)
{
    RendererDriving *self = (RendererDriving*) ehandler->user;

    return 0;
}



static void send_seek_goal_visual (RendererDriving *self) {
  
    drc_seek_goal_timed_t msgout;
    msgout.utime = self->robot_utime; //bot_timestamp_now();
    msgout.timeout = (int64_t) 1E6*self->goal_timeout; //self->goal_timeout;
    msgout.robot_name = "atlas"; // this should be set from robot state message
    if (self->visual_goal_type == VISUAL_GOAL_GLOBAL){
        msgout.type = DRC_SEEK_GOAL_TIMED_T_VISUAL_GLOBAL;
        fprintf(stderr, "Sending SEEK_GOAL (Visual, Global)\n");
        drc_seek_goal_timed_t_publish(self->lc, "SEEK_GOAL", &msgout); 
        bot_viewer_set_status_bar_message(self->viewer, "Sent SEEK_GOAL (Visual, Global) ");
    }else if (self->visual_goal_type == VISUAL_GOAL_RELATIVE){
        msgout.type = DRC_SEEK_GOAL_TIMED_T_VISUAL_RELATIVE;
        fprintf(stderr, "Sending SEEK_GOAL (Visual, Relative)\n");
        drc_seek_goal_timed_t_publish(self->lc, "SEEK_GOAL", &msgout);       
        bot_viewer_set_status_bar_message(self->viewer, "Sent SEEK_GOAL (Visual, Relative)");
    }  
}


static void on_param_widget_changed(BotGtkParamWidget *pw, const char *name, void *user)
{
    RendererDriving *self = (RendererDriving*) user;

    // Disable certain sliders depending upon goal mode
    goal_type_t goal_type_temp  = (goal_type_t) bot_gtk_param_widget_get_enum(self->pw, PARAM_GOAL_TYPE);

    if(bot_gtk_param_widget_get_bool (self->pw, PARAM_FULL_ACCELERATION)){
      bot_gtk_param_widget_set_enabled(pw, PARAM_THROTTLE_DURATION, 0);
    }
    else{
      bot_gtk_param_widget_set_enabled(pw, PARAM_THROTTLE_DURATION, 1);
    }

    if(bot_gtk_param_widget_get_bool (self->pw, PARAM_ENABLE_STEERING_ANGLE_OFFSET)){        
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE_OFFSET, 1);
    }
    else{
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE_OFFSET, 0);
    }

    if (goal_type_temp == USE_ROAD_CARROT) {
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_P_GAIN, 1);
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_D_GAIN, 1);
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_I_GAIN, 1);
    }
    else {
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_P_GAIN, 0);
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_D_GAIN, 0);
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_I_GAIN, 0);
    }

    if (goal_type_temp == USE_USER_GOAL) {
        if (self->steer_have_manip_map)
            bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE, 1);
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_LOOKAHEAD, 0);
    }
    else {
        if (self->steer_have_manip_map)
            bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE, 0);
        bot_gtk_param_widget_set_enabled (self->pw, PARAM_LOOKAHEAD, 1);
    }
    

    if(!strcmp(name, PARAM_VISUAL_GOAL)) {
        send_seek_goal_visual(self);
    }
    else if(!strcmp(name, PARAM_MAX_STEERING)){
      //make sure that the value is above or equal to the min 
      double min = bot_gtk_param_widget_get_double (self->pw, PARAM_MIN_STEERING);
      double max = bot_gtk_param_widget_get_double (self->pw, PARAM_MAX_STEERING);
      if(max < min){
	bot_gtk_param_widget_set_double(self->pw, PARAM_MAX_STEERING, min);
	bot_viewer_set_status_bar_message(self->viewer, "Max value is smaller than min");
      }      
    }
    else if(!strcmp(name, PARAM_MIN_STEERING)){
      //make sure that the value is above or equal to the min 
      double min = bot_gtk_param_widget_get_double (self->pw, PARAM_MIN_STEERING);
      double max = bot_gtk_param_widget_get_double (self->pw, PARAM_MAX_STEERING);
      if(min > max){
	bot_gtk_param_widget_set_double(self->pw, PARAM_MIN_STEERING, max);
	bot_viewer_set_status_bar_message(self->viewer, "Min value is larger than max");
      }      
    }
    else if(!strcmp(name, PARAM_MAX_GAS)){
      //make sure that the value is above or equal to the min 
      double min = bot_gtk_param_widget_get_double (self->pw, PARAM_MIN_GAS);
      double max = bot_gtk_param_widget_get_double (self->pw, PARAM_MAX_GAS);
      if(max < min){
	bot_gtk_param_widget_set_double(self->pw, PARAM_MAX_GAS, min);
	bot_viewer_set_status_bar_message(self->viewer, "Max value is smaller than min");
      }      
    }
    else if(!strcmp(name, PARAM_MIN_GAS)){
      //make sure that the value is above or equal to the min 
      double min = bot_gtk_param_widget_get_double (self->pw, PARAM_MIN_GAS);
      double max = bot_gtk_param_widget_get_double (self->pw, PARAM_MAX_GAS);
      if(min > max){
	bot_gtk_param_widget_set_double(self->pw, PARAM_MIN_GAS, max);
	bot_viewer_set_status_bar_message(self->viewer, "Min value is larger than max");
      }      
    }
    else if(!strcmp(name, PARAM_UPDATE_MANIP_MAP)){      
      //make sure that the value is above or equal to the min 
        double min_s = bot_gtk_param_widget_get_double (self->pw, PARAM_MIN_STEERING);
        double max_s = bot_gtk_param_widget_get_double (self->pw, PARAM_MAX_STEERING);
        double min_g = bot_gtk_param_widget_get_double (self->pw, PARAM_MIN_GAS);
        double max_g = bot_gtk_param_widget_get_double (self->pw, PARAM_MAX_GAS);
        drc_driving_affordance_status_t msg;
        msg.utime = bot_timestamp_now();
        msg.aff_type = "car";
        msg.aff_uid = 1;   // affordance uid
        msg.dof_name = "steering";
        msg.ee_name = "left_hand";
        
        msg.have_manip_map = 1; 
        
        msg.dof_value_0 = bot_to_radians(min_s); // initial dof value in the manip map
        msg.dof_value_1 = bot_to_radians(max_s); // final dof value in the manip map
        
        drc_driving_affordance_status_t_publish(self->lc, "DRIVING_STEERING_ACTUATION_STATUS", &msg);

        drc_driving_affordance_status_t msg_g;
        msg_g.utime = bot_timestamp_now();
        msg_g.aff_type = "car";
        msg_g.aff_uid = 1;   // affordance uid
        msg_g.dof_name = "gas";
        msg_g.ee_name = "right_foot";
        
        msg_g.have_manip_map = 1; 
        
        msg_g.dof_value_0 = min_g; // initial dof value in the manip map
        msg_g.dof_value_1 = max_g; // final dof value in the manip map
        
        drc_driving_affordance_status_t_publish(self->lc, "DRIVING_GAS_ACTUATION_STATUS", &msg_g);
    }
    else if(!strcmp(name, PARAM_GOAL_SEND)) {
        fprintf(stderr,"\nClicked NAV_GOAL_TIMED\n");
        //bot_viewer_request_pick (self->viewer, &(self->ehandler));
        activate(self, 2);
    }
    else if(!strcmp(name, PARAM_UPDATE_CAR_MODEL_REDUCED)) {
        fprintf(stderr,"\nUpdating car affordance\n");	
	if(self->car_affordance){
	  free(self->car_affordance->modelfile);
	  //self->car_affordance->utime = 0;
	  self->car_affordance->aff_store_control = 1;
	  self->car_affordance->modelfile = strdup("car_cabin_2cm.pcd");
	  drc_affordance_t_publish(self->lc, "AFFORDANCE_TRACK", self->car_affordance);
	}
	else{
	  fprintf(stderr, "No car affordance message\n");	  
	}	
    }
    else if(!strcmp(name, PARAM_UPDATE_CAR_MODEL_FULL)) {
        fprintf(stderr,"\nUpdating car affordance\n");	
	if(self->car_affordance){
	  free(self->car_affordance->modelfile);
	  //self->car_affordance->utime = 0;
	  self->car_affordance->aff_store_control = 1;
	  self->car_affordance->modelfile = strdup("car.pcd");
	  drc_affordance_t_publish(self->lc, "AFFORDANCE_TRACK", self->car_affordance);
	}
	else{
	  fprintf(stderr, "No car affordance message\n");	  
	}	
    }    
    else if(!strcmp(name, PARAM_STOP_CONTROLLER)){
        bot_viewer_set_status_bar_message(self->viewer, "Sent STOP to Controller");
        drc_driving_cmd_t msg;
        msg.utime = bot_timestamp_now();
        msg.drive_duration = 0;
        msg.throttle_ratio = 0;
        msg.throttle_duration = 0;

        int do_braking = 0;
        if(bot_gtk_param_widget_get_bool (self->pw, PARAM_DO_BRAKING)){
            do_braking = 1;
        }

        msg.kp_steer = bot_gtk_param_widget_get_double(self->pw, PARAM_P_GAIN);
        //msg.kd_steer = bot_gtk_param_widget_get_double(self->pw, PARAM_D_GAIN);
        //msg.ki_steer = bot_gtk_param_widget_get_double(self->pw, PARAM_I_GAIN);
        msg.lookahead_dist = 10 * bot_gtk_param_widget_get_double(self->pw, PARAM_LOOKAHEAD);
        msg.steering_angle_degrees = -bot_gtk_param_widget_get_double(self->pw, PARAM_STEERING_ANGLE);

        goal_type_t goal_type = (goal_type_t) bot_gtk_param_widget_get_enum(self->pw, PARAM_GOAL_TYPE);
        
        if(do_braking){
            if(goal_type == USE_ROAD_CARROT){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_CARROT_B;
            }
            if(goal_type == USE_ROAD_ARC){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_ARC_B;
            }
            else if(goal_type == USE_TLD_WITH_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_WITH_ROAD_B;
            }
            else if(goal_type == USE_TLD_IGNORE_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_IGNORE_ROAD_B;
            }
            else if(goal_type == USE_USER_GOAL){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_USER_HEADING_B;
            }    
        }  
        else{
            if(goal_type == USE_ROAD_CARROT){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_CARROT_NB;
            }
            if(goal_type == USE_ROAD_ARC){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_ARC_NB;
            }
            else if(goal_type == USE_TLD_WITH_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_WITH_ROAD_NB;
            }
            else if(goal_type == USE_TLD_IGNORE_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_IGNORE_ROAD_NB;
            }
            else if(goal_type == USE_USER_GOAL){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_USER_HEADING_NB;
            }
        }
        
        drc_driving_cmd_t_publish(self->lc, "DRIVING_CONTROLLER_COMMAND", &msg);        
    }
    else if(!strcmp(name, PARAM_SEND_DRIVE_COMMAND)){
        drc_driving_cmd_t msg;
        msg.utime = bot_timestamp_now();
        msg.drive_duration = (bot_gtk_param_widget_get_double(self->pw, PARAM_DRIVE_TIME)) * 10;
        msg.throttle_ratio = (bot_gtk_param_widget_get_double(self->pw, PARAM_THROTTLE_RATIO)) * 100;

	if(bot_gtk_param_widget_get_bool (self->pw, PARAM_FULL_ACCELERATION)){
	  msg.throttle_duration = msg.drive_duration;
	}
	else{
	  msg.throttle_duration = fmin(bot_gtk_param_widget_get_double(self->pw, PARAM_THROTTLE_DURATION)*10, msg.drive_duration);
	}

        int do_braking = 0;
        if(bot_gtk_param_widget_get_bool (self->pw, PARAM_DO_BRAKING)){
            do_braking = 1;
        }

	fprintf(stderr, "Drive duration : %f Acceleration duration : %f\n", (msg.drive_duration) / 10.0, (msg.throttle_duration)/10.0);

        msg.kp_steer = bot_gtk_param_widget_get_double(self->pw, PARAM_P_GAIN);
        //msg.kd_steer = bot_gtk_param_widget_get_double(self->pw, PARAM_D_GAIN);
        //msg.ki_steer = bot_gtk_param_widget_get_double(self->pw, PARAM_I_GAIN);
        msg.lookahead_dist = (bot_gtk_param_widget_get_double(self->pw, PARAM_LOOKAHEAD)) * 10;
        msg.steering_angle_degrees = -bot_gtk_param_widget_get_double(self->pw, PARAM_STEERING_ANGLE);

        goal_type_t goal_type = (goal_type_t) bot_gtk_param_widget_get_enum(self->pw, PARAM_GOAL_TYPE);
        if(do_braking){
            if(goal_type == USE_ROAD_CARROT){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_CARROT_B;
            }
            if(goal_type == USE_ROAD_ARC){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_ARC_B;
            }
            else if(goal_type == USE_TLD_WITH_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_WITH_ROAD_B;
            }
            else if(goal_type == USE_TLD_IGNORE_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_IGNORE_ROAD_B;
            }
            else if(goal_type == USE_USER_GOAL){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_USER_HEADING_B;
            }
        }       
        else{
            if(goal_type == USE_ROAD_CARROT){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_CARROT_NB;
            }
            if(goal_type == USE_ROAD_ARC){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_ROAD_LOOKAHEAD_ARC_NB;
            }
            else if(goal_type == USE_TLD_WITH_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_WITH_ROAD_NB;
            }
            else if(goal_type == USE_TLD_IGNORE_ROAD){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_TLD_LOOKAHEAD_IGNORE_ROAD_NB;
            }
            else if(goal_type == USE_USER_GOAL){
                msg.type = DRC_DRIVING_CMD_T_TYPE_USE_USER_HEADING_NB;
            }
        }
        drc_driving_cmd_t_publish(self->lc, "DRIVING_CONTROLLER_COMMAND", &msg);
    }
    
    bot_viewer_request_redraw(self->viewer);
}

static void on_affordance_collection(const lcm_recv_buf_t * buf, const char *channel, 
                                const drc_affordance_collection_t *msg, void *user){
  RendererDriving *self = (RendererDriving*) user;
  drc_affordance_t *car_affordance = NULL;
  for(int i = 0; i < msg->naffs; i++){
    if(!strcmp("car", msg->affs[i].otdf_type)){
      if(self->car_affordance != NULL){
	drc_affordance_t_destroy(self->car_affordance);
      }
      self->car_affordance = drc_affordance_t_copy(&msg->affs[i]);      
    }
  }
}

static void on_est_robot_state (const lcm_recv_buf_t * buf, const char *channel, 
                                const drc_robot_state_t *msg, void *user){
    RendererDriving *self = (RendererDriving*) user;
  
    self->robot_utime =msg->utime;
  
    // Remove the pose roll and pitch:
    Eigen::Quaterniond quat = Eigen::Quaterniond(msg->pose.rotation.w, msg->pose.rotation.x, 
                                                 msg->pose.rotation.y, msg->pose.rotation.z);
    double ypr[3];
    quat_to_euler(quat, ypr[0], ypr[1], ypr[2]);
    ypr[1] =0; ypr[2] =0;
    Eigen::Quaterniond quat_yaw_only = euler_to_quat(ypr[0], ypr[1], ypr[2]);
    self->robot_pose.setIdentity();
    self->robot_pose.translation()  << msg->pose.translation.x, msg->pose.translation.y, msg->pose.translation.z;
    self->robot_pose.rotate(quat_yaw_only);    
    self->robot_yaw = ypr[0]; // for convenece keep the yaw

    bot_viewer_request_redraw(self->viewer);
}

static void on_controller_values(const lcm_recv_buf_t * buf, const char *channel, 
                                 const drc_driving_controller_values_t *msg, void *user){
    RendererDriving *self = (RendererDriving*) user;

    if(self->controller_values)
        drc_driving_controller_values_t_destroy(self->controller_values);
    self->controller_values = drc_driving_controller_values_t_copy(msg);

    bot_viewer_request_redraw(self->viewer);
}

static void on_driving_affordance_status(const lcm_recv_buf_t * buf, const char *channel, 
                                         const drc_driving_affordance_status_t *msg, void *user){
    RendererDriving *self = (RendererDriving*) user;

    if (!strcmp (channel, "DRIVING_STEERING_ACTUATION_STATUS")) {
        double new_min = min(msg->dof_value_0, msg->dof_value_1);
        double new_max = max(msg->dof_value_0, msg->dof_value_1);
        if (msg->have_manip_map) {
            if ( (fabs (new_min - self->steer_min_rad) > 0.01) || (fabs (new_max - self->steer_max_rad) > 0.01) ) {
                double value = bot_gtk_param_widget_get_double (self->pw, PARAM_STEERING_ANGLE);
                fprintf (stdout, "changing range to [%f, %f] and setting value %f\n", bot_to_degrees(new_min), bot_to_degrees(new_max), value);
                bot_gtk_param_widget_modify_double (self->pw, PARAM_STEERING_ANGLE, bot_to_degrees(new_min),
                                                    bot_to_degrees(new_max), PARAM_STEERING_DELTA, value);
                bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE, 1);
            }
        }
        else 
            bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE, 0);

        self->steer_have_manip_map = msg->have_manip_map;
        self->steer_min_rad = new_min;
        self->steer_max_rad = new_max;

       
    } else if (!strcmp (channel, "DRIVING_GAS_ACTUATION_STATUS")) {

        double new_min = min(msg->dof_value_0, msg->dof_value_1);
        double new_max = max(msg->dof_value_0, msg->dof_value_1);

        if (msg->have_manip_map) {
            if ( (fabs (new_min - self->gas_pedal_min) > 0.01) || (fabs (new_max - self->gas_pedal_max) > 0.01) ) {
                double value = bot_gtk_param_widget_get_double (self->pw, PARAM_THROTTLE_RATIO);
                bot_gtk_param_widget_modify_double (self->pw, PARAM_THROTTLE_RATIO, new_min, new_max,
                                                    PARAM_THROTTLE_DELTA, value);
            }
            bot_gtk_param_widget_set_enabled (self->pw, PARAM_THROTTLE_RATIO, 1);
        }
        else
            bot_gtk_param_widget_set_enabled (self->pw, PARAM_THROTTLE_RATIO, 0);

        self->gas_pedal_have_manip_map = msg->have_manip_map;
        self->gas_pedal_min = new_min;
        self->gas_pedal_max = new_max;
        

    } else if (!strcmp (channel, "DRIVING_BRAKE_ACTUATION_STATUS")) {
        double new_min = min(msg->dof_value_0, msg->dof_value_1);
        double new_max = max(msg->dof_value_0, msg->dof_value_1);
        if (msg->have_manip_map) {
            if ( (fabs (new_min - self->brake_pedal_min) > 0.01) || (fabs (new_max - self->brake_pedal_max) > 0.01) ) {
                double value = bot_gtk_param_widget_get_double (self->pw, PARAM_BRAKE);
                bot_gtk_param_widget_modify_double (self->pw, PARAM_BRAKE, new_min, new_max,
                                                    PARAM_BRAKE_DELTA, value);
            }
            bot_gtk_param_widget_set_enabled (self->pw, PARAM_BRAKE, 1);
        }
        else
            bot_gtk_param_widget_set_enabled (self->pw, PARAM_BRAKE, 0);

        self->brake_pedal_have_manip_map = msg->have_manip_map;
        self->brake_pedal_min = new_min;
        self->brake_pedal_max = new_max;
    }

    return;
}


static void on_driving_controller_status(const lcm_recv_buf_t * buf, const char *channel, 
                                         const drc_driving_controller_status_t *msg, void *user){
    RendererDriving *self = (RendererDriving*) user;

    if(self->controller_status)
        drc_driving_controller_status_t_destroy(self->controller_status);
    self->controller_status = drc_driving_controller_status_t_copy(msg);


    bot_viewer_request_redraw(self->viewer);
}

static void on_driving_ground_truth_status(const lcm_recv_buf_t * buf, const char *channel, 
                                           const drc_driving_status_t *msg, void *user){
    RendererDriving *self = (RendererDriving*) user;

    if(self->ground_truth_status)
        drc_driving_status_t_destroy(self->ground_truth_status);
    self->ground_truth_status = drc_driving_status_t_copy(msg);


    bot_viewer_request_redraw(self->viewer);
}

static void on_pointing_vector(const lcm_recv_buf_t * buf, const char *channel, 
                               const perception_pointing_vector_t *msg, void *user){
    RendererDriving *self = (RendererDriving*) user;
    // cout << "got pointing vector\n";

    self->last_tld_tracker_update = bot_timestamp_now();
    
    // Convert vector into point very far away
    double scale = 100.0;
    double goal_pos[]={scale*msg->vec[0], scale*msg->vec[1],scale* msg->vec[2]};
  
    int status_local,status_body;
    Eigen::Isometry3d cam_to_local;
    Eigen::Isometry3d cam_to_body;
    status_local = self->bot_frames_cpp->get_trans_with_utime( self->bot_frames , "CAMERA",  "local", msg->utime, cam_to_local);
    status_body = self->bot_frames_cpp->get_trans_with_utime(  self->bot_frames , "CAMERA",  "body", msg->utime, cam_to_body);

    if ((0 == status_local) ||(0 == status_body  )) {
        std::cerr << "SensorDataReceiver: cannot get transform from CAMERA to frame" << std::endl;
    }

    //  Eigen::Vector4d pos_vec1= Eigen::Vector4d(goal_pos[0], goal_pos[1], goal_pos[2], 1);
    self->local_pointpose_to  =  cam_to_local* Eigen::Vector4d(goal_pos[0], goal_pos[1], goal_pos[2], 1);
    self->local_pointpose_from=  cam_to_local* Eigen::Vector4d(0,0,0, 1);
  
    Eigen::Vector4d pos_vec= Eigen::Vector4d(goal_pos[0], goal_pos[1], goal_pos[2], 1);
    self->body_pointpose_to  =  cam_to_body*pos_vec;
    Eigen::Vector4d null_vec= Eigen::Vector4d(0,0,0, 1);
    self->body_pointpose_from=  cam_to_body*null_vec;

    // cout << "bottom\n";
  
    /*
      self->robot_utime =msg->utime;
  
      // Remove the pose roll and pitch:
      Eigen::Quaterniond quat = Eigen::Quaterniond(msg->pose.rotation.w, msg->pose.rotation.x, 
      msg->pose.rotation.y, msg->pose.rotation.z);
      double ypr[3];
      quat_to_euler(quat, ypr[0], ypr[1], ypr[2]);
      ypr[1] =0; ypr[2] =0;
      Eigen::Quaterniond quat_yaw_only = euler_to_quat(ypr[0], ypr[1], ypr[2]);
      self->robot_pose.setIdentity();
      self->robot_pose.translation()  << msg->pose.translation.x, msg->pose.translation.y, msg->pose.translation.z;
      self->robot_pose.rotate(quat_yaw_only);    
      self->robot_yaw = ypr[0]; // for convenece keep the yaw

      // Determine the left and right max turning circles:
      Eigen::Isometry3d offset;
      offset.setIdentity();
      offset.translation()  << 0, self->min_turn_radius,0;
      self->center_arc_left = self->robot_pose * offset;
      offset.translation()  << 0,- (self->min_turn_radius) ,0;
      self->center_arc_right = self->robot_pose * offset;
      bot_viewer_request_redraw(self->viewer);*/
}



static void
_free (BotRenderer *renderer)
{
    free (renderer);
}

BotRenderer *renderer_driving_new (BotViewer *viewer, int render_priority, lcm_t *lcm, BotParam * param, BotFrames * frames)
{
    RendererDriving *self = new RendererDriving();
    //  RendererDriving *self = (RendererDriving*) calloc (1, sizeof (RendererDriving));
    self->car_affordance = NULL;
    
    self->viewer = viewer;
    self->renderer.draw = _draw;
    self->renderer.destroy = _free;
    self->renderer.name = RENDERER_NAME;
    self->renderer.user = self;
    self->renderer.enabled = 1;
    
    BotEventHandler *ehandler = &self->ehandler;
    ehandler->name = (char*) RENDERER_NAME;
    ehandler->enabled = 1;
    ehandler->pick_query = NULL;
    ehandler->key_press = key_press;
    ehandler->hover_query = NULL;
    ehandler->mouse_press = mouse_press;
    ehandler->mouse_release = mouse_release;
    ehandler->mouse_motion = mouse_motion;
    ehandler->user = self;

    bot_viewer_add_event_handler(viewer, &self->ehandler, render_priority);

    self->lc = lcm; //globals_get_lcm_full(NULL,1);
    self->bot_param = param;
    self->bot_frames = frames;  
    self->robot_pose.setIdentity();
  
    self->visual_goal_type = VISUAL_GOAL_GLOBAL;
  
    self->goal_timeout =5.0;
    self->min_turn_radius =3.8; /// Assumed Example: http://www.utvguide.net/polaris_ev_lsv.htm
    self->wheel_separation = 1.829; // metres
    self->max_velocity = 2.0; // m/s  - about 4.5MPH or 7.2km/hr
    self->center_arc_left.setIdentity();
    self->center_arc_right.setIdentity();
    self->last_tld_tracker_update = 0; 
    self->last_road_detection_update = 0; 
    self->controller_values = NULL;
    double kp_default;

    int status = bot_param_get_double (param, "driving.control.kp_steer_default", &kp_default);

    if(status == -1){
        fprintf(stderr, "Error getting param - kp_steer\n");
        kp_default = 5.0;
    }

    double lookahead_default;
    status = bot_param_get_double (param, "driving.control.lookahead_distance_default", &lookahead_default);

    if(status == -1){
        fprintf(stderr, "Error getting param - lookahead\n");
        lookahead_default = 10.0;
    }
  
    // TODO: set these to null at init:
    // self->local_pointpose_from | self->local_pointpose_to
  
    drc_robot_state_t_subscribe(self->lc,"EST_ROBOT_STATE",on_est_robot_state,self); 
    perception_pointing_vector_t_subscribe(self->lc,"OBJECT_BEARING",on_pointing_vector,self); 
    
    drc_affordance_collection_t_subscribe(self->lc, "AFFORDANCE_COLLECTION", on_affordance_collection, self);

    // Status messages
    drc_driving_controller_values_t_subscribe(self->lc, "DRIVING_CONTROLLER_COMMAND_VALUES", on_controller_values, self);
    drc_driving_status_t_subscribe (self->lc, "DRC_DRIVING_GROUND_TRUTH_STATUS", on_driving_ground_truth_status, self);
    //drc_driving_controller_status_t_subscribe (self->lc, "DRC_DRIVING_CONTROLLER_STATUS", on_driving_controller_status, self);

    // Actuation manipulation status messages
    drc_driving_affordance_status_t_subscribe (self->lc, "DRIVING_STEERING_ACTUATION_STATUS", on_driving_affordance_status, self);
    drc_driving_affordance_status_t_subscribe (self->lc, "DRIVING_GAS_ACTUATION_STATUS", on_driving_affordance_status, self);
    drc_driving_affordance_status_t_subscribe (self->lc, "DRIVING_BRAKE_ACTUATION_STATUS", on_driving_affordance_status, self);

    // heartbeat for system status monitoring (every 1sec)
    g_timeout_add (1000, heartbeat_cb, self);
  
    self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());
    bot_gtk_param_widget_add_booleans (self->pw, BOT_GTK_PARAM_WIDGET_TOGGLE_BUTTON, PARAM_ENABLE, 0, NULL);
    
    bot_gtk_param_widget_add_buttons(self->pw, PARAM_UPDATE_CAR_MODEL_REDUCED, NULL);
    
    bot_gtk_param_widget_add_buttons(self->pw, PARAM_UPDATE_CAR_MODEL_FULL, NULL);
 
    bot_gtk_param_widget_add_separator (self->pw, "Driving controller Params");

    bot_gtk_param_widget_add_double(self->pw, PARAM_LOOKAHEAD, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 1.0, fmax(20.0, lookahead_default), 0.2, lookahead_default);

    bot_gtk_param_widget_add_double(self->pw, PARAM_P_GAIN, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, fmax(20.0, kp_default), 0.01, kp_default);

    bot_gtk_param_widget_add_double(self->pw, PARAM_I_GAIN, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, 10.0, 0.01, 0);

    bot_gtk_param_widget_add_double(self->pw, PARAM_D_GAIN, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, 10.0, 0.01, 0);

    bot_gtk_param_widget_add_booleans (self->pw, BOT_GTK_PARAM_WIDGET_CHECKBOX, PARAM_DO_BRAKING, 0, NULL);
    
    bot_gtk_param_widget_add_booleans (self->pw, BOT_GTK_PARAM_WIDGET_CHECKBOX, PARAM_ENABLE_STEERING_ANGLE_OFFSET, 0, NULL);

    bot_gtk_param_widget_add_double(self->pw, PARAM_STEERING_ANGLE_OFFSET, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, -20, 20, PARAM_STEERING_DELTA, 0);

    bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE_OFFSET, 0);

    bot_gtk_param_widget_add_separator (self->pw, "Driving Controller");


    bot_gtk_param_widget_add_double(self->pw, PARAM_STEERING_ANGLE, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, -180, 180, PARAM_STEERING_DELTA, 0);
    bot_gtk_param_widget_set_enabled (self->pw, PARAM_STEERING_ANGLE, 0);

    //bot_gtk_param_widget_add_buttons(self->pw, PARAM_UPDATE, NULL);

    bot_gtk_param_widget_add_double(self->pw, PARAM_DRIVE_TIME, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, MAX_DRIVE_TIME, 0.1, DEFAULT_DRIVE_TIME);

    bot_gtk_param_widget_add_booleans(self->pw, BOT_GTK_PARAM_WIDGET_CHECKBOX, PARAM_FULL_ACCELERATION, 1, NULL);

    bot_gtk_param_widget_add_double(self->pw, PARAM_THROTTLE_DURATION, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, MAX_ACCELERATION_TIME, 0.1, DEFAULT_ACCELERATION_TIME);

    bot_gtk_param_widget_add_double(self->pw, PARAM_THROTTLE_RATIO, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, 1.0, PARAM_THROTTLE_DELTA, 0.04);

    bot_gtk_param_widget_add_double(self->pw, PARAM_BRAKE, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, 1.0, PARAM_BRAKE_DELTA, 0.04);

    bot_gtk_param_widget_add_enum(self->pw, PARAM_GOAL_TYPE, BOT_GTK_PARAM_WIDGET_MENU, USE_ROAD_CARROT, "Use Road (Carrot)", USE_ROAD_CARROT, "Use Road (Arc)", USE_ROAD_ARC, "Use TLD (Road)", USE_TLD_WITH_ROAD,  "Use TLD (No Road)", USE_TLD_IGNORE_ROAD, "Use User Goal", USE_USER_GOAL, NULL);

    bot_gtk_param_widget_add_buttons(self->pw, PARAM_SEND_DRIVE_COMMAND, NULL);

    bot_gtk_param_widget_add_buttons(self->pw, PARAM_STOP_CONTROLLER, NULL);
  
    /*bot_gtk_param_widget_add_separator (self->pw, "Simulate Manip Map Threshold");

    bot_gtk_param_widget_add_double(self->pw, PARAM_MAX_STEERING, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, -90, 90, 0.1, 35);

    
    bot_gtk_param_widget_add_double(self->pw, PARAM_MIN_STEERING, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, -90, 90, 0.1, -35);
    
    bot_gtk_param_widget_add_double(self->pw, PARAM_MAX_GAS, 
                                    BOT_GTK_PARAM_WIDGET_SLIDER, 0, 1.0, 0.01, 0.1);

    bot_gtk_param_widget_add_double(self->pw, PARAM_MIN_GAS, 
    BOT_GTK_PARAM_WIDGET_SLIDER, 0, 1.0, 0.01, 0);


    bot_gtk_param_widget_add_buttons(self->pw, PARAM_UPDATE_MANIP_MAP, NULL);*/

    g_signal_connect(G_OBJECT(self->pw), "changed", G_CALLBACK(on_param_widget_changed), self);
    self->renderer.widget = GTK_WIDGET(self->pw);

    //bot_viewer_request_pick (self->viewer, &(self->ehandler));
    
    bot_gtk_param_widget_set_enabled(self->pw, PARAM_THROTTLE_DURATION, 0);
    
    self->active = 0;

    return &self->renderer;
}

void setup_renderer_driving(BotViewer *viewer, int render_priority, lcm_t *lcm, BotParam * param,
                            BotFrames * frames)
{
    bot_viewer_add_renderer_on_side(viewer, renderer_driving_new(viewer, render_priority, lcm, param, frames),
                                    render_priority , 0);
}