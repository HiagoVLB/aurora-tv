"""The webOS entry point must preserve argument boundaries before invoking CMake."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class EasyBuildTests(unittest.TestCase):
    def test_cmake_arguments_with_spaces(self):
        root = Path(__file__).resolve().parents[2]
        with tempfile.TemporaryDirectory(prefix="aurora-build-args-") as tmp:
            work = Path(tmp)
            bin_dir = work / "bin"
            bin_dir.mkdir()
            args_file = work / "arguments.jsonl"
            cmake = bin_dir / "cmake"
            cmake.write_text(
                "#!/usr/bin/env python3\n"
                "import json, os, sys\n"
                "with open(os.environ['AURORA_TEST_ARGS_FILE'], 'a') as f:\n"
                "    f.write(json.dumps(sys.argv[1:]) + '\\n')\n"
            )
            cpack = bin_dir / "cpack"
            cpack.write_text("#!/bin/sh\nexit 0\n")
            cmake.chmod(0o755)
            cpack.chmod(0o755)
            toolchain = work / "toolchain.cmake"
            toolchain.touch()
            env = dict(os.environ, CI="1", PATH=f"{bin_dir}{os.pathsep}{os.environ['PATH']}",
                       TOOLCHAIN_FILE=str(toolchain), CMAKE_BINARY_DIR=str(work / "build with spaces"),
                       AURORA_TEST_ARGS_FILE=str(args_file))
            options = ["-DWEBOS_APPINFO_TITLE=Aurora Test Build", "-DCMAKE_PREFIX_PATH=/a path/with spaces"]
            subprocess.run(["bash", "scripts/webos/easy_build.sh", *options], cwd=root,
                           env=env, check=True, capture_output=True, text=True)
            calls = [json.loads(line) for line in args_file.read_text().splitlines()]
            self.assertEqual(calls[0][-2:], options)
            self.assertEqual(calls[1], ["--build", env["CMAKE_BINARY_DIR"]])


if __name__ == "__main__":
    unittest.main()
