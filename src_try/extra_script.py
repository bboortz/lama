Import("env")

env.Append(
    LIBPATH=[env["PROJECT_DIR"] + "/lama-packet/target/xtensa-esp32-none-elf/release"],
    LIBS=["lama_packet"]
#    LIBPATH=[env["PROJECT_DIR"]],
)

# Force correct link order
env.Append(
    LINKFLAGS=["-Wl,--whole-archive", "-llama_packet", "-Wl,--no-whole-archive"]
)

# Add Rust static library to linker flags
#env.Append(
#    LIBPATH=[env["PROJECT_DIR"]],
#    LIBS=["lama_packet"]
#)

print("Added Rust library: liblama_packet.a")


