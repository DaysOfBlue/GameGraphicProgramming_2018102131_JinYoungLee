#pragma once

#include "Cube/CustomCube.h"

CustomCube::CustomCube(const std::filesystem::path& textureFilePath) :
	BaseCube(textureFilePath)
{}


void CustomCube::Update(FLOAT deltaTime) {
	
	static FLOAT s_totalTime = 0.0f;
	s_totalTime += deltaTime;

	m_world = XMMatrixTranslation(4.0f, XMScalarSin(s_totalTime), 4.0f) * XMMatrixRotationZ(s_totalTime);

}


//HRESULT CustomCube::Initialize(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pImmediateContext)
//{
//	HRESULT hr = BaseCube::Initialize(pDevice, pImmediateContext);
//	if (FAILED(hr))
//	{
//		return hr;
//	}
//
//	return S_OK;
//}
