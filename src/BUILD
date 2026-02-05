load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "lib",
    hdrs = ["lib.h"],
    includes = ["."],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    copts = [
        "-std=c++23",
    ],
    linkopts = [
        "-pthread",
        "-lcantera_shared",
        "-lfmt",
        "-lpthread"
    ],
    deps = [":lib"],
)

cc_binary(
    name = "generate_cfd",
    srcs = ["generate_cfd.cpp"],
    copts = [
        "-std=c++23",
    ],
    linkopts = [

    ]
)
