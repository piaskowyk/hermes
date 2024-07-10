# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import tempfile
from asyncio import create_subprocess_exec, subprocess, TimeoutError, wait_for
from dataclasses import dataclass, field
from typing import List, Optional

from .preprocess import StrictMode
from .progress import TestCaseResult, TestResultCode
from .typing_defs import OptExpectedFailure, PathT

COMPILE_ARGS = ["-test262", "-fno-static-builtins"]
ES6_ARGS = ["-Xes6-promise", "-Xes6-proxy"]
EXTRA_RUN_ARGS = ["-Xhermes-internal-test-methods"]
USE_MICROTASK_FLAG = ["-Xmicrotask-queue"]

TIMEOUT_COMPILER = 200
TIMEOUT_VM = 200


@dataclass
class ExtraCompileVMArgs(object):
    """
    Extra compiler and vm args that may be necessary for some testsuites.
    """

    compile_args: List[str] = field(default_factory=list)
    vm_args: List[str] = field(default_factory=list)


@dataclass
class CompileRunArgs(object):
    test_name: str
    """A succicnt test name for printing out."""
    strict_mode: StrictMode
    """Whether to compile/run with strict mode."""
    binary_directory: PathT
    """Directory path of Hermes executables."""
    expected_failure: OptExpectedFailure
    """Expected failure in test262, see TestCase for more details."""
    disable_handle_san: bool
    """Disable handle sanitizer to improve speed."""
    lazy: bool
    """Force lazy evaluation."""
    shermes: bool
    """Run with shermes."""
    opt: bool
    """Enable optimizer, i.e., use -O flag instead of -O0."""
    extra_compile_vm_args: Optional[ExtraCompileVMArgs] = None
    """Extra compile/run arguments given by specific testsuites."""


async def run(
    base_file_name: str,
    file_to_run: PathT,
    expected_failure_phase: str,
    compile_run_args: CompileRunArgs,
) -> Optional[TestCaseResult]:
    # Run the generated bytecode/native code or source code
    hermes_exe = os.path.join(compile_run_args.binary_directory, "hermes")
    cmd_args = []
    lazy = compile_run_args.lazy
    if not compile_run_args.shermes:
        cmd_args.append(hermes_exe)
    cmd_args += [file_to_run] + ES6_ARGS + EXTRA_RUN_ARGS + USE_MICROTASK_FLAG
    if lazy:
        cmd_args.append("-lazy")
        # In lazy mode, we should apply the same compile flags as compile_and_run_single().
        cmd_args += COMPILE_ARGS
        if StrictMode.STRICT in compile_run_args.strict_mode:
            cmd_args.append("-strict")
    else:
        if not compile_run_args.shermes:
            # Be able to recognize the output file as bytecode.
            cmd_args.append("-b")
    if compile_run_args.extra_compile_vm_args:
        cmd_args += compile_run_args.extra_compile_vm_args.vm_args
        if lazy:
            cmd_args += compile_run_args.extra_compile_vm_args.compile_args
    if compile_run_args.disable_handle_san:
        cmd_args += ["-gc-sanitize-handles=0"]
    env = {"LC_ALL": "en_US.UTF-8"}
    if sys.platform == "linux":
        env["ICU_DATA"] = compile_run_args.binary_directory
    proc = await create_subprocess_exec(
        *cmd_args, env=env, stderr=subprocess.PIPE, stdout=subprocess.PIPE
    )
    stdout, stderr = (None, None)
    try:
        (stdout, stderr) = await wait_for(proc.communicate(), timeout=TIMEOUT_COMPILER)
    except TimeoutError:
        msg = f"FAIL: Execution of binary timed out for {file_to_run}"
        output = f"Run command: {' '.join(cmd_args)}\n"
        # Kill the subprocess and all its child processes
        proc.kill()
        return TestCaseResult(
            compile_run_args.test_name, TestResultCode.EXECUTE_TIMEOUT, msg, output
        )

    output = f"Run command: {' '.join(cmd_args)}\n"
    if stdout:
        output += f"stdout:\n {stdout.decode('utf-8')}"
    if stderr:
        output += f"stderr:\n {stderr.decode('utf-8')}"

    # Check if the run succeeded
    if proc.returncode:
        # Negative return code means that the subprocess is terminated with a
        # system signal. See more details in compile_and_run().
        if proc.returncode < 0:
            msg = f"FAIL: Execution terminated with {proc.returncode}"
            return TestCaseResult(
                compile_run_args.test_name, TestResultCode.EXECUTE_FAILED, msg, output
            )
        elif (lazy and expected_failure_phase == "") or (
            not lazy and expected_failure_phase != "runtime"
        ):
            # In lazy mode, we can't check the exact error phase, since some could
            # be parse phase (e.g., SyntaxError). But we can check that if the
            # test is not expecting any error, the invocation of Hermes shouldn't
            # fail, even in lazy mode. This check is conservative, but may still
            # capture some issues in lazy mode.
            msg = f"FAIL: Execution of {base_file_name} threw unexpected error"
            return TestCaseResult(
                compile_run_args.test_name, TestResultCode.EXECUTE_FAILED, msg, output
            )
        else:
            msg = f"PASS: Execution of {base_file_name} threw an error as expected"
            return TestCaseResult(
                compile_run_args.test_name, TestResultCode.TEST_PASSED, msg
            )
    else:
        if (lazy and expected_failure_phase != "") or (
            not lazy and expected_failure_phase == "runtime"
        ):
            # Similar to above, in lazy mode, we can't check the exact error
            # phase. But we should check that the given test at least failed,
            # if it's expected for a reason.
            msg = f"FAIL: Expected execution of {base_file_name} to throw"
            return TestCaseResult(
                compile_run_args.test_name, TestResultCode.EXECUTE_FAILED, msg, output
            )
    return None


