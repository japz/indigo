// Copyright (c) 2018 Thomas Stibor
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Thomas Stibor <thomas@stibor.net>

/** INDIGO GPHOTO2 CCD driver
 \file indigo_ccd_gphoto2.c
 */

#define DRIVER_VERSION 0x0003
#define DRIVER_NAME "indigo_ccd_gphoto2"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-list.h>
#include <gphoto2/gphoto2-version.h>
#include <gphoto2/gphoto2-list.h>
#include <libusb-1.0/libusb.h>
#include <libraw/libraw.h>
#include "indigo_ccd_gphoto2.h"
#include "dslr_model_info.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define STRNCMP(str1, str2)				\
	((strlen(str1) == strlen(str2)) &&		\
	 (strncmp(str1, str2, strlen(str1)) == 0))

#define GPHOTO2_NAME_DSLR			 "GPhoto2"
#define GPHOTO2_NAME_SHUTTER			 "Shutter time"
#define GPHOTO2_NAME_ISO			 "ISO"
#define GPHOTO2_NAME_COMPRESSION		 "Compression"
#define GPHOTO2_NAME_WHITEBALANCE		 "Whitebalance"
#define GPHOTO2_NAME_ZOOM_PREVIEW                "Liveview zoom"
#define GPHOTO2_NAME_ZOOM_PREVIEW_ON_ITEM        "5"
#define GPHOTO2_NAME_ZOOM_PREVIEW_OFF_ITEM       "1"
#define GPHOTO2_NAME_ZOOM_PREVIEW_ON             "On"
#define GPHOTO2_NAME_ZOOM_PREVIEW_OFF            "Off"
#define GPHOTO2_NAME_MIRROR_LOCKUP		 "Use mirror lockup"
#define GPHOTO2_NAME_MIRROR_LOCKUP_ITEM_NAME	 "MIRROR_LOCKUP"
#define GPHOTO2_NAME_LIBGPHOTO2			 "Gphoto2 library"
#define GPHOTO2_NAME_LIBGPHOTO2_VERSION		 "Version"
#define GPHOTO2_NAME_DELETE_IMAGE                "Delete downloaded image"
#define GPHOTO2_NAME_DELETE_IMAGE_ON_ITEM        "ON"
#define GPHOTO2_NAME_DELETE_IMAGE_OFF_ITEM       "OFF"
#define GPHOTO2_NAME_DELETE_IMAGE_ON             "On"
#define GPHOTO2_NAME_DELETE_IMAGE_OFF            "Off"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM           "Debayer algorithm"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_LIN_NAME  "DEBAYER_LINEAR"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_VNG_NAME  "DEBAYER_VNG"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_PPG_NAME  "DEBAYER_PPG"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_AHD_NAME  "DEBAYER_AHD"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_DCB_NAME  "DEBAYER_DCB"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_DHT_NAME  "DEBAYER_DHT"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_LIN_LABEL "Linear"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_VNG_LABEL "VNG"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_PPG_LABEL "PPG"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_AHD_LABEL "AHD"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_DCB_LABEL "DCB"
#define GPHOTO2_NAME_DEBAYER_ALGORITHM_DHT_LABEL "DHT"

#define GPHOTO2_DEBAYER_ALGORITHM_PROPERTY_NAME	 "GPHOTO2_DEBAYER_ALGORITHM"
#define GPHOTO2_LIBGPHOTO2_VERSION_PROPERTY_NAME "GPHOTO2_LIBGPHOTO2_VERSION"
#define GPHOTO2_LIBGPHOTO2_VERSION_ITEM_NAME     "LIBGPHOTO2_VERSION"

#define NIKON_ISO				"iso"
#define NIKON_COMPRESSION			"imagequality"
#define NIKON_SHUTTERSPEED			"shutterspeed"
#define NIKON_WHITEBALANCE			"whitebalance"
#define NIKON_CAPTURE_TARGET			"capturetarget"
#define NIKON_MEMORY_CARD			"Memory card"
#define NIKON_BULB_MODE                         "bulb"
#define NIKON_BULB_MODE_LABEL                   "Bulb Mode"

#define EOS_ISO					NIKON_ISO
#define EOS_COMPRESSION				"imageformat"
#define EOS_SHUTTERSPEED			NIKON_SHUTTERSPEED
#define EOS_WHITEBALANCE			NIKON_WHITEBALANCE
#define EOS_CAPTURE_TARGET			NIKON_CAPTURE_TARGET
#define EOS_MEMORY_CARD				NIKON_MEMORY_CARD
#define EOS_ZOOM_PREVIEW                        "eoszoom"
#define EOS_REMOTE_RELEASE		        "eosremoterelease"
#define EOS_PRESS_FULL			        "Press Full"
#define EOS_RELEASE_FULL		        "Release Full"
#define EOS_CUSTOMFUNCEX			"customfuncex"
#define EOS_MIRROR_LOCKUP_ENABLE		"20,1,3,14,1,60f,1,1"
#define EOS_MIRROR_LOCKUP_DISABLE		"20,1,3,14,1,60f,1,0"
#define EOS_VIEWFINDER                          "viewfinder"
#define EOS_REMOTE_RELEASE_LABEL                "Canon EOS Remote Release"
#define EOS_BULB_MODE                           NIKON_BULB_MODE
#define EOS_BULB_MODE_LABEL                     NIKON_BULB_MODE_LABEL

#define SONY_COMPRESSION			NIKON_COMPRESSION

#define TIMER_COUNTER_STEP_SEC                  0.1   /* 100 ms. */

#define UNUSED(x)				(void)(x)
#define MAX_DEVICES				8
#define PRIVATE_DATA				((gphoto2_private_data *)device->private_data)
#define DSLR_ISO_PROPERTY			(PRIVATE_DATA->dslr_iso_property)
#define DSLR_SHUTTER_PROPERTY			(PRIVATE_DATA->dslr_shutter_property)
#define DSLR_COMPRESSION_PROPERTY		(PRIVATE_DATA->dslr_compression_property)
#define DSLR_WHITEBALANCE_PROPERTY		(PRIVATE_DATA->dslr_whitebalance_property)
#define DSLR_MIRROR_LOCKUP_PROPERTY		(PRIVATE_DATA->dslr_mirror_lockup_property)
#define DSLR_MIRROR_LOCKUP_ITEM			(PRIVATE_DATA->dslr_mirror_lockup_property->items)
#define DSLR_DELETE_IMAGE_PROPERTY		(PRIVATE_DATA->dslr_delete_image_property)
#define DSLR_DELETE_IMAGE_ON_ITEM		(PRIVATE_DATA->dslr_delete_image_property->items + 0)
#define DSLR_DELETE_IMAGE_OFF_ITEM		(PRIVATE_DATA->dslr_delete_image_property->items + 1)
#define DSLR_DEBAYER_ALGORITHM_PROPERTY		(PRIVATE_DATA->dslr_debayer_algorithm_property)
#define DSLR_DEBAYER_ALGORITHM_LIN_ITEM		(PRIVATE_DATA->dslr_debayer_algorithm_property->items + 0)
#define DSLR_DEBAYER_ALGORITHM_VNG_ITEM		(PRIVATE_DATA->dslr_debayer_algorithm_property->items + 1)
#define DSLR_DEBAYER_ALGORITHM_PPG_ITEM		(PRIVATE_DATA->dslr_debayer_algorithm_property->items + 2)
#define DSLR_DEBAYER_ALGORITHM_AHD_ITEM		(PRIVATE_DATA->dslr_debayer_algorithm_property->items + 3)
#define DSLR_DEBAYER_ALGORITHM_DCB_ITEM		(PRIVATE_DATA->dslr_debayer_algorithm_property->items + 4)
#define DSLR_DEBAYER_ALGORITHM_DHT_ITEM		(PRIVATE_DATA->dslr_debayer_algorithm_property->items + 5)
#define GPHOTO2_LIBGPHOTO2_VERSION_PROPERTY	(PRIVATE_DATA->dslr_libgphoto2_version_property)
#define GPHOTO2_LIBGPHOTO2_VERSION_ITEM		(PRIVATE_DATA->dslr_libgphoto2_version_property->items)
#define COMPRESSION                             (PRIVATE_DATA->gphoto2_compression_id)
#define DSLR_ZOOM_PREVIEW_PROPERTY              (PRIVATE_DATA->dslr_zoom_preview_property)
#define DSLR_ZOOM_PREVIEW_ON_ITEM		(PRIVATE_DATA->dslr_zoom_preview_property->items + 0)
#define DSLR_ZOOM_PREVIEW_OFF_ITEM		(PRIVATE_DATA->dslr_zoom_preview_property->items + 1)
#define is_connected				gp_bits

enum vendor {
	CANON = 0,
	NIKON,
	SONY,
	OTHER
};

const char * sony_shutterspeeds[] = {
	"100/10",
	"130/10",
	"150/10",
	"200/10",
	"250/10",
	"300/10",
};
#define sony_shutterspeeds_count (sizeof (sony_shutterspeeds) / sizeof (const char *))

struct gphoto2_id_s {
	char name[32];
	char name_extended[128];
	uint8_t bus;
	uint8_t port;
	uint16_t vendor;
	uint16_t product;
};

typedef struct {
	Camera *camera;
	struct gphoto2_id_s gphoto2_id;
	char libgphoto2_version[16];
	bool delete_downloaded_image;
	char *buffer;
	unsigned long int buffer_size;
	unsigned long int buffer_size_max;
	char filename_suffix[9];
	enum vendor vendor;
	char *gphoto2_compression_id;
	char *name_best_jpeg_format;
	char *name_pure_raw_format;
	float mirror_lockup_secs;
	bool shutterspeed_bulb;
	bool has_single_bulb_mode;
	bool has_eos_remote_release;
	int debayer_algorithm;
	indigo_property *dslr_shutter_property;
	indigo_property *dslr_iso_property;
	indigo_property *dslr_compression_property;
	indigo_property *dslr_whitebalance_property;
	indigo_property *dslr_zoom_preview_property;
	indigo_property *dslr_mirror_lockup_property;
	indigo_property *dslr_delete_image_property;
	indigo_property *dslr_debayer_algorithm_property;
	indigo_property *dslr_libgphoto2_version_property;
	indigo_timer *exposure_timer, *counter_timer;
	pthread_mutex_t driver_mutex;
} gphoto2_private_data;

