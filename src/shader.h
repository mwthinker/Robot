#ifndef ROBOT_SHADER_H
#define ROBOT_SHADER_H

#include "shader.ps.h"
#include "shader.vs.h"

#include <sdl/color.h>
#include <sdl/gpu.h>
#include <glm/glm.hpp>

#include <array>

namespace robot {

	// Does not need to be std140 because we define the layout in Shader struct.
	struct Vertex {
		glm::vec3 position;
		glm::vec2 tex;
		glm::vec4 color;
		glm::vec3 normal;
	};
	static_assert(sdl::VertexType<Vertex>, "Vertex must satisfy VertexType");

	struct Shader {
		void load(SDL_GPUDevice* gpuDevice);
		
		static void uploadProjectionMatrix(SDL_GPUCommandBuffer* commandBuffer, const glm::mat4& projection);

		static void uploadLightingData(SDL_GPUCommandBuffer* commandBuffer,
			const glm::vec3& lightPos,
			float lightRadius,
			sdl::Color color,
			float ambientStrength,
			float shininess,
			const glm::vec3& cameraPos);

		static constexpr std::array<SDL_GPUVertexAttribute, 4> attributes = {
			// position maps to TEXCOORD0
			SDL_GPUVertexAttribute{
				.location = 0,
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.offset = offsetof(Vertex, position)
			},
			// tex maps to TEXCOORD1
			SDL_GPUVertexAttribute{
					.location = 1,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = offsetof(Vertex, tex)
			},
			// color maps to TEXCOORD2
			SDL_GPUVertexAttribute{
					.location = 2,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = offsetof(Vertex, color)
			},
			// normal maps to TEXCOORD3
			SDL_GPUVertexAttribute{
					.location = 3,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(Vertex, normal)
			}
		};

		sdl::GpuShader vertexShader;
		sdl::GpuShader fragmentShader;
	};

}

#endif
