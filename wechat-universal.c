/*
 * wechat-universal - wrapper for WeChat Universal
 * Copyright (C) 2024 Guoxin "7Ji" Pu <pugokushin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * This wrapper tries to enclose wechat bwrap opening and closing operations to
 * both provide a much user-friendly experience and limit the exposed data.

 * It is a busybox-like multi-call program that provides the following applets:
 * - start, to start wechat-universal, after trying to activate the existing
 * wechat window by emulating the "activate" opearation as if user has clicked
 * the wechat tray icon with their left button. This is based on the fact that
 * there could never be multiple wechat sessions running anyway as Tencent does
 * not allow it. This should be much beneficial to Gnome users without 3rdparty
 * libappindicator.
 * - stop, to stop wechat-universal via D-Bus, by emulating the "exit" operation
 * as if user has clicked "quit" in wechat's own right button notifier menu
 *
 * Unsurprisingly this links to libsystemd for sd-bus functionality, but that's
 * pretty much all it needs. While I could in theory also implement my own 
 * rootless mapping routines to drop bwrap, I decide to keep it for the sake
 * of the package name.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>
#include <linux/limits.h>
#include <systemd/sd-bus-protocol.h>
#include <systemd/sd-bus.h>

#define APPLET_START "start"
#define LEN_APPLET_START sizeof(APPLET_START) - 1
#define APPLET_STOP "stop"
#define LEN_APPLET_STOP sizeof(APPLET_STOP) - 1
#define LEN_APPLET_MAX LEN_APPLET_START
#define DBUS_NAME_DBUS "org.freedesktop.DBus"
#define DBUS_PATH_DBUS "/org/freedesktop/DBus"
#define DBUS_METHOD_LIST_NAMES "ListNames"
#define DBUS_INTERFACE_DBUS DBUS_NAME_DBUS
#define DBUS_PATH_NOTIFIER "/StatusNotifierItem"
#define DBUS_INTERFACE_PROPERTIES "org.freedesktop.DBus.Properties"
#define DBUS_METHOD_GET "Get"
#define DBUS_ARG_ITEM "org.kde.StatusNotifierItem"
#define DBUS_ARG_ID "Id"
#define ID_WECHAT "wechat"
#define LEN_ID_WECHAT sizeof(ID_WECHAT) - 1
#define DBUS_PREFIX_NOTIFIER "org.kde.StatusNotifierItem-"
#define LEN_DBUS_PREFIX_NOTIFIER sizeof(DBUS_PREFIX_NOTIFIER) - 1
#define DBUS_PATH_MENUBAR "/MenuBar"
#define DBUS_INTERFACE_MENU "com.canonical.dbusmenu"
#define DBUS_METHOD_EVENT "Event"
#define DBUS_MENU_ID_QUIT 1
#define DBUS_EVENT_TYPE_CLICKED "clicked"
#define DBUS_EVENT_DATA "Quit WeCaht"
#define DBUS_TIMESTAMP 0

#define println(format, arg...) printf(format"\n", ##arg)
#define println_with_prefix(prefix, format, arg...) \
    println("["prefix"] "format, ##arg)
#define println_with_prefix_and_source(prefix, format, arg...) \
    println_with_prefix(prefix, "%s:%d: "format, __FUNCTION__, __LINE__, ##arg)

#define println_info(format, arg...) println_with_prefix("INFO", format, ##arg)
#define println_warn(format, arg...) println_with_prefix("WARN", format, ##arg)
#define println_error(format, arg...) \
    println_with_prefix_and_source("ERROR", format, ##arg)
#define println_error_with_raw_error(format, error, arg...) \
    println_error(format", errno: %d, error: %s", ##arg, error, strerror(error))
#define println_error_with_dbus(format, error, arg...) \
    println_error_with_raw_error(format, -error, ##arg)
#define println_error_with_errno(format, arg...) \
    println_error_with_raw_error(format, errno, ##arg)

#define declare_types(name, arg...) char const TYPES_##name[] = {arg, '\0'};

declare_types(STRING_STRING, SD_BUS_TYPE_STRING, SD_BUS_TYPE_STRING);
declare_types(VARIANT_STRING, SD_BUS_TYPE_VARIANT, SD_BUS_TYPE_STRING);
declare_types(VARIANT, SD_BUS_TYPE_VARIANT);
declare_types(STRING, SD_BUS_TYPE_STRING);
declare_types(INT32_STRING_VARIANT_UINT32, 
    SD_BUS_TYPE_INT32, SD_BUS_TYPE_STRING, SD_BUS_TYPE_VARIANT, \
    SD_BUS_TYPE_UINT32);

enum applet {
    AppletInvalid,
    AppletStart,
    AppletStop,
};

char const *get_env_or_default(
    char const *const restrict name, 
    char const *const restrict value_default
) {
    char const *env = getenv(name);
    if (!env) {
        env = value_default;
    }
    return env;
}

sd_bus *get_bus() {
    sd_bus *bus = NULL;
    int r;

    r = sd_bus_default_user(&bus);
    if (r < 0) {
        println_error_with_dbus("Failed to get user SD-Bus", r);
        return NULL;
    }
    if (!bus) {
        println_error("Failed to get user SD-Bus, unknown error");
    }
    return bus;
}

bool is_wechat(
    sd_bus *const restrict bus,
    char const *const restrict notifier
) {
    sd_bus_message *reply = NULL;
    int r;
    bool is_or_not = false;
    char const *id = NULL;

    r = sd_bus_call_method(bus, notifier, DBUS_PATH_NOTIFIER, 
        DBUS_INTERFACE_PROPERTIES, DBUS_METHOD_GET, 
        NULL, &reply, TYPES_STRING_STRING, 
        DBUS_ARG_ITEM, DBUS_ARG_ID);
    if (r < 0) {
        println_error_with_dbus("Failed to call D-Bus Get method", r);
        return false;
    }
    r = sd_bus_message_read(reply, TYPES_VARIANT, TYPES_STRING, &id);
    if (r < 0) {
        println_error_with_dbus("Failed to read D-Bus message", r);
        goto free_reply;
    }
    if (id && !strncmp(id, ID_WECHAT, LEN_ID_WECHAT)) {
        is_or_not = true;
    }
free_reply:
    sd_bus_message_unref(reply);
    return is_or_not;
}

char *get_notifier(
    sd_bus *const restrict bus
) {
    sd_bus_message *reply = NULL;
    char const *name = NULL;
    char *notifier = NULL;
    int r;
    bool reading = true;

    r = sd_bus_call_method(
        bus, DBUS_NAME_DBUS, DBUS_PATH_DBUS, 
        DBUS_INTERFACE_DBUS, DBUS_METHOD_LIST_NAMES, 
        NULL, &reply, NULL);
    if (r < 0) {
        println_error_with_dbus("Failed to list D-Bus dest names", r);
        return NULL;
    }
    r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, 
        TYPES_STRING);
    if (r != 1) {
        println_error_with_dbus("Failed to enter D-Bus array of names", r);
        goto free_reply;
    }
    while (reading) {
        /*
         We use `sd_bus_message_read_basic` instead of `sd_bus_message_skip`,
         as `_skip` calls `_read_basic` anyway, we just inline it to save some
         function jumps
         */
        r = sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, 
            notifier ? NULL : &name);
        if (r == 0) {
            reading = false;
        } else if (r < 0) {
            println_error_with_dbus("Failed to read D-Bus message of name", r);
            goto free_reply;
        }
        if (notifier) continue;
        if (name && name[0] && !strncmp(
            name, DBUS_PREFIX_NOTIFIER, LEN_DBUS_PREFIX_NOTIFIER) &&
            is_wechat(bus, name)
        ) {
            println_info("Found wechat notifier item '%s'", name);
            notifier = strndup(name, NAME_MAX);
            if (!notifier) {
                println_error_with_errno(
                    "Failed to duplicate notifier name");
            }
        }
        name = NULL;
    }
    r = sd_bus_message_exit_container(reply);
    if (r != 1) {
        println_error_with_dbus("Failed to close D-Bus array of names ", r);
        if (notifier) {
            free(notifier);
            notifier = NULL;
        }
    }
