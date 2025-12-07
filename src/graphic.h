#ifndef ZOMBIE_GRAPHIC_H
#define ZOMBIE_GRAPHIC_H

#include "shader.h"

#include <sdl/batch.h>
#include <sdl/gpu.h>
#include <sdl/color.h>
#include <sdl/gpuutil.h>
#include <sdl/util.h>

#include <SDL3/SDL_gpu.h>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>

#include <concepts>
#include <span>
#include <stack>

namespace robot {

	constexpr float Pi = glm::pi<float>();

	inline sdl::SdlSurface createSdlSurface(int w, int h, sdl::Color color) {
		auto s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
		SDL_FillSurfaceRect(s, nullptr, color.toImU32());
		return sdl::createSdlSurface(s);
	}

	// Can't be stored.
	struct GpuData {
		std::span<const Vertex> vertices;
		std::span<const uint32_t> indices;
		SDL_GPUBuffer* indexBuffer;
		SDL_GPUBuffer* vertexBuffer;
		SDL_GPUTransferBuffer* vertexTransferBuffer;
		SDL_GPUTransferBuffer* indexTransferBuffer;
		SDL_GPUGraphicsPipeline* pipeline;
	};

	enum class DrawMode {
		Light,
		NoLight
	};
	
	class TrianglesBuffer {
	public:

		GpuData prepareGpuData(SDL_GPUDevice* gpuDevice, SDL_GPUGraphicsPipeline* pipeline) {
			auto indices_ = batch_.indices();
			auto vertices_ = batch_.vertices();
			SDL_GPUBuffer* indexBuffer = indexBuffer_.get(gpuDevice, SDL_GPU_BUFFERUSAGE_INDEX, indices_);
			SDL_GPUBuffer* vertexBuffer = vertexBuffer_.get(gpuDevice, SDL_GPU_BUFFERUSAGE_VERTEX, vertices_);
			SDL_GPUTransferBuffer* vertexTransferBuffer = vertexTransferBuffer_.get(gpuDevice, vertices_);
			SDL_GPUTransferBuffer* indexTransferBuffer = indexTransferBuffer_.get(gpuDevice, indices_);
			
			return GpuData{
				.vertices = vertices_,
				.indices = indices_,
				.indexBuffer = indexBuffer,
				.vertexBuffer = vertexBuffer,
				.vertexTransferBuffer = vertexTransferBuffer,
				.indexTransferBuffer = indexTransferBuffer,
				.pipeline = pipeline
			};
		}

		sdl::Batch<Vertex>& batch() {
			return batch_;
		}

	private:
		sdl::TransferBuffer vertexTransferBuffer_;
		sdl::TransferBuffer indexTransferBuffer_;
		sdl::Buffer vertexBuffer_;
		sdl::Buffer indexBuffer_;
		sdl::Batch<Vertex> batch_;
	};

	class LinesBuffer {
	public:
		GpuData prepareGpuData(SDL_GPUDevice* gpuDevice, SDL_GPUGraphicsPipeline* pipeline) {
			auto indices_ = batch().indices();
			auto vertices_ = batch().vertices();
			SDL_GPUBuffer* indexBuffer = indexBuffer_.get(gpuDevice, SDL_GPU_BUFFERUSAGE_INDEX, indices_);
			SDL_GPUBuffer* vertexBuffer = vertexBuffer_.get(gpuDevice, SDL_GPU_BUFFERUSAGE_VERTEX, vertices_);
			SDL_GPUTransferBuffer* vertexTransferBuffer = vertexTransferBuffer_.get(gpuDevice, vertices_);
			SDL_GPUTransferBuffer* indexTransferBuffer = indexTransferBuffer_.get(gpuDevice, indices_);

			return GpuData{
				.vertices = vertices_,
				.indices = indices_,
				.indexBuffer = indexBuffer,
				.vertexBuffer = vertexBuffer,
				.vertexTransferBuffer = vertexTransferBuffer,
				.indexTransferBuffer = indexTransferBuffer,
				.pipeline = pipeline
			};
		}

