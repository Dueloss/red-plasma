#include <red_plasma/engine/engine_api.h>
#include <new>
#include <string>

struct rp_engine {
    rp_logger_t logger{};
    std::string last_error;
};

static void rp_log(const rp_logger_t& lg, int level, const char* msg)
{
    if(!lg.callback) return;
    if(level < lg.min_level) return;
    lg.callback(lg.user, level, msg);
}

static rp_result_t rp_fail(rp_engine_t* e, rp_result_t code, const char* msg)
{
    if(e) e->last_error = msg ? msg : "unknown error";
    return code;
}

rp_result_t rp_engine_create(
    const rp_engine_desc_t* desc,
    const rp_native_window_t* window,
    rp_engine_t** out_engine)
{
    if (!desc || !window || !out_engine) return RP_ERR_INVALID_ARGUMENT;
    *out_engine = nullptr;

    auto* e = new (std::nothrow) rp_engine();
    if(!e) return RP_ERR_UNKNOWN;

    e->logger = desc->logger;
    rp_log(e->logger, 1, "rp_engine_create: created engine (stub)");

    // Vulkan + surface creation come next.
    // For now, return "unsupported" so the API + linking is proven.
    *out_engine = e;
    return rp_fail(e, RP_ERR_UNSUPPORTED_PLATFORM, "Vulkan not implemented yet (stub).");
}

rp_result_t rp_engine_resize(rp_engine_t* engine, uint32_t, uint32_t)
{
    if(!engine) return RP_ERR_INVALID_ARGUMENT;
    return RP_OK;
}

rp_result_t rp_engine_render(rp_engine_t* engine)
{
    if (!engine) return RP_ERR_INVALID_ARGUMENT;
    return RP_OK;
}

rp_result_t rp_engine_wait_idle(rp_engine_t* engine)
{
    if(!engine) return RP_ERR_INVALID_ARGUMENT;
    return RP_OK;
}

void rp_engine_destroy(rp_engine_t* engine)
{
    delete engine;
}

const char* rp_engine_last_error(const rp_engine_t* engine)
{
    if(!engine) return "engine is null";
    return engine->last_error.c_str();
}
