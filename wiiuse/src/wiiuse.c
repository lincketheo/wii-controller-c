/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 *
 */

/**
 *	@file
 *	@brief General wiimote operations.
 *
 *	The file includes functions that handle general
 *	tasks.  Most of these are functions that are part
 *	of the API.
 */

#include "io.h" /* for wiiuse_handshake, etc */
#include "os.h" /* for wiiuse_os_* */
#include "wiiuse_internal.h"

#include <stdio.h>  /* for printf, FILE */
#include <stdlib.h> /* for malloc, free */
#include <string.h> /* for memcpy, memset */

static int g_banner = 0;
static const char g_wiiuse_version_string[] = WIIUSE_VERSION;

/**
 *	@brief Returns the version of the library.
 */
const char *wiiuse_version() { return g_wiiuse_version_string; }

/**
 *	@brief Output FILE stream for each wiiuse_loglevel.
 */
FILE *logtarget[4];

/**
 *	@brief Specify an alternate FILE stream for a log level.
 *
 *	@param loglevel The loglevel, for which the output should be set.
 *
 *	@param logfile A valid, writeable <code>FILE*</code>, or 0, if output
 *should be disabled.
 *
 *  The default <code>FILE*</code> for all loglevels is <code>stderr</code>
 */
void wiiuse_set_output(enum wiiuse_loglevel loglevel, FILE *logfile) {
  logtarget[(int)loglevel] = logfile;
}

/**
 *	@brief Clean up wiimote_t array created by wiiuse_init()
 */
void wiiuse_cleanup(struct wiimote_t **wm, int wiimotes) {
  int i = 0;

  if (!wm) {
    return;
  }

  WIIUSE_INFO("wiiuse clean up...");

  for (; i < wiimotes; ++i) {
    wiiuse_disconnect(wm[i]);
    wiiuse_cleanup_platform_fields(wm[i]);
    free(wm[i]);
  }

  free(wm);

  return;
}

/**
 *	@brief Initialize an array of wiimote structures.
 *
 *	@param wiimotes		Number of wiimote_t structures to create.
 *
 *	@return An array of initialized wiimote_t structures.
 *
 *	@see wiiuse_connect()
 *
 *	The array returned by this function can be passed to various
 *	functions, including wiiuse_connect().
 */
struct wiimote_t **wiiuse_init(int wiimotes) {
  int i = 0;
  struct wiimote_t **wm = NULL;

  /*
   *	Please do not remove this banner.
   *	GPL asks that you please leave output credits intact.
   *	Thank you.
   *
   *	This banner is only displayed once so that if you need
   *	to call this function again it won't be intrusive.
   *
   *	2018: Replaced wiiuse.net with sourceforge project, since
   *	wiiuse.net is now abandoned and "parked".
   */
  if (!g_banner) {
    printf("wiiuse v" WIIUSE_VERSION " loaded.\n"
           "  De-facto official fork at http://github.com/wiiuse/wiiuse\n"
           "  Original By: Michael Laforest <thepara[at]gmail{dot}com> "
           "<https://sourceforge.net/projects/wiiuse/>\n");
    g_banner = 1;
  }

  logtarget[0] = stderr;
  logtarget[1] = stderr;
  logtarget[2] = stderr;
  logtarget[3] = stderr;

  if (!wiimotes) {
    return NULL;
  }

  wm = (struct wiimote_t **)malloc(sizeof(struct wiimote_t *) * wiimotes);

  for (i = 0; i < wiimotes; ++i) {
    wm[i] = (struct wiimote_t *)malloc(sizeof(struct wiimote_t));
    memset(wm[i], 0, sizeof(struct wiimote_t));

    wm[i]->unid = i + 1;
    wiiuse_init_platform_fields(wm[i]);

    wm[i]->state = WIIMOTE_INIT_STATES;
    wm[i]->flags = WIIUSE_INIT_FLAGS;

    wm[i]->event = WIIUSE_NONE;

    wm[i]->exp.type = EXP_NONE;
    wm[i]->expansion_state = 0;

    wiiuse_set_aspect_ratio(wm[i], WIIUSE_ASPECT_4_3);
    wiiuse_set_ir_position(wm[i], WIIUSE_IR_ABOVE);

    wm[i]->orient_threshold = 0.5f;
    wm[i]->accel_threshold = 5;

    wm[i]->accel_calib.st_alpha = WIIUSE_DEFAULT_SMOOTH_ALPHA;

    wm[i]->type = WIIUSE_WIIMOTE_REGULAR;
  }