		sdl::Batch<Vertex>& batch() {
			return batch_;
		}

	private:
		sdl::TransferBuffer vertexTransferBuffer_;
		sdl::TransferBuffer indexTransferBuffer_;
		sdl::Buffer vertexBuffer_;
		sdl::Buffer indexBuffer_;
		sdl::Batch<Vertex> batch_;
	};

	class Graphic {
	public:
		static constexpr glm::vec2 NoTexture{-1.0f, -1.0f};
		static constexpr glm::vec2 NoLight{-2.0f, -2.0f};
		static constexpr glm::vec2 NoProjection{-3.0f, -3.0f};

		Graphic() {
			loadIdentityMatrix();
		}

		void preLoop(SDL_GPUDevice* gpuDevice, SDL_GPUSampleCount gpuSampleCount) {
			shader_.load(gpuDevice);
			setupTrianglesPipeline(gpuDevice, gpuSampleCount);

			auto transparentSurface = createSdlSurface(1, 1, sdl::color::White);
			texture_ = sdl::uploadSurface(gpuDevice, transparentSurface.get());

			sampler_ = sdl::createGpuSampler(gpuDevice, SDL_GPUSamplerCreateInfo{
				.min_filter = SDL_GPU_FILTER_NEAREST,
				.mag_filter = SDL_GPU_FILTER_NEAREST,
				.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
				.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
				.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
				.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
			});
		}

		void setupLinesPipeline(SDL_GPUDevice* gpuDevice, SDL_GPUSampleCount gpuSampleCount) {
			SDL_GPUVertexBufferDescription vertexBufferDescriptions{
				.slot = 0,
				.pitch = sizeof(Vertex),
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
			};

			SDL_GPUColorTargetDescription colorTargetDescription{
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.blend_state = SDL_GPUColorTargetBlendState{
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.enable_blend = true
			}
			};

			SDL_GPUDepthStencilState depthStencilState{
				.compare_op = SDL_GPU_COMPAREOP_LESS,
				.back_stencil_state = {
					.fail_op = SDL_GPU_STENCILOP_KEEP,
					.pass_op = SDL_GPU_STENCILOP_KEEP,
					.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
					.compare_op = SDL_GPU_COMPAREOP_ALWAYS,
				},
				.front_stencil_state = {
						.fail_op = SDL_GPU_STENCILOP_KEEP,
						.pass_op = SDL_GPU_STENCILOP_KEEP,
						.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
						.compare_op = SDL_GPU_COMPAREOP_ALWAYS,
				},
				.compare_mask = 0,
				.write_mask = 0,
				.enable_depth_test = true,
				.enable_depth_write = true,
				.enable_stencil_test = false
			};

			SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{
				.vertex_shader = shader_.vertexShader.get(),
				.fragment_shader = shader_.fragmentShader.get(),
				.vertex_input_state = SDL_GPUVertexInputState{
					.vertex_buffer_descriptions = &vertexBufferDescriptions,
					.num_vertex_buffers = 1,
					.vertex_attributes = shader_.attributes.data(),
					.num_vertex_attributes = static_cast<Uint32>(shader_.attributes.size())
				},
				.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST,
				.multisample_state = SDL_GPUMultisampleState{
						.sample_count = gpuSampleCount
				},
				.depth_stencil_state = depthStencilState,
				.target_info = SDL_GPUGraphicsPipelineTargetInfo{
					.color_target_descriptions = &colorTargetDescription,
					.num_color_targets = 1,
					.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
					.has_depth_stencil_target = true
				}
			};
			linesPipeline_ = sdl::createGpuGraphicsPipeline(gpuDevice, pipelineInfo);
		}

