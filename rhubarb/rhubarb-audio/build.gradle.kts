tasks.register("testCoverage") {
    group = "Rhubarb"
    doLast {
        val environmentVariables = mapOf(
            "RUSTFLAGS" to "-Cinstrument-coverage",
            "LLVM_PROFILE_FILE" to File(project.projectDir, "rhubarb-audio-%p-%m.profraw").path,
        )
        project.exec {
            commandLine = listOf("cargo", "build")
            environment(environmentVariables)
        }
        project.exec {
            commandLine = listOf("cargo", "test")
            environment(environmentVariables)
        }
        project.exec {
            commandLine = listOf(
                "grcov",
                "--source-dir", ".",
                "--binary-path", "../target/debug/deps",
                "--output-type", "html",
                "--branch", "--ignore-not-existing",
                "--output-path", "../target/debug/coverage",
                ".",
            )
        }
        project.delete(fileTree(project.projectDir) { include("*.profraw") })
    }
}
