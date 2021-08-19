/*
 * enable_service.m
 *
 * Responsibility: Paul Hegarty
 *
 * Enables/Disables a Services Menu item.
 * Doesn't matter what language you specify the name in.
 */

#import <appkit/Application.h>
#import <appkit/Listener.h>
#import <string.h>
#import <stdio.h>
#import <libc.h>
#import <sys/errno.h>

void main(int argc, char *argv[])
{
    char *name;
    int count, theError;
    BOOL enable = YES, alreadyEnabled;
    char buffer[1024];

    if (argc < 2) {
	fprintf(stderr, "Usage: enable_service name	enables Services Menu item\n"
			"   or: disable_service name	disables Services Menu item\n");
	exit(1);
    }
    name = strrchr(argv[0], '/');
    if (name) {
	*name++ = '\0';
    } else {
	name = argv[0];
    }
    if (!strcmp(name, "enable_service")) {
	enable = YES;
    } else if (!strcmp(name, "disable_service")) {
	enable = NO;
    } else {
	fprintf(stderr, "Usage: enable_service name	enables Services Menu item\n"
			"   or: disable_service name	disables Services Menu item\n");
	exit(1);
    }
    count = 1;
    buffer[0] = '\0';
    while (count < argc) {
	strcat(buffer, argv[count]);
	count++;
	if (count != argc) strcat(buffer, " ");
    }
    alreadyEnabled = NXIsServicesMenuItemEnabled(buffer);
    if (enable && alreadyEnabled) {
	printf("Item \"%s\" is already enabled.\n", buffer);
	exit(0);
    } else if (!enable && !alreadyEnabled) {
	printf("Item \"%s\" is not enabled.\n", buffer);
	exit(0);
    } else {
	if (theError = NXSetServicesMenuItemEnabled(buffer, enable)) {
	    fprintf(stderr, "enable/disable_service: couldn't %s \"%s\": %s\n",
			enable ? "enable" : "disable", buffer, strerror(theError));
	}
    }
}
