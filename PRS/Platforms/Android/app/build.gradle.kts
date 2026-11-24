plugins {
    id("com.android.application")
}

android {
    namespace = "dev.prs.mobileshell"
    compileSdk = 34
    ndkVersion = "27.0.12077973"

    defaultConfig {
        applicationId = "dev.prs.mobileshell"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "0.1"

        ndk {
            // The soft raster presenter and the future Vulkan path both target arm64.
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }

        externalNativeBuild {
            cmake {
                // No Java sources (NativeActivity), so no C++ JNI glue is needed.
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DPRS_BUILD_PROGRAMS=OFF",
                    "-DPRS_BUILD_TESTS=OFF"
                )
                cppFlags += "-std=c++20"
            }
        }
    }

    externalNativeBuild {
        cmake {
            // The engine's own CMake project; Games/MobileShell builds libmobile_shell.so.
            path = file("../../../CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }
}
