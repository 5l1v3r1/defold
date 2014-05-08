Defold
======

Repository for engine, editor and server.

Licenses
========

* **Sony Vectormath Library**: [http://bullet.svn.sourceforge.net/viewvc/bullet/trunk/Extras/vectormathlibrary](http://bullet.svn.sourceforge.net/viewvc/bullet/trunk/Extras/vectormathlibrary) - **BSD**
* **json**: Based on [https://bitbucket.org/zserge/jsmn/src](https://bitbucket.org/zserge/jsmn/src) - **MIT**
* **zlib**: [http://www.zlib.net](http://www.zlib.net) - **zlib**
* **axTLS**: [http://axtls.sourceforge.net](http://axtls.sourceforge.net) - **BSD**
* **stb_image** [http://nothings.org/](http://nothings.org) **Public domain**
* **stb_vorbis** [http://nothings.org/](http://nothings.org) **Public domain**
* **facebook** [https://github.com/facebook/facebook-ios-sdk](https://github.com/facebook/facebook-ios-sdk) **Apache**
* **glfw** [http://www.glfw.org](http://www.glfw.org) **zlib/libpng**
* **lua** [http://www.lua.org](http://www.lua.org) **MIT**
* **box2d** [http://box2d.org](http://box2d.org) **zlib**
* **bullet** [http://bulletphysics.org](http://bulletphysics.org) **zlib**
* **vp8** [http://www.webmproject.org](http://www.webmproject.org) **BSD**
* **openal** [http://kcat.strangesoft.net/openal.html](http://kcat.strangesoft.net/openal.html) **LGPL**
* **alut** [https://github.com/vancegroup/freealut](https://github.com/vancegroup/freealut) was **BSD** but changed to **LGPL**
* **md5** Based on md5 in axTLS


Tagging
-------

New tag

    # git tag -a MAJOR.MINOR [SHA1]
    SHA1 is optional

Push tags

    # git push origin --tags


Folder Structure
----------------

**ci** - Continious integration related files

**com.dynamo.cr** - _Content repository_. Editor and server

**engine** - Engine

**packages** - External packages

**scripts** - Build and utility scripts

**share** - Misc shared stuff used by other tools. Waf build-scripts, valgrind suppression files, etc.

Content pipeline
----------------

The primary build tool is bob. Bob is used for the editor but also for engine-tests.
In the first build-step a standalone version of bob is built. A legacy pipeline, waf/python and some classes from bob.jar,
is still used for gamesys and for built-in content. This might be changed in the future but integrating bob with waf 1.5.x
is pretty hard as waf 1.5.x is very restrictive where source and built content is located. Built-in content is compiled
, via .arc-files, to header-files, installed to $DYNAMO_HOME, etc In other words tightly integrated with waf.

Byte order/endian
-----------------

By convention all graphics resources are expliticly in little-ending and specifically ByteOrder.LITTLE_ENDIAN in Java. Currently we support
only little endian architectures. If this is about to change we would have to byte-swap at run-time or similar.
As run-time editor code and pipeline code often is shared little-endian applies to both. For specific editor-code ByteOrder.nativeOrder() is
the correct order to use.


iOS Debugging
-------------

* Make sure that you build with **--disable-ccache**. Otherwise lldb can't set breakpoints (all pending). The
  reason is currently unknown. The --disable-ccache option is available in waf and in build.py.
* Create a new empty iOS project (Other/Empty)
* Create a new scheme with Project>New Scheme...
* Select executable (dmengine.app)
* Make sure that debugger is lldb. Otherwise debuginfo is not found for static libraries when compiled with clang for unknown reason

iOS Crashdumps
--------------

From: [http://stackoverflow.com/a/13576028](http://stackoverflow.com/a/13576028)

    symbol address = slide + stack address - load address

* The slide value is the value of vmaddr in LC_SEGMENT cmd (Mostly this is 0x1000). Run the following to get it:

      otool -arch ARCHITECTURE -l "APP_BUNDLE/APP_EXECUTABLE" | grep -B 3 -A 8 -m 2 "__TEXT"

  Replace ARCHITECTURE with the actual architecture the crash report shows, e.g. armv7. Replace APP_BUNDLE/APP_EXECUTABLE with the path to the actual executable.

* The stack address is the hex value from the crash report.

* The load address can be is the first address showing in the Binary Images section at the very front of the line which contains your executable. (Usually the first entry).



Android
-------

By convention we currently have a weak reference to struct android\_app \* called g\_AndroidApp. 
g\_AndroidApp is set by glfw and used by dlib. This is more or less a circular dependency. See sys.cpp and android_init.c. 
Life-cycle support should probably be moved to dlib at some point.

### Android Resources and R.java

Long story short. Static resources on Android are referred by an integer identifier. These identifiers are generated to a file R.java.
The id:s generated are conceptually a serial number and with no guarantees about uniqueness. Due to this limitations **all** identifiers
must be generated when the final application is built. As a consequence all resources must be available and it's not possible to package
library resources in a jar. Moreover, one identical *R.java* must be generated for every package/library linked with the final application.

This is a known limitation on Android.

**NOTE:** Never ever package compiled **R*.class-files** with third party libraries as it doesn't work in general.

**NOTE2:** android_native_app_glue.c from the NDK has been modified to fix a back+virtual keyboard bug in OS 4.1 and 4.2, the modified version is in the glfw source.

### Android SDK/NDK


* Download SDK Tools 21.1 from here: [http://developer.android.com/sdk/index.html](http://developer.android.com/sdk/index.html).
  Drill down to *DOWNLOAD FOR OTHER PLATFORMS* and *SDK Tools Only*. Change URL to ...21.1.. 
  Do not upgrade SDK tools as we rely on the deprecated tool apkbuilder removed in 21.1+
* Launch android tool and install Android 4.2.2 (API 17). Do **not** upgrade SDK tools as
  mentioned above
* Download NDK 8e: [http://developer.android.com/tools/sdk/ndk/index.html](http://developer.android.com/tools/sdk/ndk/index.html)
* Put NDK/SDK in ~/android/android-ndk-r8e and ~/android/android-sdk respectively 

### Android testing

Copy executable (or directory) with

    # adb push <DIR_OR_DIR> /data/local/tmp

When copying directories append directory name to destination path. It's oterhwise skipped

Run exec with:

    # adb shell /data/local/tmp/....

For interactive shell run "adb shell"

### Caveats

If the app is started programatically, the life cycle behaves differently. Deactivating the app and then activating it by clicking on it results in a new 
create message being sent (onCreate/android_main). The normal case is for the app to continue through e.g. onStart.

### Android debugging

* Go to application bundle-dir in build/default/...,  e.g. build/default/examples/simple_gles2.android
* Install and launch application
* Run ndk-gdb from android ndk
* Debug

### Life-cycle and GLFW

NDK uses a separate thread which runs the game, separate from the Android UI thread.

The main life cycle (LC) of an android app is controlled by the following events, received on the game thread:

* _glfwPreMain(struct* android_app), corresponds to create
* APP_CMD_START, (visible)
* APP_CMD_RESUME
* APP_CMD_GAINED_FOCUS
* APP_CMD_LOST_FOCUS
* APP_CMD_PAUSE
* APP_CMD_STOP, (invisible)
* APP_CMD_SAVE_STATE
* APP_CMD_DESTROY

After APP_CMD_PAUSE, the process might be killed by the OS without APP_CMD_DESTROY being received.

Window life cycle (LC), controls the window (app_activity->window) and might happen at any point while the app is visible:

* APP_CMD_INIT_WINDOW
* APP_CMD_TERM_WINDOW

Specifics of exactly when they are received depend on manufacturer, OS version etc.

The graphics resources used are divided into Context and Surface:

* Context
  * EGLDisplay display
  * EGLContext context
  * EGLConfig config
* Surface
  * EGLSurface surface

GLFW functions called by the engine are:

* _glfwPlatformInit (Context creation)
* _glfwPlatformOpenWindow (Surface creation)
* _glfwPlatformCloseWindow (Surface destruction)
* _glfwPlatformTerminate (implicit Context destruction)

Some implementation details to note:

* _glfwPreMain pumps the LC commands until the window has been created (APP_CMD_INIT_WINDOW) before proceeding to boot the app (engine-main).
  This should be possible to streamline so that content loading can start faster.
* The engine continues to pump the LC commands as a part of polling for input (glfw)
* OpenWindow is the first time when the window dimensions are known, which controls screen orientation.
* The glfw window is considered open (_glfwWin.opened) from APP_CMD_INIT_WINDOW until APP_CMD_DESTROY, which is app termination
* The glfw window is considered iconified (_glfwWin.iconified) when not visible to user, which stops buffer swapping and controls poll timeouts
* Between CloseWindow and OpenWindow the GL context is temp-stored in memory (ordinary struct is memset to 0 by glfw in CloseWindow)
* When rebooting the engine (when using the dev app), essentially means CloseWindow followed by OpenWindow.
* APP_CMD_TERM_WINDOW might do Context destruction before _glfwPlatformTerminate, depending on which happens first
* _glfwPlatformTerminate pumps the LC commands until the Context has been destroyed

### Pulling APKs from device

E.g. when an APK produces a crash, backing it up is always a good idea before you attempt to fix it.

# Determine package name:
  adb shell pm list packages
# Get the path on device:
  adb shell pm path <package-name>
# Pull the APK to local disk
  adb pull <package-path>

OpenGL and jogl
---------------

Prior to GLCanvas#setCurrent the GLDrawableFactory must be created on OSX. This might be a bug but the following code works:

        GLDrawableFactory factory = GLDrawableFactory.getFactory(GLProfile.getGL2ES1());
        this.canvas.setCurrent();
		this.context = factory.createExternalGLContext();

Typically the getFactory and createExternalGLContext are in the same statement. The exception thrown is "Error: current Context (CGL) null, no Context (NS)" and might be related to loading of shared libraries that seems to triggered when the factory is
created. Key is probably that GLCanvas.setCurrnet fails to set current context before the factory is created. The details
are unknown though.

Asset loading
-------------

Assets can be loaded from file-system, from an archive or over http.

See *dmResource::LoadResource* for low-level loading of assets, *dmResource* for general resource loading and *engine.cpp*
for initialization. A current limitation is that we don't have a specific protocol for *resource:* For file-system, archive
and http url schemes *file:*, *arc:* and *http:* are used respectively. See dmConfigFile for the limitation about the absence 
of a resource-scheme.

### Http Cache

Assets loaded with dmResource are cached locally. A non-standard batch-oriented cache validation mechanism 
used if available in order to speed up the cache-validation process. See dlib, *dmHttpCache* and *ConsistencyPolicy*, for more information.

Engine Extensions
-----------------

Script extensions can be created using a simple exensions mechanism. To add a new extension to the engine the only required step is to link with the
extension library and set "exported_symbols" in the wscript, see note below.

*NOTE:* In order to avoid a dead-stripping bug with static libraries on OSX/iOS a constructor symbol must be explicitly exported with "exported_symbols"
in the wscript-target. See extension-test.

### Facebook Extension

How to package a new Android Facebook SDK:

* Download the SDK
* Replicate a structure based on previous FB SDK package (rooted at share/java within the package)
* From within the SDK:
  * copy bin/facebooksdk.jar into share/java/facebooksdk.jar
  * copy res/* into share/java/res/facebook
* tar/gzip the new structure

Energy Consumption
------------------


**Android**

      adb shell dumpsys cpuinfo

