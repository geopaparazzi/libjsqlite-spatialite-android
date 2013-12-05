# Makefile for SQLite Java wrapper using MS VC++ 6 and SUN JDK1.4
#
# Assumes that the SQLite sources have been unpacked
# below this directory as sqlite-3-see

# Change this to the location where your Java2 SDK is installed
JAVA_HOME =	C:\j2sdk1.4.2_03

# Change this to the location where your Java2 Runtime is installed
JRE_HOME =	"$(JAVA_HOME)\jre"

# See comment at top
SQLITE3 =	sqlite-3-see

CC =		cl
SQLITE3_INC =	$(SQLITE3)
SQLITE3_LIB =	$(SQLITE3)\libsqlite3see.lib
JAVAH =		"$(JAVA_HOME)\bin\javah" -jni
JAVA_RUN =	"$(JAVA_HOME)\bin\java"
JAVAC =		"$(JAVA_HOME)\bin\javac"
JAVADOC =	"$(JAVA_HOME)\bin\javadoc"
JAR =		"$(JAVA_HOME)\bin\jar"
JNIINCLUDE =	"$(JAVA_HOME)\include"

JDBCVER =	JDBC2x

EXTENSION_DIR =	"$(JRE_HOME)\lib\ext"

NATIVE_LIB_DIR =	"$(JRE_HOME)\bin"

CFLAGS =	-nologo -Oti -Gs -W3 -D_MT -D_WIN32 \
		-I$(SQLITE3_INC) -I$(JNIINCLUDE) -I$(JNIINCLUDE)\win32 \
		-DHAVE_SQLITE_SET_AUTHORIZER=1 -DHAVE_SQLITE3=1 \
		-DHAVE_SQLITE3_MALLOC=1 \
		-DHAVE_SQLITE3_PREPARE_V2=1 \
		-DHAVE_SQLITE3_PREPARE16_V2=1 \
		-DHAVE_SQLITE3_BIND_ZEROBLOB=1 \
		-DHAVE_SQLITE3_CLEAR_BINDINGS=1 \
		-DHAVE_SQLITE3_COLUMN_TABLE_NAME16=1 \
		-DHAVE_SQLITE3_COLUMN_DATABASE_NAME16=1 \
		-DHAVE_SQLITE3_COLUMN_ORIGIN_NAME16=1 \
		-DHAVE_SQLITE3_BIND_PARAMETER_COUNT=1 \
		-DHAVE_SQLITE3_BIND_PARAMETER_NAME=1 \
		-DHAVE_SQLITE3_BIND_PARAMETER_INDEX=1 \
		-DHAVE_SQLITE3_RESULT_ZEROBLOB=1 \
		-DHAVE_SQLITE3_KEY=1 \
		-DHAVE_SQLITE3_INCRBLOBIO=1 \
		-DHAVE_SQLITE3_SHARED_CACHE=1

CFLAGS_DLL =	$(CFLAGS) -D_DLL -Gs -GD -nologo -LD -MD

CFLAGS_EXE =	-I. -I$(SQLITE3_INC) -DHAVE_SQLITE3=1 -Gs -GX -D_WIN32 -nologo

LIBS =	$(SQLITE3_LIB)

.SUFFIXES: .java .class .jar

.java.class:
	$(JAVAC) $(JAVAC_FLAGS) "$<"

CLASSES = \
	SQLite/Authorizer.class \
	SQLite/BusyHandler.class \
	SQLite/Callback.class \
	SQLite/Database.class \
	SQLite/Exception.class \
	SQLite/Shell.class \
	SQLite/TableResult.class \
	SQLite/Function.class \
	SQLite/FunctionContext.class \
	SQLite/Constants.class \
	SQLite/Trace.class \
	SQLite/Vm.class \
	SQLite/Stmt.class \
	SQLite/Blob.class \
	SQLite/BlobR.class \
	SQLite/BlobW.class \
	SQLite/ProgressHandler.class \
	SQLite/StringEncoder.class \
	SQLite/SQLDump.class \
	SQLite/SQLRestore.class \
	SQLite/Backup.class \
	SQLite/Profile.class