		void setupTrianglesPipeline(SDL_GPUDevice* gpuDevice, SDL_GPUSampleCount gpuSampleCount) {
			SDL_GPUVertexBufferDescription vertexBufferDescriptions{
				.slot = 0,
				.pitch = sizeof(Vertex),
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
			};

			SDL_GPUColorTargetDescription colorTargetDescription{
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.blend_state = SDL_GPUColorTargetBlendState{
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.enable_blend = true
				}
			};

			SDL_GPUDepthStencilState depthStencilState{
				.compare_op = SDL_GPU_COMPAREOP_LESS,
				.back_stencil_state = {
					.fail_op = SDL_GPU_STENCILOP_KEEP,
					.pass_op = SDL_GPU_STENCILOP_KEEP,
					.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
					.compare_op = SDL_GPU_COMPAREOP_ALWAYS,
				},
				.front_stencil_state = {
						.fail_op = SDL_GPU_STENCILOP_KEEP,
						.pass_op = SDL_GPU_STENCILOP_KEEP,
						.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
						.compare_op = SDL_GPU_COMPAREOP_ALWAYS,
				},
				.compare_mask = 0,
				.write_mask = 0,
				.enable_depth_test = true,
				.enable_depth_write = true,
				.enable_stencil_test = false
			};

			SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{
				.vertex_shader = shader_.vertexShader.get(),
				.fragment_shader = shader_.fragmentShader.get(),
				.vertex_input_state = SDL_GPUVertexInputState{
					.vertex_buffer_descriptions = &vertexBufferDescriptions,
					.num_vertex_buffers = 1,
					.vertex_attributes = shader_.attributes.data(),
					.num_vertex_attributes = static_cast<Uint32>(shader_.attributes.size())
				},
				.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				.multisample_state = SDL_GPUMultisampleState{
					.sample_count = gpuSampleCount
				},
				.depth_stencil_state = depthStencilState,
				.target_info = SDL_GPUGraphicsPipelineTargetInfo{
					.color_target_descriptions = &colorTargetDescription,
					.num_color_targets = 1,
					.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
					.has_depth_stencil_target = true
				}
			};
			trianglesPipeline_ = sdl::createGpuGraphicsPipeline(gpuDevice, pipelineInfo);
		}

		void pushMatrix() {
			matrices_.push(matrices_.top());
		}

		void popMatrix() {
			if (matrices_.size() > 1) {
				matrices_.pop();
			}
		}

		void loadIdentityMatrix() {
			while (!matrices_.empty()) {
				matrices_.pop();
			}
			matrices_.push(glm::mat4{1.0f});
		}

		void translate(const glm::vec3& translation) {
			matrices_.top() = glm::translate(matrices_.top(), translation);
		}

		void rotate(float angleRadians, const glm::vec3& axis) {
			matrices_.top() = glm::rotate(matrices_.top(), angleRadians, axis);
		}

		void scale(const glm::vec3& scale) {
			matrices_.top() = glm::scale(matrices_.top(), scale);
		}

		void multiplyMatrix(const glm::mat4& matrix) {
			matrices_.top() = matrices_.top() * matrix;
		}

		const glm::mat4& getMatrix() const {
			return matrices_.top();
		}

