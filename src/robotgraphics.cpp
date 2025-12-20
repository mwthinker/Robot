#include "robotgraphics.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/geometric.hpp>

#include <cmath>
#include <algorithm>

namespace robot {

	namespace {

		/// Converts the joint angles for the C-code for the robot to angles
		/// suited for the DH-representation (and the real robot).
		std::array<float, 6> convertAngles(const std::array<float, 6>& angles) {
			// Angles[2] is defined relative to the horizontal plane
			return {
				angles[0],
				angles[1] - Pi / 2,
				angles[2] + Pi - angles[1],
				angles[3],
				(angles[4] + Pi) * (-1),
				angles[5] - Pi
			};
		}
	}

	RobotGraphics::RobotGraphics() {
		initDefaultDH();
	};

	glm::mat4 RobotGraphics::getH(float theta, int n) const {
		// Using the standard DH-representation.
		// GLM uses column-major ordering, so we must transpose
		float ca = std::cos(dh_.alpha[n]);
		float sa = std::sin(dh_.alpha[n]);
		float ct = std::cos(theta);
		float st = std::sin(theta);

		return glm::mat4{
			 ct,                       st,        0, 0,
			-st * ca,             ct * ca,       sa, 0,
			 st * sa,            -ct * sa,       ca, 0,
			 dh_.a[n] * ct, dh_.a[n] * st, dh_.d[n], 1
		};
	}

