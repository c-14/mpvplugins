// Build with: gcc -o mvi.so mvi.c `pkg-config --cflags mpv` -shared -fPIC -lm
// Warning: do not link against libmpv.so! Read:
//    https://mpv.io/manual/master/#linkage-to-libmpv
// The pkg-config call is for adding the proper client.h include path.

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mpv/client.h>

// Allow changing a property with by zoom-adjusted amount
void zoom_invariant_add(mpv_handle *handle, const char *prop, double amount)
{
	double zoom, prop_value;
	int ret;

	ret = mpv_get_property(handle, "video-zoom", MPV_FORMAT_DOUBLE, &zoom);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	amount = amount / pow(2, zoom);

	ret = mpv_get_property(handle, prop, MPV_FORMAT_DOUBLE, &prop_value);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	prop_value += amount;
	ret = mpv_set_property(handle, prop, MPV_FORMAT_DOUBLE, &prop_value);
	if (ret != MPV_ERROR_SUCCESS)
		fprintf(stderr, "%s\n", mpv_error_string(ret));
}

// Resets the pan if the entire image would be visible
void zoom_check_center(mpv_handle *handle)
{
	double zoom, rot;
	double dw, dh;
	int scaled, ret;

	ret = mpv_get_property(handle, "video-zoom", MPV_FORMAT_DOUBLE, &zoom);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	ret = mpv_get_property(handle, "video-rotate", MPV_FORMAT_DOUBLE, &rot);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	rot = rot * M_PI / 180;
	ret = mpv_get_property(handle, "video-unscaled", MPV_FORMAT_FLAG, &scaled);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	scaled = !scaled;

	ret = mpv_get_property(handle, "dwidth", MPV_FORMAT_DOUBLE, &dw);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	dw = dw * pow(2, zoom);
	ret = mpv_get_property(handle, "dheight", MPV_FORMAT_DOUBLE, &dh);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	dh = dh * pow(2, zoom);

	// Adjust for rotation
	double asr, acr;
	asr = fabs(sin(rot));
	acr = fabs(cos(rot));
	dw = dw * acr + dh * asr;
	dh = dh * acr + dw * asr;

	// No property seems to exist for the actual window size, try this instead
	double ow, oh;
	ret = mpv_get_property(handle, "osd-width", MPV_FORMAT_DOUBLE, &ow);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	ret = mpv_get_property(handle, "osd-height", MPV_FORMAT_DOUBLE, &oh);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}

	if ((dw <= ow && dh <= oh) || (scaled && zoom <= 0.0)) {
		double zero = 0.0;
		ret = mpv_set_property(handle, "video-pan-x", MPV_FORMAT_DOUBLE, &zero);
		if (ret != MPV_ERROR_SUCCESS) {
			fprintf(stderr, "%s\n", mpv_error_string(ret));
			return;
		}
		ret = mpv_set_property(handle, "video-pan-y", MPV_FORMAT_DOUBLE, &zero);
		if (ret != MPV_ERROR_SUCCESS) {
			fprintf(stderr, "%s\n", mpv_error_string(ret));
			return;
		}
	}
}

static inline int positive_modulo(int i, int n) {
	return (i % n + n) % n;
}

// Rotates the video while maintaining 0 <= prop < 360
void rotate_video(mpv_handle *handle, int amount)
{
	int64_t rot;
	int ret;

	ret = mpv_get_property(handle, "video-rotate", MPV_FORMAT_INT64, &rot);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
	printf("rot = %" PRId64 "....\n", rot);
	rot = positive_modulo((rot + amount), 360);
	printf("rot = %" PRId64 "....\n", rot);
	ret = mpv_set_property(handle, "video-rotate", MPV_FORMAT_INT64, &rot);
	if (ret != MPV_ERROR_SUCCESS) {
		fprintf(stderr, "%s\n", mpv_error_string(ret));
		return;
	}
}

#define assert(x) \
	if (!x) { \
		fputs("Failed to parse script-message", stderr); \
		return; \
	}

void handle_message(mpv_handle *handle, struct mpv_event_client_message *m)
{
	if (m->num_args < 1)
		return;

	if (strcmp(m->args[0], "zoom-invariant-add") == 0) {
		assert(m->num_args == 3);
		double amount = strtod(m->args[2], NULL);
		zoom_invariant_add(handle, m->args[1], amount);
	} else if (strcmp(m->args[0], "zoom-check-center") == 0) {
		assert(m->num_args == 1);
		zoom_check_center(handle);
	} else if (strcmp(m->args[0], "rotate-video") == 0) {
		assert(m->num_args == 2);
		double amount = strtoll(m->args[1], NULL, 10);
		rotate_video(handle, amount);
	}
}

int mpv_open_cplugin(mpv_handle *handle)
{
	struct mpv_event_client_message *m;
	while (1) {
		mpv_event *event = mpv_wait_event(handle, -1);
		switch (event->event_id) {
			case MPV_EVENT_SHUTDOWN:
				return 0;
			case MPV_EVENT_CLIENT_MESSAGE:
				m = event->data;
				handle_message(handle, m);
				break;
		}
	}
	return 0;
}