		void addSolidCube(float size, sdl::Color color) {
			trianglesBuffer_.batch().startBatch();
			
			float halfSize = size / 2.0f;
			
			// Back face (normal pointing in -Z direction)
			glm::vec4 normalBack{0.0f, 0.0f, -1.0f, 0.0f};
			addVertex(glm::vec3{-halfSize, -halfSize, -halfSize}, NoTexture, color, normalBack);
			addVertex(glm::vec3{halfSize, -halfSize, -halfSize}, NoTexture, color, normalBack);
			addVertex(glm::vec3{halfSize, halfSize, -halfSize}, NoTexture, color, normalBack);
			addVertex(glm::vec3{-halfSize, halfSize, -halfSize}, NoTexture, color, normalBack);
			
			// Front face (normal pointing in +Z direction)
			glm::vec4 normalFront{0.0f, 0.0f, 1.0f, 0.0f};
			addVertex(glm::vec3{-halfSize, -halfSize, halfSize}, NoTexture, color, normalFront);
			addVertex(glm::vec3{halfSize, -halfSize, halfSize}, NoTexture, color, normalFront);
			addVertex(glm::vec3{halfSize, halfSize, halfSize}, NoTexture, color, normalFront);
			addVertex(glm::vec3{-halfSize, halfSize, halfSize}, NoTexture, color, normalFront);
			
			// Left face (normal pointing in -X direction)
			glm::vec4 normalLeft{-1.0f, 0.0f, 0.0f, 0.0f};
			addVertex(glm::vec3{-halfSize, -halfSize, -halfSize}, NoTexture, color, normalLeft);
			addVertex(glm::vec3{-halfSize, -halfSize, halfSize}, NoTexture, color, normalLeft);
			addVertex(glm::vec3{-halfSize, halfSize, halfSize}, NoTexture, color, normalLeft);
			addVertex(glm::vec3{-halfSize, halfSize, -halfSize}, NoTexture, color, normalLeft);
			
			// Right face (normal pointing in +X direction)
			glm::vec4 normalRight{1.0f, 0.0f, 0.0f, 0.0f};
			addVertex(glm::vec3{halfSize, -halfSize, -halfSize}, NoTexture, color, normalRight);
			addVertex(glm::vec3{halfSize, -halfSize, halfSize}, NoTexture, color, normalRight);
			addVertex(glm::vec3{halfSize, halfSize, halfSize}, NoTexture, color, normalRight);
			addVertex(glm::vec3{halfSize, halfSize, -halfSize}, NoTexture, color, normalRight);
			
			// Top face (normal pointing in +Y direction)
			glm::vec4 normalTop{0.0f, 1.0f, 0.0f, 0.0f};
			addVertex(glm::vec3{-halfSize, halfSize, -halfSize}, NoTexture, color, normalTop);
			addVertex(glm::vec3{halfSize, halfSize, -halfSize}, NoTexture, color, normalTop);
			addVertex(glm::vec3{halfSize, halfSize, halfSize}, NoTexture, color, normalTop);
			addVertex(glm::vec3{-halfSize, halfSize, halfSize}, NoTexture, color, normalTop);
			
			// Bottom face (normal pointing in -Y direction)
			glm::vec4 normalBottom{0.0f, -1.0f, 0.0f, 0.0f};
			addVertex(glm::vec3{-halfSize, -halfSize, -halfSize}, NoTexture, color, normalBottom);
			addVertex(glm::vec3{halfSize, -halfSize, -halfSize}, NoTexture, color, normalBottom);
			addVertex(glm::vec3{halfSize, -halfSize, halfSize}, NoTexture, color, normalBottom);
			addVertex(glm::vec3{-halfSize, -halfSize, halfSize}, NoTexture, color, normalBottom);

			trianglesBuffer_.batch().insertIndices({
				0, 1, 2, 2, 3, 0,		// Back face
				4, 5, 6, 6, 7, 4,		// Front face
				8, 9, 10, 10, 11, 8,	// Left face
				12, 13, 14, 14, 15, 12,	// Right face
				16, 17, 18, 18, 19, 16,	// Top face
				20, 21, 22, 22, 23, 20	// Bottom face
			});
		}

