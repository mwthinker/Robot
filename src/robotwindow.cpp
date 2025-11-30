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

		sdl::GpuTexture createDepthTexture(SDL_GPUDevice* gpuDevice, int width, int height, SDL_GPUSampleCount sampleCount) {
			SDL_PropertiesID props = SDL_CreateProperties();
			// Only for D3D12 to ensure depth is cleared to 1.0f, ignored on other backends
			SDL_SetFloatProperty(props, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

			SDL_GPUTextureCreateInfo info{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
				.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
				.width = static_cast<Uint32>(width),
				.height = static_cast<Uint32>(height),
				.layer_count_or_depth = 1,
				.num_levels = 1,
				.sample_count = sampleCount,
				.props = props
			};

			auto texture = sdl::createGpuTexture(gpuDevice, info);
			SDL_DestroyProperties(props);
			return texture;
		}

		SDL_GPUTextureCreateInfo createColorTextureInfo(int width, int height, SDL_GPUTextureFormat format, SDL_GPUSampleCount sampleCount) {
			return SDL_GPUTextureCreateInfo{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = format,
				.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
				.width = static_cast<Uint32>(width),
				.height = static_cast<Uint32>(height),
				.layer_count_or_depth = 1,
				.num_levels = 1,
				.sample_count = sampleCount
			};
		}

		sdl::GpuTexture createColorTexture(SDL_GPUDevice* gpuDevice, int width, int height, SDL_GPUSampleCount sampleCount) {
			SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
			SDL_GPUTextureCreateInfo textureCreateInfo = createColorTextureInfo(width, height, format, sampleCount);
			
			if (!SDL_GPUTextureSupportsSampleCount(gpuDevice, format, sampleCount)) {
				// Fallback to no multisampling
				textureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
			}

			if (textureCreateInfo.sample_count == SDL_GPU_SAMPLECOUNT_1) {
				textureCreateInfo.usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
			}
			
			return sdl::createGpuTexture(
				gpuDevice,
				textureCreateInfo
			);
		}

		sdl::GpuTexture createResolveTexture(SDL_GPUDevice* gpuDevice, int width, int height) {
			SDL_GPUTextureCreateInfo textureCreateInfo = {
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
				.width = static_cast<Uint32>(width),
				.height = static_cast<Uint32>(height),
				.layer_count_or_depth = 1,
				.num_levels = 1
			};

			return sdl::createGpuTexture(
				gpuDevice,
				textureCreateInfo
			);
		}
	}

	RobotWindow::RobotWindow() {
		setSize(800, 800);
		setTitle("Robot");
		setShowDemoWindow(false);
		setShowColorWindow(false);
	}

	void RobotWindow::preLoop() {
		shader_.load(gpuDevice_);

		setupPipeline();
	}

	void RobotWindow::setupPipeline() {
		// Check MSAA support first
		if (!SDL_GPUTextureSupportsSampleCount(gpuDevice_, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, gpuSampleCount_)) {
			gpuSampleCount_ = SDL_GPU_SAMPLECOUNT_1;
		}

		// describe the vertex buffers
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
				.num_vertex_attributes = (Uint32)shader_.attributes.size()
		},
			.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			.multisample_state = SDL_GPUMultisampleState{
				.sample_count = gpuSampleCount_
		},
			.depth_stencil_state = depthStencilState,
			.target_info = SDL_GPUGraphicsPipelineTargetInfo{
				.color_target_descriptions = &colorTargetDescription,
				.num_color_targets = 1,
				.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
				.has_depth_stencil_target = true
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

		int w, h;
		SDL_GetWindowSize(window_, &w, &h);

		depthTexture_ = createDepthTexture(
			gpuDevice_,
			w, h,
			gpuSampleCount_
		);
		renderTexture_ = createColorTexture(
			gpuDevice_,
			w, h,
			gpuSampleCount_
		);
		resolveTexture_ = createResolveTexture(
			gpuDevice_,
			w, h
		);
	}

	void RobotWindow::renderImGui(const sdl::DeltaTime& deltaTime) {
		ImGui::MainWindow("Main", [&]() {
			ImGui::Begin("Robot Control");
			ImGui::Text("Use arrow keys to rotate view");
			ImGui::Text("Use PageUp/PageDown to zoom in/out");
			ImGui::Text("Use Q/A, W/S, E/D, R/F, T/G, Y/H to control joint angles");

			for (size_t i = 0; i < angles_.size(); ++i) {
				ImGui::SliderFloat(
					fmt::format("Joint {}", i + 1).c_str(),
					&angles_[i],
					-180.f,
					180.f
				);
			}
			ImGui::End();

			const auto& jointPositions = robot_.getJointPositions();
			ImGui::Begin("Joint Positions");
			for (size_t i = 0; i < jointPositions.size(); ++i) {
				const auto& pos = jointPositions[i];
				ImGui::Text(
					"Joint %d: (%.2f, %.2f, %.2f)",
					static_cast<int>(i + 1),
					pos.x,
					pos.y,
					pos.z
				);
			}
			ImGui::End();

			// Camera position
			ImGui::Begin("Camera Position");
			ImGui::SliderFloat("Radius", &view_.r, 0.1f, 20.f);
			ImGui::SliderFloat("Theta", &view_.theta, 0.01f, glm::pi<float>()/2);
			ImGui::SliderFloat("Phi", &view_.phi, -glm::pi<float>(), glm::pi<float>());
			ImGui::End();

			// Graphic settings
			ImGui::Begin("Graphic Settings");
			
			ImGui::Checkbox("Display Light Bulb", &displayLightBulb_);
			ImGui::SliderFloat("Light Position", &lightPos_.z, 2.f, 10.f);
			static ImVec4 color = lightColor_;
			ImGui::ColorEdit3("Light Color", (float*)& color);
			lightColor_ = sdl::Color{color.x, color.y, color.z, color.w};
			ImGui::SliderFloat("Light Radius", &lightRadius_, 0.1f, 20.f);
			ImGui::SliderFloat("Ambient Strength", &lightAmbientStrength_, 0.f, 1.f);
			ImGui::SliderFloat("Shininess", &lightShininess_, 1.f, 128.f);

			std::array items = {"SDL_GPU_SAMPLECOUNT_1", "SDL_GPU_SAMPLECOUNT_2", "SDL_GPU_SAMPLECOUNT_4", "SDL_GPU_SAMPLECOUNT_8"};
			static int item = static_cast<int>(gpuSampleCount_);
			if (ImGui::Combo("MSAA Sample Count", &item, items.data(), static_cast<int>(items.size()))) {
				gpuSampleCount_ = static_cast<SDL_GPUSampleCount>(item);
				setupPipeline();
			}
			
			ImGui::End();
		});
	}

	void RobotWindow::renderFrame(const sdl::DeltaTime& deltaTime, SDL_GPUTexture* swapchainTexture, SDL_GPUCommandBuffer* commandBuffer) {
		SDL_GPUDepthStencilTargetInfo depthTargetInfo{
			.texture = depthTexture_.get(),
			.clear_depth = 1.0f,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_DONT_CARE,
			.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
			.cycle = false
		};
		camera_.update(deltaTime, view_);

		graphic_.clear();
		graphic_.loadIdentityMatrix();

		std::array<float, 6> anglesInRad_;
		for (size_t i = 0; i < angles_.size(); ++i) {
			anglesInRad_[i] = glm::radians(angles_[i]);
		}

		robot_.draw(graphic_, anglesInRad_);
		drawFloor();
		if (displayLightBulb_) {
			graphic_.loadIdentityMatrix();
			graphic_.translate(lightPos_);
			graphic_.addSolidSphere(0.1f, 10, 10, lightColor_);
		}
		
		graphic_.uploadToGpu(gpuDevice_, commandBuffer);

		SDL_GPUColorTargetInfo colorTargetInfo{
			.texture = renderTexture_.get(),
			.clear_color = clearColor_,
			.load_op = SDL_GPU_LOADOP_CLEAR
		};
		if (gpuSampleCount_ == SDL_GPU_SAMPLECOUNT_1) {
			colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
		} else {
			colorTargetInfo.store_op = SDL_GPU_STOREOP_RESOLVE;
			colorTargetInfo.resolve_texture = resolveTexture_.get();
		}
		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthTargetInfo);

		int w, h;
		SDL_GetWindowSize(window_, &w, &h);
		reshape(commandBuffer, renderPass, w, h);

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

		SDL_GPUTexture* blitSourceTexture = (colorTargetInfo.resolve_texture != nullptr) ? colorTargetInfo.resolve_texture : colorTargetInfo.texture;
		SDL_GPUBlitInfo blitInfo{
			.source = {
				.texture = blitSourceTexture,
				.x = 0,
				.y = 0,
				.w = (Uint32) w,
				.h = (Uint32) h
			},
			.destination = {
				.texture = swapchainTexture,
				.x = 0,
				.y = 0,
				.w = (Uint32) w,
				.h = (Uint32) h
			},
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.filter = SDL_GPU_FILTER_LINEAR
		};

		SDL_BlitGPUTexture(commandBuffer, &blitInfo);
	}

	void RobotWindow::reshape(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, int width, int height) {
		static const float kFovY = 40;
		
		SDL_GPUViewport viewPort{
			.x = 0,
			.y = 0,
			.w = (float) width,
			.h = (float) height,
			.min_depth = 0.f,
			.max_depth = 1.f
		};
		SDL_SetGPUViewport(renderPass, &viewPort);

		// Compute the viewing parameters based on a fixed fov and viewing
		// a canonical box centered at the origin
		float nearDist = 0.5f * 0.1f / tan((kFovY / 2.f) * Pi / 180.f);
		float farDist = nearDist + 100.f;
		float aspect = (float) width / height;
		auto projection = glm::perspective(glm::radians(kFovY), aspect, nearDist, farDist);

		glm::vec3 center{0.0f, 0.0f, 0.7f};
		glm::vec3 up{0.0f, 0.0f, 1.0f};
		glm::vec3 eye = camera_.getEye();
		glm::mat4 viewMatrix = glm::lookAt(eye, center, up);

		shader_.uploadProjectionMatrix(commandBuffer, projection * viewMatrix);
		shader_.uploadLightingData(commandBuffer, lightPos_, lightRadius_, lightColor_, lightAmbientStrength_, lightShininess_, eye);
	}

	void RobotWindow::drawFloor() {
		const float floorSize = 5.f;
		const float step = 0.5f;
		sdl::Color color1 = sdl::color::html::LightGray;
		sdl::Color color2 = sdl::color::html::Gray;
		for (float x = -floorSize; x < floorSize; x += step) {
			for (float y = -floorSize; y < floorSize; y += step) {
				sdl::Color color = (((int)((x + floorSize) / step) + (int)((y + floorSize) / step)) % 2 == 0) ? color1 : color2;
				graphic_.addRectangle({x, y}, {step, step}, color);
			}
		}
	}

	void RobotWindow::processEvent(const SDL_Event& windowEvent) {
		switch (windowEvent.type) {
			case SDL_EVENT_WINDOW_RESIZED:
				if (SDL_GetWindowID(window_) == windowEvent.window.windowID) {
					depthTexture_ = createDepthTexture(
						gpuDevice_,
						windowEvent.window.data1, windowEvent.window.data2,
						gpuSampleCount_
					);
					renderTexture_ = createColorTexture(
						gpuDevice_,
						windowEvent.window.data1, windowEvent.window.data2,
						gpuSampleCount_
					);
					resolveTexture_ = createResolveTexture(
						gpuDevice_,
						windowEvent.window.data1, windowEvent.window.data2
					);
				}
				break;
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			case SDL_EVENT_QUIT:
				sdl::Window::quit();
				break;
			case SDL_EVENT_KEY_DOWN:
				switch (windowEvent.key.key) {
					case SDLK_ESCAPE:
						sdl::Window::quit();
						break;
					case SDLK_LEFT:
						view_.phi += -0.05f;
						break;
					case SDLK_RIGHT:
						view_.phi += 0.05f;
						break;
					case SDLK_UP:
						view_.theta += -0.05f;
						break;
					case SDLK_DOWN:
						view_.theta += 0.05f;
						break;
					case SDLK_PAGEUP:
						view_.r += 0.1f;
						break;
					case SDLK_PAGEDOWN:
						view_.r += -0.1f;
						break;

					case SDLK_Q:
						angles_[0] += 5.f;
						break;
					case SDLK_A:
						angles_[0] -= 5.f;
						break;
					case SDLK_W:
						angles_[1] += 5.f;
						break;
					case SDLK_S:
						angles_[1] -= 5.f;
						break;
					case SDLK_E:
						angles_[2] += 5.f;
						break;
					case SDLK_D:
						angles_[2] -= 5.f;
						break;
					case SDLK_R:
						angles_[3] += 5.f;
						break;
					case SDLK_F:
						angles_[3] -= 5.f;
						break;
					case SDLK_T:
						angles_[4] += 5.f;
						break;
					case SDLK_G:
						angles_[4] -= 5.f;
						break;
					case SDLK_Y:
						angles_[5] += 5.f;
						break;
					case SDLK_H:
						angles_[5] -= 5.f;
						break;
				}
				break;
		}
	}

}