PRIVATE_CLASSES = \
	SQLite/DBDump.class \
	SQLite/JDBC.class \
	SQLite/JDBCDriver.class \
	SQLite/$(JDBCVER)/JDBCConnection.class \
	SQLite/$(JDBCVER)/JDBCStatement.class \
	SQLite/$(JDBCVER)/JDBCResultSet.class \
	SQLite/$(JDBCVER)/JDBCResultSetMetaData.class \
	SQLite/$(JDBCVER)/JDBCDatabaseMetaData.class \
	SQLite/$(JDBCVER)/JDBCPreparedStatement.class \
	SQLite/$(JDBCVER)/BatchArg.class \
	SQLite/$(JDBCVER)/TableResultX.class \
	SQLite/$(JDBCVER)/DatabaseX.class

DOCSRCS = \
	SQLite/BusyHandler.java \
	SQLite/Callback.java \
	SQLite/Database.java \
	SQLite/Exception.java \
	SQLite/TableResult.java \
	SQLite/Function.java \
	SQLite/FunctionContext.java \
	SQLite/Constants.java \
	SQLite/Trace.java \
	SQLite/Vm.java \
	SQLite/Stmt.java \
	SQLite/Blob.java \
	SQLite/ProgressHandler.java \
	SQLite/StringEncoder.java \
	SQLite/Backup.java \
	SQLite/Profile.java

all:	sqlite.jar sqlite_jni.dll

SQLite/Constants.java:	mkconst.exe $(SQLITE3)/libsqlite3.lib VERSION
	mkconst > SQLite/Constants.java

$(CLASSES):	SQLite/Constants.java

sqlite.jar:	$(CLASSES) $(PRIVATE_CLASSES)
	$(JAR) cmf manifest sqlite.jar $(CLASSES) $(PRIVATE_CLASSES)

native/sqlite_jni.h:	SQLite/Database.class SQLite/Vm.class \
	SQLite/FunctionContext.class SQLite/Stmt.class SQLite/Blob.class \
	SQLite/Backup.class SQLite/Profile.class
	$(JAVAH) -o native/sqlite_jni.h SQLite.Database SQLite.Vm \
	    SQLite.FunctionContext SQLite.Stmt SQLite.Blob SQLite.Backup \
	    SQLite.Profile

sqlite_jni.dll:	native/sqlite_jni.h native/sqlite_jni.c
	$(CC) $(CFLAGS_DLL) native/sqlite_jni.c $(LIBS)

mkconst.exe:	native/mkconst.c $(SQLITE3)/libsqlite3.lib
	$(CC) $(CFLAGS_EXE) native/mkconst.c

fixup.exe:	native/fixup.c
	$(CC) $(CFLAGS_EXE) native/fixup.c

mkopc3.exe:	native/mkopc3.c
	$(CC) $(CFLAGS_EXE) native/mkopc3.c

$(SQLITE3)/libsqlite3.lib:	fixup.exe mkopc3.exe
	cd $(SQLITE3)
	nmake -f ..\$(SQLITE3).mak
	cd ..

test:
	$(JAVAC) test.java
	$(JAVA_RUN) -Djava.library.path=. test

test3:
	$(JAVAC) test3.java
	$(JAVA_RUN) -Djava.library.path=. test3

clean:
	cd $(SQLITE3)
	nmake -f ..\$(SQLITE3).mak clean
	cd ..
	del SQLite\*.class
	del SQLite\$(JDBCVER)\*.class
	del native\sqlite_jni.h
	del *.obj
	del *.exe
	del *.dll
	del SQLite\Constants.java
	del test.class
	del test3.class
	del sqlite.jar

install:
	copy sqlite.jar $(EXTENSION_DIR)
	copy sqlite_jni.dll $(NATIVE_LIB_DIR)
