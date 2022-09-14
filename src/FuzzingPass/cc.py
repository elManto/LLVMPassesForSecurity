#!/usr/bin/env python3

import subprocess
import sys
import os

script_dir = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
aflpp_path = os.path.join(script_dir, "..", "..", "AFLplusplus")
runtime_path = os.path.join(aflpp_path, "./afl-compiler-rt.o")


assert(os.path.isfile(runtime_path) and "runtime file doesn't exist")

is_cxx = "++" in sys.argv[0]

def cc_exec(args):
    if os.getenv("CUSTOM_CC"):
        cc_name = os.environ["CUSTOM_CC"]
    else:
        cc_name = "clang-12"
    if is_cxx:
        if os.getenv("CUSTOM_CXX"):
            cc_name = os.environ["CUSTOM_CXX"]
        else:
            cc_name = "clang++-12"
    argv = [cc_name] + args
    print(" ".join(argv))
    return subprocess.run(argv)


def common_opts():
    # export CFLAGS="-g -fno-inline-functions -fno-unroll-loops -fno-discard-value-names"
    return [
      "-g",
      "-fno-inline-functions",
      "-fno-discard-value-names",
    ]

def cc_mode():
    args = common_opts()
    args += sys.argv[1:]

    args += [
      "-Xclang", "-load", "-Xclang", os.path.join(script_dir, "./FuzzingPass/FuzzingPass.so"),
    ]
    return cc_exec(args)

def ld_mode():
    args = common_opts()
    
    args += sys.argv[1:]
    args += [runtime_path]

    args += [
      "-Xclang", "-load", "-Xclang", os.path.join(script_dir, "./FuzzingPass/FuzzingPass.so"),
    ]

    #args += ["-ldl"]
    
    return cc_exec(args)

def is_ld_mode():
    return not ("--version" in sys.argv or "--target-help" in sys.argv or
                "-c" in sys.argv or "-E" in sys.argv or "-S" in sys.argv or
                "-shared" in sys.argv)


if len(sys.argv) <= 1:
  cc_exec(sys.argv[1:])
elif is_ld_mode():
    ld_mode()
else:
    cc_mode()
