from pathlib import Path
import argparse
import multiprocessing
import os
import shutil
import shlex
import subprocess
import sys


def run_command(command: list[str], cwd=None, env=None):
    command = [str(part) for part in command]
    try:
        print(command)
        subprocess.run(command, check=True, cwd=cwd, env=env)
    except subprocess.CalledProcessError as e:
        print(f"Error: Command failed with exit code {e.returncode}")
        sys.exit(e.returncode)


def run_command_capture(command: list[str], cwd=None, env=None) -> str:
    command = [str(part) for part in command]
    try:
        result = subprocess.run(command, check=True, cwd=cwd, env=env, capture_output=True, text=True)
    except subprocess.CalledProcessError as e:
        if e.stdout:
            print(e.stdout, end="")
        if e.stderr:
            print(e.stderr, end="", file=sys.stderr)
        print(f"Error: Command failed with exit code {e.returncode}")
        sys.exit(e.returncode)
    return result.stdout


def run_command_result(command: list[str], cwd=None, env=None) -> subprocess.CompletedProcess:
    command = [str(part) for part in command]
    print(command)
    return subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True)

def source_envsetup(cangjie_envsetup: Path, env) -> dict[str, str]:
    shell_command = f"source {shlex.quote(str(cangjie_envsetup))} >/dev/null && env -0"
    result = subprocess.run(["bash", "-lc", shell_command], capture_output=True, env=env)
    if result.returncode != 0:
        print(f"Error: Failed to source {cangjie_envsetup}")
        if result.stderr:
            print(result.stderr.decode(errors="replace"), end="")
        sys.exit(result.returncode)

    sourced_env = env.copy()
    for item in result.stdout.split(b"\0"):
        if not item:
            continue
        key, value = item.split(b"=", 1)
        sourced_env[os.fsdecode(key)] = os.fsdecode(value)
    return sourced_env

def cjc_is_darwin(env) -> bool:
    result = subprocess.run(["cjc", "-v"], check=True, capture_output=True, text=True, env=env)
    return "darwin" in (result.stdout + result.stderr).lower()

def plugin_path(build_dir: Path, env) -> Path:
    suffix = ".dylib" if cjc_is_darwin(env) else ".so"
    return build_dir / ("libhotfix-plugin" + suffix)

def clean(build_dir: Path):
    if build_dir.exists():
        print(f"Cleaning {build_dir}...")
        shutil.rmtree(build_dir)
    else:
        print("Nothing to clean.")


def build_plugin(project_dir: Path, build_dir: Path, cangjie_stdx_root: Path, args, env):
    build_dir.mkdir(parents=True, exist_ok=True)
    plugin = plugin_path(build_dir, env)

    # Difference from the C++ build.py: the source plugin is a CMake target,
    # while plugin-cj is itself Cangjie source. Build it directly with cjc here
    # instead of delegating to another .sh file.
    run_command([
        "cjc",
        "-O2",
        "-j", args.jobs,
        "--output-type=dylib",
        "-p", project_dir / "src",
        "--import-path", cangjie_stdx_root,
        "-L", cangjie_stdx_root / "stdx",
        "-lstdx.chir",
        "-lstdx.plugin.manager",
        "--output", plugin,
    ], cwd=project_dir, env=env)
    print(f"Built: {plugin}")


def run_unit_tests(project_dir: Path, build_dir: Path, cangjie_stdx_root: Path, env) -> list[str]:
    failed_tests: list[str] = []
    test_bin = build_dir / "test" / "type_filter_test"
    test_bin.parent.mkdir(parents=True, exist_ok=True)

    # Difference from the C++ build.py: the original unit test is registered in
    # CTest by CMake. plugin-cj has a Cangjie unittest file, so build and run the
    # test binary directly from build.py.
    build_result = run_command_result([
        "cjc",
        "-O2",
        project_dir / "src" / "type_filter.cj",
        project_dir / "test" / "unit" / "type_filter_test.cj",
        "--import-path", cangjie_stdx_root,
        "-L", cangjie_stdx_root / "stdx",
        "-lstdx.chir",
        "-lstdx.unittest",
        "-o", test_bin,
    ], cwd=project_dir, env=env)
    if build_result.returncode != 0:
        if build_result.stdout:
            print(build_result.stdout, end="")
        if build_result.stderr:
            print(build_result.stderr, end="", file=sys.stderr)
        failed_tests.append("unit/type_filter_test.cj (build)")
        return failed_tests

    run_result = run_command_result([test_bin], cwd=project_dir, env=env)
    if run_result.stdout:
        print(run_result.stdout, end="")
    if run_result.stderr:
        print(run_result.stderr, end="", file=sys.stderr)
    if run_result.returncode != 0:
        failed_tests.append("unit/type_filter_test.cj")
    return failed_tests


