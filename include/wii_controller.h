/**
 * @author      : theo (theo@$HOSTNAME)
 * @file        : wii_controller
 * @created     : Tuesday Aug 20, 2019 17:51:30 MDT
 */

#ifndef WII_CONTROLLER_H

#define WII_CONTROLLER_H

// C Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Local Includes
#include "log.h"
#include "robot_control.h"
#include "wiiuse.h"

////////// DATA STRUCTURES //////////

struct controller_s;

/**
 * A button is either triggered or not, and it is associated to a callback
 *
 * @note press_callback is a callback executed on the robot. a controller mode
 * is up to the developer, however, in this itteration, the controller has two
 * modes, normal and advanced. This just adds a second dimension for what
 * buttons do. Generally, this is not a very big number
 */
struct button_s {
  void *(*callback)(struct controller_s *controller, struct robot_s *robot);
  short value;
};

/**
 * An attribute is a characteristic of a controller, a non objective one.
 *
 * @note The callback is a callback upon the robot. Again, there are multiple
 * modes
 */
struct attribute_s {
  void *(*callback)(struct controller_s *controller, struct robot_s *robot);
  float value;
};

typedef struct attribute_s attribute;
typedef struct button_s button;

// TODO Change these.
/**
 * I wanted to use classic bit shifting, but
 * because WIIMOTE_BUTTON_X is a variable and undefined,
 * there is no way to do this smartly. Thus, I want to go into the
 * wiiuse code and change this
 */
struct nunchuk_s {
  button c;
  button z;
  attribute roll;
  attribute pitch;
  attribute yaw;
  attribute angle;
  attribute magnitude;
  attribute vals_x;
  attribute vals_y;
};

struct stick_s {
  button up;
  button down;
  button right;
  button left;
  button a;
  button b;
  button one;
  button two;
  button home;
  button minus;
  button plus;
};

enum controller_state { DRIVE, ADVANCED };
struct controller_s {
  struct nunchuk_s *nunchuk;
  struct stick_s *stick;
  enum controller_state state;
};

/**
 * @brief Sets controller to zero i.e. no buttons pressed
 *
 * @param controller The controller to set to zero
 */
void set_controller_zero(struct controller_s *controller);

/**
 * @brief Handles all robot callbacks on button presses
 *
 * @param robot
 * @param controller
 * @param nunchuk
 */
void execute_callbacks(struct robot_s *robot, struct controller_s *controller);

/**
 * @brief The main event loop for the wii controller and robot
 *
 * This method listens to the wii remote and if something happend, do something
 *
 * @param robot The robot to do stuff to
 * @param wii The wiiremote struct type
 * @param controller The controller struct
 * @param nunchuk The nunchuck struct
 */
void collect_controller_state(struct robot_s *robot, struct wiimote_t *w,
                              struct controller_s *controller);

/**
 * @brief Handles a disconnect from a wii mote
 *
 * @param wm The wiimote that disconnected
 */
void handle_disconnect(wiimote *wm);

/**
 * @brief A Helper to create a new set of wii motes
 *
 * @return An array of connected wiimotes or null if none are there
 */
wiimote **wiimote_init();

/**
 * @brief Checks for alive wii motes
 *
 * @param wm The wiimote array to check
 * @param num_wiimotes The number of wiimotes to check
 *
 * @return true - alive false - dead
 */
short heart_beat(wiimote **wm, int num_wiimotes);

/**
 * @brief The main loop executed once a cycle
 *
 * @param wiimotes The wiimote array
 * @param robot The robot to do stuff to
 * @param controller The Controller structure
 * @param nunchuk the nunchuk structure
 * @param verbose Verbosity set to 1 prints all states of the controller
 */
void event_loop(wiimote **wiimotes, struct robot_s *robot,
                struct controller_s *controller);

#endif /* end of include guard WII_CONTROLLER_H */
