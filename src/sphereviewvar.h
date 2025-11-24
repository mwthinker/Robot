#pragma once

#include <glm/gtc/constants.hpp>

namespace robot {

	class SphereViewVar {
	public:
		float phi = 1.f;
		float theta = 1.f;
		float r = 0.f;

		SphereViewVar() = default;

		SphereViewVar(float r, float phi, float theta)
			: phi{phi}
			, theta{theta}
			, r{r} {
		}

		void addPhi(float add) {
			phi += add;
		}

		void addTheta(float add) {
			theta += add;
			if (!(theta < glm::pi<float>() && theta > 0)) {
				theta -= add;
			}
		}

		void addR(float add) {
			r += add;
			if (r < 0) {
				r -= add;
			}
		}
	};

}
