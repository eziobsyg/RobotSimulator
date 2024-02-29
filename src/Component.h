#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H
#include <string>
#include <cmath>
#include <vector>
#include <memory>
#include <map>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "MatrixStack.h"
#include "Shape.h"
#include "Program.h"

using namespace std;
using namespace glm;

const float PI = 3.14159265f;

class Component : public Shape {
public:
	int index;

	vec3 jointTranslation;
	vec3 jointRotation;
	vec3 meshTranslation;
	vec3 meshRotation;
	vec3 meshScale;

	vector<shared_ptr<Component>> children;
	Component(int i = 0, vec3 jt = vec3(0.0f), vec3 jr = vec3(0.0f), vec3 mt = vec3(0.0f), vec3 mr = vec3(0.0f), vec3 ms = vec3(1.0f)) :
		index(i), jointTranslation(jt), jointRotation(jr), meshTranslation(mt), meshRotation(mr), meshScale(ms) {};

	void addChild(shared_ptr<Component> child);

	void draw (const shared_ptr<Program> prog, shared_ptr<MatrixStack> MV);

	void init() override;

	void loadJointMesh(const string& jointMeshName);

protected:
	std::vector<float> jointPosBuf;
	std::vector<float> jointNorBuf;
	std::vector<float> jointTexBuf;
	unsigned jointPosBufID;
	unsigned jointNorBufID;
	unsigned jointTexBufID;

};

#endif