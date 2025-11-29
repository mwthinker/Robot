#pragma once

#include "sphereviewvar.h"

#include <sdl/util.h>

#include <glm/glm.hpp>

namespace robot {

	class Camera {
	public:
		Camera() = default;
		Camera(const SphereViewVar& view)
			: view_{view} {
		}

		void update(const sdl::DeltaTime& deltaTime, const SphereViewVar& view);

		glm::vec3 getEye() const;

	private:
		SphereViewVar view_{};
	};

}
