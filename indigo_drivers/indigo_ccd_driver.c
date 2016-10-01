//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "indigo_ccd_driver.h"
#include "indigo_timer.h"

indigo_result indigo_ccd_driver_attach(indigo_driver *driver, char *device, int version) {
  assert(driver != NULL);
  assert(device != NULL);
  if (CCD_DRIVER_CONTEXT == NULL)
    driver->driver_context = malloc(sizeof(indigo_ccd_driver_context));
  if (CCD_DRIVER_CONTEXT != NULL) {
    if (indigo_driver_attach(driver, device, version, 2) == INDIGO_OK) {
      // -------------------------------------------------------------------------------- CCD_INFO
      CCD_INFO_PROPERTY = indigo_init_number_property(NULL, device, "CCD_INFO", MAIN_GROUP, "CCD Info", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 8);
      if (CCD_INFO_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_INFO_WIDTH_ITEM, "WIDTH", "Horizontal resolution", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_HEIGHT_ITEM, "HEIGHT", "Vertical resolution", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_MAX_HORIZONAL_BIN_ITEM, "MAX_HORIZONAL_BIN", "Max vertical binning", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_MAX_VERTICAL_BIN_ITEM, "MAX_VERTICAL_BIN", "Max horizontal binning", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_PIXEL_SIZE_ITEM, "PIXEL_SIZE", "Pixel size", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_PIXEL_WIDTH_ITEM, "PIXEL_WIDTH", "Pixel width", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_PIXEL_HEIGHT_ITEM, "PIXEL_HEIGHT", "Pixel height", 0, 0, 0, 0);
      indigo_init_number_item(CCD_INFO_BITS_PER_PIXEL_ITEM, "BITS_PER_PIXEL", "Bits/pixel", 0, 0, 0, 0);
      // -------------------------------------------------------------------------------- CCD_EXPOSURE
      CCD_EXPOSURE_PROPERTY = indigo_init_number_property(NULL, device, "CCD_EXPOSURE", MAIN_GROUP, "Start exposure", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 1);
      if (CCD_EXPOSURE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_EXPOSURE_ITEM, "EXPOSURE", "Start exposure", 0, 10000, 1, 0);
      // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
      CCD_ABORT_EXPOSURE_PROPERTY = indigo_init_switch_property(NULL, device, "CCD_ABORT_EXPOSURE", MAIN_GROUP, "Abort exposure", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 1);
      if (CCD_ABORT_EXPOSURE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_switch_item(CCD_ABORT_EXPOSURE_ITEM, "ABORT_EXPOSURE", "Abort exposure", false);
      // -------------------------------------------------------------------------------- CCD_FRAME
      CCD_FRAME_PROPERTY = indigo_init_number_property(NULL, device, "CCD_FRAME", MAIN_GROUP, "Frame size", INDIGO_IDLE_STATE, INDIGO_RW_PERM, 4);
      if (CCD_FRAME_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_FRAME_LEFT_ITEM, "X", "Left", 0, 0, 1, 0);
      indigo_init_number_item(CCD_FRAME_TOP_ITEM, "Y", "Top", 0, 0, 1, 0);
      indigo_init_number_item(CCD_FRAME_WIDTH_ITEM, "WIDTH", "Width", 0, 0, 1, 0);
      indigo_init_number_item(CCD_FRAME_HEIGHT_ITEM, "HEIGHT", "Height", 0, 0, 1, 0);
      // -------------------------------------------------------------------------------- CCD_BIN
      CCD_BIN_PROPERTY = indigo_init_number_property(NULL, device, "CCD_BIN", MAIN_GROUP, "Binning", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 2);
      if (CCD_BIN_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_BIN_HORIZONTAL_ITEM, "HORIZONTAL", "Horizontal binning", 0, 1, 1, 1);
      indigo_init_number_item(CCD_BIN_VERTICAL_ITEM, "VERTICAL", "Vertical binning", 0, 1, 1, 1);
      // -------------------------------------------------------------------------------- CCD_FRAME_TYPE
      CCD_FRAME_TYPE_PROPERTY = indigo_init_switch_property(NULL, device, "CCD_FRAME_TYPE", MAIN_GROUP, "Frame type", INDIGO_IDLE_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 4);
      if (CCD_FRAME_TYPE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_switch_item(CCD_FRAME_TYPE_LIGHT_ITEM, "LIGHT", "Light frame exposure", true);
      indigo_init_switch_item(CCD_FRAME_TYPE_BIAS_ITEM, "BIAS", "Bias frame exposure", false);
      indigo_init_switch_item(CCD_FRAME_TYPE_DARK_ITEM, "DARK", "Dark frame exposure", false);
      indigo_init_switch_item(CCD_FRAME_TYPE_FLAT_ITEM, "FLAT", "Flat field frame exposure", false);
      // -------------------------------------------------------------------------------- CCD_IMAGE
      CCD_IMAGE_PROPERTY = indigo_init_blob_property(NULL, device, "CCD_IMAGE", IMAGE_GROUP, "Image", INDIGO_IDLE_STATE, 1);
      if (CCD_IMAGE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_blob_item(CCD_IMAGE_ITEM, "IMAGE", "Primary CCD image");
      // -------------------------------------------------------------------------------- CCD_COOLER
      CCD_COOLER_PROPERTY = indigo_init_switch_property(NULL, device, "CCD_COOLER", MAIN_GROUP, "Cooler control", INDIGO_IDLE_STATE, INDIGO_RO_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
      if (CCD_COOLER_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_switch_item(CCD_COOLER_ON_ITEM, "ON", "On", false);
      indigo_init_switch_item(CCD_COOLER_OFF_ITEM, "OFF", "Off", true);
      // -------------------------------------------------------------------------------- CCD_COOLER_POWER
      CCD_COOLER_POWER_PROPERTY = indigo_init_number_property(NULL, device, "CCD_COOLER_POWER", MAIN_GROUP, "Cooler power", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
      if (CCD_COOLER_POWER_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_COOLER_POWER_ITEM, "POWER", "Power (%)", 0, 0, 0, NAN);
      // -------------------------------------------------------------------------------- CCD_TEMPERATURE
      CCD_TEMPERATURE_PROPERTY = indigo_init_number_property(NULL, device, "CCD_TEMPERATURE", MAIN_GROUP, "Temperature", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 1);
      if (CCD_TEMPERATURE_PROPERTY == NULL)
        return INDIGO_FAILED;
      indigo_init_number_item(CCD_TEMPERATURE_ITEM, "TEMPERATURE", "Temperature (C)", 0, 0, 0, NAN);
      return INDIGO_OK;
    }
  }
  return INDIGO_FAILED;
}

indigo_result indigo_ccd_driver_enumerate_properties(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(driver->driver_context != NULL);
  assert(property != NULL);
  indigo_result result = INDIGO_OK;
  if ((result = indigo_driver_enumerate_properties(driver, client, property)) == INDIGO_OK) {
    if (indigo_is_connected(driver_context)) {
      if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property))
        indigo_define_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property))
        indigo_define_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
      if (indigo_property_match(CCD_FRAME_PROPERTY, property))
        indigo_define_property(driver, CCD_FRAME_PROPERTY, NULL);
      if (indigo_property_match(CCD_BIN_PROPERTY, property))
        indigo_define_property(driver, CCD_BIN_PROPERTY, NULL);
      if (indigo_property_match(CCD_FRAME_TYPE_PROPERTY, property))
        indigo_define_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
      if (indigo_property_match(CCD_IMAGE_PROPERTY, property))
        indigo_define_property(driver, CCD_IMAGE_PROPERTY, NULL);
      if (indigo_property_match(CCD_COOLER_PROPERTY, property))
        indigo_define_property(driver, CCD_COOLER_PROPERTY, NULL);
      if (indigo_property_match(CCD_COOLER_POWER_PROPERTY, property))
        indigo_define_property(driver, CCD_COOLER_POWER_PROPERTY, NULL);
      if (indigo_property_match(CCD_TEMPERATURE_PROPERTY, property))
        indigo_define_property(driver, CCD_TEMPERATURE_PROPERTY, NULL);
    }
  }
  return result;
}

static void countdown_timer_callback(indigo_driver *driver, int timer_id, void *data, double delay) {
  if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
    CCD_EXPOSURE_ITEM->number_value -= 1;
    if (CCD_EXPOSURE_ITEM->number_value >= 1) {
      indigo_update_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      indigo_set_timer(driver, 1, NULL, 1.0, countdown_timer_callback);
    }
  }
}

indigo_result indigo_ccd_driver_change_property(indigo_driver *driver, indigo_client *client, indigo_property *property) {
  assert(driver != NULL);
  assert(driver->driver_context != NULL);
  assert(property != NULL);
  if (indigo_property_match(CONNECTION_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONNECTION
    if (CONNECTION_CONNECTED_ITEM->switch_value) {
      indigo_define_property(driver, CCD_INFO_PROPERTY, NULL);
      indigo_define_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      indigo_define_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
      indigo_define_property(driver, CCD_FRAME_PROPERTY, NULL);
      indigo_define_property(driver, CCD_BIN_PROPERTY, NULL);
      indigo_define_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
      indigo_define_property(driver, CCD_IMAGE_PROPERTY, NULL);
      indigo_define_property(driver, CCD_COOLER_PROPERTY, NULL);
      indigo_define_property(driver, CCD_COOLER_POWER_PROPERTY, NULL);
      indigo_define_property(driver, CCD_TEMPERATURE_PROPERTY, NULL);
    } else {
      indigo_delete_property(driver, CCD_INFO_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_FRAME_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_BIN_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_IMAGE_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_COOLER_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_COOLER_POWER_PROPERTY, NULL);
      indigo_delete_property(driver, CCD_TEMPERATURE_PROPERTY, NULL);
    }
  } else if (indigo_property_match(CONFIG_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CONFIG
    if (CONFIG_SAVE_ITEM->switch_value) {
      if (CCD_FRAME_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_FRAME_PROPERTY);
      if (CCD_BIN_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_BIN_PROPERTY);
      if (CCD_FRAME_TYPE_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_FRAME_TYPE_PROPERTY);
      if (CCD_COOLER_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_COOLER_PROPERTY);
      if (CCD_TEMPERATURE_PROPERTY->perm == INDIGO_RW_PERM)
        indigo_save_property(CCD_TEMPERATURE_PROPERTY);
    }
  } else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_EXPOSURE
    if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      if (CCD_EXPOSURE_ITEM->number_value >= 1) {
        indigo_set_timer(driver, 1, NULL, 1.0, countdown_timer_callback);
      }
    }
  } else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_ABORT_EXPOSURE
    if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
      indigo_cancel_timer(driver, 1);
      CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
      CCD_EXPOSURE_ITEM->number_value = 0;
      indigo_update_property(driver, CCD_EXPOSURE_PROPERTY, NULL);
      CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
      indigo_update_property(driver, CCD_IMAGE_PROPERTY, NULL);
      CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
    } else {
      CCD_ABORT_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
    }
    CCD_ABORT_EXPOSURE_ITEM->switch_value = false;
    indigo_update_property(driver, CCD_ABORT_EXPOSURE_PROPERTY, CCD_ABORT_EXPOSURE_PROPERTY->state == INDIGO_OK_STATE ? "Exposure canceled" : "Failed to cancel exposure");
  } else if (indigo_property_match(CCD_FRAME_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_FRAME
    indigo_property_copy_values(CCD_FRAME_PROPERTY, property, false);
    CCD_FRAME_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(driver, CCD_FRAME_PROPERTY, NULL);
  } else if (indigo_property_match(CCD_BIN_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_BIN
    indigo_property_copy_values(CCD_BIN_PROPERTY, property, false);
    CCD_BIN_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(driver, CCD_BIN_PROPERTY, NULL);
  } else if (indigo_property_match(CCD_FRAME_TYPE_PROPERTY, property)) {
    // -------------------------------------------------------------------------------- CCD_FRAME_TYPE
    indigo_property_copy_values(CCD_FRAME_TYPE_PROPERTY, property, false);
    CCD_FRAME_TYPE_PROPERTY->state = INDIGO_OK_STATE;
    indigo_update_property(driver, CCD_FRAME_TYPE_PROPERTY, NULL);
  }
  return indigo_driver_change_property(driver, client, property);
}

indigo_result indigo_ccd_driver_detach(indigo_driver *driver) {
  assert(driver != NULL);
  return INDIGO_OK;
}

void *indigo_convert_to_fits(indigo_driver *driver, double exposure_time) {
  INDIGO_DEBUG(clock_t start = clock());
  time_t timer;
  struct tm* tm_info;
  char now[20];
  time(&timer);
  tm_info = gmtime(&timer);
  strftime(now, 20, "%Y:%m:%dT%H:%M:%S", tm_info);
  char *header = CCD_IMAGE_ITEM->blob_value;
  memset(header, ' ', FITS_HEADER_SIZE);
  int t = sprintf(header, "SIMPLE  =                     T / file conforms to FITS standard"); header[t] = ' ';
  t = sprintf(header += 80, "BITPIX  = %21d / number of bits per data pixel", (int)CCD_INFO_BITS_PER_PIXEL_ITEM->number_value); header[t] = ' ';
  t = sprintf(header += 80, "NAXIS   =                     2 / number of data axes"); header[t] = ' ';
  t = sprintf(header += 80, "NAXIS1  = %21d / length of data axis 1", (int)CCD_FRAME_WIDTH_ITEM->number_value); header[t] = ' ';
  t = sprintf(header += 80, "NAXIS2  = %21d / length of data axis 2", (int)CCD_FRAME_HEIGHT_ITEM->number_value); header[t] = ' ';
  t = sprintf(header += 80, "EXTEND  =                     T / FITS dataset may contain extensions"); header[t] = ' ';
  t = sprintf(header += 80, "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy"); header[t] = ' ';
  t = sprintf(header += 80, "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H"); header[t] = ' ';
  t = sprintf(header += 80, "BZERO   =                 32768 / offset data range to that of unsigned short"); header[t] = ' ';
  t = sprintf(header += 80, "BSCALE  =                     1 / default scaling factor"); header[t] = ' ';
  t = sprintf(header += 80, "XBINNING= %21d / horizontal binning mode", (int)CCD_BIN_HORIZONTAL_ITEM->number_value); header[t] = ' ';
  t = sprintf(header += 80, "YBINNING= %21d / vertical binning mode", (int)CCD_BIN_VERTICAL_ITEM->number_value); header[t] = ' ';
  t = sprintf(header += 80, "XPIXSZ  = %21.2g / pixel width in microns", CCD_INFO_PIXEL_WIDTH_ITEM->number_value); header[t] = ' ';
  t = sprintf(header += 80, "YPIXSZ  = %21.2g / pixel height in microns", CCD_INFO_PIXEL_HEIGHT_ITEM->number_value); header[t] = ' ';
  t = sprintf(header += 80, "EXPTIME = %21.2g / exposure time [s]", exposure_time); header[t] = ' ';
  if (!isnan(CCD_TEMPERATURE_ITEM->number_value)) {
    t = sprintf(header += 80, "CCD-TEMP= %21.2g / CCD temperature in C", CCD_TEMPERATURE_ITEM->number_value); header[t] = ' ';
  }
  t = sprintf(header += 80, "DATE    = '%s' / UTC date that FITS file was created", now); header[t] = ' ';
  t = sprintf(header += 80, "INSTRUME= '%s'%*c / instrument name", INFO_DRIVER_NAME_ITEM->text_value, (int)(19 - strlen(INFO_DRIVER_NAME_ITEM->text_value)), ' '); header[t] = ' ';
  t = sprintf(header += 80, "COMMENT   Created by INDIGO %d.%d framework, see http://www.indigo-astronomy.org", (INDIGO_VERSION_CURRENT >> 8) & 0xFF, INDIGO_VERSION_CURRENT & 0xFF); header[t] = ' ';
  t = sprintf(header += 80, "END"); header[t] = ' ';
  
  short *data = (short *)(CCD_IMAGE_ITEM->blob_value + FITS_HEADER_SIZE);
  int size = CCD_INFO_WIDTH_ITEM->number_value * CCD_INFO_HEIGHT_ITEM->number_value;
  for (int i = 0; i < size; i++) {
    int value = *data - 32768;
    *data++ = (value & 0xff) << 8 | (value & 0xff00) >> 8;
  }
  INDIGO_DEBUG(indigo_debug("RAW to FITS conversion in %gs", (clock() - start) / (double)CLOCKS_PER_SEC));
  return INDIGO_OK;
}
