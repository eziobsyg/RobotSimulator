#include "GLSL.h"
#include "Component.h"
#include "Shape.h"

void Component::addChild(shared_ptr<Component> child) {
	children.push_back(child);
}


void Component::init() {
	Shape::init();
	// Send the position array to the GPU
	glGenBuffers(1, &jointPosBufID);
	glBindBuffer(GL_ARRAY_BUFFER, jointPosBufID);
	glBufferData(GL_ARRAY_BUFFER, jointPosBuf.size() * sizeof(float), &jointPosBuf[0], GL_STATIC_DRAW);

	// Send the normal array to the GPU
	if (!jointNorBuf.empty()) {
		glGenBuffers(1, &jointNorBufID);
		glBindBuffer(GL_ARRAY_BUFFER, jointNorBufID);
		glBufferData(GL_ARRAY_BUFFER, jointNorBuf.size() * sizeof(float), &jointNorBuf[0], GL_STATIC_DRAW);
	}

	// Send the texture array to the GPU
	if (!jointTexBuf.empty()) {
		glGenBuffers(1, &jointTexBufID);
		glBindBuffer(GL_ARRAY_BUFFER, jointTexBufID);
		glBufferData(GL_ARRAY_BUFFER, jointTexBuf.size() * sizeof(float), &jointTexBuf[0], GL_STATIC_DRAW);
	}

	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);

}

void Component::draw(const shared_ptr<Program> prog, shared_ptr<MatrixStack> MV) {
	// Bind position buffer
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// Bind normal buffer
	int h_nor = prog->getAttribute("aNor");
	if (h_nor != -1 && norBufID != 0) {
		glEnableVertexAttribArray(h_nor);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID);
		glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	}

	// Bind texcoords buffer
	int h_tex = prog->getAttribute("aTex");
	if (h_tex != -1 && texBufID != 0) {
		glEnableVertexAttribArray(h_tex);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID);
		glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	}
	
	MV->pushMatrix();
	MV->translate(jointTranslation);
	MV->rotate(jointRotation.x, vec3(1.0f, 0.0f, 0.0f));
	MV->rotate(jointRotation.y, vec3(0.0f, 1.0f, 0.0f));
	MV->rotate(jointRotation.z, vec3(0.0f, 0.0f, 1.0f));

	if (index != 0) {
		MV->pushMatrix();

		glBindBuffer(GL_ARRAY_BUFFER, jointPosBufID);
		glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
		if (h_nor != -1 && jointNorBufID != 0) {
			glBindBuffer(GL_ARRAY_BUFFER, jointNorBufID);
			glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
		}

		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
		int sphereCount = jointPosBuf.size() / 3;
		glDrawArrays(GL_TRIANGLES, 0, sphereCount);

		MV->popMatrix();

		glBindBuffer(GL_ARRAY_BUFFER, posBufID);
		glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
		if (h_nor != -1 && norBufID != 0) {
			glBindBuffer(GL_ARRAY_BUFFER, norBufID);
			glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
		}
	}
	MV->pushMatrix();

	MV->translate(meshTranslation);
	MV->rotate(meshRotation.x, vec3(1.0f, 0.0f, 0.0f));
	MV->rotate(meshRotation.y, vec3(0.0f, 1.0f, 0.0f));
	MV->rotate(meshRotation.z, vec3(0.0f, 0.0f, 1.0f));
	MV->scale(meshScale);

	glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, value_ptr(MV->topMatrix()));
	// Draw
	int count = posBuf.size() / 3; // number of indices to be rendered
	glDrawArrays(GL_TRIANGLES, 0, count);

	// Disable and unbind
	if (h_tex != -1) {
		glDisableVertexAttribArray(h_tex);
	}
	if (h_nor != -1) {
		glDisableVertexAttribArray(h_nor);
	}
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	MV->popMatrix();

	for (auto& child : children) {
		child->draw(prog, MV);
	}

	MV->popMatrix();

	GLSL::checkError(GET_FILE_LINE);
}