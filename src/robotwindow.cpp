#include "robotwindow.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <sdl/gpuutil.h>

namespace robot {

	namespace {

		sdl::GpuTexture createDepthTexture(SDL_GPUDevice* gpuDevice, int width, int height, SDL_GPUSampleCount sampleCount) {
			SDL_PropertiesID props = SDL_CreateProperties();
			// Only for D3D12 to ensure depth is cleared to 1.0f, ignored on other backends
			SDL_SetFloatProperty(props, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

			auto texture = sdl::createGpuTexture(gpuDevice, SDL_GPUTextureCreateInfo{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
				.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
				.width = static_cast<Uint32>(width),
				.height = static_cast<Uint32>(height),
				.layer_count_or_depth = 1,
				.num_levels = 1,
				.sample_count = sampleCount,
				.props = props
			});
			SDL_DestroyProperties(props);
			return texture;
		}

		sdl::GpuTexture createColorTexture(SDL_GPUDevice* gpuDevice, int width, int height, SDL_GPUSampleCount sampleCount) {
			SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
			SDL_GPUTextureCreateInfo textureCreateInfo{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = format,
				.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
				.width = static_cast<Uint32>(width),
				.height = static_cast<Uint32>(height),
				.layer_count_or_depth = 1,
				.num_levels = 1,
				.sample_count = sampleCount
			};
			
			if (!SDL_GPUTextureSupportsSampleCount(gpuDevice, format, sampleCount)) {
				// Fallback to no multisampling
				textureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
			}

			if (textureCreateInfo.sample_count == SDL_GPU_SAMPLECOUNT_1) {
				textureCreateInfo.usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
			}
			
			return sdl::createGpuTexture(gpuDevice, textureCreateInfo);
		}

		sdl::GpuTexture createResolveTexture(SDL_GPUDevice* gpuDevice, int width, int height) {
			return sdl::createGpuTexture(gpuDevice, SDL_GPUTextureCreateInfo{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
				.width = static_cast<Uint32>(width),
				.height = static_cast<Uint32>(height),
				.layer_count_or_depth = 1,
				.num_levels = 1
			});
		}

	}

	RobotWindow::RobotWindow() {
		setSize(1024, 1024);
		setTitle("Robot");
		setShowDemoWindow(false);
		setShowColorWindow(false);
	}

	void RobotWindow::preLoop() {
		setupPipeline();
	}