struct capture_abort {
	CameraFile *camera_file;
	indigo_device *device;
};

static GPContext *context = NULL;
static pthread_t thread_id_capture;
static indigo_device *devices[MAX_DEVICES] = {NULL};
static libusb_hotplug_callback_handle callback_handle;

static char *debayer_algorithm_str_id(const int algorithm)
{
	switch (algorithm) {
	case 0:
		return GPHOTO2_NAME_DEBAYER_ALGORITHM_LIN_NAME;
	case 1:
		return GPHOTO2_NAME_DEBAYER_ALGORITHM_VNG_NAME;
	case 2:
		return GPHOTO2_NAME_DEBAYER_ALGORITHM_PPG_NAME;
	case 3:
		return GPHOTO2_NAME_DEBAYER_ALGORITHM_AHD_NAME;
	case 4:
		return GPHOTO2_NAME_DEBAYER_ALGORITHM_DCB_NAME;
	case 11:
		return GPHOTO2_NAME_DEBAYER_ALGORITHM_DHT_NAME;
	default:
		return "unknown";
	}
}

static int debayer_algorithm_value_id(const char *algorithm)
{
	if (STRNCMP(algorithm, GPHOTO2_NAME_DEBAYER_ALGORITHM_LIN_NAME))
		return 0;
	else if (STRNCMP(algorithm, GPHOTO2_NAME_DEBAYER_ALGORITHM_VNG_NAME))
		return 1;
	else if (STRNCMP(algorithm, GPHOTO2_NAME_DEBAYER_ALGORITHM_PPG_NAME))
		return 2;
	else if (STRNCMP(algorithm, GPHOTO2_NAME_DEBAYER_ALGORITHM_AHD_NAME))
		return 3;
	else if (STRNCMP(algorithm, GPHOTO2_NAME_DEBAYER_ALGORITHM_DCB_NAME))
		return 4;
	else if (STRNCMP(algorithm, GPHOTO2_NAME_DEBAYER_ALGORITHM_DHT_NAME))
		return 11;
	else
		return -1;	/* Unknown. */
}

static int progress_cb(void *callback_data,
		       enum LibRaw_progress stage, int iteration, int expected)
{
	(void)callback_data;

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libraw: %s, step %i/%i",
			    libraw_strprogress(stage), iteration + 1, expected);

	return 0;
}

static int process_dslr_image_debayer(indigo_device *device,
				       void *buffer, size_t buffer_size)
{
        int rc;
        libraw_data_t *raw_data;
        libraw_processed_image_t *processed_image = NULL;

        raw_data = libraw_init(0);

	/* Linear 16-bit output. */
	raw_data->params.output_bps = 16;
	/* Debayer algorithm. */
	raw_data->params.user_qual = PRIVATE_DATA->debayer_algorithm;
	/* Disable four color space. */
	raw_data->params.four_color_rgb = 0;
	/* Disable LibRaw's default histogram transformation. */
	raw_data->params.no_auto_bright = 1;
	/* Disable LibRaw's default gamma curve transformation, */
	raw_data->params.gamm[0] = raw_data->params.gamm[1] = 1.0;
	/* Do not apply an embedded color profile, enabled by LibRaw by default. */
	raw_data->params.use_camera_matrix = 0;
	/* Disable automatic white balance obtained after averaging over the entire image. */
	raw_data->params.use_auto_wb = 0;
	/* Disable white balance from the camera (if possible). */
	raw_data->params.use_camera_wb = 0;

	libraw_set_progress_handler(raw_data, &progress_cb, NULL);

	rc = libraw_open_buffer(raw_data, buffer, buffer_size);
	if (rc != LIBRAW_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d] libraw_open_buffer "
				    "failed: '%s'",
				    rc, libraw_strerror(rc));
		goto cleanup;
	}

	rc = libraw_unpack(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d] libraw_unpack "
				    "failed: '%s'",
				    rc, libraw_strerror(rc));
		goto cleanup;
	}

	rc = libraw_raw2image(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "rc:%d] libraw_raw2image "
				    "failed: '%s'",
				    rc, libraw_strerror(rc));
		goto cleanup;
	}

	rc = libraw_dcraw_process(raw_data);
	if (rc != LIBRAW_SUCCESS) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d] libraw_dcraw_process "
				    "failed: '%s'",
				    rc, libraw_strerror(rc));
		goto cleanup;
	}

	processed_image = libraw_dcraw_make_mem_image(raw_data, &rc);
	if (!processed_image) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d] libraw_dcraw_make_mem_image "
				    "failed: '%s'",
				    rc, libraw_strerror(rc));
		goto cleanup;
	}

	if (processed_image->type != LIBRAW_IMAGE_BITMAP) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "input data is not of type "
				    "LIBRAW_IMAGE_BITMAP");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}

	if (processed_image->colors != 3) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "debayered data has not 3 colors");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}

	if (!(processed_image->bits == 16 || processed_image->bits == 8)) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "8 or 16 bit is supported only");
		rc = LIBRAW_UNSPECIFIED_ERROR;
		goto cleanup;
	}

	float cam_sensor_temperature = -273.15f;
	if (raw_data->other.SensorTemperature > -273.15f)
		cam_sensor_temperature = raw_data->other.SensorTemperature;
	else if (raw_data->other.CameraTemperature > -273.15f)
		cam_sensor_temperature = raw_data->other.CameraTemperature;

	indigo_fits_keyword keywords[] = {
		{ INDIGO_FITS_STRING, "CTYPE3  ",
		  .string = "rgb",
		  "coordinate axis red=1, green=2, blue=3" },
		{ INDIGO_FITS_NUMBER, "ISOSPEED",
		  .number = raw_data->other.iso_speed,
		  "ISO camera setting" },
		{ INDIGO_FITS_NUMBER, "CCD-TEMP",
		  .number = cam_sensor_temperature,
		  "CCD temperature [celcius]" },
		{ 0 },
	};
	/* I guess nobody cools down a CCD to absolute zero. */
	if (cam_sensor_temperature == -273.15f)
		memcpy(&keywords[2], &keywords[3], sizeof(indigo_fits_keyword));

	void *data = (unsigned char *)processed_image->data;
        unsigned long int data_size = processed_image->data_size +
		FITS_HEADER_SIZE;
	int padding = 0;
        int mod2880 = data_size % 2880;
	if (mod2880)
		padding = 2880 - mod2880;
	data_size += padding;

	if (data_size > PRIVATE_DATA->buffer_size_max) {
		PRIVATE_DATA->buffer_size_max = data_size;
		PRIVATE_DATA->buffer = realloc(PRIVATE_DATA->buffer,
					       data_size);
	}
	memcpy(PRIVATE_DATA->buffer + FITS_HEADER_SIZE,
	       data, processed_image->data_size);
	PRIVATE_DATA->buffer_size = data_size;

	indigo_process_image(device, PRIVATE_DATA->buffer,
                             processed_image->width, processed_image->height,
                             processed_image->bits * processed_image->colors,
                             true, /* little_endian */
			     true, /* RBG order */
                             keywords);

	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "input data: "
			    "%d bytes -> unpacked and debayered output data: "
			    "%d bytes, colors: %d, "
			    "bits: %d, algorithm: %s", buffer_size,
			    processed_image->data_size,
			    processed_image->colors, processed_image->bits,
			    debayer_algorithm_str_id(PRIVATE_DATA->debayer_algorithm));

cleanup:
	libraw_dcraw_clear_mem(processed_image);
	libraw_free_image(raw_data);
	libraw_recycle(raw_data);
        libraw_close(raw_data);

	return rc;
}

static void vendor_identify_widget(indigo_device *device,
				   const char *property_name)
{
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	if (!strcmp(DSLR_COMPRESSION_PROPERTY_NAME, property_name)) {
		if (PRIVATE_DATA->vendor == CANON)
			COMPRESSION = strdup(EOS_COMPRESSION);
		else if (PRIVATE_DATA->vendor == NIKON)
			COMPRESSION = strdup(NIKON_COMPRESSION);
		else if (PRIVATE_DATA->vendor == SONY)
			COMPRESSION = strdup(SONY_COMPRESSION);
		else	/* EOS fallback. */
			COMPRESSION = strdup(EOS_COMPRESSION);
	}
}

static double parse_shutterspeed(const char *s)
{
	assert(s != NULL);

	const char *delim = "/'";
	uint8_t cnt = 0;
	char *token;
	double nom_denom[2] = {0, 0};
	char *str;
	char *saveptr;

	str = strdup(s);
	token = strtok_r(str, delim, &saveptr);
	while (token != NULL) {
		nom_denom[cnt++ % 2] = atof(token);
		token = strtok_r(NULL, delim, &saveptr);
	}

	if (str)
		free(str);

	return (nom_denom[1] == 0 ? nom_denom[0] :
		nom_denom[0] / nom_denom[1]);
}

static int lookup_widget(CameraWidget *widget, const char *key,
			 CameraWidget **child)
{
	int rc;

	rc = gp_widget_get_child_by_name(widget, key, child);
	if (rc < GP_OK)
		rc = gp_widget_get_child_by_label(widget, key, child);
	return rc;
}

static int exists_widget_label(const char *key, indigo_device *device)
{
	CameraWidget *widget = NULL, *child = NULL;
	int rc;

	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);

	rc = gp_camera_get_config(PRIVATE_DATA->camera, &widget,
				  context);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] camera get config failed",
				    PRIVATE_DATA->camera, context);
		goto cleanup;
	}

	rc = lookup_widget(widget, key, &child);
	if (rc < GP_OK)
		goto cleanup;

cleanup:
	gp_widget_free(widget);

	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);

	return rc;
}

