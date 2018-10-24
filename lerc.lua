project "lerc"

  local prj = project()
  local prjDir = prj.basedir

  -- -------------------------------------------------------------
  -- project
  -- -------------------------------------------------------------

  -- common project settings

  dofile (_BUILD_DIR .. "/3rdparty_static_project.lua")
  -- project specific settings

  uuid "D2ECE861-E9E0-474C-A8E9-ABB9A7DAA770"

  flags {
    "NoPCH",
  }

  files {
    "Lerc/src/Common/*.cpp",
    "Lerc/src/Common/*.h",
    "Lerc/src/Include/*.h",
    "Lerc/src/Lerc/*.cpp",
    "Lerc/src/Lerc/*.h",
    "Lerc/src/Lerc1Decode/*.cpp",
    "Lerc/src/Lerc1Decode/*.hpp",
    "Lerc/src/Lerc1Decode/*.h",
    "Lerc/src/Lerc2/*.cpp",
    "Lerc/src/Lerc2/*.h"
  }

  -- -------------------------------------------------------------
  -- configurations
  -- -------------------------------------------------------------

  if (os.is("windows") and not _TARGET_IS_WINUWP) then
    -- -------------------------------------------------------------
    -- configuration { "windows" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/3rdparty_static_win.lua")

    -- project specific configuration settings

    configuration { "windows" }

    -- -------------------------------------------------------------
    -- configuration { "windows", "Debug", "x32" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_win_x86_debug.lua")

    -- project specific configuration settings

    -- configuration { "windows", "Debug", "x32" }

    -- -------------------------------------------------------------
    -- configuration { "windows", "Debug", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_win_x64_debug.lua")

    -- project specific configuration settings

    -- configuration { "windows", "Debug", "x64" }

    -- -------------------------------------------------------------
    -- configuration { "windows", "Release", "x32" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_win_x86_release.lua")

    -- project specific configuration settings

    -- configuration { "windows", "Release", "x32" }

    -- -------------------------------------------------------------
    -- configuration { "windows", "Release", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_win_x64_release.lua")

    -- project specific configuration settings

    -- configuration { "windows", "Release", "x64" }

    -- -------------------------------------------------------------
  end

  if (os.is("linux") and not _OS_IS_ANDROID) then
    -- -------------------------------------------------------------
    -- configuration { "linux" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_linux.lua")

    -- project specific configuration settings

    configuration { "linux" }

    -- -------------------------------------------------------------
    -- configuration { "linux", "Debug", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_linux_x64_debug.lua")

    -- project specific configuration settings

    -- configuration { "linux", "Debug", "x64" }

    -- -------------------------------------------------------------
    -- configuration { "linux", "Release", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_linux_x64_release.lua")

    -- project specific configuration settings

    -- configuration { "linux", "Release", "x64" }

    -- -------------------------------------------------------------
    -- configuration { "linux", "Debug", "x32" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_linux_x86_debug.lua")

    -- project specific configuration settings

    -- configuration { "linux", "Debug", "x32" }

    -- -------------------------------------------------------------
    -- configuration { "linux", "Release", "x32" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_linux_x86_release.lua")

    -- project specific configuration settings

    -- configuration { "linux", "Release", "x32" }

    -- -------------------------------------------------------------
  end

  if (os.is("macosx") and not _OS_IS_IOS and not _OS_IS_TVOS and not _OS_IS_ANDROID) then
    -- -------------------------------------------------------------
    -- configuration { "macosx" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_mac.lua")

    -- project specific configuration settings

    configuration { "macosx" }

    -- -------------------------------------------------------------
    -- configuration { "macosx", "Debug", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_mac_x64_debug.lua")

    -- project specific configuration settings

    -- configuration { "macosx", "Debug", "x64" }

    -- -------------------------------------------------------------
    -- configuration { "macosx", "Release", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_mac_x64_release.lua")

    -- project specific configuration settings

    -- configuration { "macosx", "Release", "x64" }

    -- -------------------------------------------------------------
  end

  if (_OS_IS_IOS) then
    -- -------------------------------------------------------------
    -- configuration { "ios" } == _OS_IS_IOS
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios.lua")

    -- project specific configuration settings

    configuration { "ios*" }

    -- -------------------------------------------------------------
    -- configuration { "ios_armv7_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_armv7_debug.lua")

    -- project specific configuration settings

    -- configuration { "ios_armv7_debug" }

    -- -------------------------------------------------------------
    -- configuration { "ios_armv7_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_armv7_release.lua")

    -- project specific configuration settings

    -- configuration { "ios_armv7_release" }

    -- -------------------------------------------------------------
    -- configuration { "ios_sim_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_sim_debug.lua")

    -- project specific configuration settings

    -- configuration { "ios_sim_debug" }

    -- -------------------------------------------------------------
    -- configuration { "ios_sim_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_sim_release.lua")

    -- project specific configuration settings

    -- configuration { "ios_sim_release" }

    -- -------------------------------------------------------------
    -- configuration { "ios_arm64_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_arm64_debug.lua")

    -- project specific configuration settings

    -- configuration { "ios_arm64_debug" }

    -- -------------------------------------------------------------
    -- configuration { "ios_arm64_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_arm64_release.lua")

    -- project specific configuration settings

    -- configuration { "ios_arm64_release" }

    -- -------------------------------------------------------------
    -- configuration { "ios_sim64_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_sim64_debug.lua")

    -- project specific configuration settings

    -- configuration { "ios_sim64_debug" }

    -- -------------------------------------------------------------
    -- configuration { "ios_sim64_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_ios_sim64_release.lua")

    -- project specific configuration settings

    -- configuration { "ios_sim64_release" }

    -- -------------------------------------------------------------
  end

  if (_OS_IS_ANDROID) then
    -- -------------------------------------------------------------
    -- configuration { "android" } == _OS_IS_ANDROID
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_android.lua")

    -- project specific configuration settings

    -- configuration { "android*" }

    -- -------------------------------------------------------------
    -- configuration { "android_armv7_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_android_armv7_debug.lua")

    -- project specific configuration settings

    -- configuration { "android_armv7_debug" }

    -- -------------------------------------------------------------
    -- configuration { "android_armv7_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_android_armv7_release.lua")

    -- project specific configuration settings

    -- configuration { "android_armv7_release" }

    -- -------------------------------------------------------------
    -- configuration { "android_x86_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_android_x86_debug.lua")

    -- project specific configuration settings

    -- configuration { "android_x86_debug" }

    -- -------------------------------------------------------------
    -- configuration { "android_x86_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_android_x86_release.lua")

    -- project specific configuration settings

    -- configuration { "android_x86_release" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_armv7_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_armv7_debug.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_armv7_debug" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_armv7_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_armv7_release.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_armv7_release" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_x86_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_x86_debug.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_x86_debug" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_x86_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_x86_release.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_x86_release" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_armv8_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_armv8_debug.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_armv8_debug" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_armv8_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_armv8_release.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_armv8_release" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_x64_debug" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_x64_debug.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_x64_debug" }

    -- -------------------------------------------------------------
    -- configuration { "androidgles3_x64_release" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_androidgles3_x64_release.lua")

    -- project specific configuration settings

    -- configuration { "androidgles3_x64_release" }

    -- -------------------------------------------------------------
  end

  if (_TARGET_IS_WINUWP) then
    -- -------------------------------------------------------------
    -- configuration { "winuwp" } == _TARGET_IS_WINUWP
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_winuwp.lua")

    -- project specific configuration settings

    configuration { "windows" }

    -- -------------------------------------------------------------
    -- configuration { "winuwp_debug", "x32" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_winuwp_x86_debug.lua")

    -- project specific configuration settings

    -- configuration { "winuwp_debug", "x32" }

    -- -------------------------------------------------------------
    -- configuration { "winuwp_release", "x32" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_winuwp_x86_release.lua")

    -- project specific configuration settings

    -- configuration { "winuwp_release", "x32" }

    -- -------------------------------------------------------------
    -- configuration { "winuwp_debug", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_winuwp_x64_debug.lua")

    -- project specific configuration settings

    -- configuration { "winuwp_debug", "x64" }

    -- -------------------------------------------------------------
    -- configuration { "winuwp_release", "x64" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_winuwp_x64_release.lua")

    -- project specific configuration settings

    -- configuration { "winuwp_release", "x64" }

    -- -------------------------------------------------------------
    -- configuration { "winuwp_debug", "ARM" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_winuwp_arm_debug.lua")

    -- project specific configuration settings

    -- configuration { "winuwp_debug", "ARM" }

    -- -------------------------------------------------------------
    -- configuration { "winuwp_release", "ARM" }
    -- -------------------------------------------------------------

    -- common configuration settings

    dofile (_BUILD_DIR .. "/static_winuwp_arm_release.lua")

    -- project specific configuration settings

    -- configuration { "winuwp_release", "ARM" }

    -- -------------------------------------------------------------
  end

  if (_IS_QT) then
    -- -------------------------------------------------------------
    -- configuration { "qt" }
    -- -------------------------------------------------------------

    local qt_include_dirs = PROJECT_INCLUDE_DIRS

    -- Add additional QT include dirs
    -- table.insert(qt_include_dirs, <INCLUDE_PATH>)

    createqtfiles(project(), qt_include_dirs)

    -- -------------------------------------------------------------
  end
