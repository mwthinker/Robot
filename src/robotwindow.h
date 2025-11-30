#ifndef TESTIMGUIWINDOW_H
#define TESTIMGUIWINDOW_H

#include "graphic.h"
#include "sphereviewvar.h"
#include "robotgraphics.h"
#include "camera.h"
#include "shader.h"

#include <sdl/window.h>

namespace robot {

	class RobotWindow : public sdl::Window {
	public:
		RobotWindow();

	private:
		void preLoop() override;

		void processEvent(const SDL_Event& windowEvent) override;

		void renderImGui(const sdl::DeltaTime& deltaTime) override;

		void renderFrame(const sdl::DeltaTime& deltaTime, SDL_GPUTexture* swapchainTexture, SDL_GPUCommandBuffer* commandBuffer) override;

		void reshape(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, int width, int height);

		void drawFloor();

		Graphic graphic_;
		Shader shader_;
		sdl::GpuGraphicsPipeline graphicsPipeline_;
		sdl::GpuSampler sampler_;
		sdl::GpuTexture texture_;
		sdl::GpuTexture depthTexture_;
		sdl::GpuTexture renderTexture_;
		sdl::GpuTexture resolveTexture_;
		
		SDL_GPUSampleCount gpuSampleCount_ = SDL_GPU_SAMPLECOUNT_1;

		RobotGraphics robot_;

		SphereViewVar view_{
			.phi = -1.4f,
			.theta = 1,
			.r = 8.5
		};
		std::array<float, 6> angles_{0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

		Camera camera_{view_};

		glm::vec3 lightPos_{0.0f, 0.f, 5.0f};
		sdl::Color lightColor_ = sdl::color::White;
		float lightRadius_ = 10.f;
		float lightAmbientStrength_ = 0.2f;
	};

}

#endif