		void addSolidSphere(float radius, unsigned int slices, unsigned int stacks, sdl::Color color, DrawMode drawMode = DrawMode::Light) {
			trianglesBuffer_.batch().startBatch();

			// Top vertex
			glm::vec3 topPos{0.0f, radius, 0.0f};
			glm::vec4 topNormal{0.0f, 1.0f, 0.0f, 0.0f};
			addVertex(topPos, NoTexture, color, topNormal, drawMode);

			// Generate vertices for each stack
			for (unsigned int stack = 1; stack < stacks; ++stack) {
				float stackAngle = Pi / 2.0f - Pi * stack / stacks;
				float xy = radius * std::cos(stackAngle);
				float y = radius * std::sin(stackAngle);

				for (unsigned int slice = 0; slice <= slices; ++slice) {
					float sliceAngle = 2.0f * Pi * slice / slices;
					float x = xy * std::cos(sliceAngle);
					float z = xy * std::sin(sliceAngle);

					glm::vec3 pos{x, y, z};
					glm::vec4 normal{glm::normalize(glm::vec3{x, y, z}), 0.0f};
					addVertex(pos, NoTexture, color, normal, drawMode);
				}
			}

			// Bottom vertex
			glm::vec3 bottomPos{0.0f, -radius, 0.0f};
			glm::vec4 bottomNormal{0.0f, -1.0f, 0.0f, 0.0f};
			addVertex(bottomPos, NoTexture, color, bottomNormal, drawMode);

			// Generate indices for top cap
			for (unsigned int slice = 0; slice < slices; ++slice) {
				trianglesBuffer_.batch().insertIndices({
					0, slice + 1, slice + 2
				});
			}

			// Generate indices for middle stacks
			for (unsigned int stack = 0; stack < stacks - 2; ++stack) {
				unsigned int k1 = 1 + stack * (slices + 1);
				unsigned int k2 = k1 + slices + 1;

				for (unsigned int slice = 0; slice < slices; ++slice) {
					trianglesBuffer_.batch().insertIndices({
						k1 + slice, k2 + slice, k2 + slice + 1,
						k2 + slice + 1, k1 + slice + 1, k1 + slice
					});
				}
			}

			// Generate indices for bottom cap
			unsigned int bottomVertex = 1 + (stacks - 1) * (slices + 1);
			unsigned int lastStackStart = 1 + (stacks - 2) * (slices + 1);
			for (unsigned int slice = 0; slice < slices; ++slice) {
				trianglesBuffer_.batch().insertIndices({
					lastStackStart + slice, bottomVertex, lastStackStart + slice + 1
				});
			}
		}

		void addRectangle(const glm::vec2& pos, const glm::vec2& size, sdl::Color color) {
			glm::vec3 pos3{pos, 0.0f};
			glm::vec3 normal = glm::vec3{0.0f, 0.0f, 1.0f};

			trianglesBuffer_.batch().startBatch();
			addVertex(pos3, NoTexture, color, normal);
			addVertex({pos3.x + size.x, pos3.y, pos3.z}, NoTexture, color, normal);
			addVertex({pos3.x + size.x, pos3.y + size.y, pos3.z}, NoTexture, color, normal);
			addVertex({pos3.x, pos3.y + size.y, pos3.z}, NoTexture, color, normal);

			trianglesBuffer_.batch().insertIndices({
				0, 1, 2,
				2, 3, 0 
			});
		}

		void clear() {
			trianglesBuffer_.batch().clear();
			linesBuffer_.batch().clear();
		}

		void addLine(const glm::vec3& p1, const glm::vec3& p2, float pixelSize, sdl::Color color, int viewportWidth, int viewportHeight) {
			// Transform to clip space
			glm::vec4 cp1 = projectionMatrix_ * viewMatrix_ * glm::vec4(p1, 1.0f);
			glm::vec4 cp2 = projectionMatrix_ * viewMatrix_ * glm::vec4(p2, 1.0f);

			// Avoid degenerate W
			if (cp1.w == 0.0f || cp2.w == 0.0f)
				return;

			// Convert to NDC
			glm::vec3 ndc1 = glm::vec3(cp1) / cp1.w;
			glm::vec3 ndc2 = glm::vec3(cp2) / cp2.w;

			// 2D direction in NDC
			glm::vec2 dir = glm::vec2(ndc2 - ndc1);
			float len = glm::length(dir);
			if (len < 1e-6f)
				return; // Line parallel to camera view axis; zero screen length.

			// Perpendicular in NDC
			glm::vec2 offset = 0.5f * glm::normalize(glm::vec2{-dir.y, dir.x});
			offset.x *= (pixelSize / viewportWidth);
			offset.y *= (pixelSize / viewportHeight);

			// Build quad in clip space
			glm::vec3 p11{ndc1.x - offset.x, ndc1.y - offset.y, ndc1.z};
			glm::vec3 p22{ndc2.x - offset.x, ndc2.y - offset.y, ndc2.z};
			glm::vec3 p33{ndc2.x + offset.x, ndc2.y + offset.y, ndc2.z};
			glm::vec3 p44{ndc1.x + offset.x, ndc1.y + offset.y, ndc1.z};

			trianglesBuffer_.batch().startBatch();

			addVertex(p11, NoProjection, color, {}, DrawMode::Light);
			addVertex(p22, NoProjection, color, {}, DrawMode::Light);
			addVertex(p33, NoProjection, color, {}, DrawMode::Light);
			addVertex(p44, NoProjection, color, {}, DrawMode::Light);

			trianglesBuffer_.batch().insertIndices({0, 1, 2, 2, 3, 0});
		}

