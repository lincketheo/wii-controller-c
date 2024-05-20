/**
 * @author      : theo (theo@varnsen)
 * @file        : robot_control
 * @brief A Wii controller logic controller - interracts with wiiuse and wii
 * controllers
 *
 * This file uses wiiuse to create controller and robot logic
 *
 * @created     : Friday Aug 09, 2019 12:18:16 MDT
 * @bugs  No known bugs
 */

#ifndef WII_H

#define WII_H

// C Includes
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Local Includes
#include "log.h"
#include "wiiuse.h"

#define MAX_WIIMOTES 1

#define VAR_SPEED (1 << 0)
#define VERBOSE (1 << 1)
#define INSYNC (1 << 2)
#define NONLIN (1 << 3)
#define DISCLINANG (1 << 4)
#define DEBUG (1 << 5)
#define ADVNCD (1 << 6)

////////// Data Structures //////////

// A lock for when we interact with the robot
pthread_mutex_t p_lock;

struct robot_s; // Forward declare

// In c++, you would call this a virtual class
struct peripheral {
  void (*stop)(struct robot_s *robot);
  void (*start)(struct robot_s *robot);
  void (*loop)(struct robot_s *robot); // normally null???
};

// A type of peripheral
struct drive_train {
  struct peripheral *p;

  // triggered on an event, calls this method
  void *(*on_vel_callback)(struct robot_s *robot, int lin, int ang);

  int linear_vel;
  int angular_vel;

  int cruising_speed;
  int speed_increment;
  int max_speed;
};

// A type of peripheral
struct camera {
  struct peripheral *p;

  void *(*on_angle_turn)(struct robot_s *robot, int ang); // between 0 and 365;
  void *(*take_picture)(struct robot_s *robot);

  int angular_vel;         // can be negative
  uint8_t state : 1;       // on or off
  uint8_t pic_request : 1; // immediately turns back to 0
};

enum gun_state { DEACTIVATED, ARMED, TRIGGERED };
struct gun {
  struct peripheral *p;

  void *(*on_arm_callback)(struct robot_s *robot, int left,
                           int right);                 // arm spins the motors
  void *(*on_trigger_callback)(struct robot_s *robot); // trigger drops the ball

  enum gun_state state; // 0 - deactivated 1 - armed 2 - triggered (everything
                        // else modulod)

  int left_mag;  // can ish be negative
  int right_mag; // can ish be negative
};

// A robot type with all the information a robot needs
struct robot_s {
  struct peripheral *p;

  struct drive_train *drive;
  struct gun *gun;
  struct camera *camera;

  uint8_t options;
  float period;
};

/**
 * @brief Start the robot and all its peripherals
 *
 * @param robot The robot and its peripherals
 */
void start_robot(struct robot_s *robot);

/**
 * @brief Stop the robot and all its peripherals
 *
 * @param robot The robot to stop
 */
void stop_robot(struct robot_s *robot);

/**
 * @brief Loop the robot
 *
 * @param robot The robot to loop
 */
void loop_robot(struct robot_s *robot);

/**
 * @brief Sets robot options by orring the option
 *
 * @param robot The robot to set option on
 * @param option The option - available options are at the top
 */
void robot_setopt(struct robot_s *robot, uint8_t option);

/**
 * @brief Unsets robot options by anding not the option
 *
 * @param robot The robot to remove the option on
 * @param option The option to set
 */
void robot_unsetopt(struct robot_s *robot, uint8_t option);

/**
 * @brief Changes option
 *
 * @param robot The robot to change the option on
 * @param option The option to change
 */
void robot_changeopt(struct robot_s *robot, uint8_t option);

/**
 * @brief Creates a default robot
 *
 * @return default robot, this is an empty robot, you need to set it's
 * attributes
 */
struct robot_s create_robot();

////////// DRIVE TRAIN METHODS //////////
/**
 * @brief Increments the robot's speed rather than just putting the speed on
 *
 * @param robot The robot to incriment
 * @param ang Angular magnitude to incriment by
 * @param lin Linear magnitude to incriment by
 */
void *increment(struct robot_s *robot, int ang, int lin);

/**
 * @brief Just makes the wheels turn, no incremental value
 *
 * @param robot The robot to change values
 * @param ang The angular to give the robot (1 -1 0)
 * @param lin the linear to give the robot
 */