async def compile_with_args(
    test_name: str,
    expect_compile_failure: bool,
    args: List[str],
) -> Optional[TestCaseResult]:
    """
    Run hermesc with given arguments.

    Returns:
    None, if the compilation is successful, or the compilation throws as
        expected.
    TestCaseResult, if the compilation fails (and it's not expected).
    """

    proc = await create_subprocess_exec(
        *args,
        stderr=subprocess.PIPE,
        stdout=subprocess.PIPE,
    )
    stdout, stderr = (None, None)
    try:
        (stdout, stderr) = await wait_for(proc.communicate(), timeout=TIMEOUT_COMPILER)
    except TimeoutError:
        msg = f"FAIL: Compilation timed out, args: {args}"
        output = f"Run command: {' '.join(args)}"
        proc.kill()
        return TestCaseResult(test_name, TestResultCode.COMPILE_TIMEOUT, msg, output)

    output = f"Run command: {' '.join(args)}\n"
    if stdout:
        output += f"stdout:\n {stdout.decode('utf-8')}"
    if stderr:
        output += f"stderr:\n {stderr.decode('utf-8')}"

    # Check if the compilation succeeded.
    if proc.returncode:
        # Negative return code means that the subprocess is terminated
        # by a system signal (e.g., -11 for SIGSEGV).
        # Hermes may exit with return code:
        # - 0 for success
        # - 1-8 for compiler failure (represented in the CompileStatus
        #   enum)
        # - -1 (converted to 255) in HVM when failed to open file
        # - 1 for all other failures (e.g., used in hermes_fatal())
        # We want to treat terminated exeuction differently to capture
        # bugs like memory corruption.
        if proc.returncode < 0:
            msg = f"FAIL: Execution terminated with {proc.returncode}"
            return TestCaseResult(test_name, TestResultCode.COMPILE_FAILED, msg, output)

        # If compilation failed and it's not expected, consider it a failure.
        if not expect_compile_failure:
            msg = f"FAIL: Compilation failed with command: {args}"
            return TestCaseResult(test_name, TestResultCode.COMPILE_FAILED, msg, output)
    else:
        # If the compliation succeeded but a compilation failure is expected,
        # it's also considered a failure.
        if expect_compile_failure:
            msg = "FAIL: Compilation failure expected on with Hermes"
            return TestCaseResult(test_name, TestResultCode.COMPILE_FAILED, msg, output)

    return None


async def compile_and_run_single(
    js_source_file: PathT,
    compile_run_args: CompileRunArgs,
) -> Optional[TestCaseResult]:
    """
    Compile and run the given source file, return None if it's passed,
    otherwise, return the corresponding TestCaseResult.
    """

    expected_failure_phase = (
        compile_run_args.expected_failure["phase"]
        if compile_run_args.expected_failure
        else ""
    )
    if compile_run_args.lazy:
        # In lazy mode, run the source file directly.
        return await run(
            os.path.basename(js_source_file),
            js_source_file,
            expected_failure_phase,
            compile_run_args,
        )

    file_to_run = f"{js_source_file}.out"
    if compile_run_args.shermes:
        hermes_exe = os.path.join(compile_run_args.binary_directory, "shermes")
    else:
        hermes_exe = os.path.join(compile_run_args.binary_directory, "hermes")
    cmd_args = [
        hermes_exe,
        str(js_source_file),
        "-o" if compile_run_args.shermes else "-out",
        file_to_run,
    ] + COMPILE_ARGS
    if not compile_run_args.shermes:
        cmd_args.append("-emit-binary")

    if compile_run_args.extra_compile_vm_args:
        cmd_args += compile_run_args.extra_compile_vm_args.compile_args

    if StrictMode.STRICT in compile_run_args.strict_mode:
        cmd_args.append("-strict")

    # Whether compilation is expected to fail.
    expect_compile_failure = expected_failure_phase == "parse"
    if compile_run_args.opt:
        cmd_args.append("-O")
    else:
        cmd_args.append("-O0")
    # If compiling failed (not as expected), return the result immediately.
    if unexpected_compile_result := await compile_with_args(
        compile_run_args.test_name,
        expect_compile_failure,
        cmd_args,
    ):
        return unexpected_compile_result

    # If compilation failure is not expected, we should have the bytecode
    # ready to run.
    if not expect_compile_failure:
        # If run failed (not as expected), return it immediately
        if run_result := await run(
            os.path.basename(js_source_file),
            file_to_run,
            expected_failure_phase,
            compile_run_args,
        ):
            return run_result
    return None


