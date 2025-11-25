#ifndef TESTIMGUIWINDOW_H
#define TESTIMGUIWINDOW_H

#include "graphic.h"
#include "sphereviewvar.h"
#include "robotgraphics.h"

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

		SphereViewVar view_{8.f, 2.f, 1.f};
		std::array<float, 6> angles_{0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
	};

}

#endif
