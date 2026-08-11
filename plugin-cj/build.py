from pathlib import Path
import argparse
import multiprocessing
import os
import shutil
import shlex
import subprocess
import sys


def run_command(command: list[str], cwd=None, env=None):
    try:
        print(command)
        subprocess.run(command, check=True, cwd=cwd, env=env)
    except subprocess.CalledProcessError as e:
        print(f"Error: Command failed with exit code {e.returncode}")
        sys.exit(e.returncode)


def run_envsetup_command(command: list[str], cangjie_envsetup: Path, cwd=None, env=None):
    quoted_command = " ".join(shlex.quote(str(part)) for part in command)
    shell_command = f"source {shlex.quote(str(cangjie_envsetup))} && {quoted_command}"
    run_command(["bash", "-lc", shell_command], cwd=cwd, env=env)


def clean(build_dir: Path):
    if build_dir.exists():
        print(f"Cleaning {build_dir}...")
        shutil.rmtree(build_dir)
    else:
        print("Nothing to clean.")


def build_plugin(project_dir: Path, build_dir: Path, cangjie_envsetup: Path, cangjie_stdx_root: Path, env):
    build_dir.mkdir(parents=True, exist_ok=True)
    plugin = build_dir / "libhotfix-plugin-cj.so"

    # Difference from the C++ build.py: the source plugin is a CMake target,
    # while plugin-cj is itself Cangjie source. Build it directly with cjc here
    # instead of delegating to another .sh file.
    run_envsetup_command([
        "cjc",
        "--output-type=dylib",
        "-p", project_dir / "src",
        "--import-path", cangjie_stdx_root,
        "-L", cangjie_stdx_root / "stdx",
        "-lstdx.chir",
        "-lstdx.plugin.manager",
        "--output", plugin,
    ], cangjie_envsetup, cwd=project_dir, env=env)
    print(f"Built: {plugin}")


def run_unit_tests(project_dir: Path, build_dir: Path, cangjie_envsetup: Path, cangjie_stdx_root: Path, env):
    test_bin = build_dir / "test" / "type_filter_test"
    test_bin.parent.mkdir(parents=True, exist_ok=True)

    # Difference from the C++ build.py: the original unit test is registered in
    # CTest by CMake. plugin-cj has a Cangjie unittest file, so build and run the
    # test binary directly from build.py.
    run_envsetup_command([
        "cjc",
        project_dir / "src" / "type_filter.cj",
        project_dir / "test" / "unit" / "type_filter_test.cj",
        "--import-path", cangjie_stdx_root,
        "-L", cangjie_stdx_root / "stdx",
        "-lstdx.chir",
        "-lstdx.unittest",
        "-o", test_bin,
    ], cangjie_envsetup, cwd=project_dir, env=env)
    run_envsetup_command([test_bin], cangjie_envsetup, cwd=project_dir, env=env)


def build(args, project_dir: Path, build_dir: Path):
    env = os.environ.copy()
    toolchain = Path(args.cangjie_toolchain_path).resolve()
    cangjie_envsetup = toolchain / "envsetup.sh"
    cangjie_stdx_root = Path(args.cangjie_stdx_root).resolve() if args.cangjie_stdx_root else Path(
        "/home/s00827109/Projects/cangjie_stdx/target/linux_x86_64_cjnative/dynamic"
    )

    if not cangjie_envsetup.is_file():
        print(f"{toolchain} is not a proper Cangjie toolchain dir.")
        sys.exit(1)

    print(f"Project directory: {project_dir}")
    print(f"Build directory:   {build_dir}")

    env["CANGJIE_STDX_ROOT"] = str(cangjie_stdx_root)
    env["LD_LIBRARY_PATH"] = f"{build_dir}:{cangjie_stdx_root / 'stdx'}:{env.get('LD_LIBRARY_PATH', '')}"

    build_plugin(project_dir, build_dir, cangjie_envsetup, cangjie_stdx_root, env)

    if args.run_tests:
        print("------------------------------------------------")
        print("Running unit tests...")
        print("------------------------------------------------")
        run_unit_tests(project_dir, build_dir, cangjie_envsetup, cangjie_stdx_root, env)

        print("------------------------------------------------")
        print(f"Running functional tests in {args.run_tests} mode")
        print("------------------------------------------------")

        ext = "expected.stub" if args.run_tests == "stub" else "expected"
        run_command([
            str(project_dir / "test" / "run_tests.sh"),
            ext,
            str(build_dir),
            str(toolchain),
        ], cwd=project_dir / "test" / "test_data", env=env)


def main():
    parser = argparse.ArgumentParser(description="build / clean")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="build the project")
    build_parser.add_argument(
        "cangjie_toolchain_path",
        help="Path to cangjie toolchain",
    )
    build_parser.add_argument("-t", "--build-type",
                              choices=["debug", "release"],
                              default="debug",
                              help="Build configuration (accepted for source build.py compatibility)")
    build_parser.add_argument(
        "--cangjie-stdx-root",
        help="Path to built stdx dynamic root; defaults to build.sh CANGJIE_STDX_ROOT",
    )
    build_parser.add_argument(
        "--run-tests",
        choices=["normal", "stub"],
        help="Run unit tests and selected functional tests after build",
    )
    build_parser.add_argument("-j", "--jobs",
                              type=int,
                              default=multiprocessing.cpu_count(),
                              help=f"Number of parallel jobs (accepted for source build.py compatibility; plugin-cj build is single cjc invocation)")

    subparsers.add_parser("clean", help="clean build artifacts")

    args = parser.parse_args()
    project_dir = Path(__file__).parent.resolve()
    build_dir = project_dir / "output"

    if args.command == "clean":
        clean(build_dir)
    elif args.command == "build":
        build(args, project_dir, build_dir)


if __name__ == "__main__":
    main()
