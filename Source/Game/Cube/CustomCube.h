#pragma once
#include "Cube/BaseCube.h"


class CustomCube : public BaseCube {

public:
	CustomCube() = default;
	~CustomCube() = default;

	void Update(FLOAT deltaTime);
	virtual HRESULT Initialize(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pImmediateContext) override;

protected:
	XMMATRIX mSpinBF;
	XMMATRIX mOrbitBF;
	FLOAT mDeltaTime;
};