  return wm;
}

/**
 *	@brief	The wiimote disconnected.
 *
 *	@param wm	Pointer to a wiimote_t structure.
 */
void wiiuse_disconnected(struct wiimote_t *wm) {
  if (!wm) {
    return;
  }

  WIIUSE_INFO("Wiimote disconnected [id %i].", wm->unid);

  /* disable the connected flag */
  WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_CONNECTED);

  /* reset a bunch of stuff */
  wm->leds = 0;
  wm->state = WIIMOTE_INIT_STATES;
  wm->read_req = NULL;
#ifndef WIIUSE_SYNC_HANDSHAKE
  wm->handshake_state = 0;
#endif
  wm->btns = 0;
  wm->btns_held = 0;
  wm->btns_released = 0;

  wm->event = WIIUSE_DISCONNECT;
}

/**
 *	@brief	Enable or disable the rumble.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param status	1 to enable, 0 to disable.
 */
void wiiuse_rumble(struct wiimote_t *wm, int status) {
  byte buf;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return;
  }

  /* make sure to keep the current lit leds */
  buf = wm->leds;

  if (status) {
    WIIUSE_DEBUG("Starting rumble...");
    WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_RUMBLE);
    buf |= 0x01;
  } else {
    WIIUSE_DEBUG("Stopping rumble...");
    WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_RUMBLE);
    buf &= ~(0x01);
  }

  /* preserve IR state */
  if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR)) {
    buf |= 0x04;
  }

  wiiuse_send(wm, WM_CMD_RUMBLE, &buf, 1);
}

/**
 *	@brief	Toggle the state of the rumble.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 */
void wiiuse_toggle_rumble(struct wiimote_t *wm) {
  if (!wm) {
    return;
  }

  wiiuse_rumble(wm, !WIIMOTE_IS_SET(wm, WIIMOTE_STATE_RUMBLE));
}

/**
 *	@brief	Set the enabled LEDs.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param leds		What LEDs to enable.
 *
 *	\a leds is a bitwise or of WIIMOTE_LED_1, WIIMOTE_LED_2, WIIMOTE_LED_3,
 *or WIIMOTE_LED_4.
 */
void wiiuse_set_leds(struct wiimote_t *wm, int leds) {
  byte buf;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return;
  }

  /* remove the lower 4 bits because they control rumble */
  wm->leds = (leds & 0xF0);

  /* make sure if the rumble is on that we keep it on */
  if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_RUMBLE)) {
    wm->leds |= 0x01;
  }

  buf = wm->leds;

  wiiuse_send(wm, WM_CMD_LED, &buf, 1);
}

/**
 *	@brief	Set if the wiimote should report motion sensing.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param status	1 to enable, 0 to disable.
 *
 *	Since reporting motion sensing sends a lot of data,
 *	the wiimote saves power by not transmitting it
 *	by default.
 */
void wiiuse_motion_sensing(struct wiimote_t *wm, int status) {
  if (status) {
    WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_ACC);
  } else {
    WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_ACC);
  }

  wiiuse_set_report_type(wm);
}

/**
 *	@brief	Set the report type based on the current wiimote state.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	@return The report type sent.
 *
 *	The wiimote reports formatted packets depending on the
 *	report type that was last requested.  This function will
 *	update the type of report that should be sent based on
 *	the current state of the device.
 */
