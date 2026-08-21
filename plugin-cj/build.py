from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys

CJPM_MODULE_DIR = "hotfix-plugin"
CJPM_PACKAGE_NAME = "hotfixplugin"

def run_command(command: list[str], cwd=None, env=None):
    command = [str(part) for part in command]
    try:
        print(command)
        subprocess.run(command, check=True, cwd=cwd, env=env)
    except subprocess.CalledProcessError as e:
        print(f"Error: Command failed with exit code {e.returncode}")
        sys.exit(e.returncode)

def run_command_result(command: list[str], cwd=None, env=None) -> subprocess.CompletedProcess:
    command = [str(part) for part in command]
    print(command)
    return subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True)

def require_tool(tool: str, env) -> None:
    if shutil.which(tool, path=env.get("PATH")) is None:
        print(f"{tool} is not found in PATH.")
        sys.exit(1)

def cjpm_module_dir(project_dir: Path) -> Path:
    return project_dir / CJPM_MODULE_DIR

def cjpm_target_dir(build_dir: Path) -> Path:
    return build_dir / "cjpm-target"

def cjpm_profile_dir(build_dir: Path, args=None) -> Path:
    if args is not None and args.build_type == "debug":
        return cjpm_target_dir(build_dir) / "debug"
    return cjpm_target_dir(build_dir) / "release"

def cjpm_artifact_path(build_dir: Path, env, args=None) -> Path:
    suffix = ".dylib" if cjc_is_darwin(env) else ".so"
    return cjpm_profile_dir(build_dir, args) / CJPM_PACKAGE_NAME / ("lib" + CJPM_PACKAGE_NAME + suffix)

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

def build_plugin(project_dir: Path, build_dir: Path, args, env):
    build_dir.mkdir(parents=True, exist_ok=True)
    plugin = plugin_path(build_dir, env)

    module_dir = cjpm_module_dir(project_dir)
    if not (module_dir / "cjpm.toml").exists():
        print(f"cjpm module not found: {module_dir}")
        sys.exit(1)

    command = ["cjpm", "build", "-j", "4", "--target-dir", cjpm_target_dir(build_dir)]
    if args.build_type == "debug":
        command.append("-g")
    run_command(command, cwd=module_dir, env=env)

    artifact = cjpm_artifact_path(build_dir, env, args)
    if not artifact.exists():
        print(f"cjpm build finished but artifact was not found: {artifact}")
        sys.exit(1)
    shutil.copy2(artifact, plugin)
    print(f"Built: {plugin}")

def run_unit_tests(project_dir: Path, build_dir: Path, env):
    module_dir = cjpm_module_dir(project_dir)
    command = ["cjpm", "test", "-j", "4", "--no-color", "--target-dir", cjpm_target_dir(build_dir)]
    test_result = run_command_result(command, cwd=module_dir, env=env)
    if test_result.stdout:
        print(test_result.stdout, end="")
    if test_result.stderr:
        print(test_result.stderr, end="", file=sys.stderr)
    if test_result.returncode != 0:
        sys.exit(test_result.returncode)

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

def run_functional_tests_at_compile_level(
    project_dir: Path,
    build_dir: Path,
    run_tests_mode: str,
    filter_tests: set[str] | None,
    test_compile_level: str,
    env,
):
    failed_tests: list[str] = []
    test_data_dir = cjpm_module_dir(project_dir) / "test" / "functional"
    test_env = env.copy()
    if run_tests_mode == "stub":
        expected_ext = "expected.stub"
    else:
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
            f"-{test_compile_level}",
            test_file.name,
            "lib/hotfix.a",
            "--import-path", "lib",
            "--plugin", plugin,
            #"--dump-chir",
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

        print("Comparing results")
        actual_data = run_result.stdout
        expected_data = expected.read_text()
        cleanup_functional_test_outputs(test_data_dir)

        actual_lines = normalize_diff_text(actual_data)
        expected_lines = normalize_diff_text(expected_data)
        if actual_lines == expected_lines:
            print("Passed")
            continue
        print("Differs")
        print("Actual")
        print(f"{actual_data}")
        print("Expected")
        print(f"{expected_data}")
        failed_tests.append(test_file.name)

    return failed_tests

def run_functional_tests(
    project_dir: Path,
    build_dir: Path,
    run_tests_mode: str,
    filter_tests: set[str] | None,
    test_compile_levels: list[str],
    env,
):
    all_failed_tests: list[str] = []
    for test_compile_level in test_compile_levels:
        print("------------------------------------------------")
        print(f"Running functional tests with -{test_compile_level}")
        print("------------------------------------------------")
        failed_tests = run_functional_tests_at_compile_level(
            project_dir,
            build_dir,
            run_tests_mode,
            filter_tests,
            test_compile_level,
            env,
        )
        if failed_tests:
            all_failed_tests.extend(f"{test_compile_level}: {test}" for test in failed_tests)
    return all_failed_tests

def build(args, project_dir: Path, build_dir: Path):
    env = os.environ.copy()

    if "CANGJIE_HOME" not in env:
        print("CANGJIE_HOME is not set.")
        sys.exit(1)

    if "CANGJIE_STDX_PATH" not in env:
        print("CANGJIE_STDX_PATH is not set.")
        sys.exit(1)

    require_tool("cjc", env)
    require_tool("cjpm", env)

    cangjie_stdx_path = Path(env["CANGJIE_STDX_PATH"]).resolve()
    env["HOTFIX_STUB_TEST"] = "true" if args.run_tests == "stub" else "false"
    env.setdefault("HOTFIX_PATCH_ALL", "true")

    print(f"Project directory: {project_dir}")
    print(f"Build directory:   {build_dir}")
    print(f"cjpm module:       {cjpm_module_dir(project_dir)}")

    env["LD_LIBRARY_PATH"] = (
        f"{build_dir}:{cjpm_artifact_path(build_dir, env, args).parent}:"
        f"{cangjie_stdx_path}:{env.get('LD_LIBRARY_PATH', '')}"
    )

    build_plugin(project_dir, build_dir, args, env)

    if args.run_tests:
        print("------------------------------------------------")
        print("Running unit tests...")
        print("------------------------------------------------")
        run_unit_tests(project_dir, build_dir, env)

        print("------------------------------------------------")
        print(f"Running functional tests in {args.run_tests} mode")
        print("------------------------------------------------")
        failed_tests: list[str] = []
        filter_tests = parse_filter_tests(args.filter_tests)
        failed_tests.extend(run_functional_tests(
            project_dir,
            build_dir,
            args.run_tests,
            filter_tests,
            args.test_compile_level,
            env,
        ))
        if failed_tests:
            print("Failed tests:")
            for test in failed_tests:
                print(f"  {test}")
            sys.exit(1)
        else:
            print("All tests passed")

def main():
    parser = argparse.ArgumentParser(description="build / clean")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="build the project")
    build_parser.add_argument("-t", "--build-type",
                              choices=["debug", "release"],
                              default="debug",
                              help="Build configuration (default: debug)")
    build_parser.add_argument(
        "--run-tests",
        choices=["normal", "stub"],
        help="Run unit tests and selected functional tests after build",
    )
    build_parser.add_argument(
        "--filter-tests",
        help="Comma-separated functional test names to run; requires --run-tests",
    )
    build_parser.add_argument(
        "--test-compile-level",
        nargs="+",
        choices=["O0", "O1", "O2"],
        default=["O0", "O1", "O2"],
        help="Compile optimization levels for functional tests (default: O0 O1 O2)",
    )

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
