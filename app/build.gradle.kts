plugins {
    id("com.android.application")
    // Add the Google services gradle plugin
    //id("com.google.gms.google-services")
    kotlin("android")
    kotlin("android.extensions")
}

val kotlinVersion: String by ext
val ndkSideBySideVersion: String by ext
val cmakeVersion: String by ext
val buildStagingDir: String by ext

val keystorePropertiesFile = rootProject.file("keystore.properties")

fun loadKeystoreProperties(file: File): Map<String, String> {
    return file.reader().use { reader ->
        reader.readLines()
            .filter { it.isNotBlank() && !it.startsWith("#") }
            .map {
                val (key, value) = it.split("=")
                key.trim() to value.trim()
            }
            .toMap()
    }
}

val keystoreProperties = loadKeystoreProperties(keystorePropertiesFile)

android {
    ndkVersion = ndkSideBySideVersion
    ndkPath = "/media/DATA/Dev/android-sdk/ndk/$ndkVersion"
    compileSdkVersion(31)

    defaultConfig {
        minSdk = 19
        targetSdk = 34
        versionCode = 33
        versionName = "1.033"
        multiDexEnabled = true

//        testInstrumentationRunner = "android.support.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                arguments.apply {
                    System.getenv("ANDROID_CCACHE")?.let { add("-D ANDROID_CCACHE=$it") }
                    arguments += listOf("-D URHO3D_LIB_TYPE=STATIC", "-D URHO3D_ANGELSCRIPT=0", "-D URHO3D_LUA=0", "-D URHO3D_LUAJIT=0")
                    arguments += listOf("-D URHO3D_NETWORK=0")
                    arguments += listOf("-D WITH_ADS=1", "-D WITH_CINEMATICS=1", "-D WITH_LOOPTESTS=0")               
                }

                // Sets a flag to enable format macro constants for the C compiler.
                cFlags += listOf("-std=c99")
                cppFlags += listOf("-std=c++98")

                targets.add("GalaxianMatch")
            }
        }
    }

	signingConfigs {
    	create("release") {
      		keyAlias = keystoreProperties["keyAlias"]
            keyPassword = keystoreProperties["keyPassword"]
            storeFile = file(keystoreProperties["storeFile"] as String)
            storePassword = keystoreProperties["storePassword"]
    	}
 	}
 	
    flavorDimensions += "graphics"
    productFlavors {
        create("gl") {
            dimension = "graphics"
            applicationIdSuffix = ".gl"
            versionNameSuffix = "-gl"
            externalNativeBuild {
                cmake {
                    arguments += listOf("-D URHO3D_OPENGL=1")
                }
            }
            splits {
                abi {
                    isEnable = true//project.hasProperty("ANDROID_ABI")
                    reset()
                    include("armeabi-v7a", "arm64-v8a")
                    //include("armeabi-v7a", "arm64-v8a", "x86", "x86_64")
                }
            }
        }
    }

    buildTypes {
        named("debug") {
            isJniDebuggable = true
            isMinifyEnabled = false
        }
        named("release") {
            isJniDebuggable = false
            isMinifyEnabled = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            signingConfig = signingConfigs.getByName("release")
        }
    }

    lintOptions {
        isAbortOnError = false
    }

    externalNativeBuild {
        cmake {
            version = cmakeVersion
            path = project.file("../CMakeLists.txt")
            buildStagingDirectory(buildStagingDir)
        }
    }

    sourceSets {
        named("main") {
            assets.srcDir(project.file("../bin"))
        }
    }
}

val urhoGlDebugImpl by configurations.creating { isCanBeResolved = true }
val urhoGlReleaseImpl by configurations.creating { isCanBeResolved = true }

dependencies {
    urhoGlDebugImpl("io.urho3d:urho3d-lib-glDebug:Unversioned-SM")
    urhoGlReleaseImpl("io.urho3d:urho3d-lib-glRelease:Unversioned-SM")
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar", "*.aar"))))
    implementation("org.jetbrains.kotlin:kotlin-stdlib-jdk8:$kotlinVersion")
    implementation("androidx.core:core-ktx:1.3.2")
    implementation("androidx.appcompat:appcompat:1.2.0")
    implementation("androidx.constraintlayout:constraintlayout:2.0.4")
// MultiDex Dependency
    implementation("androidx.multidex:multidex:2.0.1")
// Google Services Dependencies
    implementation("com.google.android.gms:play-services-ads:22.2.0")
    implementation("com.android.billingclient:billing:6.0.1")
// Import the Firebase BoM
//    implementation(platform("com.google.firebase:firebase-bom:32.8.0"))
// TODO: Add the dependencies for Firebase products you want to use
// When using the BoM, don't specify versions in Firebase dependencies
  // https://firebase.google.com/docs/android/setup#available-libraries

// Unit Tests Dependencies
//    testImplementation("junit:junit:4.13.1")
//    androidTestImplementation("androidx.test:runner:1.3.0")
//    androidTestImplementation("androidx.test.espresso:espresso-core:3.3.0")
}


// this is for compiling the java code (same java code for Gl or Vk), not for library dependency
// just be sure that the urho version (Gl or Vk) that it's used here, is already in .m2 local repository
configurations.debugImplementation.get().extendsFrom(urhoGlDebugImpl)
configurations.releaseImplementation.get().extendsFrom(urhoGlReleaseImpl)

afterEvaluate {
    android.applicationVariants.forEach { component ->
        val config = component.name.capitalize()
        val unzipTaskName = "unzipJni$config"
        tasks {
            "generateJsonModel$config" {
                dependsOn(unzipTaskName)
            }
            register<Copy>(unzipTaskName) {
                val aar = configurations["urho${config}Impl"].resolve()
                    .first { it.name.startsWith("urho3d-lib") }
                from(zipTree(aar))
                include("urho3d/**")
                android.externalNativeBuild.cmake.buildStagingDirectory?.let { into(it) }
            }
        }
    }
}

tasks {
    register<Delete>("cleanAll") {
        dependsOn("clean")
        delete = setOf(android.externalNativeBuild.cmake.buildStagingDirectory)
    }
}
