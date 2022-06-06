#include "Cube/RotatingCube.h"


RotatingCube::RotatingCube(const XMFLOAT4& outputColor)
    : BaseCube(outputColor)
{
}

void RotatingCube::Update(_In_ FLOAT deltaTime)
{
    // Rotate cube around the origin


    XMMATRIX mOrbit = XMMatrixRotationY(-deltaTime * 2.0f);
    

    m_world = m_world * mOrbit;
}