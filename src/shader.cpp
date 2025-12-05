#include "shader.h"
#include "shader.ps.h"
#include "shader.vs.h"

#include <sdl/sdlexception.h>

namespace robot {

	namespace {

		struct alignas(16) LightDataPs {
			glm::vec4 lightPos;      // xyz + padding
			glm::vec4 lightColor;    // rgb + padding
			glm::vec4 params;        // x = radius, y = ambientStrength
			glm::vec4 cameraPos;     // xyz + padding
		};
	}

	void Shader::load(SDL_GPUDevice* gpuDevice) {
		SDL_GPUShaderCreateInfo vxCreateInfo{
			.entrypoint = "main",
			.stage = SDL_GPU_SHADERSTAGE_VERTEX,
			.num_samplers = 0,
			.num_storage_textures = 0,
			.num_storage_buffers = 0,
			.num_uniform_buffers = 1
		};
		SDL_GPUShaderCreateInfo pxCreateInfo{
			.entrypoint = "main",
			.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
			.num_samplers = 1,
			.num_storage_textures = 0,
			.num_storage_buffers = 0,
			.num_uniform_buffers = 1
		};

		auto driver = SDL_GetGPUDeviceDriver(gpuDevice);
		if (std::strcmp(driver, "vulkan") == 0) {
			vxCreateInfo.code_size = ShaderVsSpirvBytes.size();
			vxCreateInfo.code = ShaderVsSpirvBytes.data();
			vxCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;

			pxCreateInfo.code_size = ShaderPsSpirvBytes.size();
			pxCreateInfo.code = ShaderPsSpirvBytes.data();
			pxCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		} else if (std::strcmp(driver, "direct3d12") == 0) {
			vxCreateInfo.code_size = ShaderVsDxilBytes.size();
			vxCreateInfo.code = ShaderVsDxilBytes.data();
			vxCreateInfo.format = SDL_GPU_SHADERFORMAT_DXIL;

			pxCreateInfo.code_size = ShaderPsDxilBytes.size();
			pxCreateInfo.code = ShaderPsDxilBytes.data();
			pxCreateInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
		} else {
			throw sdl::SdlException("[Shader] Unsupported GPU driver for shader loading '{}'", driver);
		}
		vertexShader = sdl::createGpuShader(gpuDevice, vxCreateInfo);
		fragmentShader = sdl::createGpuShader(gpuDevice, pxCreateInfo);
	}

	void Shader::uploadProjectionMatrix(SDL_GPUCommandBuffer* commandBuffer, const glm::mat4& projection) {
		// Maps to b0 in vertex shader
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &projection, sizeof(projection));
		// Must be multiple of 16 bytes, SDL_GPU uses std140 layout for uniform buffers
		static_assert(sizeof(projection) % 16 == 0, "Uniform buffer size must be multiple of 16 bytes");
	}

	void Shader::uploadLightingData(SDL_GPUCommandBuffer* commandBuffer, const LightingData& lightingData) {

		// Maps to b1 in fragment shader
		LightDataPs lightData{
			.lightPos = glm::vec4(lightingData.lightPos, 1.0f),
			.lightColor = lightingData.lightColor,
			.params = glm::vec4(lightingData.lightRadius, lightingData.ambientStrength, lightingData.shininess, 0.0f),
			.cameraPos = glm::vec4(lightingData.cameraPos, 1.0f)
		};
		SDL_PushGPUFragmentUniformData(commandBuffer, 0, &lightData, sizeof(lightData));
	}

}