int wiiuse_set_report_type(struct wiimote_t *wm) {

  byte buf[2];
  int motion, exp, ir, balance_board;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return 0;
  }

  buf[0] = (WIIMOTE_IS_FLAG_SET(wm, WIIUSE_CONTINUOUS)
                ? 0x04
                : 0x00); /* set to 0x04 for continuous reporting */
  buf[1] = 0x00;

  /* if rumble is enabled, make sure we keep it */
  if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_RUMBLE)) {
    buf[0] |= 0x01;
  }

  motion = WIIMOTE_IS_SET(wm, WIIMOTE_STATE_ACC);
  exp = WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP);
  ir = WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR);
  balance_board = exp && (wm->exp.type == EXP_WII_BOARD);

  if (motion && ir && exp) {
    buf[1] = WM_RPT_BTN_ACC_IR_EXP;
  } else if (motion && exp) {
    buf[1] = WM_RPT_BTN_ACC_EXP;
  } else if (motion && ir) {
    buf[1] = WM_RPT_BTN_ACC_IR;
  } else if (ir && exp) {
    buf[1] = WM_RPT_BTN_IR_EXP;
  } else if (ir) {
    buf[1] = WM_RPT_BTN_ACC_IR;
  } else if (exp && balance_board) {
    if (wm->exp.wb.use_alternate_report)
      buf[1] = WM_RPT_BTN_EXP_8;
    else
      buf[1] = WM_RPT_BTN_EXP;
  } else if (exp) {
    buf[1] = WM_RPT_BTN_EXP;
  } else if (motion) {
    buf[1] = WM_RPT_BTN_ACC;
  } else {
    buf[1] = WM_RPT_BTN;
  }

  WIIUSE_DEBUG("Setting report type: 0x%x", buf[1]);

  exp = wiiuse_send(wm, WM_CMD_REPORT_TYPE, buf, 2);
  if (exp <= 0) {
    return exp;
  }

  return buf[1];
}

/**
 *	@brief	Read data from the wiimote (callback version).
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param read_cb	Function pointer to call when the data arrives from the
 *wiimote.
 *	@param buffer	An allocated buffer to store the data as it arrives from
 *the wiimote. Must be persistent in memory and large enough to hold the data.
 *	@param addr		The address of wiimote memory to read from.
 *	@param len		The length of the block to be read.
 *
 *	The library can only handle one data read request at a time
 *	because it must keep track of the buffer and other
 *	events that are specific to that request.  So if a request
 *	has already been made, subsequent requests will be added
 *	to a pending list and be sent out when the previous
 *	finishes.
 */
int wiiuse_read_data_cb(struct wiimote_t *wm, wiiuse_read_cb read_cb,
                        byte *buffer, unsigned int addr, uint16_t len) {
  struct read_req_t *req;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return 0;
  }
  if (!buffer || !len) {
    return 0;
  }

  /* make this request structure */
  req = (struct read_req_t *)malloc(sizeof(struct read_req_t));
  if (req == NULL) {
    return 0;
  }
  req->cb = read_cb;
  req->buf = buffer;
  req->addr = addr;
  req->size = len;
  req->wait = len;
  req->dirty = 0;
  req->next = NULL;

  /* add this to the request list */
  if (!wm->read_req) {
    /* root node */
    wm->read_req = req;

    WIIUSE_DEBUG("Data read request can be sent out immediately.");

    /* send the request out immediately */
    wiiuse_send_next_pending_read_request(wm);
  } else {
    struct read_req_t *nptr = wm->read_req;
    for (; nptr->next; nptr = nptr->next) {
      ;
    }
    nptr->next = req;

    WIIUSE_DEBUG("Added pending data read request.");
  }

  return 1;
}

/**
 *	@brief	Read data from the wiimote (event version).
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param buffer	An allocated buffer to store the data as it arrives from
 *the wiimote. Must be persistent in memory and large enough to hold the data.
 *	@param addr		The address of wiimote memory to read from.
 *	@param len		The length of the block to be read.
 *
 *	The library can only handle one data read request at a time
 *	because it must keep track of the buffer and other
 *	events that are specific to that request.  So if a request
 *	has already been made, subsequent requests will be added
 *	to a pending list and be sent out when the previous
 *	finishes.
 */
