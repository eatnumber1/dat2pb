load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")

filegroup(
    name = "libxml_all",
    srcs = glob(["**"]),
)

configure_make(
    name = "libxml2",
    visibility = ["//visibility:public"],
    lib_source = ":libxml_all",
    out_include_dir = "include/libxml2",
    configure_env_vars = {
        "CFLAGS": ' '.join([
            "-UDEBUG",
            "-w",
        ]),
        # Workaround for https://github.com/bazelbuild/rules_foreign_cc/issues/185
        "AR": '',
    },
    configure_options = [
        "--with-minimum",
        "--with-reader",
        "--without-zlib",
        "--without-lzma",
        "--with-threads",
        "--with-thread-alloc",
    ],
)
