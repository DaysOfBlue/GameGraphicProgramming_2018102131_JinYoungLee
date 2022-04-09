#pragma once
#include "Cube/MyCube.h"


void MyCube::Update(FLOAT deltaTime) {
	m_world *= XMMatrixRotationY(deltaTime);
}