#ifndef ROBOT_ROBOTGRAPHICS_H
#define ROBOT_ROBOTGRAPHICS_H

#include "graphic.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <array>

namespace robot {

	/// A struct containing the DH-parameters for a robot with 6 degree of freedom.
	struct RobotDHPar {
		float a[6];
		float alpha[6];
		float d[6];
	};

	class RobotGraphics {
	public:
		/// Loads the DH-parameters used in the graphic functions.
		RobotGraphics();

		/// Draws the robot, baseframe and TCP-frame
		void draw(Graphic& graphic, const std::array<float, 6>& angles);

		/// Draws the frame defined by the homogenous transformation
		/// from the base frame to the frame to be drawed.
		void drawFrame(const glm::mat4& h, float size) const;

		/// Draws a white box representing the current workspace.
		void drawWorkspace() const;

		/// Sets the current workspace.
		void setWorkspace(float xMin, float yMin, float zMin,
			float xMax, float yMax, float zMax,
			const glm::mat4& hBase2rBase);

		std::array<glm::vec4, 8> getWorkspace() {
			return workspacePositions_;
		}

		const std::array<glm::vec4, 7>& getJointPositions() const {
			return jointPositions_;
		}

	private:
		std::array<glm::vec4, 7> jointPositions_;
		RobotDHPar dh_;
		std::array<glm::vec4, 8> workspacePositions_;

		/// Returns the homogenous matrix for transformation from frame n to frame n-1
		/// where theta is the angle for joint n. It uses the DH-representation
		/// in calculations.
		glm::mat4 getH(float theta, int n) const;

		/// Loads the default values for the DH-representation (in meter).
		void initDefaultDH();

		void drawLine(Graphic& graphic, const glm::vec3& p1, const glm::vec3& p2) const;

		/// Draws the link for the robot.
		void drawCylinderLink(Graphic& graphic, const glm::vec3& pos1, const glm::vec3& pos2, float radie1, float radie2, sdl::Color color) const;

		glm::mat4 rotateZ(const glm::vec3& p1, const glm::vec3& p2) const;
	};

}

#endif
