/**
 * @author      : theo (theo@$HOSTNAME)
 * @file        : robot_control
 * @created     : Wednesday Aug 21, 2019 11:21:27 MDT
 */

#include "robot_control.h"

// The two types of drive train callbacks
void *discrete(struct robot_s *robot, int ang, int lin) {
  pthread_mutex_lock(&p_lock);

  robot->drive->linear_vel = (!(robot->options & DISCLINANG) || !ang)
                                 ? lin * robot->drive->cruising_speed
                                 : 0;
  robot->drive->angular_vel = (!(robot->options & DISCLINANG) || !lin)
                                  ? ang * robot->drive->cruising_speed
                                  : 0;

  pthread_mutex_unlock(&p_lock);

  return NULL;
}

void *increment(struct robot_s *robot, int ang, int lin) {
  pthread_mutex_lock(&p_lock);

  // TODO These tertiaries are getting pretty ugly, might want to switch to if
  // else

  if (robot->options & NONLIN) {
    robot->drive->linear_vel +=
        (!ang || !(robot->options & DISCLINANG))
            ? lin * non_linear(robot->drive->linear_vel,
                               lin * robot->drive->speed_increment,
                               robot->drive->max_speed)
            : 0;
    robot->drive->angular_vel +=
        (!lin || !(robot->options & DISCLINANG))
            ? ang * non_linear(robot->drive->angular_vel,
                               ang * robot->drive->speed_increment,
                               robot->drive->max_speed)
            : 0;
  } else {
    robot->drive->linear_vel =
        (!ang || !(robot->options & DISCLINANG))
            ? constrain(-(robot->drive->max_speed),
                        robot->drive->linear_vel +
                            lin * robot->drive->speed_increment,
                        robot->drive->max_speed)
            : 0;
    robot->drive->angular_vel =
        (!lin || !(robot->options & DISCLINANG))
            ? constrain(-(robot->drive->max_speed),
                        robot->drive->angular_vel +
                            ang * robot->drive->speed_increment,
                        robot->drive->max_speed)
            : 0;
  }

  pthread_mutex_unlock(&p_lock);
  return NULL;
}

static void drive_train_zeros(struct drive_train *dt) {
  if (dt) {
    dt->linear_vel = 0;
    dt->angular_vel = 0;
  }
}

void drive_train_stop(struct robot_s *robot) {
  if (robot)
    drive_train_zeros(robot->drive);
}

void drive_train_start(struct robot_s *robot) {
  if (robot)
    drive_train_zeros(robot->drive);
}

void attach_dt_callback(struct drive_train *dt,
                        void *(*cb)(struct robot_s *, int, int)) {
  if (dt) {
    drive_train_zeros(dt);
    dt->on_vel_callback = cb;
  }
}

void *drive_train_main_callback(struct robot_s *robot, int ang, int lin) {
  if (robot->options & VAR_SPEED)
    increment(robot, ang, lin);
  else
    discrete(robot, ang, lin);
  return NULL;
}

struct drive_train create_drive_train(int cruising, int max_speed, int speed) {
  struct drive_train dt;
  struct peripheral *p = malloc(sizeof(struct peripheral));
  dt.p = p;
  drive_train_zeros(&dt);
  dt.p->start = &drive_train_start;
  dt.p->stop = &drive_train_stop;
  dt.p->loop = NULL; // No loop function for the drive train
  attach_dt_callback(&dt, &drive_train_main_callback); // Default increment
  dt.max_speed = max_speed;
  dt.speed_increment = speed;
  dt.cruising_speed = cruising;
  return dt;
}

void *arm_gun_callback(struct robot_s *robot, int left, int right) {
  robot->gun->state = ARMED;
  robot->gun->left_mag += left;
  robot->gun->right_mag += right;
}

void *trigger_gun_callback(struct robot_s *robot) {
  if (robot->gun->state == ARMED)
    robot->gun->state = TRIGGERED;
  return NULL;
}

static void gun_zeros(struct gun *gun) {
  if (gun) {
    gun->state = DEACTIVATED;
    gun->left_mag = 0;
    gun->right_mag = 0;
  }
}
void gun_start(struct robot_s *robot) {
  if (robot)
    gun_zeros(robot->gun);
}

void gun_stop(struct robot_s *robot) {
  if (robot)
    gun_zeros(robot->gun);
}

void gun_loop(struct robot_s *robot) {
  if (robot && robot->gun && robot->gun->state == TRIGGERED) {
    if (!robot->gun->left_mag &&
        !robot->gun->right_mag) { // if both zero, deactivate
      robot->gun->state = DEACTIVATED;
    } else {
      // Adding padding so it doesn't just go straight to zero
      robot->gun->left_mag -=
          (robot->gun->left_mag <= 0) ? (-robot->gun->left_mag) : 1;
      robot->gun->right_mag -=
          (robot->gun->right_mag <= 0) ? (-robot->gun->right_mag) : 1;
    }
  }
}