void *discrete(struct robot_s *robot, int ang, int lin);

/**
 * @brief Stops the drive train, attatched to robot stop callback
 *
 * @param dt The drive train to stop
 */
void drive_train_stop(struct robot_s *dt);

/**
 * @brief Starts the drive train, attatched to robot start callback
 *
 * @param dt The drive train to start
 */
void drive_train_start(struct robot_s *dt);

/**
 * @brief Attatch a drivetrain callback to the robot
 *
 * @param dt The drive train to attatch to
 * @param type The type (0 incremental, 1 discrete)
 */
void attach_dt_callback(struct drive_train *dt,
                        void *(*cb)(struct robot_s *, int, int));

/**
 * @brief Creates a drive train
 *
 * @return A drive train
 */
struct drive_train create_drive_train(int cruising, int max_speed, int speed);

////////// GUN METHODS ///////////
/**
 * @brief The arm gun callback method
 *
 * @param robot The robot to do stuff with
 * @param left The left motor magnitude
 * @param right The right motor magnitude
 *
 * @return Null or context
 */
void *arm_gun_callback(struct robot_s *robot, int left, int right);

/**
 * @brief The trigger gun callback method
 *
 * @param robot The robot to trigger
 *
 * @return Null or context
 */
void *trigger_gun_callback(struct robot_s *robot);

/**
 * @brief Start a gun
 *
 * @param robot The robot the gun is on
 */
void gun_start(struct robot_s *robot);

/**
 * @brief Stop a gun
 *
 * @param robot The robot the gun is on
 */
void gun_stop(struct robot_s *robot);

/**
 * @brief Loop a gun
 *
 * @param robot robot to loop the gun
 */
void gun_loop(struct robot_s *robot);

/**
 * @brief Attatch an arm callback method
 *
 * @param gun The gun
 * @param cb The callback method
 */
void attach_gun_arm_callback(struct gun *gun,
                             void *(*cb)(struct robot_s *, int, int));

/**
 * @brief Attatch a trigger callback
 *
 * @param gun The gun to attatch to
 * @param cb The callback method
 */
void attach_gun_trigger_callback(struct gun *gun,
                                 void *(*cb)(struct robot_s *));

/**
 * @brief Creates a gun
 *
 * @return A gun
 */
struct gun create_gun();

////////// CAMERA METHODS //////////
/**
 * @brief A callback to turn the angle of the camera
 *
 * @param robot The robot who has the camera
 * @param ang The magnitude of the angular velocity
 *
 * @return Null or context
 */
void *on_angle_turn_callback(struct robot_s *robot, int ang);

/**
 * @brief Takes a picture
 *
 * @param robot The robot
 *
 * @return Null or  context
 */
void *on_take_picture_callback(struct robot_s *robot);

/**
 * @brief Create a camera
 *
 * @return A new camera
 */
struct camera create_camera();

/**
 * @brief Start a camera
 *
 * @param robot The robot who has the camera
 */
void camera_start(struct robot_s *robot);

/**
 * @brief Stop a camera
 *
 * @param robot The robot who has the camera
 */
void camera_stop(struct robot_s *robot);

/**
 * @brief Attach a turn callback
 *
 * @param robot The robot to attach to
 * @param cb The callback method
 */
void attach_angle_turn_callback(struct camera *cam,
                                void *(*cb)(struct robot_s *robot, int));

/**
 * @brief Attach a picture callback
 *
 * @param robot The robot to attach to
 * @param cb The callback method
 */
void attach_picture_callback(struct camera *cam,
                             void *(*cb)(struct robot_s *robot));

////////// UTILITY FUNCTIONS /////////
/**
 * @brief I know this is in math. But I didn't want to include math for just one
 * process
 *
 * @param min Minimum value
 * @param val value
 * @param max Maximum value
 *
 * @return A constrianed value
 */
int constrain(int min, int val, int max);

/**
 * @brief This is just linear, I'm lazy, haven't changed it yet
 *
 * @param current Current speed
 * @param incremental_speed Incrimental speed (robot->speed)
 * @param max_speed Maximum speed
 *
 * @return The nuew "current" sppeed
 */
int non_linear(int current, int incremental_speed, int max_speed);

/**
 * @brief Cleans up a robot and all its stuff
 *
 * @param robot The robot to clean up
 */
void robot_clean_up(struct robot_s *robot);

#endif