static int enumerate_widget(const char *key, indigo_device *device,
			    indigo_property *property)
{
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int rc;
	char *val = NULL;

	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);

	rc = gp_camera_get_config(PRIVATE_DATA->camera, &widget,
				  context);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] camera get config failed",
				    PRIVATE_DATA->camera, context);
		goto cleanup;
	}

	rc = lookup_widget(widget, key, &child);
	if (rc < GP_OK)
		goto cleanup;

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	rc = gp_widget_get_type(child, &type);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget get type failed",
				    PRIVATE_DATA->camera, context);
		goto cleanup;
	}

	/* Get the actual set value on camera. */
	rc = gp_widget_get_value(child, &val);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget get value failed",
				    PRIVATE_DATA->camera, context);
		goto cleanup;
	}

	if (type != GP_WIDGET_RADIO) {
		rc = GP_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	rc = gp_widget_count_choices(child);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget count "
				    "choices failed",
				    PRIVATE_DATA->camera, context);
		goto cleanup;
	}

	/* If property is NULL we are interested in number of choices only
	   which is stored in rc. */
	if (!property)
		goto cleanup;

	const int n_choices = rc;
	const char *widget_choice;
	int i = 0;

	while (i < n_choices) {
		rc = gp_widget_get_choice(child, i, &widget_choice);
		if (rc < GP_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "[camera:%p,context:%p] widget get "
					    "choice failed",
					    PRIVATE_DATA->camera, context);
			goto cleanup;
		}

		char label[96] = {0};

		strncpy(label, widget_choice, sizeof(label) - 1);
		if (!strcmp(property->name, DSLR_SHUTTER_PROPERTY_NAME)) {

			double shutter_d;
			shutter_d = parse_shutterspeed(widget_choice);
			if (shutter_d > 0.0)
				snprintf(label, sizeof(label), "%f", shutter_d);
		}

		/* Init and set value same as on camera. */
		indigo_init_switch_item(property->items + i,
					widget_choice,
					label,
					val && !strcmp(val, widget_choice));
		i++;
	}

	if (key == EOS_SHUTTERSPEED && PRIVATE_DATA->vendor == SONY) {
		/* Ugly hack to fill missing sony shutterspeed values */
		INDIGO_DRIVER_LOG(DRIVER_NAME, "Ugly sony hack loaded", val);
		int j = 0;
		const char *this_choice;

		while (j < sony_shutterspeeds_count) {

			char label[16] = {0};
			this_choice = sony_shutterspeeds[j];

			strncpy(label, this_choice, sizeof(label) - 1);
			double shutter_d;
			INDIGO_DRIVER_LOG(DRIVER_NAME, "[%d] Adding shutter speed %s", i, this_choice);
			shutter_d = parse_shutterspeed(this_choice);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "[%d] Shutter speed is %f", i, shutter_d);
			if (shutter_d > 0.0)
				snprintf(label, sizeof(label), "%f", shutter_d);
			INDIGO_DRIVER_LOG(DRIVER_NAME, "[%d] Label is %s", i, label);

			indigo_init_switch_item(property->items + i,
				this_choice,
				label,
				val && !strcmp(val, this_choice));
			j++;
		}
	}

cleanup:
	gp_widget_free(widget);

	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);

	return rc;
}

static int gphoto2_set_key_val(const char *key, const void *val,
			       CameraWidgetType widget_type,
			       indigo_device *device)
{
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int rc;

	pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);

	rc = gp_camera_get_config(PRIVATE_DATA->camera, &widget,
				  context);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] camera get config failed",
				    PRIVATE_DATA->camera, context);
		goto cleanup;
	}
	rc = lookup_widget(widget, key, &child);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	rc = gp_widget_get_type(child, &type);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget get type failed",
				    PRIVATE_DATA->camera, context);
		goto cleanup;
	}
	switch (type) {
	case GP_WIDGET_MENU:
	case GP_WIDGET_RADIO:
	case GP_WIDGET_TEXT:
	case GP_WIDGET_TOGGLE:
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget has bad type",
				    PRIVATE_DATA->camera, context);
		rc = GP_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	/* This is the actual set call. Note that we keep
	 * ownership of the string and have to free it if necessary.
	 */
	rc = gp_widget_set_value(child, val);
	if (rc < GP_OK) {
		goto cleanup;
	}
	/* This stores it on the camera again */
	rc = gp_camera_set_config(PRIVATE_DATA->camera, widget,
				  context);
	if (rc < GP_OK)
		goto cleanup;

cleanup:
	gp_widget_free(widget);

	pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);

	return rc;
}

static int gphoto2_set_key_val_char(const char *key, const char *val,
				    indigo_device *device)
{
	/* char* := {GP_WIDGET_TEXT, GP_WIDGET_RADIO, GP_WIDGET_MENU}. */
	return gphoto2_set_key_val(key, val, GP_WIDGET_TEXT, device);
}

static int gphoto2_set_key_val_int(const char *key, const int val,
				   indigo_device *device)
{
	/* int := {GP_WIDGET_TOGGLE, GP_WIDGET_DATE}. */
	return gphoto2_set_key_val(key, &val, GP_WIDGET_TOGGLE, device);
}

static int gphoto2_set_key_val_float(const char *key, const float val,
				     indigo_device *device)
{
	/* float := {GP_WIDGET_RANGE}. */
	return gphoto2_set_key_val(key, &val, GP_WIDGET_RANGE, device);
}

static int gphoto2_get_key_val(const char *key, char **str,
			       indigo_device *device)
{
	CameraWidget *widget = NULL, *child = NULL;
	CameraWidgetType type;
	int rc;
	char *val;

	rc = gp_camera_get_config(PRIVATE_DATA->camera, &widget,
				  context);
	if (rc < GP_OK) {
		return rc;
	}
	rc = lookup_widget(widget, key, &child);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	rc = gp_widget_get_type(child, &type);
	if (rc < GP_OK) {
		goto cleanup;
	}
	switch (type) {
	case GP_WIDGET_MENU:
	case GP_WIDGET_RADIO:
	case GP_WIDGET_TEXT:
		break;
	default:
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[camera:%p,context:%p] widget has bad type",
				    PRIVATE_DATA->camera, context);
		rc = GP_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	/* This is the actual query call. Note that we just
	 * a pointer reference to the string, not a copy... */
	rc = gp_widget_get_value(child, &val);
	if (rc < GP_OK) {
		goto cleanup;
	}

	/* Create a new copy for our caller. */
	*str = strdup(val);

cleanup:
	gp_widget_free (widget);

	return rc;
}

static void ctx_error_func(GPContext *context, const char *str, void *data)
{
	UNUSED(context);
	UNUSED(data);

	INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", str);
}

static void ctx_status_func(GPContext *context, const char *str, void *data)
{
	UNUSED(context);
	UNUSED(data);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "%s", str);
}

static int eos_mirror_lockup(const float secs, indigo_device *device)
{
	return gphoto2_set_key_val_char(EOS_CUSTOMFUNCEX, secs > 0 ?
				   EOS_MIRROR_LOCKUP_ENABLE :
				   EOS_MIRROR_LOCKUP_DISABLE,
				   device);
}

static bool can_preview(indigo_device *device)
{
	int rc;

	rc = gphoto2_set_key_val_int(EOS_VIEWFINDER, 0, device);

	return (rc == GP_OK);
}

static void counter_timer_callback(indigo_device *device)
{
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;

	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		if (CCD_EXPOSURE_ITEM->number.value - TIMER_COUNTER_STEP_SEC > 0) {
			CCD_EXPOSURE_ITEM->number.value -= TIMER_COUNTER_STEP_SEC;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			indigo_reschedule_timer(device,
						TIMER_COUNTER_STEP_SEC,
						&PRIVATE_DATA->counter_timer);
		} else
			CCD_EXPOSURE_ITEM->number.value = 0;
	}
}

static void exposure_timer_callback(indigo_device *device)
{
	void *retval;

	PRIVATE_DATA->counter_timer = NULL;
	PRIVATE_DATA->exposure_timer = NULL;

	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;

	if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
		int rc;

		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		rc = pthread_join(thread_id_capture, &retval);
		if (rc) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%d] pthread_join", rc);
			return;
		} else {
			if (retval == PTHREAD_CANCELED) {
				INDIGO_DRIVER_LOG(DRIVER_NAME,
						  "capture thread was cancelled");
				return;
			} else
				INDIGO_DRIVER_LOG(DRIVER_NAME,
						  "capture thread terminated normally");
		}
		if (!PRIVATE_DATA->buffer || PRIVATE_DATA->buffer_size == 0) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY,
					       "exposure failed");
			return;

		}
		if (CCD_IMAGE_FORMAT_RAW_ITEM->sw.value ||
		    CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value)
			indigo_process_dslr_image(device,
						  PRIVATE_DATA->buffer,
						  PRIVATE_DATA->buffer_size,
						  PRIVATE_DATA->filename_suffix);
		if (CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) {
			rc = process_dslr_image_debayer(device,
							PRIVATE_DATA->buffer,
							PRIVATE_DATA->buffer_size);
			if (rc) {
				CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, CCD_EXPOSURE_PROPERTY,
						       "debayer failed");
				return;
			}
		}
		CCD_EXPOSURE_PROPERTY->state = INDIGO_OK_STATE;
		CCD_EXPOSURE_ITEM->number.value = 0;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
	} else {
		CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY,
				       "exposure failed");
	}
}