def normalize_diff_text(text: str) -> list[str]:
    return [line.rstrip() for line in text.rstrip("\n").splitlines()]


def cleanup_functional_test_outputs(test_data_dir: Path):
    for path in test_data_dir.glob("*_CHIR"):
        if path.is_dir():
            shutil.rmtree(path)
    for pattern in ("*.cjo", "*.cjo.flag"):
        for path in test_data_dir.glob(pattern):
            path.unlink()


def parse_filter_tests(filter_tests: str | None) -> set[str] | None:
    if not filter_tests:
        return None
    return {test.strip() for test in filter_tests.split(",") if test.strip()}


def test_matches_filter(test_file: Path, filter_tests: set[str] | None) -> bool:
    if filter_tests is None:
        return True
    return test_file.name in filter_tests or test_file.stem in filter_tests


def run_functional_tests(project_dir: Path, build_dir: Path, run_tests_mode: str, filter_tests: set[str] | None, env):
    failed_tests: list[str] = []
    test_data_dir = project_dir / "test" / "functional"
    test_env = env.copy()
    test_env["HOTFIX_TEST_MODE"] = "1"
    if run_tests_mode == "stub":
        test_env["HOTFIX_STUB_TEST"] = "1"
        expected_ext = "expected.stub"
    else:
        test_env.pop("HOTFIX_STUB_TEST", None)
        expected_ext = "expected"

    print("Building patchable hotfix lib")
    hotfix_lib = test_data_dir / "lib" / "hotfix.a"
    run_command(["cjc", "-p", "hotfix", "--output-type=staticlib", "-o", hotfix_lib.name],
                cwd=test_data_dir / "lib", env=test_env)

    plugin = plugin_path(build_dir, test_env)
    all_test_files = sorted(test_data_dir.glob("*.cj"))
    if filter_tests:
        available_tests = {test_file.name for test_file in all_test_files} | {test_file.stem for test_file in all_test_files}
        missing_tests = filter_tests - available_tests
        if missing_tests:
            print(f"Fail. Unknown tests in --filter-tests: {','.join(sorted(missing_tests))}")
            return sorted(missing_tests)

    test_files = [test_file for test_file in all_test_files if test_matches_filter(test_file, filter_tests)]

    for test_file in test_files:
        print(f"Test file: {test_file.name}")
        expected_files = sorted(test_data_dir.rglob(f"{test_file.name}.{expected_ext}"))
        if not expected_files:
            print(f"Fail. File {test_file.name}.{expected_ext} to check results was not found")
            failed_tests.append(test_file.name)
            continue
        expected = expected_files[0]

        print(f"File to check results: {expected.relative_to(test_data_dir)}")
        cleanup_functional_test_outputs(test_data_dir)

        # The public stdx.chir API used by plugin-cj does not expose the source
        # file lookup used by the C++ plugin, so pass the equivalent map path.
        test_env["HOTFIX_STUB_MAP_FILE"] = f"{test_file.name}.stub.map"

        compile_result = run_command_result([
            "cjc",
            test_file.name,
            "lib/hotfix.a",
            "--import-path", "lib",
            "--plugin", plugin,
            "--dump-chir",
        ], cwd=test_data_dir, env=test_env)
        if compile_result.stdout:
            print(compile_result.stdout, end="")
        if compile_result.stderr:
            print(compile_result.stderr, end="", file=sys.stderr)
        if compile_result.returncode != 0:
            print("Failed")
            failed_tests.append(test_file.name)
            cleanup_functional_test_outputs(test_data_dir)
            continue

        run_result = run_command_result([test_data_dir / "main"], cwd=test_data_dir, env=test_env)
        if run_result.stderr:
            print(run_result.stderr, end="", file=sys.stderr)
        if run_result.returncode != 0:
            if run_result.stdout:
                print(run_result.stdout, end="")
            print("Failed")
            failed_tests.append(test_file.name)
            cleanup_functional_test_outputs(test_data_dir)
            continue
        actual_data = run_result.stdout
        expected_data = expected.read_text()
        cleanup_functional_test_outputs(test_data_dir)

        actual_lines = normalize_diff_text(actual_data)
        expected_lines = normalize_diff_text(expected_data)
        if actual_lines == expected_lines:
            print("Passed")
            continue

        print("Failed")
        failed_tests.append(test_file.name)

    if not failed_tests:
        print("All tests passed")
    return failed_tests


