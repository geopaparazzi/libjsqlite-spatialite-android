include $(CLEAR_VARS)
# ./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
LOCAL_MODULE    := libcairo

# make and listed *.o files in src ; make clean

LOCAL_CFLAGS    := \
	$(cairo_flags)

LOCAL_C_INCLUDES := \
 $(CAIRO_PATH)
LOCAL_SRC_FILES := \
	$(CAIRO_PATH)/cairo-analysis-surface.c \
	$(CAIRO_PATH)/cairo-arc.c \
	$(CAIRO_PATH)/cairo-array.c \
	$(CAIRO_PATH)/cairo-atomic.c \
	$(CAIRO_PATH)/cairo-base64-stream.c \
	$(CAIRO_PATH)/cairo-base85-stream.c \
	$(CAIRO_PATH)/cairo-bentley-ottmann.c \
	$(CAIRO_PATH)/cairo-bentley-ottmann-rectangular.c \
	$(CAIRO_PATH)/cairo-bentley-ottmann-rectilinear.c \
	$(CAIRO_PATH)/cairo-botor-scan-converter.c \
	$(CAIRO_PATH)/cairo-boxes-intersect.c \
	$(CAIRO_PATH)/cairo-boxes.c \
	$(CAIRO_PATH)/cairo-cache.c \
	$(CAIRO_PATH)/cairo-cff-subset.c \
	$(CAIRO_PATH)/cairo-clip-boxes.c \
	$(CAIRO_PATH)/cairo-clip.c \
	$(CAIRO_PATH)/cairo-clip-polygon.c \
	$(CAIRO_PATH)/cairo-clip-region.c \
	$(CAIRO_PATH)/cairo-clip-surface.c \
	$(CAIRO_PATH)/cairo-clip-tor-scan-converter.c \
	$(CAIRO_PATH)/cairo-color.c \
	$(CAIRO_PATH)/cairo-composite-rectangles.c \
	$(CAIRO_PATH)/cairo-compositor.c \
	$(CAIRO_PATH)/cairo-contour.c \
	$(CAIRO_PATH)/cairo-damage.c \
	$(CAIRO_PATH)/cairo-debug.c \
	$(CAIRO_PATH)/cairo-default-context.c \
	$(CAIRO_PATH)/cairo-deflate-stream.c \
	$(CAIRO_PATH)/cairo-device.c \
	$(CAIRO_PATH)/cairo-error.c \
	$(CAIRO_PATH)/cairo-fallback-compositor.c \
	$(CAIRO_PATH)/cairo-fixed.c \
	$(CAIRO_PATH)/cairo-font-face.c \
	$(CAIRO_PATH)/cairo-font-face-twin-data.c \
	$(CAIRO_PATH)/cairo-font-face-twin.c \
	$(CAIRO_PATH)/cairo-font-options.c \
	$(CAIRO_PATH)/cairo-freed-pool.c \
	$(CAIRO_PATH)/cairo-freelist.c \
	$(CAIRO_PATH)/cairo-ft-font.c \
	$(CAIRO_PATH)/cairo-gstate.c \
	$(CAIRO_PATH)/cairo-hash.c \
	$(CAIRO_PATH)/cairo-hull.c \
	$(CAIRO_PATH)/cairo-image-compositor.c \
	$(CAIRO_PATH)/cairo-image-info.c \
	$(CAIRO_PATH)/cairo-image-source.c \
	$(CAIRO_PATH)/cairo-image-surface.c \
	$(CAIRO_PATH)/cairo-lzw.c \
	$(CAIRO_PATH)/cairo-mask-compositor.c \
	$(CAIRO_PATH)/cairo-matrix.c \
	$(CAIRO_PATH)/cairo-mempool.c \
	$(CAIRO_PATH)/cairo-mesh-pattern-rasterizer.c \
	$(CAIRO_PATH)/cairo-misc.c \
	$(CAIRO_PATH)/cairo-mono-scan-converter.c \
	$(CAIRO_PATH)/cairo-mutex.c \
	$(CAIRO_PATH)/cairo-no-compositor.c \
	$(CAIRO_PATH)/cairo.c \
	$(CAIRO_PATH)/cairo-observer.c \
	$(CAIRO_PATH)/cairo-output-stream.c \
	$(CAIRO_PATH)/cairo-paginated-surface.c \
	$(CAIRO_PATH)/cairo-path-bounds.c \
	$(CAIRO_PATH)/cairo-path-fill.c \
	$(CAIRO_PATH)/cairo-path-fixed.c \
	$(CAIRO_PATH)/cairo-path-in-fill.c \
	$(CAIRO_PATH)/cairo-path.c \
	$(CAIRO_PATH)/cairo-path-stroke-boxes.c \
	$(CAIRO_PATH)/cairo-path-stroke.c \
	$(CAIRO_PATH)/cairo-path-stroke-polygon.c \
	$(CAIRO_PATH)/cairo-path-stroke-traps.c \
	$(CAIRO_PATH)/cairo-path-stroke-tristrip.c \
	$(CAIRO_PATH)/cairo-pattern.c \
	$(CAIRO_PATH)/cairo-pdf-operators.c \
	$(CAIRO_PATH)/cairo-pdf-shading.c \
	$(CAIRO_PATH)/cairo-pdf-surface.c \
	$(CAIRO_PATH)/cairo-pen.c \
	$(CAIRO_PATH)/cairo-png.c \
	$(CAIRO_PATH)/cairo-polygon-intersect.c \
	$(CAIRO_PATH)/cairo-polygon.c \
	$(CAIRO_PATH)/cairo-polygon-reduce.c \
	$(CAIRO_PATH)/cairo-ps-surface.c \
	$(CAIRO_PATH)/cairo-raster-source-pattern.c \
	$(CAIRO_PATH)/cairo-recording-surface.c \
	$(CAIRO_PATH)/cairo-rectangle.c \
	$(CAIRO_PATH)/cairo-rectangular-scan-converter.c \
	$(CAIRO_PATH)/cairo-region.c \
	$(CAIRO_PATH)/cairo-rtree.c \
	$(CAIRO_PATH)/cairo-scaled-font.c \
	$(CAIRO_PATH)/cairo-scaled-font-subsets.c \
	$(CAIRO_PATH)/cairo-script-surface.c \
	$(CAIRO_PATH)/cairo-shape-mask-compositor.c \
	$(CAIRO_PATH)/cairo-slope.c \
	$(CAIRO_PATH)/cairo-spans-compositor.c \
	$(CAIRO_PATH)/cairo-spans.c \
	$(CAIRO_PATH)/cairo-spline.c \
	$(CAIRO_PATH)/cairo-stroke-dash.c \
	$(CAIRO_PATH)/cairo-stroke-style.c \
	$(CAIRO_PATH)/cairo-surface-clipper.c \
	$(CAIRO_PATH)/cairo-surface-fallback.c \
	$(CAIRO_PATH)/cairo-surface.c \
	$(CAIRO_PATH)/cairo-surface-observer.c \
	$(CAIRO_PATH)/cairo-surface-offset.c \
	$(CAIRO_PATH)/cairo-surface-snapshot.c \
	$(CAIRO_PATH)/cairo-surface-subsurface.c \
	$(CAIRO_PATH)/cairo-surface-wrapper.c \
	$(CAIRO_PATH)/cairo-svg-surface.c \
	$(CAIRO_PATH)/cairo-time.c \
	$(CAIRO_PATH)/cairo-tor22-scan-converter.c \
	$(CAIRO_PATH)/cairo-tor-scan-converter.c \
	$(CAIRO_PATH)/cairo-toy-font-face.c \
	$(CAIRO_PATH)/cairo-traps-compositor.c \
	$(CAIRO_PATH)/cairo-traps.c \
	$(CAIRO_PATH)/cairo-tristrip.c \
	$(CAIRO_PATH)/cairo-truetype-subset.c \
	$(CAIRO_PATH)/cairo-type1-fallback.c \
	$(CAIRO_PATH)/cairo-type1-glyph-names.c \
	$(CAIRO_PATH)/cairo-type1-subset.c \
	$(CAIRO_PATH)/cairo-type3-glyph-surface.c \
	$(CAIRO_PATH)/cairo-unicode.c \
	$(CAIRO_PATH)/cairo-user-font.c \
	$(CAIRO_PATH)/cairo-version.c \
	$(CAIRO_PATH)/cairo-wideint.c \
	$(CAIRO_PATH)/cairo-xcb-connection-core.c \
	$(CAIRO_PATH)/cairo-xcb-connection.c \
	$(CAIRO_PATH)/cairo-xcb-connection-render.c \
	$(CAIRO_PATH)/cairo-xcb-connection-shm.c \
	$(CAIRO_PATH)/cairo-xcb-screen.c \
	$(CAIRO_PATH)/cairo-xcb-shm.c \
	$(CAIRO_PATH)/cairo-xcb-surface-core.c \
	$(CAIRO_PATH)/cairo-xcb-surface.c \
	$(CAIRO_PATH)/cairo-xcb-surface-render.c \
	$(CAIRO_PATH)/cairo-xlib-core-compositor.c \
	$(CAIRO_PATH)/cairo-xlib-display.c \
	$(CAIRO_PATH)/cairo-xlib-fallback-compositor.c \
	$(CAIRO_PATH)/cairo-xlib-render-compositor.c \
	$(CAIRO_PATH)/cairo-xlib-screen.c \
	$(CAIRO_PATH)/cairo-xlib-source.c \
	$(CAIRO_PATH)/cairo-xlib-surface.c \
	$(CAIRO_PATH)/cairo-xlib-surface-shm.c \
	$(CAIRO_PATH)/cairo-xlib-visual.c \
	$(CAIRO_PATH)/cairo-xlib-xcb-surface.c 
include $(BUILD_STATIC_LIBRARY)





