/**
 * @author      : theo (theo@$HOSTNAME)
 * @file        : wii_controller
 * @created     : Tuesday Aug 20, 2019 18:13:11 MDT
 */

#include "wii_controller.h"

wiimote **wiimote_init() {
  wiimote **wiimotes;
  int found, connected;
  wiimotes = wiiuse_init(MAX_WIIMOTES);
  found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
  if (!found) {
    log_warn("No wiimotes found \n");
    return NULL;
  }

  connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
  if (connected) {
    log_info("Connected to %i wiimotes (of %i found)\n", connected, found);
  } else {
    log_error("Failed to connect to any wiimote\n");
    return NULL;
  }

  wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_1);
  wiiuse_rumble(wiimotes[0], 1);
  usleep(200000);
  wiiuse_rumble(wiimotes[0], 0);
  return wiimotes;
}

void handle_disconnect(wiimote *wm) {
  printf("\n\n ----- DISCONNECTED [wiimote %d] ----- \n\n", wm->unid);
}

short heart_beat(wiimote **wm, int num_wiimotes) {
  if (!wm)
    return 0;
  for (int i = 0; i < num_wiimotes; i++)
    if (wm[i] && WIIMOTE_IS_CONNECTED(wm[i]))
      return 1;
  return 0;
}

void event_loop(wiimote **wiimotes, struct robot_s *robot,
                struct controller_s *controller) {
  if (wiiuse_poll(wiimotes, MAX_WIIMOTES)) {
    int i = 0;
    for (; i < MAX_WIIMOTES; ++i) {
      switch (wiimotes[i]->event) {
      case WIIUSE_EVENT:
        /* a generic event occurred */
        collect_controller_state(robot, wiimotes[i], controller);
        break;
      case WIIUSE_DISCONNECT:
      case WIIUSE_UNEXPECTED_DISCONNECT:
        /* the wiimote disconnected */
        handle_disconnect(wiimotes[i]);
        break;
      case WIIUSE_READ_DATA:
        break;
      case WIIUSE_NUNCHUK_INSERTED:
        log_info("Nunchuk inserted.\n");
        break;
      case WIIUSE_CLASSIC_CTRL_INSERTED:
        log_info("Classic controller inserted.\n");
        break;
      case WIIUSE_WII_BOARD_CTRL_INSERTED:
        log_info("Balance board controller inserted.\n");
        break;
      case WIIUSE_MOTION_PLUS_ACTIVATED:
        log_info("Motion+ was activated\n");
        break;
      case WIIUSE_NUNCHUK_REMOVED:
      case WIIUSE_CLASSIC_CTRL_REMOVED:
      case WIIUSE_GUITAR_HERO_3_CTRL_REMOVED:
      case WIIUSE_WII_BOARD_CTRL_REMOVED:
      case WIIUSE_MOTION_PLUS_REMOVED:
        /* some expansion was removed */
        log_info("An expansion was removed.\n");
        break;
      default:
        break;
      }
    }
  }
  if (robot->options & VERBOSE) {
    if (robot->options & ADVNCD) {
      printf("ADVANCED\n");
      printf("Settings: \n"
             "Variable Speed:  | %d |  --- Verbose:            | %d | --- In "
             "sync:   | %d | \n"
             "NonLinear:       | %d |  --- Discrete Lin Ang:   | %d | --- "
             "Debug:     | %d | \n",
             (robot->options & VAR_SPEED), (robot->options & VERBOSE) >> 1,
             (robot->options & INSYNC) >> 2, (robot->options & NONLIN) >> 3,
             (robot->options & DISCLINANG) >> 4, (robot->options & DEBUG) >> 6);
    }
    printf("robot: Angular - %d  Linear - %d\n", robot->drive->angular_vel,
           robot->drive->linear_vel);
    printf("\ncontr: home - %d, plus - %d, minus - %d, A - %d, B - %d, ONE - "
           "%d, TWO - %d \nup - %d, down - %d, left - %d, right - %d\n\n",
           controller->stick->home.value, controller->stick->plus.value,
           controller->stick->minus.value, controller->stick->a.value,
           controller->stick->b.value, controller->stick->one.value,
           controller->stick->two.value, controller->stick->up.value,
           controller->stick->down.value, controller->stick->left.value,
           controller->stick->right.value);
    if (robot->gun) {
      printf("gun: State - %d Left - %d Right - %d\n", robot->gun->state,
             robot->gun->left_mag, robot->gun->right_mag);
    }
  }
  (*robot->p->loop)(robot);
}

void set_controller_zero(struct controller_s *controller) {
  if (controller->nunchuk) {
    controller->nunchuk->c.value = 0;
    controller->nunchuk->z.value = 0;
  }

  if (controller->stick) {
    controller->stick->up.value = 0;
    controller->stick->down.value = 0;
    controller->stick->right.value = 0;
    controller->stick->left.value = 0;
    controller->stick->a.value = 0;
    controller->stick->b.value = 0;
    controller->stick->one.value = 0;
    controller->stick->two.value = 0;
    controller->stick->home.value = 0;
    controller->stick->minus.value = 0;
    controller->stick->plus.value = 0;
  }
}

