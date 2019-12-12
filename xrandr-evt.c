#define NAPI_EXPERIMENTAL

#include <node_api.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <uv.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>

#include <node_api.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

typedef struct
{
        napi_threadsafe_function *function;
} threadParameter_t;
void *thread(void *args)
{
        threadParameter_t *params = (threadParameter_t *)args;

        xcb_connection_t *conn;
        xcb_screen_t *screen;
        xcb_window_t window;
        xcb_generic_event_t *evt;
        xcb_randr_screen_change_notify_event_t *randr_evt;
        xcb_timestamp_t last_time = 0;
        int monitor_connected = 1;

        conn = xcb_connect(NULL, NULL);

        screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
        window = screen->root;
        xcb_randr_select_input(conn, window, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
        xcb_flush(conn);

        while ((evt = xcb_wait_for_event(conn)) != NULL)
        {
                if (evt->response_type & XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE)
                {
                        randr_evt = (xcb_randr_screen_change_notify_event_t *)evt;
                        if (last_time != randr_evt->timestamp)
                        {
                                last_time = randr_evt->timestamp;
                                monitor_connected = !monitor_connected;
                                // printf("XRANDR EVT connected\n");
                                napi_call_threadsafe_function(*params->function, NULL, napi_tsfn_blocking);
                        }
                }
                free(evt);
        }
        xcb_disconnect(conn);
        return NULL;
}
void call_js_cb(napi_env env,
                napi_value js_callback,
                void *context,
                void *data)
{
        napi_status status;
        napi_value element;
        status = napi_create_string_utf8(env, "change", NAPI_AUTO_LENGTH, &element);
        if (status != napi_ok)
                printf("Failed to napi_create_uint32");
        napi_value global;
        status = napi_get_global(env, &global);
        if (status != napi_ok)
                printf("Failed to napi_get_global");

        status = napi_call_function(env, global, js_callback, 1, &element, NULL);
        if (status != napi_ok)
                printf("Failed to napi_call_function");
}
napi_value changeX(napi_env env, napi_callback_info info)
{
        napi_status status;
        napi_value argv[2];
        size_t argc = 2;
        uint32_t mask;
        uint32_t values[1];

        status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
        if (status != napi_ok)
        {
                printf("status: %d\n", status);
                napi_throw_error(env, NULL, "Failed to parse arguments");
        }
        if (argc < 2)
        {
                napi_throw_error(env, NULL, "Too few arguments");
                return NULL;
        }
        uint32_t x;
        xcb_window_t windowId;
        napi_get_value_uint32(env, argv[0], &windowId);
        napi_get_value_uint32(env, argv[1], &x);
        mask = XCB_CONFIG_WINDOW_X;
        values[0] = x;
        xcb_connection_t *xcb_connection;
        xcb_connection = xcb_connect(NULL, NULL);
        xcb_void_cookie_t cookie = xcb_configure_window_checked(xcb_connection,
                                                                windowId,
                                                                mask,
                                                                values);
        xcb_generic_error_t * error = xcb_request_check(xcb_connection, cookie);

        if (error != NULL)
        {
                printf("error: %d for window %d x: %d\n", error->error_code, windowId, x);
                napi_throw_error(env, NULL, "Failed to parse arguments");
        }
        xcb_flush(xcb_connection);
        xcb_disconnect(xcb_connection);
        return NULL;
}
napi_value callEmit(napi_env env, napi_callback_info info)
{
        napi_status status;
        napi_value argv[1];
        size_t argc = 1;
        status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
        if (status != napi_ok)
        {
                printf("status: %d\n", status);
                napi_throw_error(env, NULL, "Failed to parse arguments");
        }
        if (argc < 1)
        {
                napi_throw_error(env, NULL, "Too few arguments");
                return NULL;
        }

        napi_value resourceName;
        napi_create_string_utf8(env,
                                "testResourceName",
                                NAPI_AUTO_LENGTH,
                                &resourceName);
        napi_value resource;
        napi_create_string_utf8(env,
                                "testResource",
                                NAPI_AUTO_LENGTH,
                                &resource);
        threadParameter_t *params = (threadParameter_t *)malloc(sizeof(threadParameter_t));
        params->function = malloc(sizeof(napi_threadsafe_function));
        status = napi_create_threadsafe_function(env,
                                                 argv[0],
                                                 resource,
                                                 resourceName,
                                                 0,
                                                 2,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 call_js_cb,
                                                 params->function);
        if (status != napi_ok)
        {
                napi_throw_error(env, NULL, "Failed to napi_create_threadsafe_function");
                return NULL;
        }
        pthread_t tid;
        pthread_create(&tid, NULL, thread, (void *)params);
        //
        return NULL;
}

napi_value init_all(napi_env env, napi_value exports)
{
        napi_value callEmitFn;
        napi_create_function(env, NULL, 0, callEmit, NULL, &callEmitFn);
        napi_set_named_property(env, exports, "callEmit", callEmitFn);
        napi_value changeXFn;
        napi_create_function(env, NULL, 0, changeX, NULL, &changeXFn);
        napi_set_named_property(env, exports, "changeX", changeXFn);
        return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init_all)
