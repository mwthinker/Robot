#ifndef ZOMBIE_GRAPHIC_H
#define ZOMBIE_GRAPHIC_H

#include "shader.h"

#include <sdl/batch.h>
#include <sdl/gpu.h>
#include <sdl/color.h>
#include <sdl/gpuutil.h>

#include <SDL3/SDL_gpu.h>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/constants.hpp>

#include <concepts>
#include <span>
#include <stack>

namespace robot {

	namespace {
		constexpr float Pi = glm::pi<float>();
	}

	template <sdl::VertexType Vertex>
	class GraphicBuffer {
	public:
		void bindBuffers(SDL_GPURenderPass* renderPass) {
			SDL_GPUBufferBinding vertexBinding{
				.buffer = vertexBuffer_.get(),
				.offset = 0
			};
			SDL_BindGPUVertexBuffers(
				renderPass,
				0,
				&vertexBinding,
				1
			);

			SDL_GPUBufferBinding indexBinding{
				.buffer = indexBuffer_.get(),
				.offset = 0
			};
			SDL_BindGPUIndexBuffer(
				renderPass,
				&indexBinding,
				SDL_GPU_INDEXELEMENTSIZE_32BIT
			);
		}

		void uploadToGpu(SDL_GPUDevice* gpuDevice, SDL_GPUCommandBuffer* commandBuffer, sdl::Batch<Vertex>& batch) {
			auto indices_ = batch.indices();
			auto vertices_ = batch.vertices();

			SDL_GPUBuffer* indexBuffer = indexBuffer_.get(gpuDevice, SDL_GPU_BUFFERUSAGE_INDEX, indices_);
			SDL_GPUBuffer* vertexBuffer = vertexBuffer_.get(gpuDevice, SDL_GPU_BUFFERUSAGE_VERTEX, vertices_);

			SDL_GPUTransferBuffer* vertexTransferBuffer = vertexTransferBuffer_.get(gpuDevice, vertices_);
			SDL_GPUTransferBuffer* indexTransferBuffer = indexTransferBuffer_.get(gpuDevice, indices_);

			sdl::gpuCopyPass(commandBuffer, [&](SDL_GPUCopyPass* copyPass) {
				SDL_GPUTransferBufferLocation vertexLocation{
					.transfer_buffer = vertexTransferBuffer,
					.offset = 0
				};

				SDL_GPUBufferRegion vertexRegion{
					.buffer = vertexBuffer,
					.offset = 0,
					.size = static_cast<Uint32>(vertices_.size() * sizeof(Vertex))
				};

				SDL_UploadToGPUBuffer(
					copyPass,
					&vertexLocation,
					&vertexRegion,
					false);

				SDL_GPUTransferBufferLocation indexLocation{
					.transfer_buffer = indexTransferBuffer,
					.offset = 0
				};

				SDL_GPUBufferRegion indexRegion{
					.buffer = indexBuffer,
					.offset = 0,
					.size = static_cast<Uint32>(indices_.size() * sizeof(uint32_t))
				};

				SDL_UploadToGPUBuffer(copyPass,
					&indexLocation,
					&indexRegion,
					false);
			});
		}

		void draw(SDL_GPURenderPass* renderPass, sdl::Batch<Vertex>& batch) {
			SDL_DrawGPUIndexedPrimitives(
				renderPass,
				static_cast<Uint32>(batch.indices().size()),
				1,
				0,
				0,
				0
			);
		}

		sdl::TransferBuffer vertexTransferBuffer_;
		sdl::TransferBuffer indexTransferBuffer_;
		sdl::Buffer vertexBuffer_;
		sdl::Buffer indexBuffer_;
	};

	class Graphic {
	public:
		static constexpr glm::vec2 NoTexture{-1.0f, -1.0f};

		Graphic() {
			loadIdentityMatrix();
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
			batch_.startBatch();
			
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

			batch_.insertIndices({
				0, 1, 2, 2, 3, 0,		// Back face
				4, 5, 6, 6, 7, 4,		// Front face
				8, 9, 10, 10, 11, 8,	// Left face
				12, 13, 14, 14, 15, 12,	// Right face
				16, 17, 18, 18, 19, 16,	// Top face
				20, 21, 22, 22, 23, 20	// Bottom face
			});
		}

		void addSolidSphere(float radius, unsigned int slices, unsigned int stacks, sdl::Color color) {
			batch_.startBatch();

			// Top vertex
			glm::vec3 topPos{0.0f, radius, 0.0f};
			glm::vec4 topNormal{0.0f, 1.0f, 0.0f, 0.0f};
			addVertex(topPos, NoTexture, color, topNormal);

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
					addVertex(pos, NoTexture, color, normal);
				}
			}

			// Bottom vertex
			glm::vec3 bottomPos{0.0f, -radius, 0.0f};
			glm::vec4 bottomNormal{0.0f, -1.0f, 0.0f, 0.0f};
			addVertex(bottomPos, NoTexture, color, bottomNormal);

			// Generate indices for top cap
			for (unsigned int slice = 0; slice < slices; ++slice) {
				batch_.insertIndices({
					0, slice + 1, slice + 2
				});
			}

			// Generate indices for middle stacks
			for (unsigned int stack = 0; stack < stacks - 2; ++stack) {
				unsigned int k1 = 1 + stack * (slices + 1);
				unsigned int k2 = k1 + slices + 1;

				for (unsigned int slice = 0; slice < slices; ++slice) {
					batch_.insertIndices({
						k1 + slice, k2 + slice, k2 + slice + 1,
						k2 + slice + 1, k1 + slice + 1, k1 + slice
					});
				}
			}

