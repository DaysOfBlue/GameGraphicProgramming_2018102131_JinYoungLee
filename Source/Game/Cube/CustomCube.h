#pragma once
#include "Cube/BaseCube.h"


class CustomCube : public BaseCube {

public:
	
	CustomCube(const std::filesystem::path& textureFilePath);
	CustomCube(const CustomCube& other) = delete;
	CustomCube(CustomCube&& other) = delete;
	CustomCube& operator=(const CustomCube& other) = delete;
	CustomCube& operator=(CustomCube&& other) = delete;
	~CustomCube() = default;

	virtual void Update(FLOAT deltaTime) override;
	//virtual HRESULT Initialize(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pImmediateContext) override;
};