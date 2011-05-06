/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#include <stdlib.h>
#include <iostream>

#include "glimports.hpp"
#include "glws.hpp"


namespace glws {


static Display *display = NULL;
static int screen = 0;

class GlxVisual : public Visual
{
public:
    XVisualInfo *visinfo;

    GlxVisual(XVisualInfo *vi) :
        visinfo(vi)
    {}

    ~GlxVisual() {
        XFree(visinfo);
    }
};


class GlxDrawable : public Drawable
{
public:
    Window window;

    GlxDrawable(const Visual *vis, int w, int h) :
        Drawable(vis, w, h)
    {
        XVisualInfo *visinfo = dynamic_cast<const GlxVisual *>(visual)->visinfo;

        Window root = RootWindow(display, screen);

        /* window attributes */
        XSetWindowAttributes attr;
        attr.background_pixel = 0;
        attr.border_pixel = 0;
        attr.colormap = XCreateColormap(display, root, visinfo->visual, AllocNone);
        attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;

        unsigned long mask;
        mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

        int x = 0, y = 0;

        window = XCreateWindow(
            display, root,
            x, y, width, height,
            0,
            visinfo->depth,
            InputOutput,
            visinfo->visual,
            mask,
            &attr);

        XSizeHints sizehints;
        sizehints.x = x;
        sizehints.y = y;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize | USPosition;
        XSetNormalHints(display, window, &sizehints);

        const char *name = "glretrace";
        XSetStandardProperties(
            display, window, name, name,
            None, (char **)NULL, 0, &sizehints);

        XMapWindow(display, window);
    }

    ~GlxDrawable() {
        XDestroyWindow(display, window);
    }

    void
    resize(int w, int h) {
        glXWaitGL();
        Drawable::resize(w, h);
        XResizeWindow(display, window, w, h);
        glXWaitX();
    }

    void swapBuffers(void) {
        glXSwapBuffers(display, window);
    }
};


class GlxContext : public Context
{
public:
    GLXContext context;

    GlxContext(const Visual *vis, GLXContext ctx) :
        Context(vis),
        context(ctx)
    {}

    ~GlxContext() {
        glXDestroyContext(display, context);
    }
};


class GlxWindowSystem : public WindowSystem
{
public:
    GlxWindowSystem() {
        if (!display) {
            display = XOpenDisplay(NULL);
            if (!display) {
                std::cerr << "error: unable to open display " << XDisplayName(NULL) << "\n";
                exit(1);
            }
            screen = DefaultScreen(display);
        }
    }

    ~GlxWindowSystem() {
        XCloseDisplay(display);
    }

    Visual *
    createVisual(bool doubleBuffer) {
        int single_attribs[] = {
            GLX_RGBA,
            GLX_RED_SIZE, 1,
            GLX_GREEN_SIZE, 1,
            GLX_BLUE_SIZE, 1,
            GLX_DEPTH_SIZE, 1,
            GLX_STENCIL_SIZE, 1,
            None
        };

        int double_attribs[] = {
            GLX_RGBA,
            GLX_RED_SIZE, 1,
            GLX_GREEN_SIZE, 1,
            GLX_BLUE_SIZE, 1,
            GLX_DOUBLEBUFFER,
            GLX_DEPTH_SIZE, 1,
            GLX_STENCIL_SIZE, 1,
            None
        };

        XVisualInfo *visinfo;

        visinfo = glXChooseVisual(display, screen, doubleBuffer ? double_attribs : single_attribs);

        return new GlxVisual(visinfo);
    }

    Drawable *
    createDrawable(const Visual *visual, int width, int height)
    {
        return new GlxDrawable(visual, width, height);
    }

    Context *
    createContext(const Visual *visual)
    {
        XVisualInfo *visinfo = dynamic_cast<const GlxVisual *>(visual)->visinfo;
        GLXContext context = glXCreateContext(display, visinfo, NULL, True);
        return new GlxContext(visual, context);
    }

    bool
    makeCurrent(Drawable *drawable, Context *context)
    {
        if (!drawable || !context) {
            return glXMakeCurrent(display, None, NULL);
        } else {
            GlxDrawable *glxDrawable = dynamic_cast<GlxDrawable *>(drawable);
            GlxContext *glxContext = dynamic_cast<GlxContext *>(context);

            return glXMakeCurrent(display, glxDrawable->window, glxContext->context);
        }
    }

    bool
    processEvents(void) {
        while (XPending(display) > 0) {
            XEvent event;
            XNextEvent(display, &event);
            // TODO
        }
        return true;
    }
};


WindowSystem *createNativeWindowSystem(void) {
    return new GlxWindowSystem();
}


} /* namespace glretrace */
