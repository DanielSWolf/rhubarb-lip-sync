use std::{
    env::var_os,
    fs::remove_dir_all,
    path::{Path, PathBuf},
    process::Command,
};

fn exec(current_dir: impl AsRef<Path>, command: &str, args: &[&str]) {
    let exit_status = Command::new(command)
        .args(args)
        .current_dir(current_dir)
        .status()
        .unwrap();
    let exit_code = exit_status.code().unwrap();
    assert_eq!(exit_code, 0);
}

fn checkout(parent_dir: impl AsRef<Path>, git_url: &str, refname: &str, dir_name: &str) -> PathBuf {
    let repo_dir = parent_dir.as_ref().join(dir_name);

    if repo_dir.exists() && !repo_dir.join(".git").join("config").exists() {
        // The repository directory exists, but is not a valid Git repository.
        // Delete and check out from scratch.
        remove_dir_all(&repo_dir).unwrap();
    }

    if repo_dir.exists() {
        // If the repo directory's contents got corrupted, Git may not consider it a valid
        // repository, applying the following commands to the Rhubarb repository instead.
        // Prevent any resulting data loss this by failing if the repo was modified.
        exec(&repo_dir, "git", &["diff", "--exit-code"]);

        exec(&repo_dir, "git", &["reset", "--hard"]);
        exec(&repo_dir, "git", &["fetch"]);
        exec(&repo_dir, "git", &["checkout", refname]);
    } else {
        exec(
            &parent_dir,
            "git",
            &["clone", "--branch", refname, git_url, dir_name],
        );
    };

    repo_dir
}

struct OggBuildResult {
    ogg_include_dir: PathBuf,
}

fn build_ogg(parent_dir: impl AsRef<Path>) -> OggBuildResult {
    let repo_dir = checkout(
        &parent_dir,
        "https://github.com/xiph/ogg.git",
        "v1.3.5",
        "ogg",
    );
    let include_dir = repo_dir.join("include");
    let src_dir = repo_dir.join("src");
    cc::Build::new()
        .include(&include_dir)
        .files(["bitwise.c", "framing.c"].map(|name| src_dir.join(name)))
        .compile("ogg");

    println!("cargo:rustc-link-lib=static=ogg");
    OggBuildResult {
        ogg_include_dir: include_dir,
    }
}

struct VorbisBuildResult {
    vorbis_utils_path: PathBuf,
}

fn build_vorbis(
    parent_dir: impl AsRef<Path>,
    ogg_include_dir: impl AsRef<Path>,
) -> VorbisBuildResult {
    let repo_dir = checkout(
        &parent_dir,
        "https://github.com/xiph/vorbis.git",
        "v1.3.7",
        "vorbis",
    );
    let include_dir = repo_dir.join("include");
    let src_dir = repo_dir.join("lib");
    let vorbis_utils_path =
        Path::new(env!("CARGO_MANIFEST_DIR")).join("src/ogg_audio_clip/vorbis-utils.c");
    cc::Build::new()
        .include(&include_dir)
        .include(&ogg_include_dir)
        .files(
            [
                "bitrate.c",
                "block.c",
                "codebook.c",
                "envelope.c",
                "floor0.c",
                "floor1.c",
                "info.c",
                "lpc.c",
                "lsp.c",
                "mapping0.c",
                "mdct.c",
                "psy.c",
                "registry.c",
                "res0.c",
                "sharedbook.c",
                "smallft.c",
                "synthesis.c",
                "vorbisfile.c",
                "window.c",
            ]
            .iter()
            .map(|name| src_dir.join(name)),
        )
        .file(&vorbis_utils_path)
        .compile("vorbis");

    println!("cargo:rustc-link-lib=static=vorbis");
    VorbisBuildResult { vorbis_utils_path }
}

fn build_libsamplerate(parent_dir: impl AsRef<Path>) {
    let version = "0.2.2";
    let repo_dir = checkout(
        &parent_dir,
        "https://github.com/libsndfile/libsamplerate.git",
        version,
        "libsamplerate",
    );
    let include_dir = repo_dir.join("include");
    let src_dir = repo_dir.join("src");
    cc::Build::new()
        .define("PACKAGE", "\"libsamplerate\"")
        .define("VERSION", format!("\"{version}\"").as_str())
        .define("HAVE_STDBOOL_H", "1")
        .define("ENABLE_SINC_MEDIUM_CONVERTER", "1")
        .include(&include_dir)
        .files(
            ["samplerate.c", "src_linear.c", "src_sinc.c", "src_zoh.c"]
                .map(|name| src_dir.join(name)),
        )
        .compile("libsamplerate");

    println!("cargo:rustc-link-lib=static=libsamplerate");
}

fn main() {
    let out_dir = Path::new(&var_os("OUT_DIR").unwrap()).to_path_buf();
    println!("cargo:rustc-link-search=native={}", out_dir.display());

    let OggBuildResult { ogg_include_dir } = build_ogg(&out_dir);
    let VorbisBuildResult { vorbis_utils_path } = build_vorbis(&out_dir, ogg_include_dir);
    build_libsamplerate(out_dir);

    println!("cargo:rerun-if-changed={}", vorbis_utils_path.display());
}