free_reply:
    sd_bus_message_unref(reply);
    return notifier;
}

int stop_notifier(
    sd_bus *const restrict bus,
    char const *const restrict notifier
) {
    int r;
    r = sd_bus_call_method(
        bus, notifier, DBUS_PATH_MENUBAR, 
        DBUS_INTERFACE_MENU, DBUS_METHOD_EVENT, 
        NULL, NULL, 
        TYPES_INT32_STRING_VARIANT_UINT32,
        DBUS_MENU_ID_QUIT, // the id of the item which received the event
        DBUS_EVENT_TYPE_CLICKED, // the type of event
        TYPES_STRING, // variant type: string,
        DBUS_EVENT_DATA, // event-specific data
        DBUS_TIMESTAMP // The time that the event occured if available or
                       //  the time the message was sent if not
    );
    if (r < 0) {
        println_error_with_dbus("Failed to call D-Bus ListNames method", r);
        return -1;
    }
    return 0;
}

int applet_stop() {
    sd_bus *bus;
    char *notifier;
    int r = -1;

    bus = get_bus();
    if (!bus) {
        return -1;
    }
    notifier = get_notifier(bus);
    if (!notifier) {
        println_warn("Couldn't find notifier item of WeChat, either it was not "
            "started or our D-Bus queries failed");
        goto free_bus;
    }
    r = stop_notifier(bus, notifier);
free_notifier:
    free(notifier);
free_bus:
    sd_bus_flush_close_unref(bus);
    return r;
}

