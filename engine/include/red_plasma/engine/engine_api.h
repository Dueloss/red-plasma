#pragma once

#include "red_plasma/engine/engine_api.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct rp_engine rp_engine_t;

typedef enum rp_result
{
    RP_OK = 0,
    RP_ERR_INVALID_ARGUMENT,
    RP_ERR_UNSUPPORTED_PLATFORM,
    RP_ERR_VULKAN_INIT_FAILED,
    RP_ERR_SURFACE_FAILD,
    RP_ERR_SWAPCHAIN_FAILD,
    RP_ERR_DEVICE_LOST,
    RP_ERR_UNKNOWN
} rp_result_t;

typedef void (*rp_log_fn)(void* user, int level, const char* msg);

typedef struct rp_logger
{
    rp_log_fn callback;
    void* user;
    int min_level;
} rp_logger_t;

typedef enum rp_window_system
{
    RP_WS_X11,
    RP_WS_WAYLAND,
    RP_WS_WIN32
} rp_window_system_t;

typedef struct rp_native_window
{
    rp_window_system_t ws;
    union
    {
        struct {void* display; uint64_t window;} x11;
        struct {void* display; void* surface;} wayland;
        struct {void* hwnd;} win32;
    } handle;
} rp_native_window_t;

typedef struct rp_engine_desc
{
    rp_logger_t logger;
    uint32_t width;
    uint32_t height;
    int enable_validation; // 0/1
} rp_engine_desc_t;

rp_result_t rp_engine_create(
    const rp_engine_desc_t* rp_engine_desc,
    const rp_native_window_t* window,
    rp_engine_t** out_engine
);

rp_result_t rp_engine_resize(rp_engine_t* engine, uint32_t width, uint32_t height);
rp_result_t rp_engine_render(rp_engine_t* engine);
rp_result_t rp_engine_wait_idle(rp_engine_t* engine);

void rp_engine_destroy(rp_engine_t* engine);
const char* rp_engine_last_error(const rp_engine_t* engine);
#ifdef __cplusplus
}
#endif
