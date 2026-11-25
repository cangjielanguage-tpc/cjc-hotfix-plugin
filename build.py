from pathlib import Path
import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys


ROOT_DIR = Path(__file__).parent.resolve()
OUTPUT_DIR = ROOT_DIR / "output"
PLUGIN_PROJECT_DIR = ROOT_DIR / "cjc-plugin"
PATCH_GEN_PROJECT_DIR = ROOT_DIR / "patch-gen"
PLUGIN_BUILD_DIR = OUTPUT_DIR / "cjc-plugin"
PATCH_GEN_BUILD_DIR = OUTPUT_DIR / "patch-gen"


def run_command(command, *, cwd=None, env=None):
    command = [str(part) for part in command]
    print(command, flush=True)
    subprocess.run(command, check=True, cwd=cwd, env=env)


def clean():
    if OUTPUT_DIR.exists():
        print(f"Cleaning {OUTPUT_DIR}...")
        shutil.rmtree(OUTPUT_DIR)
    else:
        print("Nothing to clean.")


def build_environment(cangjie_path):
    env = os.environ.copy()
    library_var = "DYLD_LIBRARY_PATH" if sys.platform == "darwin" else "LD_LIBRARY_PATH"
    env[library_var] = os.pathsep.join(
        filter(None, (str(cangjie_path / "tools" / "lib"), env.get(library_var)))
    )
    return env


def configure_and_build(args, project_dir, build_dir, definitions, env):
    build_dir.mkdir(parents=True, exist_ok=True)
    command = [
        "cmake",
        "-S", project_dir,
        "-B", build_dir,
        f"-DCANGJIE_PATH={args.cangjie_toolchain_path}",
        f"-DCMAKE_BUILD_TYPE={args.build_type.capitalize()}",
        *definitions,
    ]
    run_command(command, env=env)
    run_command(["cmake", "--build", build_dir, "--parallel", args.jobs], env=env)


def build_plugin(args):
    env = build_environment(args.cangjie_toolchain_path)
    ios_package_init = "ON" if args.target_ios else "OFF"
    definitions = [
        f"-DIOS_TARGET={ios_package_init}",
        f"-DTEST={'ON' if args.run_tests else 'OFF'}",
        f"-DSTUB_TEST={'ON' if args.run_tests == 'stub' else 'OFF'}",
    ]
    local_googletest = env.get("LOCAL_GOOGLETESTS_PATH")
    if args.run_tests and local_googletest:
        definitions.append(
            f"-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST={Path(local_googletest).expanduser().resolve()}"
        )
    configure_and_build(args, PLUGIN_PROJECT_DIR, PLUGIN_BUILD_DIR, definitions, env)
    if not args.run_tests:
        return

    run_command(
        ["ctest", "--output-on-failure", "--parallel", args.jobs],
        cwd=PLUGIN_BUILD_DIR,
        env=env,
    )
    expected_extension = "expected.stub" if args.run_tests == "stub" else "expected"
    run_command(
        [
            "bash", PLUGIN_PROJECT_DIR / "test" / "run_tests.sh",
            expected_extension, PLUGIN_BUILD_DIR, args.cangjie_toolchain_path,
        ],
        cwd=PLUGIN_PROJECT_DIR / "test" / "test_data",
        env=env,
    )


def build_patch_gen(args):
    env = build_environment(args.cangjie_toolchain_path)
    tests_enabled = args.run_tests and args.build_type == "release"
    configure_and_build(
        args, PATCH_GEN_PROJECT_DIR, PATCH_GEN_BUILD_DIR,
        [f"-DTEST={'ON' if tests_enabled else 'OFF'}"], env,
    )
    if not args.run_tests:
        return
    if not tests_enabled:
        print("Skipping patch-gen functional tests in debug mode")
        return

    for opt_level in args.test_compile_level:
        run_command(
            [
                "bash", PATCH_GEN_PROJECT_DIR / "test" / "run_tests.sh",
                PATCH_GEN_BUILD_DIR, args.cangjie_toolchain_path,
                args.cjc_plugin_path, opt_level, *args.filter_tests,
            ],
            cwd=PATCH_GEN_PROJECT_DIR / "test" / "test_data",
            env=env,
        )


def add_build_options(parser):
    parser.add_argument(
        "--cangjie-toolchain", dest="cangjie_toolchain_path",
        default=os.environ.get("CANGJIE_HOME"),
        help="Path to the Cangjie SDK (default: CANGJIE_HOME)",
    )
    parser.add_argument(
        "-t", "--build-type", choices=["debug", "release"], default="debug",
        help="Build configuration (default: debug)",
    )
    parser.add_argument(
        "-j", "--jobs", type=int, default=multiprocessing.cpu_count(),
        help="Number of parallel build jobs (default: CPU count)",
    )


def main():
    parser = argparse.ArgumentParser(description="Build and test the Cangjie hotfix tools")
    commands = parser.add_subparsers(dest="command", required=True)

    plugin_parser = commands.add_parser("build-plugin", help="Build the C++ compiler plugin")
    add_build_options(plugin_parser)
    plugin_mode = plugin_parser.add_mutually_exclusive_group()
    plugin_mode.add_argument("--run-tests", choices=["normal", "stub"])
    plugin_mode.add_argument(
        "--target-ios", action="store_true",
        help="Build the hotfix plugin for iOS app targets",
    )

    patch_parser = commands.add_parser("build-patch-gen", help="Build the CHIR patch generator")
    add_build_options(patch_parser)
    patch_parser.add_argument("--plugin", dest="cjc_plugin_path", required=True)
    patch_parser.add_argument("--run-tests", action="store_true")
    patch_parser.add_argument("--filter-tests", nargs="+", default=[])
    patch_parser.add_argument(
        "--test-compile-level", "--cjc-opt-level", nargs="+",
        choices=["O0", "O1", "O2"], default=["O0", "O1", "O2"],
        help="Cangjie optimization levels for patch-gen tests (default: O0 O1 O2)",
    )
    commands.add_parser("clean", help="Remove all build artifacts")

    args = parser.parse_args()
    if args.command == "clean":
        clean()
        return
    if not args.cangjie_toolchain_path:
        parser.error("source the Cangjie envsetup.sh or pass --cangjie-toolchain")
    args.cangjie_toolchain_path = Path(args.cangjie_toolchain_path).expanduser().resolve()
    if not (args.cangjie_toolchain_path / "include").is_dir():
        parser.error(f"Cangjie SDK headers not found in {args.cangjie_toolchain_path}")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if args.command == "build-patch-gen":
        args.cjc_plugin_path = Path(args.cjc_plugin_path).expanduser().resolve()
        if not args.cjc_plugin_path.is_file():
            parser.error(f"CJC plugin library not found: {args.cjc_plugin_path}")
        if args.filter_tests and not args.run_tests:
            parser.error("--filter-tests requires --run-tests")
        build_patch_gen(args)
    else:
        build_plugin(args)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
