# Invoked at build time via cmake -P to convert compiled SPIR-V binaries into
# an embeddable C++ header.  Called with:
#   -DVERT_SPV=<path>  -DFRAG_SPV=<path>  -DOUT_FILE=<path>

file(READ "${VERT_SPV}" vert_hex HEX)
string(REGEX REPLACE "(..)" "0x\\1, " vert_bytes "${vert_hex}")
string(LENGTH "${vert_hex}" vert_hex_len)
math(EXPR vert_size "${vert_hex_len} / 2")

file(READ "${FRAG_SPV}" frag_hex HEX)
string(REGEX REPLACE "(..)" "0x\\1, " frag_bytes "${frag_hex}")
string(LENGTH "${frag_hex}" frag_hex_len)
math(EXPR frag_size "${frag_hex_len} / 2")

file(WRITE "${OUT_FILE}"
"#ifndef _FLATEARTH_RENDERER_VULKAN_BUILTIN_SHADERS_HPP\n"
"#define _FLATEARTH_RENDERER_VULKAN_BUILTIN_SHADERS_HPP\n"
"\n"
"#include <cstddef>\n"
"#include <cstdint>\n"
"\n"
"namespace flatearth::renderer::vulkan::shaders {\n"
"\n"
"alignas(4) static constexpr uint8_t kObjectShaderVert[] = { ${vert_bytes} };\n"
"static constexpr size_t kObjectShaderVertSize = ${vert_size};\n"
"\n"
"alignas(4) static constexpr uint8_t kObjectShaderFrag[] = { ${frag_bytes} };\n"
"static constexpr size_t kObjectShaderFragSize = ${frag_size};\n"
"\n"
"} // namespace flatearth::renderer::vulkan::shaders\n"
"\n"
"#endif // _FLATEARTH_RENDERER_VULKAN_BUILTIN_SHADERS_HPP\n"
)
