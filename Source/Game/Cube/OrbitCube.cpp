#pragma once

#include "Cube/OrbitCube.h"

OrbitCube::OrbitCube(const std::filesystem::path& textureFilePath) :
	BaseCube(textureFilePath),
	mSpinBF(XMMatrixIdentity()),
	mOrbitBF(XMMatrixIdentity())
{}

void OrbitCube::Update(FLOAT deltaTime) {
	XMMATRIX mTranslate = XMMatrixTranslation(-4.0f, 0.0f, 0.0f);
	XMMATRIX mScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);	
	XMMATRIX mSpin = XMMatrixRotationZ(deltaTime * -1.0f);
	XMMATRIX mOrbit = XMMatrixRotationY(deltaTime * 2.0f * -1.0f);

	mSpinBF  *= mSpin;
	mOrbitBF *= mOrbit;

	m_world = mScale  * mSpinBF * mTranslate * mOrbitBF;

}