static void streaming_timer_callback(indigo_device *device)
{
	if (!CONNECTION_CONNECTED_ITEM->sw.value)
		return;

	int rc = 0;
	CameraFile *camera_file = NULL;
	char *buffer = NULL;
	unsigned long int buffer_size = 0;

	INDIGO_DRIVER_LOG(DRIVER_NAME, "streaming timer callback started");

	rc = gp_file_new(&camera_file);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p] gp_file_new",
				    rc,
				    PRIVATE_DATA->camera,
				    context);
		goto cleanup;
	}

	rc = gp_file_set_mime_type(camera_file, GP_MIME_JPEG);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p] gp_file_set_mime_type",
				    rc,
				    PRIVATE_DATA->camera,
				    context);
		goto cleanup;
	}

	rc = gphoto2_set_key_val_int(EOS_VIEWFINDER, 1, device);
	if (rc < GP_OK)
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p] "
				    "gphoto2_set_key_val_int '%s' '%d'",
				    rc,
				    PRIVATE_DATA->camera,
				    context,
				    EOS_VIEWFINDER,
				    1);

	while (CCD_STREAMING_COUNT_ITEM->number.value != 0) {

		pthread_mutex_lock(&PRIVATE_DATA->driver_mutex);

		rc = gp_camera_capture_preview(PRIVATE_DATA->camera,
					       camera_file,
					       context);
		if (rc < GP_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "[rc:%d,camera:%p,context:%p] "
					    "gp_camera_capture_preview",
					    rc,
					    PRIVATE_DATA->camera,
					    context);
			pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
			goto cleanup;
		}

		/* Memory of buffer free'd by gp_file_unref(). */
		rc = gp_file_get_data_and_size(camera_file,
					       (const char**)&buffer,
					       &buffer_size);
		if (rc < GP_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%d,camera:%p,context:%p] "
					    "gp_file_get_data_and_size",
					    rc,
					    PRIVATE_DATA->camera,
					    context);
			pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);
			goto cleanup;
		}

		if (buffer_size > PRIVATE_DATA->buffer_size_max) {
			PRIVATE_DATA->buffer_size_max = buffer_size;
			PRIVATE_DATA->buffer = realloc(PRIVATE_DATA->buffer,
						       PRIVATE_DATA->
						       buffer_size_max);
		}
		memcpy(PRIVATE_DATA->buffer, buffer, buffer_size);
		PRIVATE_DATA->buffer_size = buffer_size;

		pthread_mutex_unlock(&PRIVATE_DATA->driver_mutex);

		*CCD_IMAGE_ITEM->blob.url = 0;
		CCD_IMAGE_ITEM->blob.value = PRIVATE_DATA->buffer;
		CCD_IMAGE_ITEM->blob.size = PRIVATE_DATA->buffer_size;
		strncpy(CCD_IMAGE_ITEM->blob.format, ".jpeg", INDIGO_NAME_SIZE);
		CCD_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);

		if (CCD_STREAMING_COUNT_ITEM->number.value > 0)
			CCD_STREAMING_COUNT_ITEM->number.value -= 1;

		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
	}

	rc = gphoto2_set_key_val_int(EOS_VIEWFINDER, 0, device);
	if (rc < GP_OK)
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p] "
				    "gphoto2_set_key_val_int '%s' '%d'",
				    rc,
				    PRIVATE_DATA->camera,
				    context,
				    EOS_VIEWFINDER,
				    0);

cleanup:
	if (camera_file)
		gp_file_unref(camera_file);

	CCD_STREAMING_PROPERTY->state = rc ? INDIGO_ALERT_STATE : INDIGO_OK_STATE;
	indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);
}

static void thread_capture_abort(void *user_data)
{
	int rc;
	struct capture_abort *capture_abort;
	indigo_device *device;

	capture_abort = (struct capture_abort *)user_data;
	device = capture_abort->device;

	rc = gphoto2_set_key_val_char(EOS_REMOTE_RELEASE, EOS_RELEASE_FULL,
				 device);
	if (rc < GP_OK)
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p] "
				    "gphoto2_set_key_val_char '%s' '%s'",
				    rc,
				    PRIVATE_DATA->camera,
				    context,
				    EOS_REMOTE_RELEASE,
				    EOS_RELEASE_FULL);

	if (capture_abort->camera_file)
		gp_file_unref(capture_abort->camera_file);

	INDIGO_DRIVER_LOG(DRIVER_NAME, "capture thread aborted");
}

static void *thread_capture(void *user_data)
{
	int rc;
	CameraFile *camera_file = NULL;
	CameraFilePath camera_file_path;
	indigo_device *device;
	struct capture_abort capture_abort;
	struct timespec tp;
	char *buffer = NULL;
	unsigned long int buffer_size = 0;

	device = (indigo_device *)user_data;
	INDIGO_DRIVER_LOG(DRIVER_NAME, "capture thread started");

	rc = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (rc) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%d] pthread_setcancelstate");
		rc = -EINVAL;
		goto cleanup;
	}

	/* Store images on memory card. */
	rc = gphoto2_set_key_val_char(EOS_CAPTURE_TARGET, EOS_MEMORY_CARD, device);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%d,camera:%p,context:%p] "
				    "gphoto2_set_key_val_char '%s' '%s'",
				    rc,
				    PRIVATE_DATA->camera,
				    context,
				    EOS_CAPTURE_TARGET,
				    EOS_MEMORY_CARD);
		goto cleanup;
	}

	/* This sets also camera_file->accesstype = GP_FILE_ACCESSTYPE_MEMORY. */
	rc = gp_file_new(&camera_file);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p] gp_file_new",
				    rc,
				    PRIVATE_DATA->camera,
				    context);
		goto cleanup;
	}

	capture_abort.camera_file = camera_file;
	capture_abort.device = device;
	pthread_cleanup_push(thread_capture_abort, &capture_abort);

	if (PRIVATE_DATA->mirror_lockup_secs) {
		rc = gphoto2_set_key_val_char(EOS_REMOTE_RELEASE, EOS_PRESS_FULL,
					      device);
		if (rc < GP_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "[rc:%d,camera:%p,context:%p] "
					    "gphoto2_set_key_val_char '%s' '%s'",
					    rc,
					    PRIVATE_DATA->camera,
					    context,
					    EOS_REMOTE_RELEASE,
					    EOS_PRESS_FULL);
			goto cleanup;
		}

		rc = gphoto2_set_key_val_char(EOS_REMOTE_RELEASE, EOS_RELEASE_FULL,
					      device);
		if (rc < GP_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "[rc:%d,camera:%p,context:%p] "
					    "gphoto2_set_key_val_char '%s' '%s'",
					    rc,
					    PRIVATE_DATA->camera,
					    context,
					    EOS_REMOTE_RELEASE,
					    EOS_RELEASE_FULL);
			goto cleanup;
		}

		sleep(PRIVATE_DATA->mirror_lockup_secs);
	}

	PRIVATE_DATA->counter_timer =
		indigo_set_timer(device, TIMER_COUNTER_STEP_SEC,
				 counter_timer_callback);

	PRIVATE_DATA->exposure_timer =
		indigo_set_timer(device,
				 CCD_EXPOSURE_ITEM->number.target,
				 exposure_timer_callback);

	if (PRIVATE_DATA->has_eos_remote_release) {
		rc = gphoto2_set_key_val_char(EOS_REMOTE_RELEASE,
					      EOS_PRESS_FULL,
					      device);
		if (rc < GP_OK) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "[rc:%d,camera:%p,context:%p] "
					    "gphoto2_set_key_val_char '%s' '%s'",
					    rc,
					    PRIVATE_DATA->camera,
					    context,
					    EOS_REMOTE_RELEASE,
					    EOS_PRESS_FULL);
			goto cleanup;
		}
	}

	/* Bulb capture. */
	if (PRIVATE_DATA->shutterspeed_bulb)
		while (CCD_EXPOSURE_ITEM->number.value)
			usleep(TIMER_COUNTER_STEP_SEC * 1000000UL);

	/* Function will release the shutter. */
	rc = gp_camera_capture(PRIVATE_DATA->camera, GP_CAPTURE_IMAGE,
			       &camera_file_path,
			       context);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p]"
				    " gp_camera_capture",
				    rc,
				    PRIVATE_DATA->camera,
				    context);
		goto cleanup;
	}

	rc = gp_camera_file_get(PRIVATE_DATA->camera, camera_file_path.folder,
				camera_file_path.name, GP_FILE_TYPE_NORMAL,
				camera_file, context);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "[rc:%d,camera:%p,context:%p]"
				    " gp_camera_file_get",
				    rc,
				    PRIVATE_DATA->camera,
				    context);
		goto cleanup;
	}

	/* Memory of buffer free'd by this function when previously allocated. */
	rc = gp_file_get_data_and_size(camera_file, (const char**)&buffer,
				       &buffer_size);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%d,camera:%p,context:%p] "
				    "gp_file_get_data_and_size",
				    rc,
				    PRIVATE_DATA->camera,
				    context);
		goto cleanup;
	}

	/* If new image is larger than current buffer, then
	   increase buffer size. Otherwise it will fit in the old buffer. */
	if (buffer_size > PRIVATE_DATA->buffer_size_max) {
		PRIVATE_DATA->buffer_size_max = buffer_size;
		PRIVATE_DATA->buffer = realloc(PRIVATE_DATA->buffer,
					       PRIVATE_DATA->
					       buffer_size_max);
	}
	memcpy(PRIVATE_DATA->buffer, buffer, buffer_size);
	PRIVATE_DATA->buffer_size = buffer_size;

	char *suffix;

	memset(PRIVATE_DATA->filename_suffix, 0,
	       sizeof(PRIVATE_DATA->filename_suffix));
	suffix = strstr(camera_file_path.name, ".");
	if (suffix)
		strncpy(PRIVATE_DATA->filename_suffix, suffix,
			sizeof(PRIVATE_DATA->filename_suffix));

	if (PRIVATE_DATA->delete_downloaded_image) {
		rc = gp_camera_file_delete(PRIVATE_DATA->camera,
					   camera_file_path.folder,
					   camera_file_path.name,
					   context);
		if (rc < GP_OK)
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "[rc:%d,camera:%p,context:%p] "
					    "gp_camera_file_delete",
					    rc,
					    PRIVATE_DATA->camera,
					    context);
	}

cleanup:
	/* If GP_FILE_ACCESSTYPE_MEMORY (which it is), then gp_file_unref
	   free's buffer and set buffer_size = 0, when reference counter == 0. */
	if (camera_file)
		gp_file_unref(camera_file);

	pthread_cleanup_pop(0);

	return NULL;
}

