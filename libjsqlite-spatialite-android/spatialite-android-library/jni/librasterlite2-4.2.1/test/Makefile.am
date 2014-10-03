check_PROGRAMS = test_coverage test_palette \
	test_section test_raster \
	test_mask test_webp test_gif \
	test1 test2 test3 test4 test5 \
	test6 test7 test8 test9 test10 \
	test11 test12 test13 test14 \
	test15 test16 test17 test18 \
	test19 test20 test_tifin \
	test_wr_tiff test_wms1 test_wms2  \
	test_paint check_sql_stmt \
	test_map_nile_8 test_map_nile_u8 \
	test_map_nile_u16 test_map_nile_32 \
	test_map_nile_u32 test_map_nile_flt \
	test_map_nile_dbl test_map_ascii \
	test_map_rgb test_map_gray \
	test_map_srtm test_map_mono \
	test_map_indiana test_load_wms \
	test_map_noref test_map_trento \
	test_map_trieste test_map_infrared \
	test_map_orbetello test_raster_symbolizer \
	test_svg test_raw test_openjpeg

AM_CPPFLAGS = -I@srcdir@/../headers @LIBXML2_CFLAGS@
AM_LDFLAGS = -L../src -lrasterlite2 @LIBCAIRO_LIBS@ @LIBPNG_LIBS@ \
	@LIBWEBP_LIBS@ @LIBLZMA_LIBS@ @LIBSPATIALITE_LIBS@ \
	@LIBCURL_LIBS@ @LIBXML2_LIBS@ $(GCOV_FLAGS)

TESTS = $(check_PROGRAMS)

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = jpeg1.jpg jpeg2.jpg png1.png mask1.png \
	webp_no_alpha.webp gif1.gif mono3t.tif mono3s.tif \
	mono4t.tif mono4s.tif gray-tiled.tif gray-striped.tif \
	plt-tiled.tif plt-striped.tif rgb-tiled.tif \
	rgb-striped.tif test_under_valgrind.sh \
	asprintf4win.h fnmatch_impl4win.h \
	fnmatch4win.h scandir4win.h \
	map_samples/usgs-gray/gray1.tif \
	map_samples/usgs-gray/gray2.tif \
	map_samples/usgs-rgb/rgb1.tif \
	map_samples/usgs-rgb/rgb2.tif \
	map_samples/usgs-srtm/srtm1.tif \
	map_samples/usgs-srtm/srtm2.tif \
	map_samples/usgs-nile-8/nile1-int8.tif \
	map_samples/usgs-nile-8/nile2-int8.tif \
	map_samples/usgs-nile-u8/nile1-uint8.tif \
	map_samples/usgs-nile-u8/nile2-uint8.tif \
	map_samples/usgs-nile-u16/nile1-uint16.tif \
	map_samples/usgs-nile-u16/nile2-uint16.tif \
	map_samples/usgs-nile-32/nile1-int32.tif \
	map_samples/usgs-nile-32/nile2-int32.tif \
	map_samples/usgs-nile-u32/nile1-uint32.tif \
	map_samples/usgs-nile-u32/nile2-uint32.tif \
	map_samples/usgs-nile-flt/nile1-flt.tif \
	map_samples/usgs-nile-flt/nile2-flt.tif \
	map_samples/usgs-nile-dbl/nile1-dbl.tif \
	map_samples/usgs-nile-dbl/nile2-dbl.tif \
	map_samples/usgs-indiana/indiana1.tif \
	map_samples/usgs-indiana/indiana2.tif \
	map_samples/ctrt/cap1.tif \
	map_samples/ctrt/cap2.tif \
	map_samples/ascii/ascii1.asc \
	map_samples/ascii/ascii2.asc \
	map_samples/trento-rgb/trento-rgb1.jpg \
	map_samples/trento-rgb/trento-rgb1.jgw \
	map_samples/trento-rgb/trento-rgb2.jpg \
	map_samples/trento-rgb/trento-rgb2.jgw \
	map_samples/trento-gray/trento-gray1.jpg \
	map_samples/trento-gray/trento-gray1.jgw \
	map_samples/trento-gray/trento-gray2.jpg \
	map_samples/trento-gray/trento-gray2.jgw \
	map_samples/orbview3-trieste/trieste1.tif \
	map_samples/orbview3-trieste/trieste2.tif \
	map_samples/tuscany-infrared/infrared1.tif \
	map_samples/tuscany-infrared/infrared2.tfw \
	map_samples/tuscany-infrared/infrared2.tif \
	map_samples/orbview3-orbetello/orbetello1.tif \
	map_samples/orbview3-orbetello/orbetello2.tfw \
	map_samples/orbview3-orbetello/orbetello2.tif \
	raster_symbolizer_1.xml raster_symbolizer_2.xml \
	raster_symbolizer_3.xml raster_symbolizer_4.xml \
	raster_symbolizer_5.xml raster_symbolizer_6.xml \
	srtm_categ.xml srtm_interp.xml srtm_brightness.xml \
	srtm_relief_25.xml srtm_relief_75.xml ir_gray.xml \
	ir_gray_gamma.xml ir_false_color1.xml \
	ir_false_color2.xml ir_false_color1_gamma.xml \
	ir_false_color2_gamma.xml rgb_histogram.xml \
	rgb_normalize.xml gray_gamma.xml gray_histogram.xml \
	gray_normalize.xml rgb_normalize2.xml \
	rgb_histogram2.xml rgb_gamma.xml gray_normalize2.xml \
	gray_histogram2.xml gray_gamma2.xml group_style_1.xml \
	group_style_2.xml bicycle.svg car_repair.svg \
	Car_Yellow.svg Circle_and_quadratic_bezier.svg \
	Coat_of_arms_Holy_See.svg doctors.svg fastfood.svg \
	Flag_of_the_United_Kingdom.svg Flag_of_the_United_States.svg \
	hospital.svg jewelry.svg motorcycle.svg \
	Negative_and_positive_skew_diagrams_(English).svg \
	Netherlands_roundel.svg pharmacy.svg photo.svg restaurant.svg \
	Roundel_of_the_Royal_Canadian_Air_Force_(1946-1965).svg \
	Roundel_of_the_Syrian_Air_Force.svg \
	Royal_Coat_of_Arms_of_the_United_Kingdom.svg \
	supermarket.svg tobacco.svg Cevennes2.jp2

SUBDIRS = sql_stmt_security_tests sql_stmt_tests 

