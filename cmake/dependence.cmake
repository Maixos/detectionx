# ------------------------------------------------------------
# System
# ------------------------------------------------------------
#find_package(PkgConfig REQUIRED)

# ------------------------------------------------------------
# X3RDPARTY
# ------------------------------------------------------------
string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" X3RDPARTY_TARGET_PLATFORM)

set(X3RDPARTY_SHARED_COMPONENTS OCV480 FFMPEG_MPP VCODECX RTSPX MQTTX TOOLKITX)
set(X3RDPARTY_STATIC_COMPONENTS YAML_CPP)

include(/usr/local/x3rdparty/x3rdparty.cmake)

# 最终导出变量包括：
#   - ${PACKAGE_NAME}_LIBS：每个依赖组件的动态库
#   - ${PACKAGE_NAME}_STATIC_LIBS：每个依赖组件的静态库
#   - ${PACKAGE_NAME}_INCLUDE_DIRS：每个依赖组件的头文件
#   - X3RDPARTY_INCLUDE_DIRS：所有库的头文件
#   - X3RDPARTY_SHARED_LIBS：所有共享库
#   - X3RDPARTY_STATIC_LIBS：所有静态库
#   - X3RDPARTY_LINK_LIBS：所有动态库和静态库
#   - X3RDPARTY_INSTALL_FILES：所有动态库和静态库的文件
