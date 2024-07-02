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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <unistd.h>
#include <pwd.h>
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
#define DBUS_INTERFACE_NOTIFIER "org.kde.StatusNotifierItem"
#define DBUS_ARG_ITEM DBUS_INTERFACE_NOTIFIER
#define DBUS_ARG_ID "Id"
#define ID_WECHAT "wechat"
#define LEN_ID_WECHAT sizeof(ID_WECHAT) - 1
#define DBUS_PREFIX_NOTIFIER  DBUS_INTERFACE_NOTIFIER "-"
#define LEN_DBUS_PREFIX_NOTIFIER sizeof(DBUS_PREFIX_NOTIFIER) - 1
#define DBUS_PATH_MENUBAR "/MenuBar"
#define DBUS_INTERFACE_MENU "com.canonical.dbusmenu"
#define DBUS_METHOD_EVENT "Event"
#define DBUS_MENU_ID_QUIT 1
#define DBUS_EVENT_TYPE_CLICKED "clicked"
#define DBUS_EVENT_DATA "Quit WeCaht"
#define DBUS_TIMESTAMP 0
#define DBUS_METHOD_ACTIVATE "Activate"

#define ENV_DATA    "WECHAT_DATA_DIR"
#define ENV_BINDS   "CUSTOM_BINDS"
#define ENV_BINDS_CONFIG "CUSTOM_BINDS_CONFIG"
#define ENV_IME     "IME_WORKAROUND"

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
declare_types(INT32_INT32, SD_BUS_TYPE_INT32, SD_BUS_TYPE_INT32);

enum applet {
    AppletInvalid,
    AppletStart,
    AppletStop,
};

enum ime {
    ImeNone,
    ImeAuto,
    ImeFcitx,
    ImeIbus
};

struct binds_list {
    char *buffer;
    size_t buffer_used, 
        buffer_allocated, 
        *offsets, 
        offsets_count, 
        offsets_allocated,
        len_home;
};

void help_start(
    char const *const restrict arg0
) {
    char *lang;
    bool cn;
     
    lang = getenv("LANG");
    cn = lang && !strncmp(lang, "zh_CN", 5);
    fputs(arg0, stdout);
    // These would be collapsed at runtime
    if (cn) {
        puts(""
            " (--data [微信数据文件夹])"
            " (--bind [自定义绑定挂载] (--bind ...)))"
            " (--ime [输入法])"
            " (--help)\n\n"
            "    --"
            "data [微信数据文件夹]\t"
                "微信数据文件夹的路径，绝对路径，或相对于用户HOME的相对路径。"
                "默认："
                    "~/Documents/Wechat_Data；"
                "环境变量："
                    ENV_DATA"\n"
            "    --"
            "bind [自定义绑定挂载]\t"
                "自定义的绑定挂载，可被声明多次，绝对路径，"
                "或相对于用户HOME的相对路径。"
                "环境变量："
                    ENV_BINDS" （用冒号:分隔，与PATH相似）\n"
            "    --"
            "binds-config [文件]\t"
                "以每行一个的方式列明应被绑定挂载的路径的纯文本配置文件，"
                "每行定义与--bind一致。"
                "默认："
                    "~/.config/wechat-universal/binds.list；"
                "环境变量："
                    ENV_BINDS_CONFIG"\n"
            "    --"
            "ime [输入法名称或特殊值]\t"
                "应用输入法对应环境变量修改，可支持："
                    "fcitx (不论是否为5), "
                    "ibus，"
                "特殊值："
                    "none不应用，"
                    "auto自动判断。"
                "默认："
                    "auto；"
                "环境变量："
                    ENV_IME"\n"
            "    --"
            "help\n");
    } else {
        puts(""
            " (--data [wechat data])"
            " (--bind [custom bind] (--bind ...)))"
            " (--ime [ime])"
            " (--help)\n\n"
            "    --"
            "data [wechat data]\t"
                "Path to Wechat_Data folder, absolute path, or relative path "
                "to user home, "
                "default: "
                    "~/Documents/Wechat_Data, "
                "as environment: "
                    ENV_DATA"\n"
            "    --"
            "bind [custom bind]\t"
                "Custom bindings, could be specified multiple times, absolute "
                "path, or relative path to user home, "
                "as environment: "
                    ENV_BINDS" (colon ':' seperated like PATH)\n"
            "    --"
            "binds-config [file]\t"
                "Path to text file that contains one --bind value per line, "
                "default: "
                    "~/.config/wechat-universal/binds.list, "
                "as environment: "
                    ENV_BINDS_CONFIG"\n"
            "    --"
            "ime [input method]\t"
                "Apply IME-specific workaround, "
                "support: "
                    "fcitx (also for 5), "
                    "ibus, "
                "default: "
                    "auto, "
                "as environment: "
                    ENV_IME"\n"
            "    --"
            "help\n");
    }
}

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
    if (id && !strncmp(id, ID_WECHAT, LEN_ID_WECHAT + 1)) {
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
    free(notifier);
free_bus:
    sd_bus_flush_close_unref(bus);
    return r;
}


