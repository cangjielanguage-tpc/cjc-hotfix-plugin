from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROOT_DIR = Path(__file__).parent.resolve()
PLUGIN_PROJECT_DIR = ROOT_DIR / "cjc-plugin"
PATCH_GEN_PROJECT_DIR = ROOT_DIR / "patch-gen"
OUTPUT_DIR = ROOT_DIR / "output"
PLUGIN_BUILD_DIR = OUTPUT_DIR / "cjc-plugin"
PATCH_GEN_BUILD_DIR = OUTPUT_DIR / "patch-gen"


def run_command(command: list[object], cwd: Path | None = None, env=None) -> None:
    command = [str(part) for part in command]
    print(command)
    try:
        subprocess.run(command, check=True, cwd=cwd, env=env)
    except subprocess.CalledProcessError as error:
        print(f"Error: command failed with exit code {error.returncode}")
        sys.exit(error.returncode)


def run_command_result(
    command: list[object],
    cwd: Path | None = None,
    env=None,
    *,
    combine_output: bool = False,
    stream_output: bool = False,
) -> subprocess.CompletedProcess:
    command = [str(part) for part in command]
    print(command, flush=True)
    stderr = subprocess.STDOUT if combine_output else (None if stream_output else subprocess.PIPE)
    return subprocess.run(
        command,
        cwd=cwd,
        env=env,
        stdout=None if stream_output else subprocess.PIPE,
        stderr=stderr,
        text=True,
    )


def require_tool(tool: str, env) -> None:
    if shutil.which(tool, path=env.get("PATH")) is None:
        print(f"{tool} is not found in PATH.")
        sys.exit(1)


def cjc_is_darwin(env) -> bool:
    result = subprocess.run(
        ["cjc", "-v"], check=True, capture_output=True, text=True, env=env
    )
    return "darwin" in (result.stdout + result.stderr).lower()


def plugin_path(env) -> Path:
    suffix = ".dylib" if cjc_is_darwin(env) else ".so"
    return PLUGIN_BUILD_DIR / f"libhotfix-plugin{suffix}"


def cjpm_target_dir() -> Path:
    return PLUGIN_BUILD_DIR / "cjpm-target"


def cjpm_profile_dir(args) -> Path:
    profile = "debug" if args.build_type == "debug" else "release"
    return cjpm_target_dir() / profile


def cjpm_artifact_path(args, env) -> Path:
    suffix = ".dylib" if cjc_is_darwin(env) else ".so"
    return cjpm_profile_dir(args) / "hotfixplugin" / f"libhotfixplugin{suffix}"


def clean() -> None:
    if OUTPUT_DIR.exists():
        print(f"Cleaning {OUTPUT_DIR}...")
        shutil.rmtree(OUTPUT_DIR)
    else:
        print("Nothing to clean.")


def build_plugin_artifact(args, env) -> None:
    PLUGIN_BUILD_DIR.mkdir(parents=True, exist_ok=True)
    command: list[object] = [
        "cjpm",
        "build",
        "-j",
        "4",
        "--target-dir",
        cjpm_target_dir(),
    ]
    if args.build_type == "debug":
        command.append("-g")
    run_command(command, cwd=PLUGIN_PROJECT_DIR, env=env)

    artifact = cjpm_artifact_path(args, env)
    if not artifact.exists():
        print(f"cjpm build finished but artifact was not found: {artifact}")
        sys.exit(1)
    destination = plugin_path(env)
    shutil.copy2(artifact, destination)
    print(f"Built: {destination}")


