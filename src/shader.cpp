#include "shader.h"
#include "shader.ps.h"
#include "shader.vs.h"

#include <sdl/sdlexception.h>

namespace robot {

	namespace {

		struct alignas(16) LightPs {
			glm::vec4 position;	// xyz + padding
			glm::vec4 color;	// rgba
			glm::vec4 params;	// x = radius, y = ambientStrength, z = shininess + padding
		};

		struct alignas(16) LightDataPs {
			std::array<LightPs, 4> lights;
			alignas(16) int numLights;
			glm::vec4 cameraPos; // xyz + padding
		};
	}

	static_assert(sizeof(LightPs) == 48, "LightPs size mismatch");
	static_assert(sizeof(LightDataPs) == 224, "LightDataPs size mismatch");
	//static_assert(sizeof(sdl::Color) == 16, "Color size must be 16 bytes (4 floats)");

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
		static_assert(sizeof(projection) % 16 == 0, "SDL_GPU uses std140 layout, uniform buffer size must be multiple of 16 bytes");
	}

	void Shader::uploadLightingData(SDL_GPUCommandBuffer* commandBuffer, const LightingData& lightingData) {
		LightDataPs lightData{
			.cameraPos = glm::vec4(lightingData.cameraPos, 1.0f)
		};
		int i = 0;
		for (const auto& light : lightingData.lights) {
			if (i >= lightData.lights.size()) {
				break;
			}
			if (!light.enabled) {
				continue;
			}

			lightData.lights[i] = LightPs{
				.position = glm::vec4(light.position, 1.0f),
				.color = light.color,
				.params = glm::vec4(light.radius, light.ambientStrength, light.shininess, 0.0f)
			};
			++i;
		}
		lightData.numLights = i;
		// Maps to b1 in fragment shader
		SDL_PushGPUFragmentUniformData(commandBuffer, 0, &lightData, sizeof(lightData));
	}

}
