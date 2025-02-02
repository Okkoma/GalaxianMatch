buildscript {
    extra["kotlinVersion"] = "1.4.10"
    val kotlinVersion: String by extra
    repositories {
        mavenCentral()
        google()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:4.2.2")
        classpath("org.jetbrains.kotlin:kotlin-gradle-plugin:$kotlinVersion")
    }
}

//plugins {
//  // Add the dependency for the Google services Gradle plugin
//  id("com.google.gms.google-services") version "4.4.1" apply false
//}

val kotlinVersion: String by ext

allprojects {
    repositories {
        mavenLocal()
        mavenCentral()
        google()
    }
    buildscript {
        ext {
            set("kotlinVersion", kotlinVersion)
            set("ndkSideBySideVersion", "21.4.7075529")
            set("cmakeVersion", "3.17.3+")
            set("buildStagingDir", ".cxx")
        }
    }
}

tasks {
    wrapper {
        distributionType = Wrapper.DistributionType.ALL
    }
    "prepareKotlinBuildScriptModel" {
        listOf("Debug", "Release").forEach {
          dependsOn(":app:unzipJni$it")
        }
    }
    register<Delete>("clean") {
        // Clean the build artifacts generated by the Gradle build system only, but keep the buildDir
        rootProject.buildDir.listFiles { _, name -> name == "intermediates" || name == "kotlin" }?.let {
            delete = it.toSet()
        }
    }
    register<Delete>("cleanAll") {
        dependsOn("clean")
    }
}