			// Generate indices for bottom cap
			unsigned int bottomVertex = 1 + (stacks - 1) * (slices + 1);
			unsigned int lastStackStart = 1 + (stacks - 2) * (slices + 1);
			for (unsigned int slice = 0; slice < slices; ++slice) {
				batch_.insertIndices({
					lastStackStart + slice, bottomVertex, lastStackStart + slice + 1
				});
			}
		}

		void addRectangle(const glm::vec2& pos, const glm::vec2& size, sdl::Color color) {
			glm::vec3 pos3{pos, 0.0f};
			glm::vec3 normal = glm::vec3{0.0f, 0.0f, 1.0f};

			batch_.startBatch();
			addVertex(pos3, NoTexture, color, normal);
			addVertex({pos3.x + size.x, pos3.y, pos3.z}, NoTexture, color, normal);
			addVertex({pos3.x + size.x, pos3.y + size.y, pos3.z}, NoTexture, color, normal);
			addVertex({pos3.x, pos3.y + size.y, pos3.z}, NoTexture, color, normal);

			batch_.insertIndices({
				0, 1, 2,
				2, 3, 0 
			});
		}

		void clear() {
			batch_.clear();
		}

		void addLine(const glm::vec3& p1, const glm::vec3& p2, float width, sdl::Color color) {
			/*
			auto dp = 0.5f * width * glm::rotate(Pi / 2, glm::normalize(p2 - p1));

			glm::vec3 p1_3d{p1, 0.0f};
			glm::vec3 p2_3d{p2, 0.0f};
			glm::vec3 dp_3d{dp, 0.0f};

			batch_.startBatch();
			addVertex(p1_3d - dp_3d, NoTexture, color);
			addVertex(p2_3d - dp_3d, NoTexture, color);
			addVertex(p2_3d + dp_3d, NoTexture, color);
			addVertex(p1_3d + dp_3d, NoTexture, color);

			batch_.insertIndices({
				0, 1, 2,
				2, 3, 0 
			});
			*/
		}

		void addCircle(const glm::vec2& center, float radius, sdl::Color color, unsigned int iterations = 30, float startAngle = 0) {
			batch_.startBatch();

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
				batch_.insertIndices({
					0, i + 1, i + 2
				});
			}
		}

		void addCircleOutline(const glm::vec2& center, float radius, float width, sdl::Color color, unsigned int iterations = 30, float startAngle = 0) {
			batch_.startBatch();

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
				batch_.insertIndices({
					baseIndex, baseIndex + 1, baseIndex + 3,
					baseIndex + 3, baseIndex + 2, baseIndex
				});
			}
		}

		void addPixelLine(std::initializer_list<glm::vec2> points, sdl::Color color) {
			addPixelLine(points.begin(), points.end(), color);
		}

		void addPixelLine(std::input_iterator auto begin, std::input_iterator auto end, sdl::Color color) {
			batch_.startBatch();

			for (auto it = begin; it != end; ++it) {
				addVertex(glm::vec3{*it, 0}, NoTexture, color);
			}

			const auto size = std::distance(begin, end);
			for (int i = 1; i < size; ++i) {
				batch_.pushBackIndex(i - 1);
				batch_.pushBackIndex(i);
			}
		}

		void addPolygon(std::initializer_list<glm::vec2> points, sdl::Color color) {
			addPolygon(points.begin(), points.end(), color);
		}

		void addPolygon(std::input_iterator auto begin, std::input_iterator auto end, sdl::Color color) {
			batch_.startBatch();
			for (auto it = begin; it != end; ++it) {
				addVertex(glm::vec3{*it, 0.f}, NoTexture, color);
			}
			const auto size = std::distance(begin, end);
			for (unsigned int i = 1; i < size - 1; ++i) {
				batch_.insertIndices({0, i, i + 1});
			}
		}

		void addCylinder(float baseRadius, float topRadius, float height, unsigned int slices, unsigned int stacks, sdl::Color color) {
			batch_.startBatch();

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
					glm::vec3 normalDir = glm::normalize(glm::vec3{std::cos(angle), std::sin(angle), 0.0f});
					addVertex(glm::vec3{x, y, z}, NoTexture, color, normalDir);
				}
			}

			// Generate indices for side faces
			for (unsigned int stack = 0; stack < stacks; ++stack) {
				for (unsigned int slice = 0; slice < slices; ++slice) {
					unsigned int current = stack * (slices + 1) + slice;
					unsigned int next = current + slices + 1;

					batch_.insertIndices({
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
				batch_.insertIndices({
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
				batch_.insertIndices({
					vertexOffset, vertexOffset + slice + 2, vertexOffset + slice + 1
				});
			}
		}

		void addPixel(const glm::vec2& point, sdl::Color color, float size = 1.f) {
			addRectangle(point - glm::vec2{size * 0.5f}, glm::vec2{size}, color);
		}

		void bindBuffers(SDL_GPURenderPass* renderPass) {
			graphicBuffer_.bindBuffers(renderPass);
		}

		void uploadToGpu(SDL_GPUDevice* gpuDevice, SDL_GPUCommandBuffer* commandBuffer) {
			graphicBuffer_.uploadToGpu(gpuDevice, commandBuffer, batch_);
		}

		void draw(SDL_GPURenderPass* renderPass) {
			graphicBuffer_.draw(renderPass, batch_);
		}

	private:
		void addVertex(const glm::vec3& position, const glm::vec2& tex, sdl::Color color, const glm::vec3& normal = {}) {
			batch_.pushBack({getMatrix() * glm::vec4{position, 1}, tex, color, normal});
		}

		std::stack<glm::mat4> matrices_;

		sdl::Batch<Vertex> batch_;
		GraphicBuffer<Vertex> graphicBuffer_;
	};

}

#endif