int wiiuse_read_data(struct wiimote_t *wm, byte *buffer, unsigned int addr,
                     uint16_t len) {
  return wiiuse_read_data_cb(wm, NULL, buffer, addr, len);
}

/**
 *	@brief Send the next pending data read request to the wiimote.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	@see wiiuse_read_data()
 *
 *	This function is not part of the wiiuse API.
 */
void wiiuse_send_next_pending_read_request(struct wiimote_t *wm) {
  byte buf[6];
  struct read_req_t *req;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return;
  }
  if (!wm->read_req) {
    return;
  }

  /* skip over dirty ones since they have already been read */
  req = wm->read_req;
  while (req && req->dirty) {
    req = req->next;
  }
  if (!req) {
    return;
  }

  /* the offset is in big endian */
  to_big_endian_uint32_t(buf, req->addr);

  /* the length is in big endian */
  to_big_endian_uint16_t(buf + 4, req->size);

  WIIUSE_DEBUG("Request read at address: 0x%x  length: %i", req->addr,
               req->size);
  wiiuse_send(wm, WM_CMD_READ_DATA, buf, 6);
}

/**
 *	@brief Request the wiimote controller status.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	Controller status includes: battery level, LED status, expansions
 */
void wiiuse_status(struct wiimote_t *wm) {
  byte buf = 0;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return;
  }

  WIIUSE_DEBUG("Requested wiimote status.");

  wiiuse_send(wm, WM_CMD_CTRL_STATUS, &buf, 1);
}

/**
 *	@brief Find a wiimote_t structure by its unique identifier.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param wiimotes	The number of wiimote_t structures in \a wm.
 *	@param unid		The unique identifier to search for.
 *
 *	@return Pointer to a wiimote_t structure, or NULL if not found.
 */
struct wiimote_t *wiiuse_get_by_id(struct wiimote_t **wm, int wiimotes,
                                   int unid) {
  int i = 0;
  if (!wm) {
    return NULL;
  }

  for (; i < wiimotes; ++i) {
    if (!wm[i]) {
      continue;
    }
    if (wm[i]->unid == unid) {
      return wm[i];
    }
  }

  return NULL;
}

/**
 *	@brief	Write data to the wiimote.
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param addr			The address to write to.
 *	@param data			The data to be written to the memory
 *location.
 *	@param len			The length of the block to be written.
 */
int wiiuse_write_data(struct wiimote_t *wm, unsigned int addr, const byte *data,
                      byte len) {
  byte buf[21] = {0}; /* the payload is always 23 */

  byte *bufPtr = buf;
  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    WIIUSE_ERROR(
        "Attempt to write, but no wiimote available or not connected!");
    return 0;
  }
  if (!data || !len) {
    WIIUSE_ERROR("Attempt to write, but no data or length == 0");
    return 0;
  }

  WIIUSE_DEBUG("Writing %i bytes to memory location 0x%x...", len, addr);

#ifdef WITH_WIIUSE_DEBUG
  {
    int i = 0;
    printf("Write data is: ");
    for (; i < len; ++i) {
      printf("%x ", data[i]);
    }
    printf("\n");
  }
#endif

  /* the offset is in big endian */
  buffer_big_endian_uint32_t(&bufPtr, (uint32_t)addr);

  /* length */
  buffer_big_endian_uint8_t(&bufPtr, len);

  /* data */
  memcpy(bufPtr, data, len);

  wiiuse_send(wm, WM_CMD_WRITE_DATA, buf, 21);
  return 1;
}

/**
 *	@brief	Write data to the wiimote (callback version).
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param addr			The address to write to.
 *	@param data			The data to be written to the memory
 *location.
 *	@param len			The length of the block to be written.
 *	@param cb			Function pointer to call when the data
 *arrives from the wiimote.
 *
 *	The library can only handle one data read request at a time
 *	because it must keep track of the buffer and other
 *	events that are specific to that request.  So if a request
 *	has already been made, subsequent requests will be added
 *	to a pending list and be sent out when the previous
 *	finishes.
 */