		void addCircle(const glm::vec2& center, float radius, sdl::Color color, unsigned int iterations = 30, float startAngle = 0) {
			trianglesBuffer_.batch().startBatch();

			// Add center vertex
			addVertex(glm::vec3{center, 0.0f}, NoTexture, color);

			// Add perimeter vertices
			for (unsigned int i = 0; i <= iterations; ++i) {
				float angle = startAngle + (2.0f * Pi * i) / iterations;
				glm::vec2 pos = center + radius * glm::vec2{std::cos(angle), std::sin(angle)};
				addVertex(glm::vec3{pos, 0.0f}, NoTexture, color);
			}

			// Create triangles from center to perimeter
			for (unsigned int i = 0; i < iterations; ++i) {
				trianglesBuffer_.batch().insertIndices({
					0, i + 1, i + 2
				});
			}
		}

		void addCircleOutline(const glm::vec2& center, float radius, float width, sdl::Color color, unsigned int iterations = 30, float startAngle = 0) {
			trianglesBuffer_.batch().startBatch();

			float innerRadius = radius - width * 0.5f;
			float outerRadius = radius + width * 0.5f;

			// Add vertices for inner and outer circles
			for (unsigned int i = 0; i <= iterations; ++i) {
				float angle = startAngle + (2.0f * Pi * i) / iterations;
				glm::vec2 direction{std::cos(angle), std::sin(angle)};

				glm::vec2 innerPos = center + innerRadius * direction;
				glm::vec2 outerPos = center + outerRadius * direction;

				addVertex(glm::vec3{innerPos, 0.0f}, NoTexture, color);
				addVertex(glm::vec3{outerPos, 0.0f}, NoTexture, color);
			}

			// Create quad strips between inner and outer circles
			for (unsigned int i = 0; i < iterations; ++i) {
				unsigned int baseIndex = i * 2;
				trianglesBuffer_.batch().insertIndices({
					baseIndex, baseIndex + 1, baseIndex + 3,
					baseIndex + 3, baseIndex + 2, baseIndex
				});
			}
		}

		void addPixelLine(std::initializer_list<glm::vec2> points, sdl::Color color) {
			addPixelLine(points.begin(), points.end(), color);
		}

		void addPixelLine(std::input_iterator auto begin, std::input_iterator auto end, sdl::Color color) {
			linesBuffer_.batch().startBatch();

			for (auto it = begin; it != end; ++it) {
				addLinesVertex(glm::vec3{*it, 0}, NoTexture, color);
			}

			const auto size = std::distance(begin, end);
			for (int i = 1; i < size; ++i) {
				linesBuffer_.batch().pushBackIndex(i - 1);
				linesBuffer_.batch().pushBackIndex(i);
			}
		}

		void addPolygon(std::initializer_list<glm::vec2> points, sdl::Color color) {
			addPolygon(points.begin(), points.end(), color);
		}

		void addPolygon(std::input_iterator auto begin, std::input_iterator auto end, sdl::Color color) {
			trianglesBuffer_.batch().startBatch();
			for (auto it = begin; it != end; ++it) {
				addVertex(glm::vec3{*it, 0.f}, NoTexture, color);
			}
			const auto size = std::distance(begin, end);
			for (unsigned int i = 1; i < size - 1; ++i) {
				trianglesBuffer_.batch().insertIndices({0, i, i + 1});
			}
		}

