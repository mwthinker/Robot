#ifndef TESTIMGUIWINDOW_H
#define TESTIMGUIWINDOW_H

#include "graphic.h"

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

		Graphic graphic_;
		sdl::Shader shader_;
		sdl::GpuGraphicsPipeline graphicsPipeline_;
		sdl::GpuSampler sampler_;
		sdl::GpuTexture texture_;
	};

}

#endif