static void update_property(indigo_device *device, indigo_property *property,
			    const char *widget)
{
	assert(device != NULL);
	assert(property != NULL);
	assert(widget != NULL);

	int rc;

	for (int p = 0; p < property->count; p++) {
		if (property->items[p].sw.value) {
			rc = gphoto2_set_key_val_char(widget,
						      property->items[p].name,
						      device);
			if (rc == GP_OK) {
				property->state = INDIGO_OK_STATE;
				indigo_update_property(device, property, NULL);

				/* Bulb check and set/unset. */
				if (!strncmp(property->name,
					     DSLR_SHUTTER_PROPERTY_NAME,
					     INDIGO_NAME_SIZE))
					if (property->items[p].name[0] == 'b' ||
					    property->items[p].name[0] == 'B')
						PRIVATE_DATA->shutterspeed_bulb = true;
					else
						PRIVATE_DATA->shutterspeed_bulb = false;
			}
		}
	}
}

static void update_ccd_property(indigo_device *device,
				indigo_property *property)
{
	for (int p = 0; p < property->count; p++)
		if (property->items[p].sw.value) {
			const double value = parse_shutterspeed(
				property->items[p].name);
			CCD_EXPOSURE_ITEM->number.target = value;

			if (!PRIVATE_DATA->shutterspeed_bulb)
				CCD_EXPOSURE_ITEM->number.value = value;

			indigo_update_property(device, CCD_EXPOSURE_PROPERTY,
					       NULL);
		}
}

static void shutterspeed_closest(indigo_device *device)
{
	assert(device != NULL);

	if (!CCD_EXPOSURE_ITEM || !DSLR_SHUTTER_PROPERTY || DSLR_SHUTTER_PROPERTY->count == 0)
		return;

	const double val = CCD_EXPOSURE_ITEM->number.value;

	if (val < 0)
		return;
	if (val == 0) {
		CCD_EXPOSURE_ITEM->number.target =
			CCD_EXPOSURE_ITEM->number.value =
			CCD_EXPOSURE_ITEM->number.min;
	} else {
		double number_shutter;
		double number_closest = 3600;
		int pos_new = 0;
		int pos_old;
		for (int i = 0; i < DSLR_SHUTTER_PROPERTY->count; i++) {
			if (DSLR_SHUTTER_PROPERTY->items[i].sw.value) {
				/* If shutter is on bulb */
				if (DSLR_SHUTTER_PROPERTY->items[i].name[0] == 'b' ||
				    DSLR_SHUTTER_PROPERTY->items[i].name[0] == 'B') {
					CCD_EXPOSURE_ITEM->number.target =
						CCD_EXPOSURE_ITEM->number.value;
					return;
				}
				pos_old = i;
			}
			number_shutter = parse_shutterspeed(
				DSLR_SHUTTER_PROPERTY->items[i].name);
			double abs_diff = fabs(CCD_EXPOSURE_ITEM->number.value
					       - number_shutter);

			if (abs_diff < number_closest) {
				number_closest = abs_diff;
				pos_new = i;
			}
		}

	        DSLR_SHUTTER_PROPERTY->items[pos_old].sw.value = false;
		DSLR_SHUTTER_PROPERTY->items[pos_new].sw.value = true;

		number_shutter = parse_shutterspeed(
			DSLR_SHUTTER_PROPERTY->items[pos_new].name);

		CCD_EXPOSURE_ITEM->number.target =
			CCD_EXPOSURE_ITEM->number.value = number_shutter;
	}
}

static indigo_result ccd_attach(indigo_device *device)
{
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);

	if (indigo_ccd_attach(device, DRIVER_VERSION) == INDIGO_OK) {

		/*------------------- DEFAULT INITIALIZATION -----------------*/
		PRIVATE_DATA->buffer = NULL;
		PRIVATE_DATA->buffer_size = 0;
		PRIVATE_DATA->buffer_size_max = 0;
		PRIVATE_DATA->name_best_jpeg_format = NULL;
		PRIVATE_DATA->name_pure_raw_format = NULL;
		PRIVATE_DATA->mirror_lockup_secs = 0;

		/*--------------------- CCD_INFO --------------------*/
		char *name = PRIVATE_DATA->gphoto2_id.name;
		CCD_INFO_PROPERTY->hidden = true;
		for (int i = 0; name[i]; i++)
                        name[i] = toupper(name[i]);
		for (int i = 0; dslr_model_info[i].name; i++) {
                        if (strstr(name, dslr_model_info[i].name)) {
				CCD_INFO_WIDTH_ITEM->number.value = dslr_model_info[i].width;
				CCD_INFO_HEIGHT_ITEM->number.value = dslr_model_info[i].height;
				CCD_INFO_PIXEL_SIZE_ITEM->number.value = dslr_model_info[i].pixel_size;
				CCD_INFO_BITS_PER_PIXEL_ITEM->number.value = 16;
				CCD_INFO_PROPERTY->hidden = false;
			}
			if (strstr(name, "CANON"))
				PRIVATE_DATA->vendor = CANON;
			else if (strstr(name, "NIKON"))
				PRIVATE_DATA->vendor = NIKON;
			else if (strstr(name, "SONY"))
				PRIVATE_DATA->vendor = SONY;
			else
				PRIVATE_DATA->vendor = OTHER;
		}

		/*----------------------- IDENTIFY-VENDIR --------------------*/
		vendor_identify_widget(device, DSLR_COMPRESSION_PROPERTY_NAME);

		/*-------------------- HAS-SINGLE-BULB-MODE ------------------*/
		PRIVATE_DATA->has_single_bulb_mode =
			exists_widget_label(EOS_BULB_MODE_LABEL,
					    device) == 0;

		/*------------------- HAS-EOS-REMOTE-RELEASE -----------------*/
		PRIVATE_DATA->has_eos_remote_release =
			exists_widget_label(EOS_REMOTE_RELEASE_LABEL,
					    device) == 0;

		/*------------------------- SHUTTER-TIME -----------------------*/
		int count = enumerate_widget(EOS_SHUTTERSPEED, device, NULL);
		DSLR_SHUTTER_PROPERTY = indigo_init_switch_property(NULL,
						device->name,
						DSLR_SHUTTER_PROPERTY_NAME,
						GPHOTO2_NAME_DSLR,
						GPHOTO2_NAME_SHUTTER,
						INDIGO_OK_STATE,
						INDIGO_RW_PERM,
						INDIGO_ONE_OF_MANY_RULE,
						count);
		enumerate_widget(EOS_SHUTTERSPEED, device, DSLR_SHUTTER_PROPERTY);

		/*---------------------------- ISO -----------------------------*/
		count = enumerate_widget(EOS_ISO, device, NULL);
		DSLR_ISO_PROPERTY = indigo_init_switch_property(NULL,
								device->name,
								DSLR_ISO_PROPERTY_NAME,
								GPHOTO2_NAME_DSLR,
								GPHOTO2_NAME_ISO,
								INDIGO_OK_STATE,
								INDIGO_RW_PERM,
								INDIGO_ONE_OF_MANY_RULE,
								count);
		enumerate_widget(EOS_ISO, device, DSLR_ISO_PROPERTY);

		/*------------------------ COMPRESSION -------------------------*/
		count = enumerate_widget(COMPRESSION, device, NULL);
		DSLR_COMPRESSION_PROPERTY = indigo_init_switch_property(NULL,
									device->name,
									DSLR_COMPRESSION_PROPERTY_NAME,
									GPHOTO2_NAME_DSLR,
									GPHOTO2_NAME_COMPRESSION,
									INDIGO_OK_STATE,
									INDIGO_RW_PERM,
									INDIGO_ONE_OF_MANY_RULE,
									count);
		enumerate_widget(COMPRESSION, device, DSLR_COMPRESSION_PROPERTY);

		/*------------------------ WHITEBALANCE ----------------------*/
		count = enumerate_widget(EOS_WHITEBALANCE, device, NULL);
		DSLR_WHITEBALANCE_PROPERTY = indigo_init_switch_property(NULL,
									 device->name,
									 DSLR_WHITE_BALANCE_PROPERTY_NAME,
									 GPHOTO2_NAME_DSLR,
									 GPHOTO2_NAME_WHITEBALANCE,
									 INDIGO_OK_STATE,
									 INDIGO_RW_PERM,
									 INDIGO_ONE_OF_MANY_RULE,
									 count);
		enumerate_widget(EOS_WHITEBALANCE, device, DSLR_WHITEBALANCE_PROPERTY);

		/*----------------------- ZOOM-PREVIEW -----------------------*/
		DSLR_ZOOM_PREVIEW_PROPERTY = indigo_init_switch_property(NULL,
									 device->name,
									 DSLR_ZOOM_PREVIEW_PROPERTY_NAME,
									 GPHOTO2_NAME_DSLR,
									 GPHOTO2_NAME_ZOOM_PREVIEW,
									 INDIGO_OK_STATE,
									 INDIGO_RW_PERM,
									 INDIGO_ONE_OF_MANY_RULE,
									 2);
		indigo_init_switch_item(DSLR_ZOOM_PREVIEW_ON_ITEM,
					GPHOTO2_NAME_ZOOM_PREVIEW_ON_ITEM,
					GPHOTO2_NAME_ZOOM_PREVIEW_ON,
					false);
		indigo_init_switch_item(DSLR_ZOOM_PREVIEW_OFF_ITEM,
					GPHOTO2_NAME_ZOOM_PREVIEW_OFF_ITEM,
					GPHOTO2_NAME_ZOOM_PREVIEW_OFF,
					false);
		if (!can_preview(device))
			DSLR_ZOOM_PREVIEW_PROPERTY->hidden = true;

		/*------------------------ MIRROR-LOCKUP -----------------------*/
		DSLR_MIRROR_LOCKUP_PROPERTY = indigo_init_number_property(NULL,
									  device->name,
									  DSLR_MIRROR_LOCKUP_PROPERTY_NAME,
									  GPHOTO2_NAME_DSLR,
									  GPHOTO2_NAME_MIRROR_LOCKUP,
									  INDIGO_OK_STATE,
									  INDIGO_RW_PERM,
									  1);

		indigo_init_number_item(DSLR_MIRROR_LOCKUP_ITEM,
					GPHOTO2_NAME_MIRROR_LOCKUP_ITEM_NAME,
					"Seconds", 0, 30, 0.1, 0);

		int rc = eos_mirror_lockup(0, device);
		if (rc) {
			INDIGO_DRIVER_LOG(DRIVER_NAME, "mirror lockup not available for camera '%s'",
					  PRIVATE_DATA->gphoto2_id.name);
			DSLR_MIRROR_LOCKUP_PROPERTY->hidden = true;
		}

		/*------------------------ DELETE-IMAGE -----------------------*/
		DSLR_DELETE_IMAGE_PROPERTY = indigo_init_switch_property(NULL,
									 device->name,
									 DSLR_DELETE_IMAGE_PROPERTY_NAME,
									 GPHOTO2_NAME_DSLR,
									 GPHOTO2_NAME_DELETE_IMAGE,
									 INDIGO_OK_STATE,
									 INDIGO_RW_PERM,
									 INDIGO_ONE_OF_MANY_RULE,
									 2);
		indigo_init_switch_item(DSLR_DELETE_IMAGE_ON_ITEM,
					GPHOTO2_NAME_DELETE_IMAGE_ON_ITEM,
					GPHOTO2_NAME_DELETE_IMAGE_ON,
					false);
		indigo_init_switch_item(DSLR_DELETE_IMAGE_OFF_ITEM,
					GPHOTO2_NAME_DELETE_IMAGE_OFF_ITEM,
					GPHOTO2_NAME_DELETE_IMAGE_OFF,
					true);

		/*--------------------- DEBAYER-ALGORITHM --------------------*/
		DSLR_DEBAYER_ALGORITHM_PROPERTY = indigo_init_switch_property(NULL,
									      device->name,
									      GPHOTO2_DEBAYER_ALGORITHM_PROPERTY_NAME,
									      GPHOTO2_NAME_DSLR,
									      GPHOTO2_NAME_DEBAYER_ALGORITHM,
									      INDIGO_OK_STATE,
									      INDIGO_RW_PERM,
									      INDIGO_ONE_OF_MANY_RULE,
									      6);
		indigo_init_switch_item(DSLR_DEBAYER_ALGORITHM_LIN_ITEM,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_LIN_NAME,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_LIN_LABEL,
					false);
		indigo_init_switch_item(DSLR_DEBAYER_ALGORITHM_VNG_ITEM,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_VNG_NAME,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_VNG_LABEL,
					true);
		indigo_init_switch_item(DSLR_DEBAYER_ALGORITHM_PPG_ITEM,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_PPG_NAME,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_PPG_LABEL,
					false);
		indigo_init_switch_item(DSLR_DEBAYER_ALGORITHM_AHD_ITEM,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_AHD_NAME,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_AHD_LABEL,
					false);
		indigo_init_switch_item(DSLR_DEBAYER_ALGORITHM_DCB_ITEM,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_DCB_NAME,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_DCB_LABEL,
					false);
		indigo_init_switch_item(DSLR_DEBAYER_ALGORITHM_DHT_ITEM,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_DHT_NAME,
					GPHOTO2_NAME_DEBAYER_ALGORITHM_DHT_LABEL,
					false);
		PRIVATE_DATA->debayer_algorithm =
			debayer_algorithm_value_id(GPHOTO2_NAME_DEBAYER_ALGORITHM_VNG_NAME);


		/*--------------------- LIBGPHOTO2-VERSION --------------------*/
		GPHOTO2_LIBGPHOTO2_VERSION_PROPERTY = indigo_init_text_property(NULL,
										device->name,
										GPHOTO2_LIBGPHOTO2_VERSION_PROPERTY_NAME,
										GPHOTO2_NAME_DSLR,
										GPHOTO2_NAME_LIBGPHOTO2,
										INDIGO_OK_STATE,
										INDIGO_RO_PERM,
										1);
		indigo_init_text_item(GPHOTO2_LIBGPHOTO2_VERSION_ITEM,
				      GPHOTO2_LIBGPHOTO2_VERSION_ITEM_NAME,
				      GPHOTO2_NAME_LIBGPHOTO2_VERSION,
				      "%s",
				      PRIVATE_DATA->libgphoto2_version);

		/*----------------------- SANITY CHECK -----------------------*/
		if (!DSLR_SHUTTER_PROPERTY) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s (%s) cannot be initializated",
					    "DSLR_SHUTTER_PROPERTY", GPHOTO2_NAME_SHUTTER);
			assert(DSLR_SHUTTER_PROPERTY != NULL);
		}

		/*--------------------- CCD_EXPOSURE_ITEM --------------------*/
		double number_min = 3600;
		double number_max = -number_min;
		for (int i = 0; i < DSLR_SHUTTER_PROPERTY->count; i++) {
			/* Skip {B,b}ulb widget. */
			if (DSLR_SHUTTER_PROPERTY->items[i].name[0] == 'b' ||
			    DSLR_SHUTTER_PROPERTY->items[i].name[0] == 'B')
				continue;

			double number_shutter = parse_shutterspeed(
				DSLR_SHUTTER_PROPERTY->items[i].name);

			number_min = MIN(number_shutter, number_min);
			number_max = MAX(number_shutter, number_max);
		}
		CCD_EXPOSURE_ITEM->number.min = number_min;
		/* We could exposure actually until the universe collapses. */