void collect_controller_state(struct robot_s *robot, struct wiimote_t *wm,
                              struct controller_s *controller) {

  /**
   * There are a lot better ways of doing this.
   * But, we need to make sure the controller has all its buttons down before we
   * move on
   */
  if (controller->stick) {
    struct stick_s *temp = controller->stick;

    temp->up.value = IS_PRESSED(wm, WIIMOTE_BUTTON_UP);
    temp->down.value = IS_PRESSED(wm, WIIMOTE_BUTTON_DOWN);
    temp->left.value = IS_PRESSED(wm, WIIMOTE_BUTTON_LEFT);
    temp->right.value = IS_PRESSED(wm, WIIMOTE_BUTTON_RIGHT);
    temp->a.value = IS_PRESSED(wm, WIIMOTE_BUTTON_A);
    temp->b.value = IS_PRESSED(wm, WIIMOTE_BUTTON_B);
    temp->one.value = IS_PRESSED(wm, WIIMOTE_BUTTON_ONE);
    temp->two.value = IS_PRESSED(wm, WIIMOTE_BUTTON_TWO);
    temp->home.value = IS_PRESSED(wm, WIIMOTE_BUTTON_HOME);
    temp->minus.value = IS_PRESSED(wm, WIIMOTE_BUTTON_MINUS);
    temp->plus.value = IS_PRESSED(wm, WIIMOTE_BUTTON_PLUS);
  }
  if (controller->nunchuk && (wm->exp.type == EXP_NUNCHUK ||
                              wm->exp.type == EXP_MOTION_PLUS_NUNCHUK)) {
    struct nunchuk_t *nc = (nunchuk_t *)&wm->exp.nunchuk;

    struct nunchuk_s *nunchuk = controller->nunchuk;

    nunchuk->c.value = IS_PRESSED(nc, NUNCHUK_BUTTON_C);
    nunchuk->z.value = IS_PRESSED(nc, NUNCHUK_BUTTON_Z);
    nunchuk->roll.value = nc->orient.roll;
    nunchuk->pitch.value = nc->orient.pitch;
    nunchuk->yaw.value = nc->orient.yaw;
    nunchuk->angle.value = nc->js.ang;
    nunchuk->magnitude.value = nc->js.mag;
    nunchuk->vals_x.value = nc->js.x;
    nunchuk->vals_y.value = nc->js.y;
  }
  execute_callbacks(robot, controller);
}

void execute_callbacks(struct robot_s *robot, struct controller_s *controller) {
  if (controller->stick) {
    struct stick_s *temp = controller->stick;
    if (temp->up.value && temp->up.callback) {
      (*temp->up.callback)(controller, robot);
    }
    if (temp->down.value && temp->down.callback)
      (*temp->down.callback)(controller, robot);
    if (temp->left.value && temp->left.callback)
      (*temp->left.callback)(controller, robot);
    if (temp->right.value && temp->right.callback)
      (*temp->right.callback)(controller, robot);
    if (temp->a.value && temp->a.callback)
      (*temp->a.callback)(controller, robot);
    if (temp->b.value && temp->b.callback)
      (*temp->b.callback)(controller, robot);
    if (temp->one.value && temp->one.callback)
      (*temp->one.callback)(controller, robot);
    if (temp->two.value && temp->two.callback)
      (*temp->two.callback)(controller, robot);
    if (temp->home.value && temp->home.callback)
      (*temp->home.callback)(controller, robot);
    if (temp->minus.value && temp->minus.callback)
      (*temp->minus.callback)(controller, robot);
    if (temp->plus.value && temp->plus.callback)
      (*temp->plus.callback)(controller, robot);
  }
  if (controller->nunchuk) {
    struct nunchuk_s *temp = controller->nunchuk;

    if (temp->c.value && temp->c.callback)
      (*temp->c.callback)(controller, robot);
    if (temp->z.value && temp->z.callback)
      (*temp->z.callback)(controller, robot);
    if (temp->roll.callback)
      (*temp->roll.callback)(controller, robot);
    if (temp->pitch.callback)
      (*temp->pitch.callback)(controller, robot);
    if (temp->yaw.callback)
      (*temp->yaw.callback)(controller, robot);
    if (temp->angle.callback)
      (*temp->angle.callback)(controller, robot);
    if (temp->magnitude.callback)
      (*temp->magnitude.callback)(controller, robot);
    if (temp->vals_x.callback)
      (*temp->vals_x.callback)(controller, robot);
    if (temp->vals_y.callback)
      (*temp->vals_y.callback)(controller, robot);
  }
}
