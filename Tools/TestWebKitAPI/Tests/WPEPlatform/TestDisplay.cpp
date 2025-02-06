/*
 * Copyright (C) 2025 Igalia, S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "TestMain.h"
#include <gio/gio.h>
#include <wpe/WPEDisplay.h>
#include <wtf/glib/GUniquePtr.h>

#define LOAD_TEST_CASE(display, testCase) \
    G_STMT_START { \
        GUniquePtr<gchar> path(g_build_filename(WPE_MOCK_TEST_CASES_DIR, testCase, nullptr)); \
        GRefPtr<GFile> file = adoptGRef(g_file_new_for_path(path.get())); \
        g_object_set(display, "test-case", file.get(), NULL); \
    } G_STMT_END

static WPEDisplay* createMockDisplay(void)
{
    WPEDisplay* display = wpe_display_get_default();
    return WPE_DISPLAY(g_object_new(G_OBJECT_TYPE(display), nullptr));
}

static void testLoadExternalDisplay(Test*, gconstpointer)
{
    WPEDisplay* display = wpe_display_get_default();

    g_assert_nonnull(display);
    g_assert_cmpstr(G_OBJECT_TYPE_NAME(display), ==, "WPEDisplayMock");
    g_assert_true(display == wpe_display_get_primary());
}

static void testEmpty(Test*, gconstpointer)
{
    GRefPtr<WPEDisplay> display = adoptGRef(createMockDisplay());

    LOAD_TEST_CASE(display.get(), "empty.ini");

    g_assert_cmpuint(wpe_display_get_n_screens(display.get()), ==, 0);
}

static void testConnect(Test*, gconstpointer)
{
    GRefPtr<WPEDisplay> display = adoptGRef(createMockDisplay());
    GUniqueOutPtr<GError> error;
    GUniqueOutPtr<GError> error2;

    LOAD_TEST_CASE(display.get(), "fail-connect.ini");
    g_assert_cmpuint(wpe_display_get_n_screens(display.get()), ==, 0);

    wpe_display_connect(display.get(), &error.outPtr());
    g_assert_error(error.get(), G_IO_ERROR, G_IO_ERROR_FAILED);

    LOAD_TEST_CASE(display.get(), "succeed-connect.ini");
    g_assert_cmpuint(wpe_display_get_n_screens(display.get()), ==, 0);

    wpe_display_connect(display.get(), &error2.outPtr());
    g_assert_no_error(error2.get());
}

void beforeAll()
{
    Test::add("Display", "load-external-display", testLoadExternalDisplay);
    Test::add("Display", "empty", testEmpty);
    Test::add("Display", "connect", testConnect);
}

void afterAll()
{
}
