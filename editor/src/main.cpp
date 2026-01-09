#include <red_plasma/engine/engine_api.h>
#include <cstdio>

static void log_cb(void*, int level, const char* msg) {
    std::printf("[engine][%d] %s\n", level, msg);
}

int main() {
    rp_engine_desc_t desc{};
    desc.logger.callback = log_cb;
    desc.logger.user = nullptr;
    desc.logger.min_level = 0;
    desc.width = 1280;
    desc.height = 720;
    desc.enable_validation = 1;

    // For the stub stage, we can pass “empty” handles.
    // It should still return the stub error, proving linking + API works.
    rp_native_window_t wnd{};
    wnd.ws = RP_WS_X11;
    wnd.handle.x11.display = nullptr;
    wnd.handle.x11.window  = 0;

    rp_engine_t* engine = nullptr;
    rp_result_t r = rp_engine_create(&desc, &wnd, &engine);

    std::printf("rp_engine_create result = %d\n", (int)r);
    if (engine) {
        std::printf("last_error: %s\n", rp_engine_last_error(engine));
        rp_engine_destroy(engine);
    } else {
        std::printf("engine was not created.\n");
    }

    return (r == RP_OK) ? 0 : 1;
}
