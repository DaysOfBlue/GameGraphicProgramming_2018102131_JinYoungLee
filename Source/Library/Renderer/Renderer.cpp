#include "Renderer/Renderer.h"

namespace library
{
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::Renderer
      Summary:  Constructor
      Modifies: [m_driverType, m_featureLevel, m_d3dDevice, m_d3dDevice1,
                  m_immediateContext, m_immediateContext1, m_swapChain,
                  m_swapChain1, m_renderTargetView, m_vertexShader,
                  m_pixelShader, m_vertexLayout, m_vertexBuffer].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    Renderer::Renderer()
        : m_driverType(D3D_DRIVER_TYPE_NULL)
        , m_featureLevel(D3D_FEATURE_LEVEL_11_0)
        , m_d3dDevice()
        , m_d3dDevice1()
        , m_immediateContext()
        , m_immediateContext1()
        , m_swapChain()
        , m_swapChain1()
        , m_renderTargetView()
        , m_depthStencil()
        , m_depthStencilView()
        , m_cbChangeOnResize()
        , m_cbShadowMatrix()
        , m_pszMainSceneName(nullptr)
        , m_padding{ '\0' }
        , m_camera(XMVectorSet(0.0f, 3.0f, -6.0f, 0.0f))
        , m_projection()
        , m_scenes()
        , m_invalidTexture(std::make_shared<Texture>(L"Content/Common/InvalidTexture.png"))
        , m_shadowMapTexture()
        , m_shadowVertexShader()
        , m_shadowPixelShader()
    {
    }
   

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::Initialize

      Summary:  Creates Direct3D device and swap chain

      Args:     HWND hWnd
                  Handle to the window

      Modifies: [m_d3dDevice, m_featureLevel, m_immediateContext,
                  m_d3dDevice1, m_immediateContext1, m_swapChain1,
                  m_swapChain, m_renderTargetView, m_vertexShader,
                  m_vertexLayout, m_pixelShader, m_vertexBuffer].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

    HRESULT Renderer::Initialize(_In_ HWND hWnd)
    {
        HRESULT hr = S_OK;

        RECT rc;
        GetClientRect(hWnd, &rc);
        UINT uWidth = static_cast<UINT>(rc.right - rc.left);
        UINT uHeight = static_cast<UINT>(rc.bottom - rc.top);

        UINT uCreateDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
        uCreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_DRIVER_TYPE driverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        UINT numDriverTypes = ARRAYSIZE(driverTypes);

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };
        UINT numFeatureLevels = ARRAYSIZE(featureLevels);

        for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
        {
            m_driverType = driverTypes[driverTypeIndex];
            hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, uCreateDeviceFlags, featureLevels, numFeatureLevels,
                D3D11_SDK_VERSION, m_d3dDevice.GetAddressOf(), &m_featureLevel, m_immediateContext.GetAddressOf());

