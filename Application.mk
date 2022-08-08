APP_ABI := all
APP_PLATFORM := android-21
APP_STL := c++_static
APP_BUILD_SCRIPT := Android.mk
APP_CPPFLAGS := -static -std=c++11 -DDP_LIN -DDP_LINUX -DDP_ANDROID -I../../ -fexceptions -frtti
APP_CFLAGS := -static -DDP_LIN -DDP_LINUX -DDP_ANDROID -I../../ -fexceptions -frtti
