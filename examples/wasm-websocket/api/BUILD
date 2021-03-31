load("@rules_proto//proto:defs.bzl", "proto_library")


proto_library(
    name = "api_proto",
    srcs = ["api.proto","config.proto"],
    visibility = ["//visibility:public"],
    deps = [
        #"@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_proto"
        #"@com_google_protobuf//:struct_proto"
        "@proxy_wasm_cpp_sdk//:proxy_wasm_intrinsics_lite_proto"

    ]
)

cc_proto_library(
    name = "api_cc_proto",
    deps = [
        ":api_proto"
    ],
    visibility = ["//visibility:public"],
)