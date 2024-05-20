/**
 * @author      : theo (theo@$HOSTNAME)
 * @file        : kermit
 * @created     : Wednesday Aug 21, 2019 15:10:46 MDT
 */

#ifndef KERMIT_H

#define KERMIT_H

#include "wii_controller.h"

/**
 * Kermit is our robot
 *
 * The first thing I'm doing here is declaring callback functions
 * Notice that a callback is in the form
 *
 * void* function_name(struct controller_s *controller, structr robot_s *robot);
 *
 * This is so a user has maximal freedom to change the properties of both the
 * controller and the robot. So for example, you can make different ccontroller
 * modes that certain buttons change that then affect the robot in a different
 * way.
 *
 * I have definined the basic callbacks for kermit, read each comment to see the
 * detailed explanation for what each method does
 *
 * Note that each function is a void* to allow certain cases where a controller
 * returns a value (that you can just cast later on)
 */

/**
 * @brief The callback executed on the "up" button
 *
 * @note The up callback increases motor speeds only when the controller
 * is in the drive mode
 *
 * @param controller The controller controlling the robot
 * @param robot The robot to change - just changes the linear speed to +1 or
 * +increment
 *
 * @return nothing
 */
void *up_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE) {
    robot->drive->on_vel_callback(robot, 0, 1);
  }
  return NULL;
}

/**
 * @brief The callback executed on the down button
 *
 * @note The down callback is used to decrease / make the robot linear value
 * go backwards
 *
 * @param controller The controller to change - just checks the state
 * @param robot The robot to change - decreases / makes negative the linear _vel
 * by -1 or -increment
 *
 * @return nothing
 */
void *down_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE) {
    robot->drive->on_vel_callback(robot, 0, -1);
  }
  return NULL;
}

/**
 * @brief The callback executed on the left button
 *
 * @note The left callback is used to decrease / make the robot angular value
 * negative
 *
 * @param controller The controller to change - just checks the state
 * @param robot The robot to update - decreases the angular value / decreases
 * the angular value
 *
 * @return nothing
 */
void *left_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE) {
    robot->drive->on_vel_callback(robot, -1, 0);
  }
  return NULL;
}

/**
 * @brief The right control callback
 *
 * @note This callback increases / changes state to the speed param
 *
 * @param controller The controller to pass - just checks drive
 * @param robot The robot that the angular velocity changes
 *
 * @return nothing
 */
void *right_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE) {
    robot->drive->on_vel_callback(robot, 1, 0);
  }
  return NULL;
}

/**
 * @brief The A callback to the controller
 *
 * @note The a callback has two uses. When the controller is in drive state,
 * this increases arms the gun, allowing you to increase the top and botton
 * gun motors
 *
 * @param controller The controller to change - sees if the controller is in
 * drive or advanced state
 * @param robot The robot. Changes the gun state of the robot - puts into Arm /
 * Deactivated (triggered is very brief)
 *
 * @return nothing
 */
void *a_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE && robot->gun) {
    if (robot->gun->state == DEACTIVATED)
      robot->gun->on_arm_callback(robot, 1, 1);
    else if (robot->gun->state == ARMED)
      robot->gun->on_trigger_callback(robot);
  } else if (controller->state == ADVANCED) {
    robot_changeopt(robot, VAR_SPEED);
  }
  return NULL;
}

/**
 * @brief The callback executed on the home button of the wii controller
 *
 * @note The home callback is used to change the robot state to advanced
 * advanced mode means a user is able to change settings during runtime
 *
 * @param controller The controller to read from. If the controller is already
 * in advanced, it unsets this option
 * @param robot The robot to change into advacned mode
 *
 * @return  nothing
 */
void *home_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE) {
    robot_setopt(robot, ADVNCD);
    controller->state = ADVANCED;
  } else if (controller->state == ADVANCED) {
    robot_unsetopt(robot, ADVNCD);
    controller->state = DRIVE;
  }
}

/**
 * @brief The callback executed o nthe one button
 *
 * @note The one button is used to either increase the top motor speed on the
 * gun or change the disclin ang property
 * @note The disclinang option allows the robot to either move forward OR turn,
 * there is no combo
 *
 * @param controller  The controller to check the state on
 * @param robot The robot that is changed / altered
 *
 * @return nothing
 */
void *one_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE && robot->gun) {
    if (robot->gun->state == ARMED)
      robot->gun->on_arm_callback(robot, 0, 1);
  } else if (controller->state == ADVANCED) {
    robot_changeopt(robot, DISCLINANG);
  }
}

void *two_callback(struct controller_s *controller, struct robot_s *robot) {
  if (controller->state == DRIVE && robot->gun) {
    if (robot->gun->state == ARMED)
      robot->gun->on_arm_callback(robot, 1, 0);
  }
}

void *b_callback(struct controller_s *controller, struct robot_s *robot) {
  if (robot->drive) {
    (*robot->drive->p->stop)(robot);
  }
  return NULL;
}

struct controller_s kermit_controller() {
  struct controller_s kermit;
  kermit.nunchuk = NULL;
  kermit.state = DRIVE;

  kermit.stick = malloc(sizeof(struct stick_s));

  set_controller_zero(&kermit);

  kermit.stick->minus.callback = NULL;
  kermit.stick->plus.callback = NULL;

  kermit.stick->home.callback = home_callback;
  kermit.stick->one.callback = one_callback;
  kermit.stick->two.callback = two_callback;
  kermit.stick->up.callback = up_callback;
  kermit.stick->down.callback = down_callback;
  kermit.stick->left.callback = left_callback;
  kermit.stick->right.callback = right_callback;
  kermit.stick->a.callback = a_callback;
  kermit.stick->b.callback = b_callback;

  return kermit;
}

struct robot_s kermit_robot() {
  struct robot_s rob;
  struct peripheral *p = malloc(sizeof(struct peripheral));
  rob.p = p;
  rob.p->start = start_robot;
  rob.p->stop = stop_robot;
  rob.p->loop = loop_robot;
  rob.gun = NULL;
  rob.camera = NULL;
  struct drive_train *drive = malloc(sizeof(struct drive_train));
  *drive = create_drive_train(4, 10, 1);
  rob.drive = drive;
  struct gun *gun = malloc(sizeof(struct gun));
  *gun = create_gun();
  rob.gun = gun;
  rob.period = .01;
  start_robot(&rob);
  return rob;
}

#endif /* end of include guard KERMIT_H */
