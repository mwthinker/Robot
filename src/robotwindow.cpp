#include "robotwindow.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <sdl/gpuutil.h>

namespace robot {

	namespace {

		sdl::SdlSurface createSdlSurface(int w, int h, sdl::Color color) {
			auto s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
			SDL_FillSurfaceRect(s, nullptr, color.toImU32());
			return sdl::createSdlSurface(s);
		}

	}

	RobotWindow::RobotWindow() {
		setSize(800, 800);
		setTitle("Robot");
		setShowDemoWindow(true);
		setShowColorWindow(true);
	}

	void RobotWindow::preLoop() {
		shader_.load(gpuDevice_);

		// describe the vertex buffers
		SDL_GPUVertexBufferDescription vertexBufferDescriptions{
			.slot = 0,
			.pitch = sizeof(sdl::Vertex),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
		};

		SDL_GPUColorTargetDescription colorTargetDescription{
			.format = SDL_GetGPUSwapchainTextureFormat(gpuDevice_, getSdlWindow()),
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

		SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{
			.vertex_shader = shader_.vertexShader.get(),
			.fragment_shader = shader_.fragmentShader.get(),
			.vertex_input_state = SDL_GPUVertexInputState{
				.vertex_buffer_descriptions = &vertexBufferDescriptions,
				.num_vertex_buffers = 1,
				.vertex_attributes = shader_.attributes.data(),
				.num_vertex_attributes = (Uint32) shader_.attributes.size()
			},
			.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			.target_info = SDL_GPUGraphicsPipelineTargetInfo{
				.color_target_descriptions = &colorTargetDescription,
				.num_color_targets = 1,
			}
		};
		graphicsPipeline_ = sdl::createGpuGraphicsPipeline(gpuDevice_, pipelineInfo);

		auto transparentSurface = createSdlSurface(1, 1, sdl::color::White);
		texture_ = sdl::uploadSurface(gpuDevice_, transparentSurface.get());

		sampler_ = sdl::createGpuSampler(gpuDevice_, SDL_GPUSamplerCreateInfo{
			.min_filter = SDL_GPU_FILTER_NEAREST,
			.mag_filter = SDL_GPU_FILTER_NEAREST,
			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
		});
	}

	void RobotWindow::renderImGui(const sdl::DeltaTime& deltaTime) {
		ImGui::MainWindow("Main", [&]() {
			ImGui::Button("Hello", {100, 100});
			ImGui::Button("World", {50, 50});
		});
	}

	void RobotWindow::renderFrame(const sdl::DeltaTime& deltaTime, SDL_GPUTexture* swapchainTexture, SDL_GPUCommandBuffer* commandBuffer) {
		SDL_GPUColorTargetInfo targetInfo{
			.texture = swapchainTexture,
			.clear_color = clearColor_,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
		};
		graphic_.clear();
		graphic_.addRectangle({-0.5f, -0.5f}, {0.5f, 0.5f}, sdl::color::Blue);
		graphic_.uploadToGpu(gpuDevice_, commandBuffer);

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &targetInfo, 1, nullptr);

		int w, h;
		SDL_GetWindowSize(window_, &w, &h);
		SDL_GPUViewport viewPort{
			.x = 0,
			.y = 0,
			.w = static_cast<float>(w),
			.h = static_cast<float>(h),
			.min_depth = 0.0f,
			.max_depth = 1.0f
		};

		/*
		SDL_GPUViewport viewPort{
			.x = -100,
			.y = -100,
			.w = 100.f,
			.h = 100.f,
			.min_depth = 0.f,
			.max_depth = 1.f
		};
		SDL_SetGPUViewport(renderPass, &viewPort);
		*/
		shader_.uploadProjectionMatrix(commandBuffer, glm::mat4{1.f});
		SDL_GPUTextureSamplerBinding samplerBinding{
			.texture = texture_.get(),
			.sampler = sampler_.get()
		};
		
		SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline_.get());
		graphic_.bindBuffers(renderPass);

		SDL_BindGPUFragmentSamplers(
			renderPass,
			0,
			&samplerBinding,
			1
		);
		graphic_.draw(renderPass);

		SDL_EndGPURenderPass(renderPass);
	}

	void RobotWindow::processEvent(const SDL_Event& windowEvent) {
		switch (windowEvent.type) {
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				sdl::Window::quit();
				break;
			case SDL_EVENT_QUIT:
				sdl::Window::quit();
				break;
			case SDL_EVENT_KEY_DOWN:
				switch (windowEvent.key.key) {
					case SDLK_ESCAPE:
						sdl::Window::quit();
						break;
				}
				break;
		}
	}

}