def build(args, project_dir: Path, build_dir: Path):
    env = os.environ.copy()
    toolchain = Path(args.cangjie_toolchain_path).resolve()
    cangjie_envsetup = toolchain / "envsetup.sh"
    cangjie_stdx_root = Path(args.cangjie_stdx_root).resolve()

    if not cangjie_envsetup.is_file():
        print(f"{toolchain} is not a proper Cangjie toolchain dir.")
        sys.exit(1)

    print(f"Project directory: {project_dir}")
    print(f"Build directory:   {build_dir}")

    env = source_envsetup(cangjie_envsetup, env)
    env["LD_LIBRARY_PATH"] = f"{build_dir}:{cangjie_stdx_root / 'stdx'}:{env.get('LD_LIBRARY_PATH', '')}"
    if args.build_type == "debug":
        # C++ Debug builds enable the DEBUG preprocessor path. plugin-cj uses
        # the runtime HOTFIX_DEBUG switch for the same diagnostic behavior, so
        # make build type control it here.
        env["HOTFIX_DEBUG"] = "1"
    else:
        env.pop("HOTFIX_DEBUG", None)

    build_plugin(project_dir, build_dir, cangjie_stdx_root, args, env)

    if args.run_tests:
        failed_tests: list[str] = []
        print("------------------------------------------------")
        print("Running unit tests...")
        print("------------------------------------------------")
        failed_tests.extend(run_unit_tests(project_dir, build_dir, cangjie_stdx_root, env))

        print("------------------------------------------------")
        print(f"Running functional tests in {args.run_tests} mode")
        print("------------------------------------------------")

        failed_tests.extend(run_functional_tests(project_dir, build_dir, args.run_tests, parse_filter_tests(args.filter_tests), env))
        if failed_tests:
            print("Failed tests:")
            for test in failed_tests:
                print(f"  {test}")
            sys.exit(1)


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
                              help="Build configuration (default: debug)")
    build_parser.add_argument(
        "--cangjie-stdx-root",
        required=True,
        help="Path to built stdx dynamic root",
    )
    build_parser.add_argument(
        "--run-tests",
        choices=["normal", "stub"],
        help="Run unit tests and selected functional tests after build",
    )
    build_parser.add_argument(
        "--filter-tests",
        help="Comma-separated functional test names to run; requires --run-tests",
    )
    build_parser.add_argument("-j", "--jobs",
                              type=int,
                              default=multiprocessing.cpu_count(),
                              help=f"Number of parallel jobs (default: {multiprocessing.cpu_count()})")

    subparsers.add_parser("clean", help="clean build artifacts")

    args = parser.parse_args()
    project_dir = Path(__file__).parent.resolve()
    build_dir = project_dir / "output"

    if args.command == "build" and args.filter_tests and not args.run_tests:
        parser.error("--filter-tests requires --run-tests")

    if args.command == "clean":
        clean(build_dir)
    elif args.command == "build":
        build(args, project_dir, build_dir)


if __name__ == "__main__":
    main()