	void RobotWindow::setupPipeline() {
		graphic_.preLoop(gpuDevice_, gpuSampleCount_);
		robot_.setWorkspace(-100, -100, -100, 100, 100, 100, glm::mat4{1});

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

			static std::array<char, 64> buffer;

			for (size_t i = 0; i < angles_.size(); ++i) {
				char label[16];
				std::snprintf(label, sizeof(label), "Joint %d", (int) i + 1);
				ImGui::SliderFloat(
					label,
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
			
			static int light = 0;
			static ImVec4 color = !lightingData_.lights.empty() ? lightingData_.lights[light].color :sdl::color::Transparent;
			for (int i = 0; i < lightingData_.lights.size(); ++i) {
				char lightLabel[16];
				std::snprintf(lightLabel, sizeof(lightLabel), "Light %d", (int) i + 1);
				if (ImGui::RadioButton(lightLabel, &light, i)) {
					color = lightingData_.lights[light].color;
				}
				if (i < lightingData_.lights.size() - 1) {
					ImGui::SameLine();
				}
			}
			ImGui::SeparatorText("Light");
			if (light < lightingData_.lights.size()) {
				ImGui::Checkbox("Display Light Bulb", &lightingData_.lights[light].enabled);
				ImGui::SliderFloat3("Position", &lightingData_.lights[light].position.x, -10.f, 10.f);
				ImGui::ColorEdit3("Color", (float*)& color);
				lightingData_.lights[light].color = sdl::Color{color.x, color.y, color.z, color.w};
				ImGui::SliderFloat("Radius", &lightingData_.lights[light].radius, 0.1f, 20.f);
				ImGui::SliderFloat("Ambient Strength", &lightingData_.lights[light].ambientStrength, 0.f, 1.f);
				ImGui::SliderFloat("Shininess", &lightingData_.lights[light].shininess, 1.f, 128.f);
			}

			ImGui::SeparatorText("Anti-Aliasing");
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
		camera_.update(deltaTime, view_);

		graphic_.clear();
		graphic_.loadIdentityMatrix();

		std::array<float, 6> anglesInRad_;
		for (size_t i = 0; i < angles_.size(); ++i) {
			anglesInRad_[i] = glm::radians(angles_[i]);
		}
		int w, h;
		SDL_GetWindowSize(window_, &w, &h);

		robot_.draw(graphic_, anglesInRad_, w, h);
		drawFloor();
		for (auto& light : lightingData_.lights) {
			if (light.enabled) {
				graphic_.loadIdentityMatrix();
				graphic_.translate(light.position);
				graphic_.addSolidSphere(0.1f, 10, 10, light.color, DrawMode::NoLight);
			}
		}
		graphic_.loadIdentityMatrix();

		robot_.drawWorkspace(graphic_, w, h);
		//graphic_.addLine({0.f, 0.f, 0.f}, {0.f, 0.f, 3.f}, 3.f, sdl::color::Red, w, h);
		//graphic_.addLine({1.f, 0.f, 0.f}, {1.f, 0.f, 3.f}, 1.f, sdl::color::Red, w, h);
		//graphic_.addLine({0.f, 0.f, 0.5f}, {1.f, 0.f, 0.5f}, 1.f, sdl::color::Red, w, h);

		graphic_.gpuCopyPass(gpuDevice_, commandBuffer);
		reshape(commandBuffer, w, h);

		SDL_GPUDepthStencilTargetInfo depthTargetInfo{
			.texture = depthTexture_.get(),
			.clear_depth = 1.0f,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_DONT_CARE,
			.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
			.cycle = false
		};
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
		
		SDL_GPUViewport viewPort{
			.x = 0,
			.y = 0,
			.w = static_cast<float>(w),
			.h = static_cast<float>(h),
			.min_depth = 0.f,
			.max_depth = 1.f
		};
		SDL_SetGPUViewport(renderPass, &viewPort);

		graphic_.bindAndDraw(gpuDevice_, renderPass);

		SDL_EndGPURenderPass(renderPass);

		SDL_GPUTexture* blitSourceTexture = (colorTargetInfo.resolve_texture != nullptr) ? colorTargetInfo.resolve_texture : colorTargetInfo.texture;
		SDL_GPUBlitInfo blitInfo{
			.source = {
				.texture = blitSourceTexture,
				.x = 0,
				.y = 0,
				.w = static_cast<Uint32>(w),
				.h = static_cast<Uint32>(h)
			},
			.destination = {
				.texture = swapchainTexture,
				.x = 0,
				.y = 0,
				.w = static_cast<Uint32>(w),
				.h = static_cast<Uint32>(h)
			},
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.filter = SDL_GPU_FILTER_LINEAR
		};

		SDL_BlitGPUTexture(commandBuffer, &blitInfo);
	}

	void RobotWindow::reshape(SDL_GPUCommandBuffer* commandBuffer, int width, int height) {
		static constexpr float kFovY = 40;

		// Compute the viewing parameters based on a fixed fov and viewing
		// a canonical box centered at the origin
		static const float nearDist = 0.5f * 0.1f / std::tan(glm::radians(kFovY) / 2.f);
		static const float farDist = nearDist + 100.f;
		const float aspect = static_cast<float>(width) / height;
		auto projection = glm::perspective(glm::radians(kFovY), aspect, nearDist, farDist);

		glm::vec3 center{0.0f, 0.0f, 0.7f};
		glm::vec3 up{0.0f, 0.0f, 1.0f};
		glm::vec3 eye = camera_.getEye();
		glm::mat4 viewMatrix = glm::lookAt(eye, center, up);
		lightingData_.cameraPos = eye;
		graphic_.uploadLightingData(commandBuffer, lightingData_);
		graphic_.uploadProjectionMatrix(commandBuffer, projection, viewMatrix);
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