int applet_start(
    int argc, 
    char *argv[]
) {
    // int i;
    // for (i = 0; i < argc; ++i) {
    //     if (!strncmp(argv[i], "--help", 7)) {
    //         return 0;
    //     }
    // }
    // int c, option_index = 0;
    // struct option const long_options[] = {
    //     {"data",            required_argument,  NULL,   'd'},
    //     {"bind",            required_argument,  NULL,   'b'},
    //     {"binds-config",    required_argument,  NULL,   'B'},
    //     {"ime",             required_argument,  NULL,   'i'},
    //     {"help",            no_argument,        NULL,   'h'},
    //     {NULL,              no_argument,        NULL,     0},
    // };
    // while ((c = getopt_long(argc, argv, "d:b:B:i:h", 
    //     long_options, &option_index)) != -1) 
    // {
    //     switch (c) {
    //     case 'd':
    //         interval = strtoul(optarg, NULL, 10);
    //         break;
    //     case 'b':   // version
    //         show_version();
    //         return 0;
    //     case 'B':
    //     case 'i':
    //         show_help(argv[0]);
    //         return 0;
    //     default:
    //         println_error("Unknown option '%s'", argv[optind - 1]);
    //         return -1;
    //     }
    // }
    // (void) bus;
    // (void) argc;
    // (void) argv;
    // DBusConnection *dbus_connection = get_dbus_connection();
    // if (!dbus_connection) return -1;
    return 0;
}

enum applet applet_from_arg0(
    char const *const restrict arg0
) {
    size_t len_applet;
    char const *applet;

    applet = strrchr(arg0, '/');
    if (applet) {
        ++applet;
    } else {
        applet = arg0;
    }

    len_applet = strnlen(applet, LEN_APPLET_START);
    switch (len_applet) {
        case LEN_APPLET_STOP:
            if (!memcmp(applet, APPLET_STOP, LEN_APPLET_STOP))
                return AppletStop;
            break;
        case LEN_APPLET_START:
            if (!memcmp(applet, APPLET_START, LEN_APPLET_START))
                return AppletStart;
            break;
        default:
            break;
    }
    println_error("Unknown applet '%s', allowed: '%s', '%s'", applet, 
                    APPLET_START, APPLET_STOP);
    return AppletInvalid;
}

int main(int argc, char *argv[]) {
    if (argc < 1) {
        println_error("Arguments too few, couldn't get even applet");
        return -1;
    }
    switch (applet_from_arg0(argv[0])) {
    case AppletStart:
        return applet_start(argc, argv);
    case AppletStop:
        return applet_stop();
    default:
        return -1;
    }
}