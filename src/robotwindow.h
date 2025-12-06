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

		void reshape(SDL_GPUCommandBuffer* commandBuffer, int width, int height);

		void drawFloor();

		void setupPipeline();

		Graphic graphic_;
		sdl::GpuGraphicsPipeline graphicsPipeline_;
		sdl::GpuTexture depthTexture_;
		sdl::GpuTexture renderTexture_;
		sdl::GpuTexture resolveTexture_;
		
		SDL_GPUSampleCount gpuSampleCount_ = SDL_GPU_SAMPLECOUNT_4;

		RobotGraphics robot_;

		SphereViewVar view_{
			.phi = -1.4f,
			.theta = 1,
			.r = 8.5
		};
		std::array<float, 6> angles_{0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

		Camera camera_{view_};

		LightingData lightingData_{
			.cameraPos = glm::vec3{0.0f, 0.f, 0.f},
			.lights = {
				Light{
					.position = glm::vec3{-5.f, -5.f, 5.0f},
					.color = sdl::color::White,
					.radius = 13.f,
					.ambientStrength = 0.1f,
					.shininess = 30.f,
					.enabled = true
				},
				Light{
					.position = glm::vec3{-5.f, 5.f, 5.0f},
					.color = sdl::color::White,
					.radius = 13.f,
					.ambientStrength = 0.1f,
					.shininess = 30.f,
					.enabled = true
				},
				Light{
					.position = glm::vec3{5.f, -5.f, 5.0f},
					.color = sdl::color::White,
					.radius = 13.f,
					.ambientStrength = 0.1f,
					.shininess = 30.f,
					.enabled = true
				},
				Light{
					.position = glm::vec3{5.f, 5.f, 5.0f},
					.color = sdl::color::White,
					.radius = 13.f,
					.ambientStrength = 0.1f,
					.shininess = 30.f,
					.enabled = true
				}
			}
		};
	};

}

#endif
