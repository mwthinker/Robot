#ifndef TESTIMGUIWINDOW_H
#define TESTIMGUIWINDOW_H

#include <sdl/window.h>

namespace robot {

	class RobotWindow : public sdl::Window {
	public:
		RobotWindow();

	private:
		void processEvent(const SDL_Event& windowEvent) override;

		void renderImGui(const sdl::DeltaTime& deltaTime) override;
	};

}

#endif