int activate_notifier(
    sd_bus *const restrict bus,
    char const *const restrict notifier
) {
    int r;
    r = sd_bus_call_method(
        bus, notifier, DBUS_PATH_NOTIFIER, 
        DBUS_INTERFACE_NOTIFIER, DBUS_METHOD_ACTIVATE, 
        NULL, NULL, 
        TYPES_INT32_INT32,
        0, // X-cord to show window, wechat does not care
        0  // Y-cord to show window, wechat does not care
    );
    if (r < 0) {
        println_error_with_dbus("Failed to call D-Bus ListNames method", r);
        return -1;
    }
    return 0;
}

int binds_list_init(
    struct binds_list *const restrict list
) {
    struct passwd *passwd;

    if (!(passwd = getpwuid(getuid()))) {
        println_error_with_errno("Failed to get passwd entry of current user");
        return -1;
    }
    if (!passwd->pw_dir && !passwd->pw_dir[0]) {
        println_error("Current user does not have valid home directory");
        return -1;
    }
    list->len_home = strnlen(passwd->pw_dir, PATH_MAX);
    if (list->len_home == PATH_MAX) {
        --list->len_home;
    }
    if (!(list->buffer = malloc(list->buffer_allocated = PATH_MAX))) {
        println_error_with_errno("Failed to allocate memory for binds list");
        return -1;
    }
    if (!(list->offsets = malloc(
        (list->offsets_allocated = 16) * sizeof *list->offsets))
    ) {
        println_error_with_errno("Failed to allocate memory for binds list");
        free(list->buffer);
        return -1;
    }
    memcpy(list->buffer, passwd->pw_dir, list->len_home);
    list->buffer[list->len_home] = '\0';
    list->buffer_used = list->len_home + 1;
    list->offsets_count = 0;
    return 0;
}

int binds_list_add(
    struct binds_list *const restrict list,
    char const *const restrict name,
    size_t const len_name
) {
    size_t offset_null, new_used, new_count;
    void *buffer;
    bool need_home;

    offset_null = list->buffer_used + len_name;
    if ((need_home = name[0] != '\0')) {
        offset_null += list->len_home + 1;
    }
    if ((new_used = offset_null + 1) > list->buffer_allocated) {
        do {
            if (list->buffer_allocated == SIZE_MAX) {
                println_error("Impossible to allocate memory for more buffer");
                return -1;
            } else if (list->buffer_allocated >= SIZE_MAX / 2) {
                list->buffer_allocated = SIZE_MAX;
            } else {
                list->buffer_allocated *= 2;
            }
        } while (new_used > list->buffer_allocated);
        if (!(buffer = realloc(list->buffer, list->buffer_allocated))) 
        {
            println_error_with_errno(
                "Failed to allocate memory for bind buffer");
            return -1;
        }
        list->buffer = buffer;
    }
    if ((new_count = list->offsets_count + 1) > list->offsets_count) {
        do {
            if (list->offsets_allocated == SIZE_MAX) {
                println_error("Impossible to allocate memory for more offsets");
                return -1;
            } else if (list->offsets_allocated >= SIZE_MAX / 2) {
                list->offsets_allocated = SIZE_MAX;
            } else {
                list->offsets_allocated *= 2;
            }
        } while (new_count > list->offsets_allocated);
        if (!(buffer = realloc(list->offsets, 
            list->offsets_allocated * sizeof *list->offsets))) 
        {
            println_error_with_errno(
                "Failed to allocate memory for bind buffer");
            return -1;
        }
        list->offsets = buffer;
    }
    list->offsets[list->offsets_count++] = list->buffer_used;
    if (need_home) {
        memcpy(list->buffer + list->buffer_used,
             list->buffer, list->len_home);
        list->buffer[list->buffer_used += list->len_home] = '/';
        ++list->buffer_used;
    }
    memcpy(list->buffer + list->buffer_used, name, len_name);
    list->buffer[offset_null] = '\0';
    list->buffer_used = new_used;
    return 0;
}

#define binds_list_add_no_len(list, name) \
    binds_list_add(list, name, strnlen(name, PATH_MAX))

void binds_list_print(
    struct binds_list const *const restrict list
) {
    size_t i;

    for (i = 0; i < list->offsets_count; ++i) {
        println_info("Custom bind: '%s'", list->buffer + list->offsets[i]);
    }
}

