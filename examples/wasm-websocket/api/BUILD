load("@io_bazel_rules_go//proto:def.bzl", "go_proto_library")

proto_library(
    name = "api_proto",
    srcs = ["api.proto"],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "api_cc_proto",
    deps = [
        ":api_proto",
    ],
    visibility = ["//visibility:public"],
)