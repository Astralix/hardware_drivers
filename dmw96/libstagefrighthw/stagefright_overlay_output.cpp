

//#define LOG_NDEBUG 0
#define LOG_TAG "stagefright_overlay_output"
#include <utils/Log.h>
#include <media/stagefright/HardwareAPI.h>

#include "DSPGHardwareRenderer.h"

using android::sp;
using android::ISurface;
using android::VideoRenderer;

VideoRenderer *createRendererWithRotation(
        const sp<ISurface> &surface,
        const char *componentName,
        OMX_COLOR_FORMATTYPE colorFormat,
        size_t displayWidth, size_t displayHeight,
        size_t decodedWidth, size_t decodedHeight, int32_t rotation) {
    using android::DSPGHardwareRenderer;

	LOGV("start");

    DSPGHardwareRenderer *renderer =
        new DSPGHardwareRenderer(
                surface, displayWidth, displayHeight,
                decodedWidth, decodedHeight,
                colorFormat, rotation);

	LOGV("after DSPGHardwareRenderer constructor");

    if (renderer->initCheck() != android::OK) {
		LOGE("renderer->initCheck() != android::OK");
        delete renderer;
        renderer = NULL;
    }
	else LOGV("success");

    return renderer;
}

