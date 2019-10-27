package(default_visibility = ["//dat2pb:internal"])

package_group(
    name = "internal",
    packages = ["//dat2pb/..."],
)

cc_proto_library(
    name = "romdat_cc_proto",
    deps = [":romdat_proto"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "romdat_proto",
    srcs = ["romdat.proto"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "parser",
    visibility = ["//visibility:public"],
    srcs = ["parser.cc"],
    hdrs = ["parser.h"],
    deps = [
        ":romdat_cc_proto",
        "@rhutil//rhutil:status",
        "@abseil//absl/strings",
        "@abseil//absl/strings:str_format",
        "@libxml//:libxml2",
    ],
)

cc_test(
    name = "parser_test",
    srcs = ["parser_test.cc"],
    deps = [
        ":parser",
        ":text_format",
        "@googletest//:gtest_main",
        "@rhutil//rhutil/testing:assertions",
        "@rhutil//rhutil/testing:protobuf_assertions",
        "@rhutil//rhutil:file",
    ],
    data = glob([
        "testdata/*.dat",
        "testdata/*.textproto"
    ]),
)

cc_binary(
    name = "dat2pb",
    srcs = ["main.cc"],
    deps = [
        ":parser",
        ":romdat_cc_proto",
        "@abseil//absl/flags:parse",
        "@abseil//absl/strings",
        "@abseil//absl/types:span",
        "@com_google_protobuf//:protobuf",
        "@rhutil//rhutil:file",
        "@rhutil//rhutil:status",
    ],
)

cc_library(
    name = "text_format",
    srcs = ["text_format.cc"],
    hdrs = ["text_format.h"],
    deps = [
        ":romdat_cc_proto",
        "@rhutil//rhutil:status",
        "@com_google_protobuf//:protobuf",
        "@abseil//absl/strings:str_format",
    ],
)
