#include <getopt.h>
#include <libnotify/notify.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>

noreturn void usage(int argc, char** argv, int err);
void handle_selreqev(Display* display, XSelectionRequestEvent* selreqev, const char* string);

int main(int argc, char** argv) {
	int optch;
	char* display_name = NULL;
	while((optch = getopt(argc, argv, "d:h")) != -1) {
		switch(optch) {
		case 'd':
			display_name = optarg;
			break;
		case 'h':
			usage(argc, argv, EXIT_SUCCESS);
		case '?':
		default:
			usage(argc, argv, EXIT_FAILURE);
		}
	}

	if(optind == argc)
		usage(argc, argv, EXIT_FAILURE);

	size_t total_arg_length = 0;
	for(size_t i = optind; i < argc; i++)
		total_arg_length += 1 + strlen(argv[0]);

	char* string = malloc(total_arg_length);
	size_t j = 0;
	for(size_t i = optind; i < argc; i++) {
		size_t l = strlen(argv[i]);
		memcpy(&string[j], argv[i], l);
		j += l;
		string[j++] = ' ';
	}
	string[j] = '\0';

	Display *display = XOpenDisplay(display_name);
	if(!display) {
		fprintf(stderr, "Error: Can't open display: %s\n", display_name);
		return EXIT_FAILURE;
	}

	if(!notify_init("notifypaste")) {
		fprintf(stderr, "Error: Can't initialize libnotify\n");
		return EXIT_FAILURE;
	}

	int screen = DefaultScreen(display);
	Window root = RootWindow(display, screen);
	Window window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);
	XSelectInput(display, window, SelectionClear | SelectionRequest);
	Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
	XSetSelectionOwner(display, clipboard, window, CurrentTime);

	bool keep_going = true;
	while(keep_going) {
		XEvent ev;
		XSelectionRequestEvent* selreqev;
		XNextEvent(display, &ev);
		switch(ev.type) {
		case SelectionClear:
			keep_going = false;
			break;
		case SelectionRequest:
			selreqev = (XSelectionRequestEvent*) &ev.xselectionrequest;
			handle_selreqev(display, selreqev, string);
			break;
		}
	}

	notify_uninit();
	XCloseDisplay(display);
	free(string);
	return EXIT_SUCCESS;
}

noreturn void usage(int argc, char** argv, int err) {
	fprintf(err ? stderr : stdout, "Usage: %s [-h] [-d display] message\n", argc ? argv[0] : "notifypaste");
	exit(err);
}

void handle_selreqev(Display* display, XSelectionRequestEvent* selreqev, const char* string) {
	static int last_time = 0;

	XSelectionEvent selev;
	selev.property = None;
	selev.requestor = selreqev->requestor;
	selev.selection = selreqev->selection;
	selev.target = selreqev->target;
	selev.time = selreqev->time;
	selev.type = SelectionNotify;

	XSendEvent(display, selreqev->requestor, False, NoEventMask, (XEvent*) &selev);
	XFlush(display);

	int now = time(0);
	if(now == last_time)
		return;
	last_time = now;

	NotifyNotification* notification = notify_notification_new(string, NULL, NULL);
	notify_notification_show(notification, NULL);
}