		void addCylinder(float baseRadius, float topRadius, float height, unsigned int slices, unsigned int stacks, sdl::Color color) {
			trianglesBuffer_.batch().startBatch();

			// Generate vertices for side faces
			for (unsigned int stack = 0; stack <= stacks; ++stack) {
				float stackHeight = height * stack / stacks;
				float stackRadius = baseRadius + (topRadius - baseRadius) * stack / stacks;
				for (unsigned int slice = 0; slice <= slices; ++slice) {

					float angle = 2.0f * Pi * slice / slices;
					float x = stackRadius * std::cos(angle);
					float y = stackRadius * std::sin(angle);
					float z = stackHeight;

					// Normal perpendicular to cylinder surface (radial direction in XY plane)
					glm::vec3 normalDir{std::cos(angle), std::sin(angle), 0.0f};
					addVertex(glm::vec3{x, y, z}, NoTexture, color, normalDir);
				}
			}

			// Generate indices for side faces
			for (unsigned int stack = 0; stack < stacks; ++stack) {
				for (unsigned int slice = 0; slice < slices; ++slice) {
					unsigned int current = stack * (slices + 1) + slice;
					unsigned int next = current + slices + 1;

					trianglesBuffer_.batch().insertIndices({
						current, next, next + 1,
						next + 1, current + 1, current
					});
				}
			}
			
			// Bottom cap (base at Z=0)
			unsigned int vertexOffset = (stacks + 1) * (slices + 1);
			glm::vec3 bottomNormal{0.0f, 0.0f, -1.0f};
			addVertex(glm::vec3{0.0f, 0.0f, 0.0f}, NoTexture, color, bottomNormal);

			for (unsigned int slice = 0; slice <= slices; ++slice) {
				float angle = 2.0f * Pi * slice / slices;
				float x = baseRadius * std::cos(angle);
				float y = baseRadius * std::sin(angle);
				addVertex(glm::vec3{x, y, 0.0f}, NoTexture, color, bottomNormal);
			}

			// Bottom cap indices
			for (unsigned int slice = 0; slice < slices; ++slice) {
				trianglesBuffer_.batch().insertIndices({
					vertexOffset, vertexOffset + slice + 1, vertexOffset + slice + 2
				});
			}

			// Top cap (at Z=height)
			vertexOffset += slices + 2;
			glm::vec3 topNormal{0.0f, 0.0f, 1.0f};
			addVertex(glm::vec3{0.0f, 0.0f, height}, NoTexture, color, topNormal);

			for (unsigned int slice = 0; slice <= slices; ++slice) {
				float angle = 2.0f * Pi * slice / slices;
				float x = topRadius * std::cos(angle);
				float y = topRadius * std::sin(angle);
				addVertex(glm::vec3{x, y, height}, NoTexture, color, topNormal);
			}

			// Top cap indices - reversed winding order from bottom
			for (unsigned int slice = 0; slice < slices; ++slice) {
				trianglesBuffer_.batch().insertIndices({
					vertexOffset, vertexOffset + slice + 2, vertexOffset + slice + 1
				});
			}
		}

		void addPixel(const glm::vec2& point, sdl::Color color, float size = 1.f) {
			addRectangle(point - glm::vec2{size * 0.5f}, glm::vec2{size}, color);
		}

