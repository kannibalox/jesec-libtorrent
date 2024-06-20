load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "all_srcs",
    srcs = glob(
        include = ["**"],
        exclude = ["*.bazel"],
    ),
)

CONFIGURE_OPTIONS = [
    "no-comp",
    "no-idea",
    "no-weak-ssl-ciphers",
    "no-shared",
]

LIB_NAME = "openssl"

MAKE_TARGETS = [
    "build_libs",
    "install_dev",
]


configure_make(
    name = "openssl",
    configure_command = "config",
    configure_in_place = True,
    configure_options = CONFIGURE_OPTIONS,
    args = ["-j4"],
    env = select({
        "@platforms//os:macos": {"AR": ""},
        "//conditions:default": {},
    }),
    lib_name = LIB_NAME,
    lib_source = ":all_srcs",
    # Note that for Linux builds libssl must be listed before
    # libcrypto on the linker command-line.
    out_lib_dir = "lib64",
    out_static_libs = [
        "libssl.a",
        "libcrypto.a",
    ],
    targets = MAKE_TARGETS,
    visibility = ["//visibility:public"],
)

filegroup(
    name = "gen_dir",
    srcs = [":openssl"],
    output_group = "gen_dir",
    visibility = ["//visibility:public"],
)