async def compile_and_run(
    js_source_files: List[PathT],
    compile_run_args: CompileRunArgs,
) -> TestCaseResult:
    """
    Run the generated source files with async subprocess and return the
    result.
    """

    for js_source_file in js_source_files:
        # If any file is not passed, return the result immediately.
        if unexpected_result := await compile_and_run_single(
            js_source_file,
            compile_run_args,
        ):
            return unexpected_result

    # All files in js_source_files passed
    return TestCaseResult(compile_run_args.test_name, TestResultCode.TEST_PASSED)


async def run_hermes_simple(
    hermes_exe: PathT, test_name: str, args: List[str]
) -> TestCaseResult:
    """
    Simply invoke hermes on a JS file with given arguments and return the
    output. This is unlike the above `compile_and_run()` function, which does
    various checking such as compilation failure and runtim failure.
    """
    proc = await create_subprocess_exec(
        hermes_exe,
        *args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    stdout, stderr = (None, None)
    try:
        (stdout, stderr) = await wait_for(proc.communicate(), timeout=TIMEOUT_COMPILER)
    except TimeoutError:
        proc.kill()
        msg = "FAIL: Hermes timeout"
        details = f"Run command: {' '.join(args)}"
        return TestCaseResult(test_name, TestResultCode.COMPILE_TIMEOUT, msg, details)

    if proc.returncode:
        msg = "FAIL: Hermes failed to run"
        details = f"Run command: {' '.join(args)}\n"
        details += f"Return code: {proc.returncode}\n"
        details += f"stdout:\n {stdout.decode('utf-8')}"
        details += f"stderr:\n {stderr.decode('utf-8')}"
        return TestCaseResult(test_name, TestResultCode.TEST_FAILED, msg, details)

    # Return the evaluated output (could be JS execution result or dumped AST).
    return TestCaseResult(
        test_name, TestResultCode.TEST_PASSED, "PASS: ", stdout.decode("utf-8").strip()
    )


async def generate_ast(
    test_name: str,
    test_file: PathT,
    binary_path: PathT,
    is_flow: bool,
    transformed: bool,
) -> TestCaseResult:
    """
    Generate the AST for given source file.

    Returns:
    (TestResultCode.TEST_PASSED, JSON) if success.
    (TestResultCode.TEST_FAILED, message) if Hermes failed.
    (TestResultCode.COMPILE_TIMEOUT, message) if Hermes timeout.
    """
    args = []
    if is_flow:
        args.append("-parse-flow")
        args.append("-Xparse-component-syntax")
        args.append("-parse-jsx")
        args.append("-Xinclude-empty-ast-nodes")
    elif "JSX" in test_file:
        args.append("--parse-jsx")
    args.append("-dump-transformed-ast" if transformed else "-dump-ast")
    hermes_exe = os.path.join(binary_path, "hermes")

    # ".source.js" files has the format of "var source = \"...\";", and
    # the value of the 'source' variable should be the input to the parser.
    # So we evaluate the source with Hermes first and then parse the output.
    if test_file.endswith(".source.js"):
        with open(test_file, "rb") as f, tempfile.NamedTemporaryFile() as to_evaluate:
            # append to the original source to print the 'source' variable.
            for line in f:
                to_evaluate.write(line)
            to_evaluate.write(b"print(source);")
            to_evaluate.flush()
            with tempfile.NamedTemporaryFile() as evaluated:
                # evaluate the source to get the actual test input.
                eval_args = [to_evaluate.name]
                result = await run_hermes_simple(hermes_exe, test_name, eval_args)
                if result.code != TestResultCode.TEST_PASSED:
                    return result
                # get rid of the newline added by print().
                evaluated.write(result.output.encode("utf-8"))
                evaluated.flush()
                # run the test through Hermes parser.
                return await run_hermes_simple(
                    hermes_exe, test_name, args + [evaluated.name]
                )
    return await run_hermes_simple(hermes_exe, test_name, args + [test_file])