		void bindAndDraw(SDL_GPUDevice* gpuDevice, SDL_GPURenderPass* renderPass) {
			auto& data = gpuDatas_.emplace_back(trianglesBuffer_.prepareGpuData(gpuDevice, trianglesPipeline_.get()));

			SDL_GPUTextureSamplerBinding samplerBinding{
				.texture = texture_.get(),
				.sampler = sampler_.get()
			};

			SDL_BindGPUGraphicsPipeline(renderPass, trianglesPipeline_.get());
			
			SDL_GPUBufferBinding vertexBinding{
				.buffer = data.vertexBuffer,
				.offset = 0
			};
			SDL_BindGPUVertexBuffers(
				renderPass,
				0,
				&vertexBinding,
				1
			);

			SDL_GPUBufferBinding indexBinding{
				.buffer = data.indexBuffer,
				.offset = 0
			};
			SDL_BindGPUIndexBuffer(
				renderPass,
				&indexBinding,
				SDL_GPU_INDEXELEMENTSIZE_32BIT
			);

			SDL_BindGPUFragmentSamplers(
				renderPass,
				0,
				&samplerBinding,
				1
			);
			
			SDL_DrawGPUIndexedPrimitives(
				renderPass,
				static_cast<Uint32>(data.indices.size()),
				1,
				0,
				0,
				0
			);
		}

		void gpuCopyPass(SDL_GPUDevice* gpuDevice, SDL_GPUCommandBuffer* commandBuffer) {
			SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
			for (const auto& gpuData : gpuDatas_) {
				SDL_GPUTransferBufferLocation vertexLocation{
					.transfer_buffer = gpuData.vertexTransferBuffer,
					.offset = 0
				};
				SDL_GPUBufferRegion vertexRegion{
					.buffer = gpuData.vertexBuffer,
					.offset = 0,
					.size = static_cast<Uint32>(gpuData.vertices.size() * sizeof(Vertex))
				};
				SDL_UploadToGPUBuffer(
					copyPass,
					&vertexLocation,
					&vertexRegion,
					false
				);
				SDL_GPUTransferBufferLocation indexLocation{
					.transfer_buffer = gpuData.indexTransferBuffer,
					.offset = 0
				};
				SDL_GPUBufferRegion indexRegion{
					.buffer = gpuData.indexBuffer,
					.offset = 0,
					.size = static_cast<Uint32>(gpuData.indices.size() * sizeof(uint32_t))
				};
				SDL_UploadToGPUBuffer(copyPass,
					&indexLocation,
					&indexRegion,
					false
				);
			}
			SDL_EndGPUCopyPass(copyPass);
			gpuDatas_.clear();
		}

		void uploadProjectionMatrix(SDL_GPUCommandBuffer* commandBuffer, const glm::mat4& projection, const glm::mat4& viewMatrix) {
			projectionMatrix_ = projection;
			viewMatrix_ = viewMatrix;
			shader_.uploadProjectionMatrix(commandBuffer, projection * viewMatrix);
		}

		void uploadLightingData(SDL_GPUCommandBuffer* commandBuffer, const LightingData& lightingData) {
			shader_.uploadLightingData(commandBuffer, lightingData);
		}

	private:
		void addVertex(const glm::vec3& position, const glm::vec2& tex, sdl::Color color, const glm::vec3& normal = {}, DrawMode drawMode = DrawMode::Light) {
			trianglesBuffer_.batch().pushBack(
				Vertex{
					getMatrix() * glm::vec4{position, 1},
					DrawMode::NoLight == drawMode ? glm::vec2{-2.f, -2.f} : tex,
					color,
					glm::vec3{getMatrix() * glm::vec4{normal, 0.f}}
				}
			);
		}

		void addLinesVertex(const glm::vec3& position, const glm::vec2& tex, sdl::Color color) {
			linesBuffer_.batch().pushBack({
				getMatrix() * glm::vec4{position, 1},
				tex,
				color
			});
		}

		Shader shader_;
		sdl::GpuGraphicsPipeline trianglesPipeline_;
		sdl::GpuGraphicsPipeline linesPipeline_;
		std::stack<glm::mat4> matrices_;
		glm::mat4 projectionMatrix_;
		glm::mat4 viewMatrix_;

		TrianglesBuffer trianglesBuffer_;
		LinesBuffer linesBuffer_;
		std::vector<GpuData> gpuDatas_;

		sdl::GpuSampler sampler_;
		sdl::GpuTexture texture_;
	};

}

#endif
