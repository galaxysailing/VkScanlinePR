#include "vk_util.h"

namespace Galaxysailing {

namespace vk {

    namespace util {

        std::string errorString(VkResult errorCode)
        {
            switch (errorCode)
            {
#define STR(r) case VK_ ##r: return #r
                STR(NOT_READY);
                STR(TIMEOUT);
                STR(EVENT_SET);
                STR(EVENT_RESET);
                STR(INCOMPLETE);
                STR(ERROR_OUT_OF_HOST_MEMORY);
                STR(ERROR_OUT_OF_DEVICE_MEMORY);
                STR(ERROR_INITIALIZATION_FAILED);
                STR(ERROR_DEVICE_LOST);
                STR(ERROR_MEMORY_MAP_FAILED);
                STR(ERROR_LAYER_NOT_PRESENT);
                STR(ERROR_EXTENSION_NOT_PRESENT);
                STR(ERROR_FEATURE_NOT_PRESENT);
                STR(ERROR_INCOMPATIBLE_DRIVER);
                STR(ERROR_TOO_MANY_OBJECTS);
                STR(ERROR_FORMAT_NOT_SUPPORTED);
                STR(ERROR_SURFACE_LOST_KHR);
                STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
                STR(SUBOPTIMAL_KHR);
                STR(ERROR_OUT_OF_DATE_KHR);
                STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
                STR(ERROR_VALIDATION_FAILED_EXT);
                STR(ERROR_INVALID_SHADER_NV);
#undef STR
            default:
                return "UNKNOWN_ERROR";
            }
        }

        void exitFatal(const std::string& message, int32_t exitCode)
        {
            std::cerr << message << "\n";
            exit(exitCode);
        }

        void exitFatal(const std::string& message, VkResult resultCode)
        {
            std::cerr << message << "\n";
            exitFatal(message, (int32_t)resultCode);
        }

        std::string read_file(const std::string filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            std::string buffer;
            buffer.resize(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }

        // Returns GLSL shader source text after preprocessing.
        std::string preprocess_shader(const std::string& source_name,
            shaderc_shader_kind kind,
            const std::string& source) {
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;

            // Like -DMY_DEFINE=1
            options.AddMacroDefinition("MY_DEFINE", "1");

            shaderc::PreprocessedSourceCompilationResult result =
                compiler.PreprocessGlsl(source, kind, source_name.c_str(), options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                //std::cerr << result.GetErrorMessage();
                return "";
            }

            return { result.cbegin(), result.cend() };
        }

        // Compiles a shader to SPIR-V assembly. Returns the assembly text
        // as a string.
        std::string compile_file_to_assembly(const std::string& source_name,
            shaderc_shader_kind kind,
            const std::string& source,
            bool optimize = false) {
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;

            if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

            shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
                source, kind, source_name.c_str(), options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                //std::cerr << result.GetErrorMessage();
                return "";
            }

            return { result.cbegin(), result.cend() };
        }

        // Compiles a shader to a SPIR-V binary. Returns the binary as
        // a vector of 32-bit words.
        std::vector<uint32_t> compile_file(const std::string& source_name,
            shaderc_shader_kind kind,
            const std::string& source,
            bool optimize = false) {
            shaderc::Compiler compiler;
            shaderc::CompileOptions options;

            // Like -DMY_DEFINE=1
            options.AddMacroDefinition("MY_DEFINE", "1");
            if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

            shaderc::SpvCompilationResult module =
                compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

            if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
                //std::cerr << module.GetErrorMessage();
                return std::vector<uint32_t>();
            }

            return { module.cbegin(), module.cend() };
        }
    }


}// end of namespace vk

}// end of namespace Galaxysailing