#ifdef UNIVERSE_COLLAPSES
		CCD_EXPOSURE_ITEM->number.max = number_max;
#endif

		/*----------------------- CCD-STREAMING ----------------------*/
		if (can_preview(device)) {
			CCD_MODE_PROPERTY->count = 1;
			CCD_STREAMING_PROPERTY->hidden = false;
		}

		/*----------------- BEST-JPEG-QUALITY-PURE-RAW ---------------*/
		for (int i = 0; i < DSLR_COMPRESSION_PROPERTY->count; i++) {
			if (!PRIVATE_DATA->name_pure_raw_format && (
				    STRNCMP(DSLR_COMPRESSION_PROPERTY->items[i].name, "RAW") ||
				    STRNCMP(DSLR_COMPRESSION_PROPERTY->items[i].name, "NEF (Raw)") ||
				    STRNCMP(DSLR_COMPRESSION_PROPERTY->items[i].name, "NEF+Basic"))) {
				PRIVATE_DATA->name_pure_raw_format = strdup(DSLR_COMPRESSION_PROPERTY->items[i].name);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "CCD_IMAGE_FORMAT_PROPERTY FITS/RAW uses compression format '%s'",
						  PRIVATE_DATA->name_pure_raw_format);
			} else if (!PRIVATE_DATA->name_best_jpeg_format && (
					 STRNCMP(DSLR_COMPRESSION_PROPERTY->items[i].name, "Large Fine JPEG") ||
					 STRNCMP(DSLR_COMPRESSION_PROPERTY->items[i].name, "JPEG Fine") ||
					 STRNCMP(DSLR_COMPRESSION_PROPERTY->items[i].name, "Extra Fine"))) {
				PRIVATE_DATA->name_best_jpeg_format = strdup(DSLR_COMPRESSION_PROPERTY->items[i].name);
				INDIGO_DRIVER_LOG(DRIVER_NAME, "CCD_IMAGE_FORMAT_PROPERTY JPEG uses compression format '%s'",
						  PRIVATE_DATA->name_best_jpeg_format);
			}
		}

		/*---------------------- DEFAULT SETTINGS --------------------*/
		if (PRIVATE_DATA->name_pure_raw_format) {
			/* Default compression format is RAW. */
			indigo_set_switch(DSLR_COMPRESSION_PROPERTY,
					  indigo_get_item(DSLR_COMPRESSION_PROPERTY,
							  PRIVATE_DATA->name_pure_raw_format),
					  true);
			update_property(device, DSLR_COMPRESSION_PROPERTY, COMPRESSION);
			/* Default image format is RAW. */
			indigo_set_switch(CCD_IMAGE_FORMAT_PROPERTY,
					  CCD_IMAGE_FORMAT_RAW_ITEM, true);
		}

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_ccd_enumerate_properties(device, NULL, NULL);
	}

	return INDIGO_FAILED;
}

static indigo_result ccd_detach(indigo_device *device)
{
	assert(device != NULL);

	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);

	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);

	indigo_release_property(DSLR_SHUTTER_PROPERTY);
	indigo_release_property(DSLR_ISO_PROPERTY);
	indigo_release_property(DSLR_COMPRESSION_PROPERTY);
	indigo_release_property(DSLR_WHITEBALANCE_PROPERTY);
	indigo_release_property(DSLR_ZOOM_PREVIEW_PROPERTY);
	indigo_release_property(DSLR_MIRROR_LOCKUP_PROPERTY);
	indigo_release_property(DSLR_DELETE_IMAGE_PROPERTY);
	indigo_release_property(DSLR_DEBAYER_ALGORITHM_PROPERTY);
	indigo_release_property(GPHOTO2_LIBGPHOTO2_VERSION_PROPERTY);

	if (COMPRESSION)
		free(COMPRESSION);

	if (PRIVATE_DATA->buffer) {
		free(PRIVATE_DATA->buffer);
		PRIVATE_DATA->buffer = NULL;
		PRIVATE_DATA->buffer_size = 0;
		PRIVATE_DATA->buffer_size_max = 0;
	}

	if (PRIVATE_DATA->name_pure_raw_format)
		free(PRIVATE_DATA->name_pure_raw_format);

	if (PRIVATE_DATA->name_best_jpeg_format)
		free(PRIVATE_DATA->name_best_jpeg_format);

	return indigo_ccd_detach(device);
}

static indigo_result ccd_enumerate_properties(indigo_device *device,
					      indigo_client *client,
					      indigo_property *property)
{
	return indigo_ccd_enumerate_properties(device, client, property);
}

