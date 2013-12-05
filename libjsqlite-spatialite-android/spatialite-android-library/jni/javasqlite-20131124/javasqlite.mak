# Makefile for SQLite Java wrapper using MS VC++ 6 and SUN JDK1.4
#
# Assumes that the SQLite sources have been unpacked
# below this directory as sqlite-2.8.17

# Change this to the location where your Java2 SDK is installed
JAVA_HOME =	C:\j2sdk1.4.2_03

# Change this to the location where your Java2 Runtime is installed
JRE_HOME =	"$(JAVA_HOME)\jre"

# See comment at top
SQLITE =	sqlite-2.8.17

CC =		cl
SQLITE_INC =	$(SQLITE)
SQLITE_LIB =	$(SQLITE)\libsqlite.lib
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
		-I$(SQLITE_INC) -I$(JNIINCLUDE) -I$(JNIINCLUDE)\win32 \
		-DHAVE_SQLITE_FUNCTION_TYPE=1 -DHAVE_SQLITE_SET_AUTHORIZER=1 \
		-DHAVE_SQLITE_COMPILE=1 -DHAVE_SQLITE_TRACE=1 \
		-DHAVE_SQLITE_PROGRESS_HANDLER=1 -DHAVE_SQLITE2=1

CFLAGS_DLL =	$(CFLAGS) -D_DLL -Gs -GD -nologo -LD -MD

CFLAGS_EXE =	-I. -I$(SQLITE_INC) -Gs -GX -D_WIN32 -nologo

LIBS =	$(SQLITE_LIB)

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
	SQLite/ProgressHandler.class \
	SQLite/StringEncoder.class

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
	SQLite/StringEncoder.java

all:	sqlite.jar sqlite_jni.dll

SQLite/Constants.java:	mkconst.exe $(SQLITE)/libsqlite.lib VERSION
	mkconst > SQLite/Constants.java

$(CLASSES):	SQLite/Constants.java

sqlite.jar:	$(CLASSES) $(PRIVATE_CLASSES)
	$(JAR) cmf manifest sqlite.jar $(CLASSES) $(PRIVATE_CLASSES)

native/sqlite_jni.h:	SQLite/Database.class SQLite/Vm.class \
	SQLite/FunctionContext.class SQLite/Stmt.class SQLite/Blob.class
	$(JAVAH) -o native/sqlite_jni.h SQLite.Database SQLite.Vm \
	     SQLite.FunctionContext SQLite.Stmt SQLite.Blob

sqlite_jni.dll:	native/sqlite_jni.h native/sqlite_jni.c
	$(CC) $(CFLAGS_DLL) native/sqlite_jni.c $(LIBS)

mkconst.exe:	native/mkconst.c $(SQLITE)/libsqlite.lib
	$(CC) $(CFLAGS_EXE) native/mkconst.c

fixup.exe:	native/fixup.c
	$(CC) $(CFLAGS_EXE) native/fixup.c

mkopc.exe:	native/mkopc.c
	$(CC) $(CFLAGS_EXE) native/mkopc.c

$(SQLITE)/libsqlite.lib:	fixup.exe mkopc.exe
	cd $(SQLITE)
	nmake -f ..\$(SQLITE).mak
	cd ..

test:
	$(JAVAC) test.java
	$(JAVA_RUN) -Djava.library.path=. test db2

clean:
	del SQLite\*.class
	del SQLite\$(JDBC2VER)\*.class
	del native\sqlite_jni.h
	del *.obj
	del *.exe
	del *.dll
	del SQLite\Constants.java
	del test.class
	del sqlite.jar

install:
	copy sqlite.jar $(EXTENSION_DIR)
	copy sqlite_jni.dll $(NATIVE_LIB_DIR)
