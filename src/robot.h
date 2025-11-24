#ifndef ROBOT_H
#define ROBOT_H

#include "init.h"
#include "matrix44.h"
#include "vector4.h"
#include <iostream>
#include "robotGraphics.h"
#include "robotHaptics.h"

/* A robot class. */
class Robot : public RobotGraphics, public RobotHaptics {
public:
	
	/* Creates a non-working robot */
	Robot() {};
	
	/* Creates a robot where initial joint angles is found in thetas */
	Robot(const double thetas[]) : RobotHaptics(thetas), RobotGraphics() {};
};

#endif