static indigo_result ccd_change_property(indigo_device *device,
					 indigo_client *client,
					 indigo_property *property)
{
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);

	/*------------------------ CONNECTION --------------------------*/
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
		indigo_property_copy_values(CONNECTION_PROPERTY, property,
					    false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			device->is_connected = !!(PRIVATE_DATA->camera && context);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;

			indigo_define_property(device, DSLR_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, DSLR_ISO_PROPERTY, NULL);
			indigo_define_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
			indigo_define_property(device, DSLR_WHITEBALANCE_PROPERTY, NULL);
			indigo_define_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
			indigo_define_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
			indigo_define_property(device, DSLR_DELETE_IMAGE_PROPERTY, NULL);
			indigo_define_property(device, DSLR_DEBAYER_ALGORITHM_PROPERTY, NULL);
			indigo_define_property(device, GPHOTO2_LIBGPHOTO2_VERSION_PROPERTY, NULL);
		} else {
			if (device->is_connected) {
				indigo_delete_property(device, DSLR_SHUTTER_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_ISO_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_COMPRESSION_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_WHITEBALANCE_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_DELETE_IMAGE_PROPERTY, NULL);
				indigo_delete_property(device, DSLR_DEBAYER_ALGORITHM_PROPERTY, NULL);
				indigo_delete_property(device, GPHOTO2_LIBGPHOTO2_VERSION_PROPERTY, NULL);
				device->is_connected = false;
			}
		}
	}
	/*-------------------------- SHUTTER-TIME ----------------------------*/
	else if (indigo_property_match(DSLR_SHUTTER_PROPERTY, property)) {
			indigo_property_copy_values(DSLR_SHUTTER_PROPERTY, property, false);
			update_property(device, DSLR_SHUTTER_PROPERTY, EOS_SHUTTERSPEED);
			update_ccd_property(device, DSLR_SHUTTER_PROPERTY);
			return INDIGO_OK;
	}
	/*------------------------------ ISO ---------------------------------*/
	else if (indigo_property_match(DSLR_ISO_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_ISO_PROPERTY, property, false);
		update_property(device, DSLR_ISO_PROPERTY, EOS_ISO);
		return INDIGO_OK;
	}
	/*--------------------------- COMPRESSION ----------------------------*/
	else if (indigo_property_match(DSLR_COMPRESSION_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_COMPRESSION_PROPERTY, property, false);
		update_property(device, DSLR_COMPRESSION_PROPERTY,
				COMPRESSION);
		return INDIGO_OK;
	}
	/*--------------------------- WHITEBALANCE ----------------------------*/
	else if (indigo_property_match(DSLR_WHITEBALANCE_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_WHITEBALANCE_PROPERTY, property, false);
		update_property(device, DSLR_WHITEBALANCE_PROPERTY, EOS_WHITEBALANCE);
		return INDIGO_OK;
	}
	/*--------------------------- ZOOM-PREVIEW ---------------------------*/
	else if (indigo_property_match(DSLR_ZOOM_PREVIEW_PROPERTY, property) &&
		 CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE) {
		indigo_property_copy_values(DSLR_ZOOM_PREVIEW_PROPERTY, property, false);
		update_property(device, DSLR_ZOOM_PREVIEW_PROPERTY, EOS_ZOOM_PREVIEW);
		return INDIGO_OK;
	}
	/*-------------------------- MIRROR-LOCKUP ---------------------------*/
	else if (indigo_property_match(DSLR_MIRROR_LOCKUP_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_MIRROR_LOCKUP_PROPERTY, property, false);
		PRIVATE_DATA->mirror_lockup_secs = DSLR_MIRROR_LOCKUP_ITEM->number.target;

		int rc = eos_mirror_lockup(PRIVATE_DATA->mirror_lockup_secs, device);
		if (rc == GP_OK)
			DSLR_MIRROR_LOCKUP_PROPERTY->state = INDIGO_OK_STATE;
		else {
			PRIVATE_DATA->mirror_lockup_secs = DSLR_MIRROR_LOCKUP_ITEM->number.target =
				DSLR_MIRROR_LOCKUP_ITEM->number.value = 0;
			DSLR_MIRROR_LOCKUP_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		indigo_update_property(device, DSLR_MIRROR_LOCKUP_PROPERTY, NULL);
		return INDIGO_OK;
	}
	/*--------------------------- DELETE-IMAGE ---------------------------*/
	else if (indigo_property_match(DSLR_DELETE_IMAGE_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_DELETE_IMAGE_PROPERTY, property, false);
		PRIVATE_DATA->delete_downloaded_image = DSLR_DELETE_IMAGE_ON_ITEM->sw.value;
		DSLR_DELETE_IMAGE_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, DSLR_DELETE_IMAGE_PROPERTY, NULL);

		return INDIGO_OK;
	}
	/*------------------------- DEBAYER-ALGORITHM ------------------------*/
	else if (indigo_property_match(DSLR_DEBAYER_ALGORITHM_PROPERTY, property)) {
		indigo_property_copy_values(DSLR_DEBAYER_ALGORITHM_PROPERTY, property, false);
		DSLR_DEBAYER_ALGORITHM_PROPERTY->state = INDIGO_ALERT_STATE;
		for (int i = 0; i < DSLR_DEBAYER_ALGORITHM_PROPERTY->count; i++) {
			if (DSLR_DEBAYER_ALGORITHM_PROPERTY->items[i].sw.value) {
				PRIVATE_DATA->debayer_algorithm =
					debayer_algorithm_value_id(DSLR_DEBAYER_ALGORITHM_PROPERTY->items[i].name);
				if (PRIVATE_DATA->debayer_algorithm >= 0)
					DSLR_DEBAYER_ALGORITHM_PROPERTY->state = INDIGO_OK_STATE;
			}
		}
		indigo_update_property(device, DSLR_DEBAYER_ALGORITHM_PROPERTY, NULL);

		return INDIGO_OK;
	}
	/*------------------------- CCD-IMAGE-FORMAT -------------------------*/
	else if (indigo_property_match(CCD_IMAGE_FORMAT_PROPERTY, property)) {
		indigo_property_copy_values(CCD_IMAGE_FORMAT_PROPERTY, property,
					    false);

		/* RAW or FITS format, set to image quality pure RAW. */
		if ((CCD_IMAGE_FORMAT_RAW_ITEM->sw.value || CCD_IMAGE_FORMAT_FITS_ITEM->sw.value) && PRIVATE_DATA->name_pure_raw_format) {

			indigo_set_switch(DSLR_COMPRESSION_PROPERTY,
					  indigo_get_item(DSLR_COMPRESSION_PROPERTY,
							  PRIVATE_DATA->name_pure_raw_format),
					  true);
			update_property(device, DSLR_COMPRESSION_PROPERTY, COMPRESSION);
			CCD_IMAGE_FORMAT_PROPERTY->state = INDIGO_OK_STATE;
		} else if (CCD_IMAGE_FORMAT_JPEG_ITEM->sw.value && PRIVATE_DATA->name_best_jpeg_format) {
			indigo_set_switch(DSLR_COMPRESSION_PROPERTY,
					  indigo_get_item(DSLR_COMPRESSION_PROPERTY,
							  PRIVATE_DATA->name_best_jpeg_format),
					  true);
			update_property(device, DSLR_COMPRESSION_PROPERTY, COMPRESSION);
			CCD_IMAGE_FORMAT_PROPERTY->state = INDIGO_OK_STATE;

		} else {
			INDIGO_DRIVER_LOG(DRIVER_NAME,
					  "cannot set proper compression format for image format");
			CCD_IMAGE_FORMAT_PROPERTY->state = INDIGO_ALERT_STATE;
		}

		indigo_update_property(device, CCD_IMAGE_FORMAT_PROPERTY, NULL);
		return INDIGO_OK;
	}
	/*--------------------------- CCD-EXPOSURE ---------------------------*/
	else if (indigo_property_match(CCD_EXPOSURE_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE ||
		    CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;

		indigo_property_copy_values(CCD_EXPOSURE_PROPERTY, property, false);
		/* Find non-bulb shutterspeed closest to client value. */
		indigo_use_shortest_exposure_if_bias(device);
		shutterspeed_closest(device);
		update_property(device, DSLR_SHUTTER_PROPERTY, EOS_SHUTTERSPEED);

		CCD_EXPOSURE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
		CCD_IMAGE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);

		int rc = pthread_create(&thread_id_capture, NULL,
					thread_capture, device);
		if (rc) {
			CCD_EXPOSURE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_EXPOSURE_PROPERTY, NULL);
			CCD_IMAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, CCD_IMAGE_PROPERTY, NULL);
			return INDIGO_FAILED;
		}

		CCD_ABORT_EXPOSURE_PROPERTY->state =
			INDIGO_OK_STATE;
		indigo_update_property(device,
				       CCD_ABORT_EXPOSURE_PROPERTY, NULL);


		return INDIGO_OK;
	}
	/*------------------------ CCD-ABORT-EXPOSURE ------------------------*/
	else if (indigo_property_match(CCD_ABORT_EXPOSURE_PROPERTY, property)) {
		indigo_property_copy_values(CCD_ABORT_EXPOSURE_PROPERTY, property, false);
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE) {
			/* Only bulb captures can be abort. */
			if (PRIVATE_DATA->shutterspeed_bulb) {
				indigo_cancel_timer(device,
						    &PRIVATE_DATA->exposure_timer);
				pthread_cancel(thread_id_capture);
			} else {
				CCD_ABORT_EXPOSURE_PROPERTY->state =
					INDIGO_ALERT_STATE;
				indigo_update_property(device,
						       CCD_ABORT_EXPOSURE_PROPERTY, NULL);
				return INDIGO_FAILED;
			}
		} else if (CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE &&
			   CCD_STREAMING_COUNT_ITEM->number.value != 0) {
			CCD_STREAMING_COUNT_ITEM->number.value = 0;

			/* Streaming stops, set preview zoom (on/off) to false. */
			DSLR_ZOOM_PREVIEW_ON_ITEM->sw.value = false;
			DSLR_ZOOM_PREVIEW_OFF_ITEM->sw.value = false;
			indigo_update_property(device,
					       DSLR_ZOOM_PREVIEW_PROPERTY, NULL);
		}
	}
	/*---------------------------- CCD-STREAMING -------------------------*/
	else if (indigo_property_match(CCD_STREAMING_PROPERTY, property)) {
		if (CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE ||
		    CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE)
			return INDIGO_OK;

		indigo_property_copy_values(CCD_STREAMING_PROPERTY, property, false);
		CCD_STREAMING_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CCD_STREAMING_PROPERTY, NULL);

		CCD_EXPOSURE_ITEM->number.value = CCD_STREAMING_EXPOSURE_ITEM->number.value;
		indigo_use_shortest_exposure_if_bias(device);
		shutterspeed_closest(device);
		update_property(device, DSLR_SHUTTER_PROPERTY, EOS_SHUTTERSPEED);

		PRIVATE_DATA->exposure_timer = indigo_set_timer(device, 0,
								streaming_timer_callback);
		return INDIGO_OK;
	}
	/*--------------------------------- CONFIG ---------------------------*/
	else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL,
					     CCD_IMAGE_FORMAT_PROPERTY);
			indigo_save_property(device, NULL,
					     DSLR_ISO_PROPERTY);
			indigo_save_property(device, NULL,
					     DSLR_SHUTTER_PROPERTY);
			indigo_save_property(device, NULL,
					     DSLR_COMPRESSION_PROPERTY);
			indigo_save_property(device, NULL,
					     DSLR_WHITEBALANCE_PROPERTY);
			indigo_save_property(device, NULL,
					     DSLR_MIRROR_LOCKUP_PROPERTY);
			indigo_save_property(device, NULL,
					     DSLR_DELETE_IMAGE_PROPERTY);
			indigo_save_property(device, NULL,
					     DSLR_DEBAYER_ALGORITHM_PROPERTY);
		}
	}

	return indigo_ccd_change_property(device, client, property);
}