            if (hr == E_INVALIDARG)
            {
                // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
                hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, uCreateDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                    D3D11_SDK_VERSION, m_d3dDevice.GetAddressOf(), &m_featureLevel, m_immediateContext.GetAddressOf());
            }

            if (SUCCEEDED(hr))
            {
                break;
            }
        }
        if (FAILED(hr))
        {
            return hr;
        }

        // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
        ComPtr<IDXGIFactory1> dxgiFactory;
        {
            ComPtr<IDXGIDevice> dxgiDevice;
            hr = m_d3dDevice.As(&dxgiDevice);
            if (SUCCEEDED(hr))
            {
                ComPtr<IDXGIAdapter> adapter;
                hr = dxgiDevice->GetAdapter(&adapter);
                if (SUCCEEDED(hr))
                {
                    hr = adapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
                }
            }
        }
        if (FAILED(hr))
        {
            return hr;
        }

        // Create swap chain
        ComPtr<IDXGIFactory2> dxgiFactory2;
        hr = dxgiFactory.As(&dxgiFactory2);
        if (SUCCEEDED(hr))
        {
            // DirectX 11.1 or later
            hr = m_d3dDevice.As(&m_d3dDevice1);
            if (SUCCEEDED(hr))
            {
                m_immediateContext.As(&m_immediateContext1);
            }

            DXGI_SWAP_CHAIN_DESC1 sd =
            {
                .Width = uWidth,
                .Height = uHeight,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = {.Count = 1u, .Quality = 0u },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = 1u
            };

            hr = dxgiFactory2->CreateSwapChainForHwnd(m_d3dDevice.Get(), hWnd, &sd, nullptr, nullptr, m_swapChain1.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                hr = m_swapChain1.As(&m_swapChain);
            }
        }
        else
        {
            // DirectX 11.0 systems
            DXGI_SWAP_CHAIN_DESC sd =
            {
                .BufferDesc = {.Width = uWidth, .Height = uHeight, .RefreshRate = {.Numerator = 60, .Denominator = 1 }, .Format = DXGI_FORMAT_R8G8B8A8_UNORM },
                .SampleDesc = {.Count = 1, .Quality = 0 },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = 1u,
                .OutputWindow = hWnd,
                .Windowed = TRUE
            };

            hr = dxgiFactory->CreateSwapChain(m_d3dDevice.Get(), &sd, m_swapChain.GetAddressOf());
        }

        // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
        dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

        if (FAILED(hr))
        {
            return hr;
        }

        // Create a render target view
        ComPtr<ID3D11Texture2D> pBackBuffer;
        hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (FAILED(hr))
        {
            return hr;
        }

        hr = m_d3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        // Create texture2D texture
        D3D11_TEXTURE2D_DESC descDepth =
        {
            .Width = uWidth,
            .Height = uHeight,
            .MipLevels = 1u,
            .ArraySize = 1u,
            .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
            .SampleDesc = {.Count = 1u, .Quality = 0u },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            .CPUAccessFlags = 0u,
            .MiscFlags = 0u
        };
        hr = m_d3dDevice->CreateTexture2D(&descDepth, nullptr, m_depthStencil.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        // Create the depth stencil view
        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV =
        {
            .Format = descDepth.Format,
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
            .Texture2D = {.MipSlice = 0 }
        };
        hr = m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &descDSV, m_depthStencilView.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        m_immediateContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

        // Setup the viewport
        D3D11_VIEWPORT vp =
        {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<FLOAT>(uWidth),
            .Height = static_cast<FLOAT>(uHeight),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        };
        m_immediateContext->RSSetViewports(1, &vp);

        // Set primitive topology
        m_immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Create the constant buffers
        D3D11_BUFFER_DESC bd =
        {
            .ByteWidth = sizeof(CBChangeOnResize),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = 0
        };
        hr = m_d3dDevice->CreateBuffer(&bd, nullptr, m_cbChangeOnResize.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        // Initialize the projection matrix
        m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<FLOAT>(uWidth) / static_cast<FLOAT>(uHeight), 0.01f, 1000.0f);

        CBChangeOnResize cbChangesOnResize =
        {
            .Projection = XMMatrixTranspose(m_projection)
        };
        m_immediateContext->UpdateSubresource(m_cbChangeOnResize.Get(), 0, nullptr, &cbChangesOnResize, 0, 0);

        bd.ByteWidth = sizeof(CBLights);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0u;

        hr = m_d3dDevice->CreateBuffer(&bd, nullptr, m_cbLights.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        bd.ByteWidth = sizeof(CBShadowMatrix);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0u;

        hr = m_d3dDevice->CreateBuffer(&bd, nullptr, m_cbShadowMatrix.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        m_shadowMapTexture = std::make_shared<RenderTexture>(uWidth, uHeight);

        m_camera.Initialize(m_d3dDevice.Get());

        if (!m_scenes.contains(m_pszMainSceneName))
        {
            return E_FAIL;
        }

        hr = m_scenes[m_pszMainSceneName]->Initialize(m_d3dDevice.Get(), m_immediateContext.Get());
        if (FAILED(hr))
        {
            return hr;
        }

        hr = m_invalidTexture->Initialize(m_d3dDevice.Get(), m_immediateContext.Get());
        if (FAILED(hr))
        {
            return hr;
        }

        hr = m_shadowMapTexture->Initialize(m_d3dDevice.Get(), m_immediateContext.Get());
        if (FAILED(hr))
        {
            return hr;
        }

        const auto& mainScene = m_scenes[m_pszMainSceneName];
        
        for (UINT i = 0u; i < NUM_LIGHTS; i++)
        {
            mainScene->GetPointLight(i)->Initialize(uWidth, uHeight);
        }


        return S_OK;
    }


    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::AddScene

      Summary:  Add scene to renderer

      Args:     PCWSTR pszSceneName
                  The name of the scene
                const std::shared_ptr<Scene>&
                  The shared pointer to Scene

      Modifies: [m_scenes].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::AddScene(_In_ PCWSTR pszSceneName, _In_ const std::shared_ptr<Scene>& scene)
    {
        if (m_scenes.contains(pszSceneName))
        {
            return E_FAIL;
        }

        m_scenes[pszSceneName] = scene;

        return S_OK;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::GetSceneOrNull
      Summary:  Return scene with the given name or null
      Args:     PCWSTR pszSceneName
                  The name of the scene
      Returns:  std::shared_ptr<Scene>
                  The shared pointer to Scene
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    std::shared_ptr<Scene> Renderer::GetSceneOrNull(_In_ PCWSTR pszSceneName)
    {
        if (m_scenes.contains(pszSceneName))
        {
            return m_scenes[pszSceneName];
        }

        return nullptr;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::SetShadowMapShaders

      Summary:  Set shaders for the shadow mapping

      Args:     std::shared_ptr<ShadowVertexShader>
                  vertex shader
                std::shared_ptr<PixelShader>
                  pixel shader

      Modifies: [m_shadowVertexShader, m_shadowPixelShader].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderer::SetShadowMapShaders(_In_ std::shared_ptr<ShadowVertexShader> vertexShader, _In_ std::shared_ptr<PixelShader> pixelShader)
    {
        m_shadowVertexShader = move(vertexShader);
        m_shadowPixelShader = move(pixelShader);
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::SetMainScene
      Summary:  Set the main scene
      Args:     PCWSTR pszSceneName
                  The name of the scene
      Modifies: [m_pszMainSceneName].
      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::SetMainScene(_In_ PCWSTR pszSceneName)
    {
        if (!m_scenes.contains(pszSceneName))
        {
            return E_FAIL;
        }

        m_pszMainSceneName = pszSceneName;

        return S_OK;
    }
    
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::HandleInput

      Summary:  Add the pixel shader into the renderer and initialize it

      Args:     const DirectionsInput& directions
                  Data structure containing keyboard input data
                const MouseRelativeMovement& mouseRelativeMovement
                  Data structure containing mouse relative input data

      Modifies: [m_camera].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderer::HandleInput(_In_ const DirectionsInput& directions, _In_ const MouseRelativeMovement& mouseRelativeMovement, _In_ FLOAT deltaTime)
    {
        m_camera.HandleInput(directions, mouseRelativeMovement, deltaTime);
    }


    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::Update

      Summary:  Update the renderables each frame

      Args:     FLOAT deltaTime
                  Time difference of a frame
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderer::Update(_In_ FLOAT deltaTime)
    {
        m_scenes[m_pszMainSceneName]->Update(deltaTime);

        m_camera.Update(deltaTime);
    }


    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::Render

      Summary:  Render the frame
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderer::Render() {
        RenderSceneToTexture();


        //Clear BackBuffer
        m_immediateContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::MidnightBlue);
        //Clear the Depth Buffer
        m_immediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        const auto& mainScene = m_scenes[m_pszMainSceneName];

        //Update the camera constant buffer
        XMFLOAT4 camPosition;
        XMStoreFloat4(&camPosition, m_camera.GetEye());
        CBChangeOnCameraMovement cbCamera = {
            .View = XMMatrixTranspose(m_camera.GetView()),
            .CameraPosition = camPosition
        };
        m_immediateContext->UpdateSubresource(m_camera.GetConstantBuffer().Get(), 0, nullptr, &cbCamera, 0, 0);
        

        CBLights cbLights = {};
        for (int j = 0; j < NUM_LIGHTS; j++) {
            const auto light = mainScene->GetPointLight(j);
            cbLights.LightPositions[j] = light->GetPosition();
            cbLights.LightColors[j] = light->GetColor();
            cbLights.LightViews[j] = XMMatrixTranspose(light->GetViewMatrix());
            cbLights.LightProjections[j] = XMMatrixTranspose(light->GetProjectionMatrix());
        }
        m_immediateContext->UpdateSubresource(m_cbLights.Get(), 0, nullptr, &cbLights, 0, 0);

        

        for (auto i : mainScene->GetRenderables())
        {
            UINT rstride[2] = { sizeof(SimpleVertex), sizeof(NormalData) };
            UINT roffset[2] = { 0u, 0u };
            ID3D11Buffer* rbuffer[2] = { i.second->GetVertexBuffer().Get(), i.second->GetNormalBuffer().Get() };
            m_immediateContext->IASetVertexBuffers(0, 2, rbuffer, rstride, roffset);
            m_immediateContext->IASetIndexBuffer(i.second->GetIndexBuffer().Get(),DXGI_FORMAT_R16_UINT ,0);
            m_immediateContext->IASetInputLayout(i.second->GetVertexLayout().Get());
                   

            CBChangesEveryFrame cbChanges = {
                .World = XMMatrixTranspose(i.second->GetWorldMatrix()),
                .OutputColor = i.second->GetOutputColor(),
                .HasNormalMap = i.second->HasNormalMap()
            };
            m_immediateContext->UpdateSubresource(i.second->GetConstantBuffer().Get(), 0, nullptr, &cbChanges, 0, 0);


            m_immediateContext->VSSetShader(i.second->GetVertexShader().Get(), nullptr, 0); 
            m_immediateContext->PSSetShader(i.second->GetPixelShader().Get(), nullptr, 0);

            m_immediateContext->VSSetConstantBuffers(0, 1, m_camera.GetConstantBuffer().GetAddressOf());
            m_immediateContext->PSSetConstantBuffers(0, 1, m_camera.GetConstantBuffer().GetAddressOf());
            m_immediateContext->VSSetConstantBuffers(1, 1, m_cbChangeOnResize.GetAddressOf());
            m_immediateContext->VSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            m_immediateContext->PSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            m_immediateContext->VSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());
            m_immediateContext->PSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());

            

            

            if (i.second->HasTexture())
            {
                for (UINT k = 0u; k < i.second->GetNumMeshes(); k++)
                {
                    const UINT MaterialIndex = i.second->GetMesh(k).uMaterialIndex;
                    if (i.second->GetMaterial(MaterialIndex)->pDiffuse && i.second->GetMaterial(MaterialIndex)->pNormal)
                    {
                        ID3D11ShaderResourceView* aTextureRV[2] = {
                            i.second->GetMaterial(MaterialIndex)->pDiffuse->GetTextureResourceView().Get(),
                            i.second->GetMaterial(MaterialIndex)->pNormal->GetTextureResourceView().Get()
                        };
                        ID3D11SamplerState* aSamplers[2] = {
                            i.second->GetMaterial(MaterialIndex)->pDiffuse->GetSamplerState().Get(),
                            i.second->GetMaterial(MaterialIndex)->pNormal->GetSamplerState().Get()
                        };
                        m_immediateContext->PSSetShaderResources(0u, 2u, aTextureRV);
                        m_immediateContext->PSSetSamplers(0u, 2u, aSamplers);
                        m_immediateContext->PSSetShaderResources(2u, 1u, m_shadowMapTexture->GetShaderResourceView().GetAddressOf());
                        m_immediateContext->PSSetSamplers(2u, 1u, m_shadowMapTexture->GetSamplerState().GetAddressOf());
                    }

                    m_immediateContext->DrawIndexed(
                        i.second->GetMesh(k).uNumIndices,
                        i.second->GetMesh(k).uBaseIndex,
                        i.second->GetMesh(k).uBaseVertex);
                }
            }
            else {
                m_immediateContext->PSSetShaderResources(2u, 1u, m_shadowMapTexture->GetShaderResourceView().GetAddressOf());
                m_immediateContext->PSSetSamplers(2u, 1u, m_shadowMapTexture->GetSamplerState().GetAddressOf());

                m_immediateContext->DrawIndexed(i.second->GetNumIndices(), 0, 0);
            }

        }

        
        for (auto i : m_scenes) {
            if (i.first == m_pszMainSceneName) {

                for (auto j : i.second->GetVoxels()) {
                    UINT vstride[3] = { sizeof(SimpleVertex), sizeof(NormalData), sizeof(InstanceData) };
                    UINT voffset[3] = { 0u, 0u, 0u };
                    ID3D11Buffer* vbuffer[3] = { j->GetVertexBuffer().Get(), j->GetNormalBuffer().Get(), j->GetInstanceBuffer().Get() };
                    m_immediateContext->IASetVertexBuffers(0u, 3u, vbuffer, vstride, voffset);
                    m_immediateContext->IASetIndexBuffer(j->GetIndexBuffer().Get(), DXGI_FORMAT_R16_UINT, 0);
                    m_immediateContext->IASetInputLayout(j->GetVertexLayout().Get());

                    CBChangesEveryFrame cbChanges = {
                        .World = XMMatrixTranspose(j->GetWorldMatrix()),
                        .OutputColor = j->GetOutputColor(),
                        .HasNormalMap = j->HasNormalMap()
                    };
                    m_immediateContext->UpdateSubresource(j->GetConstantBuffer().Get(), 0, nullptr, &cbChanges, 0, 0);

                    m_immediateContext->VSSetShader(j->GetVertexShader().Get(), nullptr, 0);
                    m_immediateContext->PSSetShader(j->GetPixelShader().Get(), nullptr, 0);
                    m_immediateContext->VSSetConstantBuffers(0, 1, m_camera.GetConstantBuffer().GetAddressOf());
                    m_immediateContext->PSSetConstantBuffers(0, 1, m_camera.GetConstantBuffer().GetAddressOf());
                    m_immediateContext->VSSetConstantBuffers(1, 1, m_cbChangeOnResize.GetAddressOf());
                    m_immediateContext->VSSetConstantBuffers(2, 1, j->GetConstantBuffer().GetAddressOf());
                    m_immediateContext->PSSetConstantBuffers(2, 1, j->GetConstantBuffer().GetAddressOf());
                    m_immediateContext->VSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());
                    m_immediateContext->PSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());

                    
                    

                    if (j->HasTexture())
                    {
                        for (UINT k = 0u; k < j->GetNumMeshes(); k++)
                        {
                            
                            if (j->GetMaterial(0u)->pDiffuse && j->GetMaterial(0u)->pNormal)
                            {
                                ID3D11ShaderResourceView* aTextureRV[2] = {
                                    j->GetMaterial(0u)->pDiffuse->GetTextureResourceView().Get(),
                                    j->GetMaterial(0u)->pNormal->GetTextureResourceView().Get()
                                };
                                ID3D11SamplerState* aSamplers[2] = {
                                    j->GetMaterial(0u)->pDiffuse->GetSamplerState().Get(),
                                    j->GetMaterial(0u)->pNormal->GetSamplerState().Get()
                                };
                                m_immediateContext->PSSetShaderResources(0u, 2u, aTextureRV);
                                m_immediateContext->PSSetSamplers(0u, 2u, aSamplers);
                                m_immediateContext->PSSetShaderResources(2u, 1u, m_shadowMapTexture->GetShaderResourceView().GetAddressOf());
                                m_immediateContext->PSSetSamplers(2u, 1u, m_shadowMapTexture->GetSamplerState().GetAddressOf());

                                m_immediateContext->DrawIndexed(
                                    j->GetMesh(k).uNumIndices,
                                    j->GetMesh(k).uBaseIndex,
                                    j->GetMesh(k).uBaseVertex);
                            }
                        }
                    }
                    else {

                        m_immediateContext->PSSetShaderResources(2u, 1u, m_shadowMapTexture->GetShaderResourceView().GetAddressOf());
                        m_immediateContext->PSSetSamplers(2u, 1u, m_shadowMapTexture->GetSamplerState().GetAddressOf());
                        m_immediateContext->DrawIndexedInstanced(j->GetNumIndices(), j->GetNumInstances(), 0, 0, 0);
                    }
                    

                    


                }
            }
            
        }

        
        for (auto i : mainScene->GetModels()) 
        {
            UINT model_strides[2] = { sizeof(SimpleVertex), sizeof(NormalData) };
            UINT model_offsets[2] = { 0u, 0u };
            ID3D11Buffer* model_buffers[3] = { i.second->GetVertexBuffer().Get(), i.second->GetNormalBuffer().Get() };
            m_immediateContext->IASetVertexBuffers(0, 2, model_buffers, model_strides, model_offsets);
            m_immediateContext->IASetIndexBuffer(i.second->GetIndexBuffer().Get(), DXGI_FORMAT_R16_UINT, 0);
            m_immediateContext->IASetInputLayout(i.second->GetVertexLayout().Get());



            CBChangesEveryFrame cbChanges = {
                .World = XMMatrixTranspose(i.second->GetWorldMatrix()),
                .OutputColor = i.second->GetOutputColor(),
                .HasNormalMap = i.second->HasNormalMap()
            };
            m_immediateContext->UpdateSubresource(i.second->GetConstantBuffer().Get(), 0, nullptr, &cbChanges, 0, 0);

            //CBSkinning cbSkinning = {};
            //for (UINT j = 0u; j < i.second->GetBoneTransforms().size(); j++)
            //{
            //    cbSkinning.BoneTransforms[j] = XMMatrixTranspose(i.second->GetBoneTransforms()[j]);
            //}

            //m_immediateContext->UpdateSubresource(i.second->GetSkinningConstantBuffer().Get(),0,nullptr,&cbSkinning,0,0);

            m_immediateContext->VSSetShader(i.second->GetVertexShader().Get(), nullptr, 0);
            m_immediateContext->PSSetShader(i.second->GetPixelShader().Get(), nullptr, 0);
            m_immediateContext->VSSetConstantBuffers(1, 1, m_cbChangeOnResize.GetAddressOf());
            m_immediateContext->VSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            m_immediateContext->PSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            m_immediateContext->VSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());
            m_immediateContext->PSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());

            //m_immediateContext->VSSetConstantBuffers(4, 1, i.second->GetSkinningConstantBuffer().GetAddressOf());

            


            if (i.second->HasTexture())
            {
                for (UINT k = 0u; k < i.second->GetNumMeshes(); k++)
                {
                    const UINT MaterialIndex = i.second->GetMesh(k).uMaterialIndex;
                    if (i.second->GetMaterial(MaterialIndex)->pDiffuse && i.second->GetMaterial(MaterialIndex)->pNormal)
                    {
                        ID3D11ShaderResourceView* aTextureRV[2] = {
                            i.second->GetMaterial(MaterialIndex)->pDiffuse->GetTextureResourceView().Get(),
                            i.second->GetMaterial(MaterialIndex)->pNormal->GetTextureResourceView().Get()
                        };
                        ID3D11SamplerState* aSamplers[2] = {
                            i.second->GetMaterial(MaterialIndex)->pDiffuse->GetSamplerState().Get(),
                            i.second->GetMaterial(MaterialIndex)->pNormal->GetSamplerState().Get()
                        };
                        m_immediateContext->PSSetShaderResources(0u, 2u, aTextureRV);
                        m_immediateContext->PSSetSamplers(0u, 2u, aSamplers);

                        m_immediateContext->PSSetShaderResources(2u, 1u, m_shadowMapTexture->GetShaderResourceView().GetAddressOf());
                        m_immediateContext->PSSetSamplers(2u, 1u, m_shadowMapTexture->GetSamplerState().GetAddressOf());
                    }
                    m_immediateContext->DrawIndexed(i.second->GetMesh(k).uNumIndices, i.second->GetMesh(k).uBaseIndex, i.second->GetMesh(k).uBaseVertex);
                }
            }
            else {
                m_immediateContext->PSSetShaderResources(2u, 1u, m_shadowMapTexture->GetShaderResourceView().GetAddressOf());
                m_immediateContext->PSSetSamplers(2u, 1u, m_shadowMapTexture->GetSamplerState().GetAddressOf());
                m_immediateContext->DrawIndexed(i.second->GetNumIndices(), 0, 0);
            }

        }
        m_swapChain->Present(0,0);

    }

   /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
     Method:   Renderer::GetDriverType
     Summary:  Returns the Direct3D driver type
     Returns:  D3D_DRIVER_TYPE
                 The Direct3D driver type used
   M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
  
    D3D_DRIVER_TYPE Renderer::GetDriverType() const 
    {
        return m_driverType;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::RenderSceneToTexture

      Summary:  Render scene to the texture
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderer::RenderSceneToTexture() 
    {
        //Unbind current pixel shader resources
        ID3D11ShaderResourceView* const pSRV[2] = { NULL, NULL };
        m_immediateContext->PSSetShaderResources(0, 2, pSRV);
        m_immediateContext->PSSetShaderResources(2, 1, pSRV);

        m_immediateContext->OMSetRenderTargets(1, m_shadowMapTexture->GetRenderTargetView().GetAddressOf(), m_depthStencilView.Get());
        //Clear BackBuffer
        m_immediateContext->ClearRenderTargetView(m_shadowMapTexture->GetRenderTargetView().Get(), Colors::White);
        //Clear the Depth Buffer
        m_immediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        const auto& mainScene = m_scenes[m_pszMainSceneName];

        const auto& pointLight = mainScene->GetPointLight(0);

        
        for (auto i : mainScene->GetRenderables())
        {
            UINT uStride = sizeof(SimpleVertex);
            UINT uOffset = 0;
            m_immediateContext->IASetVertexBuffers(0u, 1u, i.second->GetVertexBuffer().GetAddressOf(), &uStride, &uOffset);
            m_immediateContext->IASetIndexBuffer(i.second->GetIndexBuffer().Get(), DXGI_FORMAT_R16_UINT, 0);
            m_immediateContext->IASetInputLayout(m_shadowVertexShader->GetVertexLayout().Get());

            CBShadowMatrix cb = {
                .World = XMMatrixTranspose(i.second->GetWorldMatrix()),
                .View = XMMatrixTranspose(pointLight->GetViewMatrix()),
                .Projection = XMMatrixTranspose(pointLight->GetProjectionMatrix()),
                .IsVoxel = FALSE
            };
            m_immediateContext->UpdateSubresource(m_cbShadowMatrix.Get(), 0, nullptr, &cb, 0, 0);
            m_immediateContext->VSSetConstantBuffers(0, 1, m_cbShadowMatrix.GetAddressOf());

            m_immediateContext->VSSetShader(m_shadowVertexShader->GetVertexShader().Get(), nullptr, 0);
            m_immediateContext->PSSetShader(m_shadowPixelShader->GetPixelShader().Get(), nullptr, 0);
          

            m_immediateContext->DrawIndexed(i.second->GetNumIndices(), 0, 0);

        }

        
        for (auto i : m_scenes) {
            if (i.first == m_pszMainSceneName) {

                for (auto j : i.second->GetVoxels()) {
                    UINT uStride = sizeof(SimpleVertex);
                    UINT uOffset = 0;
                    m_immediateContext->IASetVertexBuffers(0u, 1u, j->GetVertexBuffer().GetAddressOf(), &uStride, &uOffset);
                    m_immediateContext->IASetIndexBuffer(j->GetIndexBuffer().Get(), DXGI_FORMAT_R16_UINT, 0);
                    m_immediateContext->IASetInputLayout(j->GetVertexLayout().Get());

                    CBShadowMatrix cb = {
                        .World = XMMatrixTranspose(j->GetWorldMatrix()),
                        .View = XMMatrixTranspose(pointLight->GetViewMatrix()),
                        .Projection = XMMatrixTranspose(pointLight->GetProjectionMatrix()),
                        .IsVoxel = TRUE
                    };
                    m_immediateContext->UpdateSubresource(m_cbShadowMatrix.Get(), 0, nullptr, &cb, 0, 0);
                    m_immediateContext->VSSetConstantBuffers(0, 1, m_cbShadowMatrix.GetAddressOf());

                    m_immediateContext->VSSetShader(m_shadowVertexShader->GetVertexShader().Get(), nullptr, 0);
                    m_immediateContext->PSSetShader(m_shadowPixelShader->GetPixelShader().Get(), nullptr, 0);

                    m_immediateContext->DrawIndexedInstanced(j->GetNumIndices(), j->GetNumInstances(), 0, 0, 0);


                }
            }

        }


        for (auto i : mainScene->GetModels())
        {
            UINT uStride = sizeof(SimpleVertex);         
            UINT uOffset = 0;         
            m_immediateContext->IASetVertexBuffers(0u, 1u, i.second->GetVertexBuffer().GetAddressOf(), &uStride, &uOffset);
            m_immediateContext->IASetIndexBuffer(i.second->GetIndexBuffer().Get(), DXGI_FORMAT_R16_UINT, 0);
            m_immediateContext->IASetInputLayout(m_shadowVertexShader->GetVertexLayout().Get());

            CBShadowMatrix cb = {
                .World = XMMatrixTranspose(i.second->GetWorldMatrix()),
                .View = XMMatrixTranspose(pointLight->GetViewMatrix()),
                .Projection = XMMatrixTranspose(pointLight->GetProjectionMatrix()),
                .IsVoxel = FALSE
             };
            m_immediateContext->UpdateSubresource(m_cbShadowMatrix.Get(), 0, nullptr, &cb, 0, 0);
            m_immediateContext->VSSetConstantBuffers(0, 1, m_cbShadowMatrix.GetAddressOf());

            m_immediateContext->VSSetShader(m_shadowVertexShader->GetVertexShader().Get(), nullptr, 0);
            m_immediateContext->PSSetShader(m_shadowPixelShader->GetPixelShader().Get(), nullptr, 0);

           
            for (UINT k = 0u; k < i.second->GetNumMeshes(); k++) {
                m_immediateContext->DrawIndexed(i.second->GetMesh(k).uNumIndices, i.second->GetMesh(k).uBaseIndex, i.second->GetMesh(k).uBaseVertex);
            }
            

        }

        m_immediateContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
    }
}