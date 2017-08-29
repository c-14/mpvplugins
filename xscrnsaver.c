// Build with: gcc -o xscrnsaver.so xscrnsaver.c `pkg-config --cflags mpv` -shared -fPIC `pkg-config --libs --cflags xscrnsaver`
// Warning: do not link against libmpv.so! Read:
//    https://mpv.io/manual/master/#linkage-to-libmpv
// The pkg-config call is for adding the proper client.h include path.

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#include <mpv/client.h>

static const int PAUSE_EVENT = 1;

static int xss_suspend(Display *mDisplay, Bool suspend)
{
    int event, error, major, minor;
    if (XScreenSaverQueryExtension(mDisplay, &event, &error) != True ||
        XScreenSaverQueryVersion(mDisplay, &major, &minor) != True)
        return 0;
    if (major < 1 || (major == 1 && minor < 1))
        return 0;
    XScreenSaverSuspend(mDisplay, suspend);
    return 1;
}


int mpv_open_cplugin(mpv_handle *handle)
{
	struct mpv_event_property *p;
	Display *d = XOpenDisplay(NULL);
	Bool disabled = False;

	if (d == NULL)
		return -1;

	mpv_observe_property(handle, PAUSE_EVENT, "pause", MPV_FORMAT_FLAG);
	while (1) {
		mpv_event *event = mpv_wait_event(handle, -1);
		switch (event->event_id) {
			case MPV_EVENT_SHUTDOWN:
				if (xss_suspend(d, !disabled))
					disabled = !disabled;
				XCloseDisplay(d);
				return 0;
			case MPV_EVENT_PROPERTY_CHANGE:
				p = event->data;
				if (event->reply_userdata != PAUSE_EVENT)
					return -1;
				if (xss_suspend(d, !disabled))
					disabled = !disabled;
				break;
		}
	}
	return 0;
}
