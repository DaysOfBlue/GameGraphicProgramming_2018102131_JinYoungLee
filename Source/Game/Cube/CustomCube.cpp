#pragma once

#include "Cube/CustomCube.h"

void CustomCube::Update(FLOAT deltaTime) {
	XMMATRIX mTranslate = XMMatrixTranslation(0.0f,-4.0f, 0.0f);
	XMMATRIX mScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);
	XMMATRIX mSpin = XMMatrixRotationZ(deltaTime * -1.0f);
	XMMATRIX mOrbit = XMMatrixRotationX(deltaTime * 2.0f * -1.0f);

	if (deltaTime > 10.0f) {
		mSpinBF *= mSpin;
		mOrbitBF *= mOrbit;
	}
	else {
		mSpinBF *= -mSpin;
		mOrbitBF *= -mOrbit;
	}
	

	m_world = mScale * mSpinBF * mTranslate * mOrbitBF;

}

HRESULT CustomCube::Initialize(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pImmediateContext)
{
	HRESULT hr = BaseCube::Initialize(pDevice, pImmediateContext);
	if (FAILED(hr))
	{
		return hr;
	}

	mSpinBF = XMMatrixIdentity();
	mOrbitBF = XMMatrixIdentity();

	return S_OK;
}
