#ifndef TESTIMGUIWINDOW_H
#define TESTIMGUIWINDOW_H

#include "graphic.h"
#include "sphereviewvar.h"
#include "robotgraphics.h"
#include "camera.h"

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
		sdl::Shader shader_;
		sdl::GpuGraphicsPipeline graphicsPipeline_;
		sdl::GpuSampler sampler_;
		sdl::GpuTexture texture_;
		sdl::GpuTexture depthTexture_;
		RobotGraphics robot_;

		SphereViewVar view_{
			.phi = -1.4f,
			.theta = 1,
			.r = 8.5
		};
		std::array<float, 6> angles_{0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

		Camera camera_{view_};
	};

}

#endif
