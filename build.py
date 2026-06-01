from pathlib import Path
import argparse
import os
import shutil
import subprocess
import multiprocessing
import sys


def run_command(command: list[str], cwd=None):
    try:
        print(command)
        subprocess.run(command, check=True, cwd=cwd)
    except subprocess.CalledProcessError as e:
        print(f"Error: Command failed with exit code {e.returncode}")
        sys.exit(e.returncode)


def clean(build_dir):
    if os.path.exists(build_dir):
        print(f"Cleaning {build_dir}...")
        shutil.rmtree(build_dir)
    else:
        print("Nothing to clean.")


def build(args, project_dir, build_dir):
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    cangjie_path = os.path.abspath(args.cangjie_toolchain_path)

    cmake_cmd = [
        "cmake",
        project_dir,
        f"-DCANGJIE_PATH={cangjie_path}",
        f"-DCMAKE_BUILD_TYPE={args.build_type.capitalize()}"
    ]

    if args.run_tests:
        cmake_cmd.append(f"-DTEST=ON")
        if args.run_tests == "stub":
            cmake_cmd.append(f"-DSTUB_TEST=ON")
        else:
            cmake_cmd.append(f"-DSTUB_TEST=OFF")
    else:
        cmake_cmd.append(f"-DTEST=OFF")

    if args.run_tests == "stub":
        cmake_cmd.append(f"-DSTUB_TEST=ON")
    else:
        cmake_cmd.append(f"-DSTUB_TEST=OFF")


    local_googletests_path = os.environ.get("LOCAL_GOOGLETESTS_PATH")
    if local_googletests_path is not None:
        assert os.path.isdir(local_googletests_path)
        cmake_cmd.append(f"-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST={local_googletests_path}")

    make_cmd = ["make", f"-j{args.jobs}"]

    print(f"--- Configuring ({args.build_type}) ---")
    run_command(cmake_cmd, cwd=build_dir)

    print(f"--- Building with {args.jobs} jobs ---")
    run_command(make_cmd, cwd=build_dir)

    if args.run_tests:
        print("------------------------------------------------")
        print("Running tests via CTest...")
        print("------------------------------------------------")
        run_command(["ctest", "--output-on-failure", f"-j{args.jobs}"], cwd=build_dir)

        print( "------------------------------------------------")
        print(f"Running functional tests in {args.run_tests} mode")
        print( "------------------------------------------------")

        if args.run_tests == "stub":
            ext = "expected.stub"
        else:
            ext = "expected"

        run_command([f"{project_dir}/test/run_tests.sh", ext, build_dir, cangjie_path],
                    cwd=f"{project_dir}/test/test_data")


def main():
    parser = argparse.ArgumentParser(description="build / clean")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="build the project")
    build_parser.add_argument("cangjie_toolchain_path",
                              help="Path to cangjie toolchain")
    build_parser.add_argument("-t", "--build-type",
                              choices=["debug", "release"],
                              default="debug",
                              help="Build configuration (default: debug)")
    build_parser.add_argument("--run-tests",
                              choices=["normal", "stub"],
                              help="Run tests after successful build")
    build_parser.add_argument("-j", "--jobs",
                              type=int,
                              default=multiprocessing.cpu_count(),
                              help=f"Number of parallel jobs (default: {multiprocessing.cpu_count()})")

    subparsers.add_parser("clean", help="clean build artifacts")

    args = parser.parse_args()

    project_dir = str(Path(__file__).parent.resolve())
    build_dir = project_dir + "/output"

    if args.command == "clean":
        clean(build_dir)
    elif args.command == "build":
        print(f"Project directory: {project_dir}")
        print(f"Build directory:   {build_dir}")

        build(args, project_dir, build_dir)


if __name__ == "__main__":
    main()