int wiiuse_write_data_cb(struct wiimote_t *wm, unsigned int addr, byte *data,
                         byte len, wiiuse_write_cb write_cb) {
  struct data_req_t *req;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return 0;
  }
  if (!data || !len) {
    return 0;
  }

  req = (struct data_req_t *)malloc(sizeof(struct data_req_t));
  req->cb = write_cb;
  req->len = len;
  memcpy(req->data, data, req->len);
  req->state = REQ_READY;
  req->addr = addr; /* BIG_ENDIAN_LONG(addr); */
  req->next = NULL;
  /* add this to the request list */
  if (!wm->data_req) {
    /* root node */
    wm->data_req = req;

    WIIUSE_DEBUG("Data write request can be sent out immediately.");

    /* send the request out immediately */
    wiiuse_send_next_pending_write_request(wm);
  } else {
    struct data_req_t *nptr = wm->data_req;
    WIIUSE_DEBUG("chaud2fois");
    for (; nptr->next; nptr = nptr->next) {
      ;
    }
    nptr->next = req;

    WIIUSE_DEBUG("Added pending data write request.");
  }

  return 1;
}

/**
 *	@brief Send the next pending data write request to the wiimote.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	@see wiiuse_write_data()
 *
 *	This function is not part of the wiiuse API.
 */
void wiiuse_send_next_pending_write_request(struct wiimote_t *wm) {
  struct data_req_t *req;

  if (!wm || !WIIMOTE_IS_CONNECTED(wm)) {
    return;
  }
  req = wm->data_req;
  if (!req) {
    return;
  }
  if (!req->len) {
    return;
  }
  if (req->state != REQ_READY) {
    return;
  }

  wiiuse_write_data(wm, req->addr, req->data, req->len);

  req->state = REQ_SENT;
  return;
}

/**
 *	@brief	Send a packet to the wiimote.
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param report_type	The report type to send (WIIMOTE_CMD_LED,
 *WIIMOTE_CMD_RUMBLE, etc). Found in wiiuse.h
 *	@param msg			The payload. Might be changed by the
 *callee.
 *	@param len			Length of the payload in bytes.
 *
 *	This function should replace any write()s directly to the wiimote
 *device.
 */
int wiiuse_send(struct wiimote_t *wm, byte report_type, byte *msg, int len) {
  switch (report_type) {
  case WM_CMD_LED:
  case WM_CMD_RUMBLE:
  case WM_CMD_CTRL_STATUS: {
    /* Rumble flag for: 0x11, 0x13, 0x14, 0x15, 0x19 or 0x1a */
    if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_RUMBLE)) {
      msg[0] |= 0x01;
    }
    break;
  }
  default:
    break;
  }

#ifdef WITH_WIIUSE_DEBUG
  {
    int x;
    printf("[DEBUG] (id %i) SEND: (%.2x) %.2x ", wm->unid, report_type, msg[0]);
    for (x = 1; x < len; ++x) {
      printf("%.2x ", msg[x]);
    }
    printf("\n");
  }
#endif

  return wiiuse_os_write(wm, report_type, msg, len);
}

/**
 *	@brief Set flags for the specified wiimote.
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param enable		Flags to enable.
 *	@param disable		Flags to disable.
 *
 *	@return The flags set after 'enable' and 'disable' have been applied.
 *
 *	The values 'enable' and 'disable' may be any flags OR'ed together.
 *	Flags are defined in wiiuse.h.
 */
int wiiuse_set_flags(struct wiimote_t *wm, int enable, int disable) {
  if (!wm) {
    return 0;
  }

  /* remove mutually exclusive flags */
  enable &= ~disable;
  disable &= ~enable;

  wm->flags |= enable;
  wm->flags &= ~disable;

  return wm->flags;
}

