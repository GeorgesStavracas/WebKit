/*
 * Copyright (C) 2025 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WPEDisplayMock.h"

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <gmodule.h>

struct _WPEDisplayMock {
    WPEDisplay parent;

    struct {
        bool shouldFail;
    } connect;

    GFile* testCase;
};

G_DEFINE_DYNAMIC_TYPE(WPEDisplayMock, wpe_display_mock, WPE_TYPE_DISPLAY)

enum {
    PROP_0,
    PROP_TEST_CASE,
    N_PROPERTIES,
};

static std::array<GParamSpec*, N_PROPERTIES> sObjProperties;

static void loadTestCase(WPEDisplayMock* displayMock, GFile* testCase)
{
    g_autoptr(GKeyFile) keyFile = nullptr;
    g_autoptr(GError) error = nullptr;

    if (!g_set_object(&displayMock->testCase, testCase))
        return;

    g_debug("Loading test case %s", g_file_peek_path(testCase));

    keyFile = g_key_file_new();
    g_key_file_load_from_file(keyFile, g_file_peek_path(testCase), G_KEY_FILE_NONE, &error);
    if (error)
        g_assert_not_reached();

    displayMock->connect.shouldFail = g_key_file_get_boolean(keyFile, "Display", "FailConnect", &error);
    if (error && g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE)) {
        g_critical("Invalid value in FailConnect key: %s", error->message);
        g_assert_not_reached();
    }
}

static gboolean wpeDisplayMockConnect(WPEDisplay* display, GError** error)
{
    WPEDisplayMock* displayMock = WPE_DISPLAY_MOCK(display);

    if (displayMock->connect.shouldFail) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to connect mock display");
        return FALSE;
    }
    return TRUE;
}

static WPEView* wpeDisplayMockCreateView(WPEDisplay* display)
{
    return nullptr;
}

static WPEInputMethodContext* wpeDisplayMockCreateInputMethodContext(WPEDisplay* display)
{
    return nullptr;
}

static gpointer wpeDisplayMockGetEGLDisplay(WPEDisplay* display, GError** error)
{
    g_set_error_literal(error, WPE_EGL_ERROR, WPE_EGL_ERROR_NOT_AVAILABLE, "Can't get EGL display: no display connection matching mock connection found");
    return nullptr;
}

static WPEKeymap* wpeDisplayMockGetKeymap(WPEDisplay* display, GError** error)
{
    return nullptr;
}

static WPEBufferDMABufFormats* wpeDisplayMockGetPreferredDMABufFormats(WPEDisplay* display)
{
    return nullptr;
}

static guint wpeDisplayMockGetNScreens(WPEDisplay* display)
{
    return 0;
}

static WPEScreen* wpeDisplayMockGetScreen(WPEDisplay* display, guint index)
{
    return nullptr;
}

static const char* wpeDisplayMockGetDRMDevice(WPEDisplay* display)
{
    return nullptr;
}

static const char* wpeDisplayMockGetDRMRenderNode(WPEDisplay* display)
{
    return nullptr;
}

static gboolean wpeDisplayMockUseExplicitSync(WPEDisplay* display)
{
    return FALSE;
}

static void wpeDisplayMockDispose(GObject* object)
{
    WPEDisplayMock* displayMock = WPE_DISPLAY_MOCK(object);

    g_clear_object(&displayMock->testCase);

    G_OBJECT_CLASS(wpe_display_mock_parent_class)->dispose(object);
}

static void wpeDisplayMockGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WPEDisplayMock* displayMock = WPE_DISPLAY_MOCK(object);

    switch (propId) {
    case PROP_TEST_CASE:
        g_value_set_object(value, displayMock->testCase);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void wpeDisplayMockSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* paramSpec)
{
    WPEDisplayMock* displayMock = WPE_DISPLAY_MOCK(object);

    switch (propId) {
    case PROP_TEST_CASE:
        loadTestCase(displayMock, G_FILE(g_value_get_object(value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void wpe_display_mock_class_init(WPEDisplayMockClass* displayMockClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(displayMockClass);
    objectClass->dispose = wpeDisplayMockDispose;
    objectClass->get_property = wpeDisplayMockGetProperty;
    objectClass->set_property = wpeDisplayMockSetProperty;

    WPEDisplayClass* displayClass = WPE_DISPLAY_CLASS(displayMockClass);
    displayClass->connect = wpeDisplayMockConnect;
    displayClass->create_view = wpeDisplayMockCreateView;
    displayClass->create_input_method_context = wpeDisplayMockCreateInputMethodContext;
    displayClass->get_egl_display = wpeDisplayMockGetEGLDisplay;
    displayClass->get_keymap = wpeDisplayMockGetKeymap;
    displayClass->get_preferred_dma_buf_formats = wpeDisplayMockGetPreferredDMABufFormats;
    displayClass->get_n_screens = wpeDisplayMockGetNScreens;
    displayClass->get_screen = wpeDisplayMockGetScreen;
    displayClass->get_drm_device = wpeDisplayMockGetDRMDevice;
    displayClass->get_drm_render_node = wpeDisplayMockGetDRMRenderNode;
    displayClass->use_explicit_sync = wpeDisplayMockUseExplicitSync;

    sObjProperties[PROP_TEST_CASE] =
        g_param_spec_object("test-case", nullptr, nullptr, G_TYPE_FILE,
            static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties(objectClass, N_PROPERTIES, sObjProperties.data());
}

static void wpe_display_mock_class_finalize(WPEDisplayMockClass*)
{
}

static void wpe_display_mock_init(WPEDisplayMock* self)
{
}

G_MODULE_EXPORT void g_io_module_load(GIOModule* module)
{
    wpe_display_mock_register_type(G_TYPE_MODULE(module));

    g_io_extension_point_implement(WPE_DISPLAY_EXTENSION_POINT_NAME, WPE_TYPE_DISPLAY_MOCK, "wpe-display-mock", 1);
}

G_MODULE_EXPORT void g_io_module_unload(GIOModule*)
{
}