	void RobotGraphics::draw(Graphic& graphic, const std::array<float, 6>& angles, int viewportWidth, int viewportHeight) {
		auto thetas = convertAngles(angles);

		glm::mat4 h1 = getH(thetas[0], 0);
		glm::mat4 h2 = getH(thetas[1], 1);
		glm::mat4 h3 = getH(thetas[2], 2);
		glm::mat4 h4 = getH(thetas[3], 3);
		glm::mat4 h5 = getH(thetas[4], 4);
		glm::mat4 h6 = getH(thetas[5], 5);

		glm::vec4 zero{0, 0, 0, 1.f};
		jointPositions_[0] = zero;
		jointPositions_[1] = h1 * zero;
		jointPositions_[2] = h1 * h2 * zero;
		jointPositions_[3] = h1 * h2 * h3 * zero;
		jointPositions_[4] = h1 * h2 * h3 * h4 * zero;
		jointPositions_[5] = h1 * h2 * h3 * h4 * h5 * zero;
		glm::mat4 h = h1 * h2 * h3 * h4 * h5 * h6;
		jointPositions_[6] = h * zero; //pos[6] = TCP!

		// Draw the links of the robot.
		auto color = sdl::Color::createU32(230, 100, 40);

		graphic.pushMatrix(); // Bas-klumpen som roboten sitter på
		graphic.scale(glm::vec3{1.0f, 0.8f, 0.3f});
		graphic.translate(glm::vec3{0.0f, 0.0f, 0.15f});
		graphic.addSolidCube(0.3f, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[0]}, glm::vec3{jointPositions_[1]}, 0.05f, 0.05f, color);
		graphic.translate(glm::vec3{0.0f, 0.0f, 0.05f});
		graphic.addSolidSphere(0.05f * 1.8f, 10, 5, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[1]}, glm::vec3{jointPositions_[2]}, 0.05f, 0.03f, color);
		graphic.addSolidSphere(0.05f * 1.4f, 10, 3, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[3]}, glm::vec3{jointPositions_[5]}, 0.03f, 0.02f, color);
		graphic.addSolidSphere(0.03f * 1.4f, 10, 5, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[5]}, glm::vec3{jointPositions_[6]}, 0.02f, 0.01f, color);
		graphic.addSolidSphere(0.02f * 1.4f, 10, 3, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		graphic.translate(glm::vec3{jointPositions_[6]});
		graphic.addSolidSphere(0.01f * 1.1f, 3, 3, color);
		graphic.popMatrix();

		// Draws the TCP frame.
		drawFrame(graphic, h, 0.2f, viewportWidth, viewportHeight);

		// Draws base frame.
		drawFrame(graphic,
			glm::mat4{
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0.005f,
				0, 0, 0, 1
			}, 
			0.4f,
			viewportWidth, viewportHeight
		);
	}

	void RobotGraphics::drawFrame(Graphic& graphic, const glm::mat4& h, float size, int viewportWidth, int viewportHeight) const {
		float pixelSize = 1.8f;

		glm::vec3 origin = h[3];
		glm::vec3 xAxis = h[0];
		
		graphic.addLine(
			origin,
			origin + xAxis * size,
			pixelSize, sdl::color::Red, viewportWidth, viewportHeight
		);
		glm::vec3 yAxis = h[1];
		graphic.addLine(
			origin,
			origin + yAxis * size,
			pixelSize, sdl::color::Green, viewportWidth, viewportHeight
		);
		glm::vec3 zAxis = h[2];
		graphic.addLine(
			origin,
			origin + zAxis * size,
			pixelSize, sdl::color::Blue, viewportWidth, viewportHeight
		);
	}

	void RobotGraphics::drawWorkspace(Graphic& graphic, int viewPortWidht, int viewPortHeight) const {
		//draw a square
		graphic.addLine(workspacePositions_[0], workspacePositions_[1], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[1], workspacePositions_[2], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[2], workspacePositions_[3], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[3], workspacePositions_[0], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		////draw a square
		graphic.addLine(workspacePositions_[4], workspacePositions_[5], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[5], workspacePositions_[6], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[6], workspacePositions_[7], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[7], workspacePositions_[4], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		////draw the lines connecting the both squares
		graphic.addLine(workspacePositions_[0], workspacePositions_[4], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[1], workspacePositions_[5], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[2], workspacePositions_[6], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
		graphic.addLine(workspacePositions_[3], workspacePositions_[7], 3.f, sdl::color::White, viewPortWidht, viewPortHeight);
	}

	void RobotGraphics::setWorkspace(float xMin, float yMin, float zMin,
		float xMax, float yMax, float zMax,
		const glm::mat4& hBase2rBase) {

		workspacePositions_[0] = glm::vec4(xMin, yMin, zMin, 0);
		workspacePositions_[1] = glm::vec4(xMax, yMin, zMin, 0);
		workspacePositions_[2] = glm::vec4(xMax, yMax, zMin, 0);
		workspacePositions_[3] = glm::vec4(xMin, yMax, zMin, 0);
		workspacePositions_[4] = glm::vec4(xMin, yMin, zMax, 0);
		workspacePositions_[5] = glm::vec4(xMax, yMin, zMax, 0);
		workspacePositions_[6] = glm::vec4(xMax, yMax, zMax, 0);
		workspacePositions_[7] = glm::vec4(xMin, yMax, zMax, 0);

		for (int i = 0; i < 8; ++i) {
			workspacePositions_[i] = hBase2rBase * workspacePositions_[i];
			workspacePositions_[i] = workspacePositions_[i] * 0.001f; // graphics is in meters
		}
	}

	// --------------------- Private functions ---------------------

	void RobotGraphics::initDefaultDH() {
		// Defined in meters
		dh_.a[0] = 0.070f;
		dh_.a[1] = 0.360f;
		dh_.a[2] = 0;
		dh_.a[3] = 0;
		dh_.a[4] = 0;
		dh_.a[5] = 0;
		dh_.alpha[0] = -Pi / 2;
		dh_.alpha[1] = 0;
		dh_.alpha[2] = Pi / 2;
		dh_.alpha[3] = Pi / 2;
		dh_.alpha[4] = Pi / 2;
		dh_.alpha[5] = 0;
		dh_.d[0] = 0.352f;
		dh_.d[1] = 0;
		dh_.d[2] = 0;
		dh_.d[3] = 0.380f;
		dh_.d[4] = 0;
		dh_.d[5] = 0.065f;
	}

	glm::mat4 RobotGraphics::rotateZ(const glm::vec3& p1, const glm::vec3& p2) const {
		glm::vec3 ez = glm::normalize(p2 - p1);

		// Choose a reference vector that's not parallel to ez
		glm::vec3 up = (std::abs(ez.z) < 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);

		glm::vec3 ex = glm::normalize(glm::cross(up, ez));
		glm::vec3 ey = glm::cross(ez, ex);

		return glm::mat4{
			ex[0], ex[1], ex[2], 0,
			ey[0], ey[1], ey[2], 0,
			ez[0], ez[1], ez[2], 0,
			0, 0, 0, 1
		};
	}

	
	void RobotGraphics::drawCylinderLink(Graphic& graphic, const glm::vec3& pos1, const glm::vec3& pos2, float radie1, float radie2, sdl::Color color) const {
		graphic.translate(pos1);
		auto matrix = rotateZ(pos1, pos2);
		graphic.multiplyMatrix(matrix);
		float length = glm::length(pos2 - pos1);
		graphic.addCylinder(radie1, radie2, length, 10, 10, color);
	}

}
