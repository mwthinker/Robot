#include "robotgraphics.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/geometric.hpp>

#include <cmath>
#include <algorithm>

namespace robot {

	glm::mat4 RobotGraphics::getH(const float theta, const int n) const {
		// Using the standard DH-representation.
		// GLM uses column-major ordering, so we must transpose
		const float ca = std::cos(dh_.alpha[n]);
		const float sa = std::sin(dh_.alpha[n]);
		const float ct = std::cos(theta);
		const float st = std::sin(theta);

		// Transposed: columns become what were rows in the DH matrix
		return glm::mat4{
			ct, st, 0, 0,           // Column 0
			-st * ca, ct * ca, sa, 0,          // Column 1
			st * sa, -ct * sa, ca, 0,          // Column 2
			dh_.a[n] * ct, dh_.a[n] * st, dh_.d[n], 1    // Column 3 (translation)
		};
	}

	void RobotGraphics::draw(Graphic& graphic, const std::array<float, 6>& angles) {
		float thetas[6];
		std::copy(angles.begin(), angles.end(), thetas);
		convertAngles(thetas);

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
		jointPositions_[6] = h * zero;			//pos[6] = TCP!

		// Draw the links of the robot.
		//glLineWidth(3.0f);
		auto color = sdl::Color::createU32(230, 100, 40);

		graphic.pushMatrix(); // Bas-klumpen som roboten sitter på
		graphic.scale(glm::vec3{1.0f, 0.8f, 0.3f});
		graphic.translate(glm::vec3{0.0f, 0.0f, 0.15f});
		graphic.addSolidCube(0.3f, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[0]}, glm::vec3{jointPositions_[1]}, 0.05f, 0.05f);
		graphic.translate(glm::vec3{0.0f, 0.0f, 0.05f});
		graphic.addSolidSphere(0.05f * 1.8f, 10, 5, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[1]}, glm::vec3{jointPositions_[2]}, 0.05f, 0.03f);
		graphic.addSolidSphere(0.05f * 1.4f, 10, 3, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[3]}, glm::vec3{jointPositions_[5]}, 0.03f, 0.02f);
		graphic.addSolidSphere(0.03f * 1.4f, 10, 5, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		drawCylinderLink(graphic, glm::vec3{jointPositions_[5]}, glm::vec3{jointPositions_[6]}, 0.02f, 0.01f);
		graphic.addSolidSphere(0.02f * 1.4f, 10, 3, color);
		graphic.popMatrix();

		graphic.pushMatrix();
		graphic.translate(glm::vec3{jointPositions_[6]});
		graphic.addSolidSphere(0.01f * 1.1f, 3, 3, color);
		graphic.popMatrix();

		// Draws the TCP frame.
		drawFrame(h, 0.2f);

		// Draws base frame.
		drawFrame(
			glm::mat4{
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0.005f,
				0, 0, 0, 1
			}, 
			0.4f
		);
	}

	void RobotGraphics::drawFrame(const glm::mat4& h, float size) const {
		/*
		glLineWidth(1.8f);

		// Draw the x-axis in red.
		glColor3f(1, 0, 0);
		glBegin(GL_LINES);
		glVertex3d(h[0][3], h[1][3], h[2][3]);
		glVertex3d(h[0][3] + h[0][0] * size, h[1][3] + h[1][0] * size, h[2][3] + h[2][0] * size);
		glEnd();

		// Draw the y-axis in blue.
		glColor3f(0, 0, 1);
		glBegin(GL_LINES);
		{
			glVertex3d(h[0][3], h[1][3], h[2][3]);
			glVertex3d(h[0][3] + h[0][1] * size, h[1][3] + h[1][1] * size, h[2][3] + h[2][1] * size);
		}
		glEnd();

		// Draw the z-axis in green.
		glColor3f(0, 1, 0);
		glBegin(GL_LINES);
		{
			glVertex3d(h[0][3], h[1][3], h[2][3]);
			glVertex3d(h[0][3] + h[0][2] * size, h[1][3] + h[1][2] * size, h[2][3] + h[2][2] * size);
		}
		glEnd();
		*/
	}

	void RobotGraphics::drawWorkspace() const {
		/*
		glLineWidth(1.8f);
		glColor3f(0, 0, 0);
		glBegin(GL_LINES);
		//draw a square
		drawLine(workspacePositions_[0], workspacePositions_[1]);
		drawLine(workspacePositions_[1], workspacePositions_[2]);
		drawLine(workspacePositions_[2], workspacePositions_[3]);
		drawLine(workspacePositions_[3], workspacePositions_[0]);
		//draw a square
		drawLine(workspacePositions_[4], workspacePositions_[5]);
		drawLine(workspacePositions_[5], workspacePositions_[6]);
		drawLine(workspacePositions_[6], workspacePositions_[7]);
		drawLine(workspacePositions_[7], workspacePositions_[4]);
		//draw the lines connecting the both squares
		drawLine(workspacePositions_[0], workspacePositions_[4]);
		drawLine(workspacePositions_[1], workspacePositions_[5]);
		drawLine(workspacePositions_[2], workspacePositions_[6]);
		drawLine(workspacePositions_[3], workspacePositions_[7]);
		glEnd();
		*/
	}

	void RobotGraphics::setWorkspace(const float xMin, const float yMin, const float zMin,
		const float xMax, const float yMax, const float zMax,
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

	void RobotGraphics::convertAngles(float angles[]) const {
		// Anles[2] is defined relative to the horizontal plane
		angles[2] = angles[2] + Pi - angles[1];

		angles[1] = angles[1] - Pi / 2;
		angles[4] = (angles[4] + Pi) * (-1);
		angles[5] = (angles[5] - Pi);
	}

	void RobotGraphics::drawLine(Graphic& graphic, const glm::vec3& p1, const glm::vec3& p2) const {
		graphic.addLine(p1, p2, 3.f, sdl::color::White);
	}
	
	void RobotGraphics::rotateZ(const glm::vec3& p1, const glm::vec3& p2, float matrix[]) const {
		glm::vec3 ez = glm::normalize(p2 - p1);

		int index = 0;
		if (std::abs(ez[index]) < std::abs(ez[1])) {
			index = 1;
		}
		if (std::abs(ez[index]) < std::abs(ez[2])) {
			index = 2;
		}

		float x, y, z;
		switch (index) {
			case 0:
				y = 0.4f;
				z = 0.4f;
				x = (-ez[1] * y - ez[2] * z) / ez[0];
				break;
			case 1:
				x = 0.4f;
				z = 0.4f;
				y = (-ez[0] * x - ez[2] * z) / ez[1];
				break;
			case 2:
				x = 0.4f;
				y = 0.4f;
				z = (-ez[0] * x - ez[1] * y) / ez[2];
				break;
		}
		glm::vec3 ex = glm::normalize(glm::vec3{x, y, z});
		//cross([a1 a2 a3],[b1 b2 b3]) => [a2*b3-a3*b2, a3*b1-a1*b3, a1*b2-a2*b1]
		//glm::vec4 ey(ez[1]*ex[2]-ez[2]*ex[1], ez[2]*ex[0]-ez[0]*ex[2], ez[0]*ex[1]-ez[1]*ex[0]);
		glm::vec3 ey = glm::cross(ez, ex);

		matrix[0]  = ex[0]; matrix[1]  = ex[1];  matrix[2]  = ex[2]; matrix[3]  = 0;
		matrix[4]  = ey[0]; matrix[5]  = ey[1];  matrix[6]  = ey[2]; matrix[7]  = 0;
		matrix[8]  = ez[0]; matrix[9]  = ez[1];  matrix[10] = ez[2]; matrix[11] = 0;
		matrix[12] = 0;     matrix[13] = 0;      matrix[14] = 0;     matrix[15] = 1;
	}

	
	void RobotGraphics::drawCylinderLink(Graphic& graphic, const glm::vec3& pos1, const glm::vec3& pos2, float radie1, float radie2) const {
		float field[16];
		rotateZ(pos1, pos2, field);
		glm::vec3 diff = pos2 - pos1;
		graphic.pushMatrix();
		graphic.translate(pos1);
		graphic.multiplyMatrix(glm::mat4{
			field[0], field[4], field[8], field[12],
			field[1], field[5], field[9], field[13],
			field[2], field[6], field[10], field[14],
			field[3], field[7], field[11], field[15]});
		float length = (float) diff.length();
		graphic.addCylinder(radie1, radie2, length, 10, 10, sdl::color::Green);
	}

}