def run_plugin_unit_tests(env) -> None:
    result = run_command_result(
        ["cjpm", "test", "-j", "4", "--no-color", "--target-dir", cjpm_target_dir()],
        cwd=PLUGIN_PROJECT_DIR,
        env=env,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        sys.exit(result.returncode)


def normalize_text(text: str) -> list[str]:
    return [line.rstrip() for line in text.rstrip("\n").splitlines()]


def parse_filter_tests(filter_tests: str | None) -> set[str] | None:
    if not filter_tests:
        return None
    return {test.strip() for test in filter_tests.split(",") if test.strip()}


def test_matches_filter(test_file: Path, filter_tests: set[str] | None) -> bool:
    return filter_tests is None or test_file.name in filter_tests or test_file.stem in filter_tests


def cleanup_plugin_test_outputs(test_data_dir: Path) -> None:
    for path in test_data_dir.glob("*_CHIR"):
        if path.is_dir():
            shutil.rmtree(path)
    for pattern in ("*.cjo", "*.cjo.flag", "*.chir", "main"):
        for path in test_data_dir.glob(pattern):
            path.unlink()


def cleanup_hotfix_lib(test_data_dir: Path) -> None:
    for name in ("hotfix.a", "hotfix.cjo"):
        path = test_data_dir / "lib" / name
        if path.is_file():
            path.unlink()


def plugin_excluded_tests(test_data_dir: Path, mode: str) -> set[str]:
    exclude_file = test_data_dir / ("exclude_list.stub" if mode == "stub" else "exclude_list")
    return {
        test.strip()
        for test in exclude_file.read_text().splitlines()
        if test.strip()
    }


def run_plugin_functional_tests_at_level(args, compile_level: str, env) -> list[str]:
    failed_tests: list[str] = []
    test_data_dir = PLUGIN_PROJECT_DIR / "test" / "functional"
    test_env = env.copy()
    expected_extension = "expected.stub" if args.run_tests == "stub" else "expected"

    print("Building patchable hotfix lib")
    run_command(
        ["cjc", "-p", "hotfix", "--output-type=staticlib", "-o", "hotfix.a"],
        cwd=test_data_dir / "lib",
        env=test_env,
    )

    filter_tests = parse_filter_tests(args.filter_tests)
    all_test_files = sorted(test_data_dir.glob("*.cj"))
    if filter_tests:
        available = {path.name for path in all_test_files} | {path.stem for path in all_test_files}
        missing = filter_tests - available
        if missing:
            print(f"Fail. Unknown tests in --filter-tests: {','.join(sorted(missing))}")
            cleanup_hotfix_lib(test_data_dir)
            return sorted(missing)

    excluded = plugin_excluded_tests(test_data_dir, args.run_tests)
    test_files = [
        path
        for path in all_test_files
        if test_matches_filter(path, filter_tests)
           and path.name not in excluded
           and path.stem not in excluded
    ]

    for test_file in test_files:
        print(f"Test file: {test_file.name}")
        expected_files = sorted(test_data_dir.rglob(f"{test_file.name}.{expected_extension}"))
        if not expected_files:
            print(f"Fail. File {test_file.name}.{expected_extension} was not found")
            failed_tests.append(test_file.name)
            continue

        cleanup_plugin_test_outputs(test_data_dir)
        test_env["HOTFIX_STUB_MAP_FILE"] = f"{test_file.name}.stub.map"
        compile_result = run_command_result(
            [
                "cjc",
                f"-{compile_level}",
                test_file.name,
                "lib/hotfix.a",
                "--import-path",
                "lib",
                "--plugin",
                plugin_path(test_env),
                "--dump-chir",
            ],
            cwd=test_data_dir,
            env=test_env,
            stream_output=True,
        )
        if compile_result.returncode != 0:
            if compile_result.stdout:
                print(compile_result.stdout, end="")
            if compile_result.stderr:
                print(compile_result.stderr, end="", file=sys.stderr)
            failed_tests.append(test_file.name)
            continue

        run_result = run_command_result([test_data_dir / "main"], cwd=test_data_dir, env=test_env)
        if run_result.stderr:
            print(run_result.stderr, end="", file=sys.stderr)
        if run_result.returncode != 0:
            if run_result.stdout:
                print(run_result.stdout, end="")
            failed_tests.append(test_file.name)
            continue

        actual = run_result.stdout
        expected = expected_files[0].read_text()
        cleanup_plugin_test_outputs(test_data_dir)
        if normalize_text(actual) == normalize_text(expected):
            print("Passed")
        else:
            print(f"Differs\nActual\n{actual}\nExpected\n{expected}")
            failed_tests.append(test_file.name)

    cleanup_hotfix_lib(test_data_dir)
    return failed_tests


def build_plugin(args) -> None:
    env = os.environ.copy()
    require_tool("cjc", env)
    require_tool("cjpm", env)
    env["HOTFIX_STUB_TEST"] = "true" if args.run_tests == "stub" else "false"
    env.setdefault("HOTFIX_PATCH_ALL", "true")
    stdx_path = Path(env["CANGJIE_STDX_PATH"])
    env["LD_LIBRARY_PATH"] = os.pathsep.join(
        (
            str(PLUGIN_BUILD_DIR),
            str(cjpm_artifact_path(args, env).parent),
            str(stdx_path),
            env.get("LD_LIBRARY_PATH", ""),
        )
    )

    build_plugin_artifact(args, env)
    if not args.run_tests:
        return

    print("------------------------------------------------\nRunning unit tests...\n------------------------------------------------")
    run_plugin_unit_tests(env)
    failed_tests: list[str] = []
    for compile_level in args.test_compile_level:
        print(
            "------------------------------------------------\n"
            f"Running plugin functional tests in {args.run_tests} mode with -{compile_level}\n"
            "------------------------------------------------"
        )
        failed_tests.extend(
            f"{compile_level}: {test}"
            for test in run_plugin_functional_tests_at_level(args, compile_level, env)
        )
    report_test_results(failed_tests)


def cleanup_patch_gen_test_outputs(test_data_dir: Path, test_name: str | None = None) -> None:
    patterns = ["*.cjo", "*.chirtxt", "plugin*.chir"]
    if test_name:
        patterns.extend(
            (
                f"lib{test_name}.chir",
                f"lib{test_name}_patched.chir",
                f"difflib{test_name}.chir",
                f"difflib{test_name}_patched.chir",
                f"{test_name}.actual",
            )
        )
    for pattern in patterns:
        for path in test_data_dir.glob(pattern):
            path.unlink()


def patch_gen_expected_file(test_data_dir: Path, test_name: str, compile_level: str) -> Path | None:
    level_specific = sorted(test_data_dir.rglob(f"{test_name}.expected_{compile_level}"))
    if level_specific:
        return level_specific[0]
    print(f"No {test_name}.expected_{compile_level}; using {test_name}.expected")
    generic = sorted(test_data_dir.rglob(f"{test_name}.expected"))
    return generic[0] if generic else None


def run_patch_gen_tests_at_level(
    args, compile_level: str, plugin_file: Path, env
) -> list[str]:
    failed_tests: list[str] = []
    test_data_dir = PATCH_GEN_PROJECT_DIR / "test" / "test_data"
    filter_tests = parse_filter_tests(args.filter_tests)
    all_test_files = sorted(
        path for path in test_data_dir.glob("*.cj") if not path.stem.endswith("_patched")
    )
    if filter_tests:
        available = {path.name for path in all_test_files} | {path.stem for path in all_test_files}
        missing = filter_tests - available
        if missing:
            print(f"Fail. Unknown tests in --filter-tests: {','.join(sorted(missing))}")
            return sorted(missing)

    print("Building patchable hotfix lib")
    run_command(
        ["cjc", "-p", "hotfix", "--output-type=staticlib", "-o", "hotfix.a"],
        cwd=test_data_dir / "lib",
        env=env,
    )

    for test_file in all_test_files:
        if not test_matches_filter(test_file, filter_tests):
            continue
        test_name = test_file.stem
        print(f"Test file: {test_file.name}")
        expected_file = patch_gen_expected_file(test_data_dir, test_name, compile_level)
        if expected_file is None:
            print("Fail. Expected file was not found")
            failed_tests.append(test_file.name)
            continue

        cleanup_patch_gen_test_outputs(test_data_dir, test_name)
        base_chir = f"lib{test_name}.chir"
        compile_base = [
            "cjc",
            "-Woff",
            "unused",
            test_file.name,
            "lib/hotfix.a",
            "--import-path",
            "lib",
            "--output-type=staticlib",
            "--plugin",
            plugin_file,
            f"-{compile_level}",
            "-o",
            base_chir,
            "--emit-chir",
        ]
        base_result = run_command_result(
            compile_base, cwd=test_data_dir, env=env, stream_output=True
        )
        if base_result.returncode != 0:
            if base_result.stdout:
                print(base_result.stdout, end="")
            if base_result.stderr:
                print(base_result.stderr, end="", file=sys.stderr)
            failed_tests.append(test_file.name)
            continue

        patched_file = test_data_dir / f"{test_name}_patched.cj"
        patch_chir = f"difflib{test_name}.chir"
        generator_command: list[object] = [PATCH_GEN_BUILD_DIR / "patch-gen", base_chir]
        if patched_file.is_file():
            patched_chir = f"lib{test_name}_patched.chir"
            compile_patched = compile_base.copy()
            compile_patched[3] = patched_file.name
            compile_patched[-2] = patched_chir
            patched_result = run_command_result(
                compile_patched, cwd=test_data_dir, env=env, stream_output=True
            )
            if patched_result.returncode != 0:
                if patched_result.stdout:
                    print(patched_result.stdout, end="")
                if patched_result.stderr:
                    print(patched_result.stderr, end="", file=sys.stderr)
                failed_tests.append(test_file.name)
                continue
            generator_command.append(patched_chir)
            patch_chir = f"difflib{test_name}_patched.chir"

        generator_result = run_command_result(
            generator_command,
            cwd=test_data_dir,
            env=env,
            combine_output=True,
        )
        actual = generator_result.stdout
        actual_file = test_data_dir / f"{test_name}.actual"
        actual_file.write_text(actual)

        generated_patch = test_data_dir / patch_chir
        if generated_patch.is_file():
            print("Checking patch (.chir) validity using chir-dis")
            disassemble_result = run_command_result(["chir-dis", generated_patch], cwd=test_data_dir, env=env)
            if disassemble_result.returncode != 0:
                if disassemble_result.stdout:
                    print(disassemble_result.stdout, end="")
                if disassemble_result.stderr:
                    print(disassemble_result.stderr, end="", file=sys.stderr)
                failed_tests.append(test_file.name)
                continue

        expected = expected_file.read_text()
        if normalize_text(actual) == normalize_text(expected):
            print("Passed")
            cleanup_patch_gen_test_outputs(test_data_dir, test_name)
        else:
            print(f"Differs\nExpected:\n{expected}\nActual:\n{actual}")
            failed_tests.append(test_file.name)

    cleanup_patch_gen_test_outputs(test_data_dir)
    cleanup_hotfix_lib(test_data_dir)
    return failed_tests


def build_patch_gen(args) -> None:
    env = os.environ.copy()
    for tool in ("cmake", "cjc"):
        require_tool(tool, env)

    plugin_file = Path(args.cjc_plugin_path).expanduser().resolve()
    if not plugin_file.is_file():
        print(f"CJC plugin library was not found: {plugin_file}")
        sys.exit(1)

    PATCH_GEN_BUILD_DIR.mkdir(parents=True, exist_ok=True)
    tests_enabled = args.run_tests and args.build_type == "release"
    cmake_command: list[object] = [
        "cmake",
        PATCH_GEN_PROJECT_DIR,
        f"-DCANGJIE_PATH={env['CANGJIE_HOME']}",
        f"-DCMAKE_BUILD_TYPE={args.build_type.capitalize()}",
        f"-DTEST={'ON' if tests_enabled else 'OFF'}",
    ]
    run_command(cmake_command, cwd=PATCH_GEN_BUILD_DIR, env=env)
    run_command(
        ["cmake", "--build", ".", "--parallel"],
        cwd=PATCH_GEN_BUILD_DIR,
        env=env,
    )

    if not args.run_tests:
        return
    if not tests_enabled:
        print("Skipping patch-gen functional tests in debug mode")
        return

    failed_tests: list[str] = []
    for compile_level in args.test_compile_level:
        print(
            "------------------------------------------------\n"
            f"Running patch-gen functional tests with -{compile_level}\n"
            "------------------------------------------------"
        )
        failed_tests.extend(
            f"{compile_level}: {test}"
            for test in run_patch_gen_tests_at_level(
                args, compile_level, plugin_file, env
            )
        )
    report_test_results(failed_tests)


def report_test_results(failed_tests: list[str]) -> None:
    if failed_tests:
        print("Failed tests:")
        for test in failed_tests:
            print(f"  {test}")
        sys.exit(1)
    print("All tests passed")


def add_shared_build_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "-t",
        "--build-type",
        choices=["debug", "release"],
        default="debug",
        help="Build configuration (default: debug)",
    )
    parser.add_argument(
        "--filter-tests",
        help="Comma-separated functional test names; requires --run-tests",
    )
    parser.add_argument(
        "--test-compile-level",
        nargs="+",
        choices=["O0", "O1", "O2"],
        default=["O0", "O1"], # TODO enable O2 tests when plugin performance is improved
        help="Compiler optimization levels for functional tests (default: O0 O1 O2)",
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Build and test the Cangjie hotfix tools")
    subparsers = parser.add_subparsers(dest="command", required=True)

    plugin_parser = subparsers.add_parser("build-plugin", help="Build the Cangjie plugin")
    add_shared_build_options(plugin_parser)
    plugin_parser.add_argument(
        "--run-tests",
        choices=["normal", "stub"],
        help="Run unit tests and functional tests in normal or stub mode",
    )

    patch_gen_parser = subparsers.add_parser("build-patch-gen", help="Build patch-gen")
    add_shared_build_options(patch_gen_parser)
    patch_gen_parser.add_argument(
        "--plugin",
        dest="cjc_plugin_path",
        required=True,
        metavar="PATH",
        help="Path to the CJC plugin shared library (.so or .dylib)",
    )
    patch_gen_parser.add_argument(
        "--run-tests",
        action="store_true",
        help="Run functional tests after a release build",
    )

    subparsers.add_parser("clean", help="Remove all build artifacts")
    args = parser.parse_args()
    if args.command != "clean" and args.filter_tests and not args.run_tests:
        parser.error("--filter-tests requires --run-tests")

    if args.command == "clean":
        clean()
    elif args.command == "build-plugin":
        build_plugin(args)
    elif args.command == "build-patch-gen":
        build_patch_gen(args)


if __name__ == "__main__":
    main()
