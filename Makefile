all: svgstopdf
svgstopdf:
	gcc -o svgstopdf svgstopdf.c -pthread -I/usr/include/librsvg-2.0 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng12 -I/usr/include/poppler/glib -I/usr/include/poppler -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12 -lrsvg-2 -lm -lgdk_pixbuf-2.0 -lpoppler-glib -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lcairo
