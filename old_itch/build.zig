const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "itch-parser",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    exe.root_module.addIncludePath(b.path("include"));
    exe.root_module.addCSourceFiles(.{
        .files = &.{ "src/main.c", "src/parser.c", "src/db.c", "src/ringbuffer.c", "src/worker.c", "src/filter.c" },
        .flags = &.{ "-std=c23", "-O3", "-march=native", "-D_DEFAULT_SOURCE" },
    });

    exe.root_module.linkSystemLibrary("duckdb", .{});
    b.installArtifact(exe);

    const check_tests = b.addExecutable(.{
        .name = "test_runner",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    check_tests.root_module.addIncludePath(b.path("include"));
    check_tests.root_module.addCSourceFiles(.{
        .files = &.{ "src/main.c", "src/parser.c", "src/db.c", "src/ringbuffer.c", "src/worker.c" },
        .flags = &.{ "-std=c23", "-D_DEFAULT_SOURCE" },
    });

    const run_tests = b.addRunArtifact(check_tests);
    const test_step = b.step("test", "Run TDD assertions");
    test_step.dependOn(&run_tests.step);
}