int binds_list_add_path_like(
    struct binds_list *const restrict list,
    char const *const restrict path_like
) {
    char const *start = path_like, *end;
    int r;

    while (start && *start) {
        end = strchr(start, ':');
        if (end) {
            r = binds_list_add(list, start, end - start);
            start = end + 1;
        } else {
            r = binds_list_add_no_len(list, start);
            start = NULL;
        }
        if (r) {
            println_error(
                "Failed to add PATH-like string to binds list: '%s'", 
                path_like);
            return -1;
        }
    }
    return 0;
}

void binds_list_free(
    struct binds_list const *const restrict list
) {
    free(list->buffer);
    free(list->offsets);
}


int binds_list_init_from_env(
    struct binds_list *const restrict list
) {
    char *env;

    if (binds_list_init(list)) {
        println_error("Failed to init binds list");
        return -1;
    }
    if ((env = getenv(ENV_BINDS)) && 
        binds_list_add_path_like(list, env)) 
    {
        println_error("Failed to populate binds list from env "ENV_BINDS);
        binds_list_free(list);
        return -1;
    }
    return 0;
}

void ime_update_value(
    enum ime *const restrict ime,
    char const *const restrict value
) {
    switch (strnlen(value, 6)) {
    case 4:
        if (!memcmp(value, "none", 4)) {
            *ime = ImeNone;
            return;
        }
        if (!memcmp(value, "auto", 4)) {
            *ime = ImeAuto;
            return;
        }
        if (!memcmp(value, "ibus", 4)) {
            *ime = ImeIbus;
            return;
        }
        break;
    case 5:
        if (!memcmp(value, "fcitx", 5)) {
            *ime = ImeFcitx;
            return;
        }
        break;
    default:
        break;
    }
    println_warn("Unknown IME workaround value: '%s'", value);
}


int cli_start(
    int argc, 
    char *argv[]
) {
    int c, option_index = 0, r = -1;
    struct binds_list list;
    char data[PATH_MAX], *env;
    enum ime ime = ImeAuto;
    struct option const long_options[] = {
        {"data",            required_argument,  NULL,   'd'},
        {"bind",            required_argument,  NULL,   'b'},
        {"binds-config",    required_argument,  NULL,   'B'},
        {"ime",             required_argument,  NULL,   'i'},
        {"help",            no_argument,        NULL,   'h'},
        {NULL,              no_argument,        NULL,     0},
    };

    data[0] = '\0';
    data[PATH_MAX - 1] = '\0';
    if ((env = getenv(ENV_DATA))) {
        strncpy(data, env, PATH_MAX - 1);
    }
    if ((env = getenv(ENV_IME))) {
        ime_update_value(&ime, env);
    }
    if (binds_list_init_from_env(&list)) {
        println_error("Failed to init binds list and populate from env");
        return -1;
    }
    while ((c = getopt_long(argc, argv, "d:b:B:i:h", 
        long_options, &option_index)) != -1) 
    {
        switch (c) {
        case 'd': // --data
            strncpy(data, optarg, PATH_MAX - 1);
            break;
        case 'b': // --bind
            if (binds_list_add_no_len(&list, optarg)) {
                println_error("Failed to add bind '%s'", optarg);
                goto free_binds_list;
            }
            break;
        case 'B': // --binds-config
            break;
        case 'i': // --ime
            ime_update_value(&ime, optarg);
        case 'h':
            break;
        default:
            println_error("Unknown option '%s'", argv[optind - 1]);
            return -1;
        }
    }
    
    binds_list_print(&list);
    // if (binds_list_init())
    // struct binds_list;
    // // unsigned short binds_count = 0, binds_alloc;
    // char (*binds)[PATH_MAX], *env, *start, *end;
    // // char *env, *sep;
    

    // binds = malloc((binds_alloc = 16) * sizeof *binds);
    // if (!binds) {
    //     println_error_with_errno("Failed to allocate memory for binds");
    //     return -1;
    // }
    // env = getenv(ENV_BINDS);
    // if (env) {
    //     start = env;
    //     end = strchr(start, ':');

        


    // }

    // // getenv()



    r = 0;
free_binds_list:
    binds_list_free(&list);
    return r;
}

int try_move_foreground() {
    sd_bus *bus;
    char *notifier;
    int r = -1;

    println_info("Trying to find existing WeChat session and move it to "
                "foreground if it exists");
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
    r = activate_notifier(bus, notifier);
    free(notifier);
free_bus:
    sd_bus_flush_close_unref(bus);
    return r;

}

int applet_start(
    int argc, 
    char *argv[]
) {
    int i;

    // Early quit for --help
    for (i = 0; i < argc; ++i) {
        if (!strncmp(argv[i], "--help", 7)) {
            help_start(argv[0]);
            return 0;
        }
    }
    // if (!try_move_foreground()) return 0;
    return cli_start(argc, argv);
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