static int find_available_device_slot(void)
{
	for (uint8_t slot = 0; slot < MAX_DEVICES; slot++)
		if (!devices[slot])
			return slot;

	return -1;
}

static int find_device_slot(const struct gphoto2_id_s *gphoto2_id)
{
	for (uint8_t slot = 0; slot < MAX_DEVICES; slot++) {
		indigo_device *device = devices[slot];

		if (device &&
			(gphoto2_id->vendor == PRIVATE_DATA->gphoto2_id.vendor) &&
			(gphoto2_id->product == PRIVATE_DATA->gphoto2_id.product) &&
			(gphoto2_id->bus == PRIVATE_DATA->gphoto2_id.bus) &&
			(gphoto2_id->port == PRIVATE_DATA->gphoto2_id.port))
			return slot;
	}

	return -1;
}

static int device_connect(indigo_device *gphoto2_template,
			  const struct gphoto2_id_s *gphoto2_id, const int slot)
{
	int rc;
	gphoto2_private_data *private_data;
	indigo_device *device = NULL;

	private_data = calloc(1, sizeof(gphoto2_private_data));
	if (!private_data) {
		rc = -errno;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", strerror(errno));
		goto cleanup;
	}

	/* All devices share the same context. */
	if (!context) {
		context = gp_context_new();
		gp_context_set_error_func(context,
					  ctx_error_func, NULL);
		gp_context_set_message_func(context,
					    ctx_status_func, NULL);
	}

	/* Allocate memory for camera. */
	rc = gp_camera_new(&private_data->camera);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%d] gp_camera_new failed", rc);
		goto cleanup;
	}

	/* Initiate a connection to the camera. */
	rc = gp_camera_init(private_data->camera, context);
	if (rc < GP_OK) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%d] gp_camera_init failed", rc);
		goto cleanup;
	}

	memcpy(&private_data->gphoto2_id, gphoto2_id, sizeof(struct gphoto2_id_s));
	strncpy(private_data->libgphoto2_version, *gp_library_version(GP_VERSION_SHORT),
		sizeof(private_data->libgphoto2_version));

	device = calloc(1, sizeof(indigo_device));
	if (!device) {
		rc = -errno;
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "%s", strerror(errno));
		goto cleanup;
	}

	indigo_device *master_device = device;
	memcpy(device, gphoto2_template, sizeof(indigo_device));
	device->master_device = master_device;

	sprintf(device->name, "%s", gphoto2_id->name);
	device->private_data = private_data;

	indigo_async((void *)(void *)indigo_attach_device, device);
	devices[slot] = device;
	INDIGO_DRIVER_LOG(DRIVER_NAME, "attach device '%s' in slot '%d'",
		gphoto2_id->name_extended, slot);

	return rc;

cleanup:
	if (private_data) {
		free(private_data);
		private_data = NULL;
	}
	if (device) {
		free(device);
		device = NULL;
	}

	return rc;
}

static int device_attach(indigo_device *gphoto2_template,
			 struct gphoto2_id_s *gphoto2_id)
{
	int rc;
	int slot;
	CameraList *list = NULL;
	const char *name = NULL;
	const char *value = NULL;
	char value_libusb[12] = {0};

	snprintf(value_libusb, sizeof(value_libusb), "usb:%03d,%03d",
		 gphoto2_id->bus, gphoto2_id->port);

	rc = gp_list_new(&list);
	if (rc < GP_OK)
		goto out;

	rc = gp_camera_autodetect(list, context);
	if (rc < GP_OK)
		goto out;

	for (int c = 0; c < gp_list_count(list); c++) {
		gp_list_get_name(list, c, &name);
		gp_list_get_value(list, c, &value);

		if (strcmp(value, value_libusb))
			continue;

		strncpy(gphoto2_id->name, name, sizeof(gphoto2_id->name));
		snprintf(gphoto2_id->name_extended, sizeof(gphoto2_id->name_extended),
			 "%s %04x:%04x (bus %03d, device %03d)",
			 name, gphoto2_id->vendor, gphoto2_id->product,
			 gphoto2_id->bus, gphoto2_id->port);

		INDIGO_DRIVER_LOG(DRIVER_NAME, "auto-detected device '%s'",
				  gphoto2_id->name_extended);

		/* Device already exists in devices[...]. */
		if (find_device_slot(gphoto2_id) >= 0)
			continue;

		/* Find free slot to attach device to. */
		slot = find_available_device_slot();
		if (slot < 0) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME,
					    "no free slot for plugged "
					    "device '%s' found", gphoto2_id->name_extended);
			goto out;

		}
		rc = device_connect(gphoto2_template, gphoto2_id, slot);
		if (rc)
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "connecting failed slot '%d' device '%s'",
					    slot, gphoto2_id->name_extended);
	}

out:
	if (list)
		gp_list_free(list);

	return rc;
}

static int device_detach(const struct gphoto2_id_s *gphoto2_id)
{
	int rc = -1, slot;
	gphoto2_private_data *private_data = NULL;

	slot = find_device_slot(gphoto2_id);
	if (slot < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "no slot found for device %04x:%04x "
				    "(bus %03d, device %03d) to be detached",
				    gphoto2_id->vendor, gphoto2_id->product,
				    gphoto2_id->bus, gphoto2_id->port);
		return slot;
	}

	indigo_device **device = &devices[slot];

	if (*device && (*device)->private_data) {
		rc = indigo_detach_device(*device);
		if (rc) {
			INDIGO_DRIVER_ERROR(DRIVER_NAME, "[rc:%] failed detaching device: '%s'",
					    rc, private_data->gphoto2_id.name_extended);
			return -rc;
		}

		private_data = (*device)->private_data;
		INDIGO_DRIVER_LOG(DRIVER_NAME, "detach device '%s' from slot '%d'",
				  private_data->gphoto2_id.name_extended, slot);

		gp_camera_exit(private_data->camera, context);
		gp_camera_unref(private_data->camera);
		private_data->camera = NULL;
		free(private_data);
		private_data = NULL;
		free(*device);
		*device = NULL;
		rc = 0;
	}

	return rc;
}

static int hotplug_callback(libusb_context *ctx, libusb_device *dev,
			    libusb_hotplug_event event, void *user_data) {

	UNUSED(ctx);

	int rc;
	struct libusb_device_descriptor descriptor;
	struct gphoto2_id_s gphoto2_id;
	indigo_device *gphoto2_template = (indigo_device *)user_data;

	memset(&gphoto2_id, 0, sizeof(struct gphoto2_id_s));
	memset(&descriptor, 0, sizeof(struct libusb_device_descriptor));

	rc = libusb_get_device_descriptor(dev, &descriptor);
	if (LIBUSB_SUCCESS != rc) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME,
				    "error getting device descriptor: %d", rc);
		return rc;
	}

	gphoto2_id.bus = libusb_get_bus_number(dev);
	gphoto2_id.port = libusb_get_device_address(dev);
	gphoto2_id.vendor = descriptor.idVendor;
	gphoto2_id.product = descriptor.idProduct;

	switch (event) {
		case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {
			device_attach(gphoto2_template, &gphoto2_id);
			break;
		}
		case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {
			device_detach(&gphoto2_id);
			break;
		}
	}

	return 0;
}

indigo_result indigo_ccd_gphoto2(indigo_driver_action action, indigo_driver_info *info)
{
	static indigo_device gphoto2_template = INDIGO_DEVICE_INITIALIZER(
		"gphoto2",
		ccd_attach,
		ccd_enumerate_properties,
		ccd_change_property,
		NULL,
		ccd_detach
	);

	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;

	SET_DRIVER_INFO(info, "Gphoto2 Camera", __FUNCTION__, DRIVER_VERSION, true, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
	case INDIGO_DRIVER_INIT:
		last_action = action;
		indigo_start_usb_event_handler();
		int rc = libusb_hotplug_register_callback(NULL,
							  LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
							  LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
							  LIBUSB_HOTPLUG_ENUMERATE,
							  LIBUSB_HOTPLUG_MATCH_ANY,
							  LIBUSB_HOTPLUG_MATCH_ANY,
							  LIBUSB_HOTPLUG_MATCH_ANY,
							  hotplug_callback,
							  &gphoto2_template,
							  &callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME,
				    "libusb_hotplug_register_callback ->  %s",
				    rc < 0 ? libusb_error_name(rc) : "OK");
		return rc >= 0 ? INDIGO_OK : INDIGO_FAILED;

	case INDIGO_DRIVER_SHUTDOWN:
		last_action = action;
		libusb_hotplug_deregister_callback(NULL, callback_handle);
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "libusb_hotplug_deregister_callback");
		gp_context_unref(context);
		break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}
