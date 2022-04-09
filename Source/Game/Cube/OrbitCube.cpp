#pragma once

#include "Cube/OrbitCube.h"

void OrbitCube::Update(FLOAT deltaTime) {
	XMMATRIX mTranslate = XMMatrixTranslation(-4.0f, 0.0f, 0.0f);
	XMMATRIX mScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);	
	XMMATRIX mSpin = XMMatrixRotationZ(deltaTime * -1.0f);
	XMMATRIX mOrbit = XMMatrixRotationY(deltaTime * 2.0f * -1.0f);

	mSpinBF  *= mSpin;
	mOrbitBF *= mOrbit;

	m_world = mScale  * mSpinBF * mTranslate * mOrbitBF;

}

HRESULT OrbitCube::Initialize(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pImmediateContext)
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
