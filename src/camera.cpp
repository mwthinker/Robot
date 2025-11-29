#include "camera.h"

namespace robot {

	namespace {

		float abs(float x) {
			return (x < 0.f) ? -x : x;
		}

		float sign(float x) {
			if (abs(x) < 0.01f) {
				return 0.f;
			}

			return (x > 0.f) ? 1.f : ((x < 0.f) ? -1.f : 0.f);
		}
	}

	void Camera::update(const sdl::DeltaTime& deltaTime, const SphereViewVar& view) {
		float deltaSeconds = std::chrono::duration<float>(deltaTime).count();
		view_.phi -= sign(view_.phi - view.phi) * 1.f * deltaSeconds;
		view_.theta -= sign(view_.theta - view.theta) * 1.f * deltaSeconds;
		view_.r -= sign(view_.r - view.r) * 4.f * deltaSeconds;
	}

	glm::vec3 Camera::getEye() const {
		return glm::vec3{
			view_.r * std::cos(view_.phi) * std::sin(view_.theta),
			view_.r * std::sin(view_.phi) * std::sin(view_.theta),
			view_.r * std::cos(view_.theta)
		};
	}

}
