#pragma once

#include "Cube/BaseCube.h"


class OrbitCube : public BaseCube {

public:
    OrbitCube(const std::filesystem::path& textureFilePath);
    OrbitCube(const OrbitCube& other) = delete;
    OrbitCube(OrbitCube&& other) = delete;
    OrbitCube& operator=(const OrbitCube& other) = delete;
    OrbitCube& operator=(OrbitCube&& other) = delete;
    ~OrbitCube() = default;

    virtual void Update(_In_ FLOAT deltaTime) override;

protected:
	XMMATRIX mSpinBF;
	XMMATRIX mOrbitBF;
};