void attach_gun_arm_callback(struct gun *gun,
                             void *(*cb)(struct robot_s *, int, int)) {
  if (gun)
    gun->on_arm_callback = cb;
}

void attach_gun_trigger_callback(struct gun *gun,
                                 void *(*cb)(struct robot_s *)) {
  if (gun)
    gun->on_trigger_callback = cb;
}

struct gun create_gun() {
  struct gun gun;
  struct peripheral *p = malloc(sizeof(struct peripheral));
  gun.p = p;
  gun_zeros(&gun);
  gun.p->start = &gun_start;
  gun.p->stop = &gun_stop;
  gun.p->loop = &gun_loop;
  attach_gun_trigger_callback(&gun, &trigger_gun_callback);
  attach_gun_arm_callback(&gun, &arm_gun_callback);
  return gun;
}

void *on_angle_turn_callback(struct robot_s *robot, int ang) {
  robot->camera->angular_vel = ang;
}

void *on_take_picture_callback(struct robot_s *robot) {
  if (robot->camera->state)
    robot->camera->pic_request = 1;
}

static void camera_zeros(struct camera *cam) {
  if (cam) {
    cam->angular_vel = 0;
    cam->pic_request = 0;
  }
}

void camera_start(struct robot_s *robot) {
  if (robot)
    camera_zeros(robot->camera);
}

void camera_stop(struct robot_s *robot) {
  if (robot)
    camera_zeros(robot->camera);
}

void camera_loop(struct robot_s *robot) {
  if (robot->camera->pic_request == 1)
    robot->camera->pic_request = 0;
}

void attach_angle_turn_callback(struct camera *cam,
                                void *(*cb)(struct robot_s *, int)) {
  if (cam)
    cam->on_angle_turn = cb;
}

void attach_picture_callback(struct camera *cam,
                             void *(*cb)(struct robot_s *robot)) {
  if (cam)
    cam->take_picture = cb;
}

struct camera create_camera() {
  struct camera cam;
  struct peripheral *p = malloc(sizeof(struct peripheral));
  camera_zeros(&cam);
  cam.p = p;
  cam.p->start = &camera_start;
  cam.p->stop = &camera_stop;
  cam.p->loop = &camera_loop;
  attach_angle_turn_callback(&cam, &on_angle_turn_callback);
  attach_picture_callback(&cam, &on_take_picture_callback);
  return cam;
}

////////// ROBOT METHODS /////////
void stop_robot(struct robot_s *robot) {
  if (robot->drive && robot->drive->p->stop)
    (*robot->drive->p->stop)(robot);
  if (robot->camera && robot->camera->p->stop)
    (*robot->camera->p->stop)(robot);
  if (robot->gun && robot->gun->p->stop)
    (*robot->gun->p->stop)(robot);
}

void start_robot(struct robot_s *robot) {
  if (robot->drive && robot->drive->p->start)
    (*robot->drive->p->start)(robot);
  if (robot->camera && robot->camera->p->start)
    (*robot->camera->p->start)(robot);
  if (robot->gun && robot->gun->p->start)
    (*robot->gun->p->start)(robot);
}

void loop_robot(struct robot_s *robot) {
  if (robot->drive && robot->drive->p->loop)
    (*robot->drive->p->loop)(robot);
  if (robot->camera && robot->camera->p->loop)
    (*robot->camera->p->loop)(robot);
  if (robot->gun && robot->gun->p->loop)
    (*robot->gun->p->loop)(robot);
}

void robot_setopt(struct robot_s *robot, uint8_t option) {
  robot->options |= option;
}

void robot_unsetopt(struct robot_s *robot, uint8_t option) {
  robot->options &= ~option;
}

void robot_changeopt(struct robot_s *robot, uint8_t option) {
  if (robot->options & option)
    robot_unsetopt(robot, option);
  else
    robot_setopt(robot, option);
}

struct robot_s create_robot() {
  struct robot_s robot;
  struct peripheral *p = malloc(sizeof(struct peripheral));
  robot.camera = NULL;
  robot.drive = NULL;
  robot.gun = NULL;
  p->start = &start_robot;
  p->stop = &stop_robot;
  p->loop = &loop_robot;
  robot.p = p;
  return robot;
}

int constrain(int min, int val, int max) {
  if (val <= min)
    return min;
  if (val >= max)
    return max;
  return val;
}

int non_linear(int current, int incremental_speed, int max_speed) {
  if (current >= max_speed)
    return 0;
  if (current <= -max_speed)
    return 0;
  return incremental_speed;
}

void robot_clean_up(struct robot_s *robot) {
  if (robot == NULL)
    return;
  if (robot->p)
    free(robot->p);
  if (robot->drive && robot->drive->p) {
    free(robot->drive->p);
    free(robot->drive);
  }
  if (robot->gun && robot->gun->p) {
    free(robot->gun->p);
    free(robot->gun);
  }
  if (robot->camera && robot->camera->p) {
    free(robot->camera->p);
    free(robot->camera);
  }
}
