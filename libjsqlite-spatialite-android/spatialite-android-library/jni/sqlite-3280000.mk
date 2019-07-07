include $(CLEAR_VARS)
# -------------------
# As of 2017-02-13
# -------------------
# changes:
# Approximately 25% better performance from the R-Tree extension.
# -------------------
# As of 2019-04-16
# -------------------
# changes:
# introduced an useful SQL function that could directly interest many users of SpatiaLite:
# SELECT RTreeCheck ( 'idx_roads_geom' );
# it will check for validity and internal consistency a whole R*Tree (Spatial Index) returning a diagnostic report.
# -------------------
LOCAL_MODULE    := sqlite

# SQLite flags copied from ASOP
common_sqlite_flags := \
 -DHAVE_USLEEP=1 \
 -DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576 \
 -DSQLITE_THREADSAFE=1 \
 -DNDEBUG=1 \
 -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 \
 -DSQLITE_HAS_COLUMN_METADATA=1 \
 -DSQLITE_DEFAULT_AUTOVACUUM=1 \
 -DSQLITE_TEMP_STORE=3 \
 -DSQLITE_ENABLE_FTS3 \
 -DSQLITE_ENABLE_FTS3_BACKWARDS \
 -DSQLITE_ENABLE_RTREE=1 \
 -DSQLITE_DEFAULT_FILE_FORMAT=4

LOCAL_CFLAGS    := $(common_sqlite_flags)

# LOCAL_LDLIBS is always ignored for static libraries
# LOCAL_LDLIBS    := -llog
LOCAL_C_INCLUDES :=
LOCAL_SRC_FILES := $(SQLITE_PATH)/sqlite3.c
LOCAL_STATIC_LIBRARIES :=

include $(BUILD_STATIC_LIBRARY)