/**
 *	@brief Set the wiimote smoothing alpha value.
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param alpha		The alpha value to set. Between 0 and 1.
 *
 *	@return Returns the old alpha value.
 *
 *	The alpha value is between 0 and 1 and is used in an exponential
 *	smoothing algorithm.
 *
 *	Smoothing is only performed if the WIIMOTE_USE_SMOOTHING is set.
 */
float wiiuse_set_smooth_alpha(struct wiimote_t *wm, float alpha) {
  float old;

  if (!wm) {
    return 0.0f;
  }

  old = wm->accel_calib.st_alpha;

  wm->accel_calib.st_alpha = alpha;

  /* if there is a nunchuk set that too */
  if (wm->exp.type == EXP_NUNCHUK) {
    wm->exp.nunchuk.accel_calib.st_alpha = alpha;
  }

  return old;
}

/**
 *	@brief	Set the bluetooth stack type to use.
 *
 *	@param wm		Array of wiimote_t structures.
 *	@param wiimotes	Number of objects in the wm array.
 *	@param type		The type of bluetooth stack to use.
 */
void wiiuse_set_bluetooth_stack(struct wiimote_t **wm, int wiimotes,
                                enum win_bt_stack_t type) {
#ifdef WIIUSE_WIN32
  int i;

  if (!wm) {
    return;
  }

  for (i = 0; i < wiimotes; ++i) {
    wm[i]->stack = type;
  }
#endif
}

/**
 *	@brief	Set the orientation event threshold.
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param threshold	The decimal place that should be considered a
 *significant change.
 *
 *	If threshold is 0.01, and any angle changes by 0.01 then a significant
 *change has occurred and the event callback will be invoked.  If threshold is 1
 *then the angle has to change by a full degree to generate an event.
 */
void wiiuse_set_orient_threshold(struct wiimote_t *wm, float threshold) {
  if (!wm) {
    return;
  }

  wm->orient_threshold = threshold;
}

/**
 *	@brief	Set the accelerometer event threshold.
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param threshold	The decimal place that should be considered a
 *significant change.
 */
void wiiuse_set_accel_threshold(struct wiimote_t *wm, int threshold) {
  if (!wm) {
    return;
  }

  wm->accel_threshold = threshold;
}

/**
 *	@brief	Switch the Balance Board to use report 0x34 instead of 0x32
 *
 *	@param wm			Pointer to a wiimote_t structure.
 *	@param enabled	    enabled != 0 -> use report 0x32
 */
void wiiuse_wiiboard_use_alternate_report(struct wiimote_t *wm, int enabled) {
  if (wm->exp.type == EXP_WII_BOARD) {
    wm->exp.wb.use_alternate_report = enabled;
    wiiuse_set_report_type(wm);
  } else
    printf("Alternate report can be set only on a Balance Board!\n");
}

/**
 *	@brief Try to resync with the wiimote by starting a new handshake.
 *
 *	@param wm			Pointer to a wiimote_t structure.
 */
void wiiuse_resync(struct wiimote_t *wm) {
  if (!wm) {
    return;
  }

#ifndef WIIUSE_SYNC_HANDSHAKE
  wm->handshake_state = 0;
#endif
  wiiuse_handshake(wm, NULL, 0);
}

/**
 *	@brief Set the normal and expansion handshake timeouts.
 *
 *	@param wm				Array of wiimote_t structures.
 *	@param wiimotes			Number of objects in the wm array.
 *	@param normal_timeout	The timeout in milliseconds for a normal read.
 *	@param exp_timeout		The timeout in millisecondsd to wait for
 *an expansion handshake.
 */
void wiiuse_set_timeout(struct wiimote_t **wm, int wiimotes,
                        byte normal_timeout, byte exp_timeout) {
#ifdef WIIUSE_WIN32
  int i;

  if (!wm) {
    return;
  }

  for (i = 0; i < wiimotes; ++i) {
    wm[i]->normal_timeout = normal_timeout;
    wm[i]->exp_timeout = exp_timeout;
  }
#endif
}
