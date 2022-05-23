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
    Renderer::Renderer() :
        m_driverType(D3D_DRIVER_TYPE_NULL),
        m_featureLevel(D3D_FEATURE_LEVEL_11_0),
        m_d3dDevice(),
        m_d3dDevice1(),
        m_immediateContext(),
        m_immediateContext1(),
        m_swapChain(),
        m_swapChain1(),
        m_renderTargetView(),
        m_depthStencil(),
        m_depthStencilView(),
        m_camera(XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f)),
        m_projection(),
        m_renderables(),
        m_models(),
        m_vertexShaders(),
        m_pixelShaders(),
        m_cbLights(),
        m_aPointLights()
    {}
   

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

    HRESULT Renderer::Initialize(_In_ HWND hWnd) {
        HRESULT hr = S_OK;

        RECT rc;
        GetClientRect(hWnd, &rc);
        UINT width = rc.right - rc.left;
        UINT height = rc.bottom - rc.top;

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
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
            hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                D3D11_SDK_VERSION, m_d3dDevice.GetAddressOf(), &m_featureLevel, m_immediateContext.GetAddressOf());

            if (hr == E_INVALIDARG)
            {
                hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                    D3D11_SDK_VERSION, m_d3dDevice.GetAddressOf(), &m_featureLevel, m_immediateContext.GetAddressOf());
            }

            if (SUCCEEDED(hr))
                break;
        }
        if (FAILED(hr))
            return hr;

        ComPtr<IDXGIFactory1> dxgiFactory;
        {
            ComPtr<IDXGIDevice> dxgiDevice;
            hr = m_d3dDevice.As(&dxgiDevice);
            if (SUCCEEDED(hr))
            {
                ComPtr<IDXGIAdapter> adapter;
                hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());
                if (SUCCEEDED(hr))
                {
                    hr = adapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

                }
            }
        }
        if (FAILED(hr))
            return hr;

        ComPtr<IDXGIFactory2> dxgiFactory2;
        hr = dxgiFactory.As(&dxgiFactory2);
        if (dxgiFactory2)
        {
            hr = m_d3dDevice.As(&m_d3dDevice1);
            if (SUCCEEDED(hr))
            {
                (void)m_immediateContext.As(&m_immediateContext1);
            }

            DXGI_SWAP_CHAIN_DESC1 sd = {
                .Width = width,
                .Height = height,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = {.Count = 1, .Quality = 0},
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = 1
            };
            

            hr = dxgiFactory2->CreateSwapChainForHwnd(m_d3dDevice.Get(), hWnd, &sd, nullptr, nullptr, m_swapChain1.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                hr = m_swapChain1.As(&m_swapChain);
            }

        }
        else
        {

            DXGI_SWAP_CHAIN_DESC sd = 
            {
                .BufferDesc = 
                    {
                    .Width = width,
                    .Height = height,
                    .RefreshRate = 
                    {
                        .Numerator = 60,
                        .Denominator = 1
                    },
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,                   
                },
                .SampleDesc = 
                    {
                    .Count = 1,
                    .Quality = 0

                    },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = 1,
                .OutputWindow = hWnd,
                .Windowed = TRUE
            };
            

            hr = dxgiFactory->CreateSwapChain(m_d3dDevice.Get(), &sd, m_swapChain.GetAddressOf());
        }

        dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

        if (FAILED(hr))
            return hr;

#pragma region CreateDepthStencilBuffer
        ComPtr <ID3D11Texture2D> pBackBuffer;
        hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

        if (FAILED(hr))
            return hr;

        hr = m_d3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());

        if (FAILED(hr))
            return hr;

        // Create a Depth-Stencil Resource
        ComPtr<ID3D11Texture2D> m_depthStencil(nullptr);
        D3D11_TEXTURE2D_DESC descDepth = 
        {
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
            .SampleDesc = 
            {
                .Count = 1,
                .Quality = 0
            },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            .CPUAccessFlags = 0,
            .MiscFlags = 0
        };
        
        hr = m_d3dDevice->CreateTexture2D(&descDepth, NULL, &m_depthStencil);
        if (FAILED(hr))
            return hr;

        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {
            .Format = descDepth.Format,
            .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
            .Texture2D = {
                .MipSlice = 0
            }
        };
        
        
        hr = m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &descDSV, m_depthStencilView.GetAddressOf());
        if (FAILED(hr))
            return hr;

        m_immediateContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
#pragma endregion
    
#pragma region SetViewPort
        // Set up ViewPort
        D3D11_VIEWPORT vp = 
        {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width = (FLOAT)width,
            .Height = (FLOAT)height,
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
            
        };
        
        m_immediateContext->RSSetViewports(1, &vp);

#pragma endregion

#pragma region CreateCBChangeOnResize
        //Create CBChangeOnResize
        m_projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f);

        D3D11_BUFFER_DESC bd = {
            .ByteWidth = sizeof(CBChangeOnResize),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        CBChangeOnResize cbResize = {
            .Projection = XMMatrixTranspose(m_projection)
        };

        D3D11_SUBRESOURCE_DATA InitData =
        {
            .pSysMem = &cbResize,
            .SysMemPitch = 0,
            .SysMemSlicePitch = 0
        };

        hr = m_d3dDevice->CreateBuffer(&bd, &InitData, m_cbChangeOnResize.GetAddressOf());
        if(FAILED(hr))
        {
            return hr;
        }
        
        m_immediateContext->VSSetConstantBuffers(1, 1, m_cbChangeOnResize.GetAddressOf());
#pragma endregion
#pragma region CreatecbLightsDesc

        D3D11_BUFFER_DESC cbLightsDesc = {
            .ByteWidth = sizeof(CBLights),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
            .StructureByteStride = 0
        };

        CBLights cbLights = {};

        D3D11_SUBRESOURCE_DATA cbLightsData = {
            .pSysMem = &cbLights,
            .SysMemPitch = 0,
            .SysMemSlicePitch = 0
        };

        hr = m_d3dDevice->CreateBuffer(&cbLightsDesc, &cbLightsData, m_cbLights.GetAddressOf());
        if (FAILED(hr)) return hr;

        m_immediateContext->VSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());
        m_immediateContext->PSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());
#pragma endregion

#pragma region InitializeRenderables
        
        for (auto i : m_vertexShaders)
        {
            i.second->Initialize(m_d3dDevice.Get());
        }
        for (auto i : m_pixelShaders)
        {
            i.second->Initialize(m_d3dDevice.Get());
        }
        for (auto i : m_renderables)
        {
            i.second->Initialize(m_d3dDevice.Get(), m_immediateContext.Get());
        }
        for (auto i : m_models)
        {
            i.second->Initialize(m_d3dDevice.Get(), m_immediateContext.Get());
        }
        for (auto i : m_scenes) {
            for (auto j : i.second->GetVoxels()) {
                j->Initialize(m_d3dDevice.Get(), m_immediateContext.Get());
            }
        }
        m_camera.Initialize(m_d3dDevice.Get());
#pragma endregion

        // Set primitive topology
        m_immediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


        return S_OK;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::AddRenderable
      Summary:  Add a renderable object and initialize the object
      Args:     PCWSTR pszRenderableName
                  Key of the renderable object
                const std::shared_ptr<Renderable>& renderable
                  Unique pointer to the renderable object
      Modifies: [m_renderables].
      Returns:  HRESULT
                  Status code.
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::AddRenderable(_In_ PCWSTR pszRenderableName, _In_ const std::shared_ptr<Renderable>& renderable)
    {
        if (m_renderables.count(pszRenderableName) > 0) {
              return E_FAIL;
        }
        else {

            m_renderables.insert({ pszRenderableName, renderable });

            return S_OK;
        }
    }

    

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::AddPointLight
      Summary:  Add a point light
      Args:     size_t index
                  Index of the point light
                const std::shared_ptr<PointLight>& pointLight
                  Shared pointer to the point light object
      Modifies: [m_aPointLights].
      Returns:  HRESULT
                  Status code.
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    
    HRESULT Renderer::AddPointLight(_In_ size_t index, _In_ const std::shared_ptr<PointLight>& pPointLight)
    {
        if (index < 0 || index >= NUM_LIGHTS) {
            return E_FAIL;
        }
        else {
            m_aPointLights[index] = pPointLight;
            return S_OK;
        }
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::AddVertexShader
      Summary:  Add the vertex shader into the renderer and initialize it
      Args:     PCWSTR pszVertexShaderName
                  Key of the vertex shader
                const std::shared_ptr<VertexShader>&
                  Vertex shader to add
      Modifies: [m_vertexShaders].
      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::AddVertexShader(_In_ PCWSTR pszVertexShaderName, _In_ const std::shared_ptr<VertexShader>& vertexShader)
    {
        if (m_vertexShaders.count(pszVertexShaderName) > 0) {
            return E_FAIL;
        }
        else {

            m_vertexShaders.insert({ pszVertexShaderName, vertexShader});

            return S_OK;
        }
    }


    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::AddPixelShader
      Summary:  Add the pixel shader into the renderer and initialize it
      Args:     PCWSTR pszPixelShaderName
                  Key of the pixel shader
                const std::shared_ptr<PixelShader>&
                  Pixel shader to add
      Modifies: [m_pixelShaders].
      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::AddPixelShader(_In_ PCWSTR pszPixelShaderName, _In_ const std::shared_ptr<PixelShader>& pixelShader)
    {
        if (m_pixelShaders.count(pszPixelShaderName) > 0) {
            return E_FAIL;
        }
        else {

            m_pixelShaders.insert({ pszPixelShaderName, pixelShader });

            return S_OK;
        }
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::AddScene

      Summary:  Add a scene

      Args:     PCWSTR pszSceneName
                  Key of a scene
                const std::filesystem::path& sceneFilePath
                  File path to initialize a scene

      Modifies: [m_scenes].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::AddScene(_In_ PCWSTR pszSceneName, const std::filesystem::path& sceneFilePath) 
    {
        HRESULT hr = S_OK;
        if (m_scenes.count(pszSceneName) > 0) {
            return E_FAIL;
        }
        else {
            std::shared_ptr<Scene> make_scene(new Scene(sceneFilePath));
            m_scenes.insert({ pszSceneName, make_scene});
        }

        return hr;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::AddModel

      Summary:  Add a model object

      Args:     PCWSTR pszModelName
                  Key of the model object
                const std::shared_ptr<Model>& pModel
                  Shared pointer to the model object

      Modifies: [m_models].

      Returns:  HRESULT
                  Status code.
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::AddModel(_In_ PCWSTR pszModelName, _In_ const std::shared_ptr<Model>& pModel)
    {
        if (m_models.count(pszModelName) > 0) {
            return E_FAIL;
        }
        else {

            m_models.insert({ pszModelName, pModel });

            return S_OK;
        }
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::SetMainScene

      Summary:  Set the main scene

      Args:     PCWSTR pszSceneName
                  Name of the scene to set as the main scene

      Modifies: [m_pszMainSceneName].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::SetMainScene(_In_ PCWSTR pszSceneName) 
    {
        HRESULT hr = S_OK;
        
        if (m_scenes.count(pszSceneName) > 0) {
            m_pszMainSceneName = pszSceneName;

            return hr;
        }
        else {
            return E_FAIL;
        }
        
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
    
    
    void Renderer::Update(_In_ FLOAT deltaTime) {

        for (auto& i : m_renderables) 
        {
            i.second->Update(deltaTime);

        }
        
        for (auto& i : m_models)
        {
            i.second->Update(deltaTime);
        }

        for (auto& j : m_aPointLights)
        {
            j->Update(deltaTime);
        }

        m_camera.Update(deltaTime);
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::Render
      Summary:  Render the frame
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderer::Render() {
        //Clear BackBuffer
        m_immediateContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::MidnightBlue);
        //Clear the Depth Buffer
        m_immediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;

        //Update the camera constant buffer
        XMFLOAT4 camPosition;
        XMStoreFloat4(&camPosition, m_camera.GetEye());
        CBChangeOnCameraMovement cbCamera = {
            .View = XMMatrixTranspose(m_camera.GetView()),
            .CameraPosition = camPosition
        };
        m_immediateContext->UpdateSubresource(m_camera.GetConstantBuffer().Get(), 0, nullptr, &cbCamera, 0, 0);
        m_immediateContext->VSSetConstantBuffers(0, 1, m_camera.GetConstantBuffer().GetAddressOf());
        m_immediateContext->PSSetConstantBuffers(0, 1, m_camera.GetConstantBuffer().GetAddressOf());

        //Update the lights constant buffer
        CBLights cbLights = {};
        for (int i = 0; i < NUM_LIGHTS; i++) {
            cbLights.LightPositions[i] = m_aPointLights[i]->GetPosition();
            cbLights.LightColors[i] = m_aPointLights[i]->GetColor();
        }
        m_immediateContext->UpdateSubresource(m_cbLights.Get(), 0, nullptr, &cbLights, 0, 0);
        m_immediateContext->VSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());
        m_immediateContext->PSSetConstantBuffers(3, 1, m_cbLights.GetAddressOf());

        
        
        for (auto i : m_renderables)
        {
            m_immediateContext->IASetVertexBuffers(0, 1, i.second->GetVertexBuffer().GetAddressOf(), &stride, &offset);
            m_immediateContext->IASetIndexBuffer(i.second->GetIndexBuffer().Get(),DXGI_FORMAT_R16_UINT ,0);
            m_immediateContext->IASetInputLayout(i.second->GetVertexLayout().Get());

            

            CBChangesEveryFrame cbChanges = {
                .World = XMMatrixTranspose(i.second->GetWorldMatrix()),
                .OutputColor = i.second->GetOutputColor()
            };
            m_immediateContext->UpdateSubresource(i.second->GetConstantBuffer().Get(), 0, nullptr, &cbChanges, 0, 0);

            m_immediateContext->VSSetShader(i.second->GetVertexShader().Get(), nullptr, 0); 
            m_immediateContext->PSSetShader(i.second->GetPixelShader().Get(), nullptr, 0);
            m_immediateContext->VSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            m_immediateContext->PSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            

            if (i.second->HasTexture())
            {
                for (UINT k = 0u; k < i.second->GetNumMeshes(); k++)
                {
                    const UINT materialIndex = i.second->GetMesh(k).uMaterialIndex;
                    if (i.second->GetMaterial(materialIndex).pDiffuse)
                    {
                        m_immediateContext->PSSetShaderResources(0u, 1u, i.second->GetMaterial(materialIndex).pDiffuse->GetTextureResourceView().GetAddressOf());
                        m_immediateContext->PSSetSamplers(0u, 1u, i.second->GetMaterial(materialIndex).pDiffuse->GetSamplerState().GetAddressOf());
                    }
                    m_immediateContext->DrawIndexed(
                        i.second->GetMesh(k).uNumIndices,
                        i.second->GetMesh(k).uBaseIndex,
                        i.second->GetMesh(k).uBaseVertex);
                }
            }
            else {
                m_immediateContext->DrawIndexed(i.second->GetNumIndices(), 0, 0);
            }

        }

        UINT strides[2] = { sizeof(SimpleVertex), sizeof(InstanceData) };
        UINT offsets[2] = { 0u, 0u };
        for (auto i : m_scenes) {
            if (i.first == m_pszMainSceneName) {

                for (auto j : i.second->GetVoxels()) {

                    ID3D11Buffer* buffer[2] = { j->GetVertexBuffer().Get(), j->GetInstanceBuffer().Get() };
                    m_immediateContext->IASetVertexBuffers(0u, 2u, buffer, strides, offsets);
                    m_immediateContext->IASetIndexBuffer(j->GetIndexBuffer().Get(), DXGI_FORMAT_R16_UINT, 0);
                    m_immediateContext->IASetInputLayout(j->GetVertexLayout().Get());

                    CBChangesEveryFrame cbChanges = {
                    .World = XMMatrixTranspose(j->GetWorldMatrix()),
                    .OutputColor = j->GetOutputColor()
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

                    m_immediateContext->DrawIndexedInstanced(j->GetNumIndices(), j->GetNumInstances(), 0, 0, 0);


                }
            }
            
        }

        UINT model_strides[2] = { sizeof(SimpleVertex), sizeof(AnimationData) };
        UINT model_offsets[2] = { 0u, 0u };
        for (auto i : m_models) 
        {
            ID3D11Buffer* model_buffers[2] = { i.second->GetVertexBuffer().Get(), i.second->GetAnimationBuffer().Get() };
            m_immediateContext->IASetVertexBuffers(0, 2, model_buffers, model_strides, model_offsets);
            m_immediateContext->IASetIndexBuffer(i.second->GetIndexBuffer().Get(), DXGI_FORMAT_R16_UINT, 0);
            m_immediateContext->IASetInputLayout(i.second->GetVertexLayout().Get());



            CBChangesEveryFrame cbChanges = {
                .World = XMMatrixTranspose(i.second->GetWorldMatrix()),
                .OutputColor = i.second->GetOutputColor()
            };
            m_immediateContext->UpdateSubresource(i.second->GetConstantBuffer().Get(), 0, nullptr, &cbChanges, 0, 0);

            CBSkinning cbSkinning = {};
            auto& transforms = i.second->GetBoneTransforms();
            for (UINT i = 0u; i < transforms.size(); i++)
            {
                cbSkinning.BoneTransforms[i] = XMMatrixTranspose(transforms[i]);
            }

            m_immediateContext->UpdateSubresource(i.second->GetSkinningConstantBuffer().Get(),0,nullptr,&cbSkinning,0,0);
            m_immediateContext->VSSetShader(i.second->GetVertexShader().Get(), nullptr, 0);
            m_immediateContext->PSSetShader(i.second->GetPixelShader().Get(), nullptr, 0);
            m_immediateContext->VSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            m_immediateContext->PSSetConstantBuffers(2, 1, i.second->GetConstantBuffer().GetAddressOf());
            m_immediateContext->VSSetConstantBuffers(4, 1, i.second->GetSkinningConstantBuffer().GetAddressOf());


            if (i.second->HasTexture())
            {
                for (UINT j = 0u; j < i.second->GetNumMeshes(); j++)
                {
                    const UINT materialIndex = i.second->GetMesh(j).uMaterialIndex;
                    if (i.second->GetMaterial(materialIndex).pDiffuse)
                    {
                        m_immediateContext->PSSetShaderResources(0u, 1u, i.second->GetMaterial(materialIndex).pDiffuse->GetTextureResourceView().GetAddressOf());
                        m_immediateContext->PSSetSamplers(0u, 1u, i.second->GetMaterial(materialIndex).pDiffuse->GetSamplerState().GetAddressOf());
                    }
                    m_immediateContext->DrawIndexed(
                        i.second->GetMesh(j).uNumIndices,
                        i.second->GetMesh(j).uBaseIndex,
                        i.second->GetMesh(j).uBaseVertex);
                }
            }
            else {
                m_immediateContext->DrawIndexed(i.second->GetNumIndices(), 0, 0);
            }

        }
        m_swapChain->Present(0,0);

    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
     Method:   Renderer::SetVertexShaderOfRenderable
     Summary:  Sets the vertex shader for a renderable
     Args:     PCWSTR pszRenderableName
                 Key of the renderable
               PCWSTR pszVertexShaderName
                 Key of the vertex shader
     Modifies: [m_renderables].
     Returns:  HRESULT
                 Status code
   M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
   
    HRESULT Renderer::SetVertexShaderOfRenderable(_In_ PCWSTR pszRenderableName, _In_ PCWSTR pszVertexShaderName) 
    {
        HRESULT hr = S_OK;
        
        std::unordered_map<std::wstring, std::shared_ptr<Renderable>>::const_iterator renderAble = m_renderables.find(pszRenderableName);
        std::unordered_map<std::wstring, std::shared_ptr<VertexShader>>::const_iterator vertexShader = m_vertexShaders.find(pszVertexShaderName);
        if (renderAble != m_renderables.end() && vertexShader != m_vertexShaders.end())
        {
            renderAble->second->SetVertexShader(vertexShader->second);
        }
        else 
        {
            return E_FAIL;
        }
        return hr;
    }

   /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
     Method:   Renderer::SetPixelShaderOfRenderable
     Summary:  Sets the pixel shader for a renderable
     Args:     PCWSTR pszRenderableName
                 Key of the renderable
               PCWSTR pszPixelShaderName
                 Key of the pixel shader
     Modifies: [m_renderables].
     Returns:  HRESULT
                 Status code
   M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::SetPixelShaderOfRenderable(_In_ PCWSTR pszRenderableName, _In_ PCWSTR pszPixelShaderName) 
    {
        HRESULT hr = S_OK;
        
        std::unordered_map<std::wstring, std::shared_ptr<Renderable>>::const_iterator renderAble = m_renderables.find(pszRenderableName);
        std::unordered_map<std::wstring, std::shared_ptr<PixelShader>>::const_iterator pixelShader = m_pixelShaders.find(pszPixelShaderName);
        if (renderAble != m_renderables.end() && pixelShader != m_pixelShaders.end())
        {
            renderAble->second->SetPixelShader(pixelShader->second);
        }
        else
        {
            return E_FAIL;
        }
        return hr;
    }

    

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::SetVertexShaderOfScene

      Summary:  Sets the vertex shader for the voxels in a scene

      Args:     PCWSTR pszSceneName
                  Key of the scene
                PCWSTR pszVertexShaderName
                  Key of the vertex shader

      Modifies: [m_scenes].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::SetVertexShaderOfScene(_In_ PCWSTR pszSceneName, _In_ PCWSTR pszVertexShaderName)
    {
        std::unordered_map<std::wstring, std::shared_ptr<Scene>>::const_iterator scene = m_scenes.find(pszSceneName);
        std::unordered_map<std::wstring, std::shared_ptr<VertexShader>>::const_iterator vertexShader = m_vertexShaders.find(pszVertexShaderName);
        if (scene != m_scenes.end() && vertexShader != m_vertexShaders.end())
        {
            for (auto i : scene->second->GetVoxels()) {
                i->SetVertexShader(vertexShader->second);
            }
        }
        else
        {
            return E_FAIL;
        }
    }

    

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::SetPixelShaderOfScene

      Summary:  Sets the pixel shader for the voxels in a scene

      Args:     PCWSTR pszSceneName
                  Key of the renderable
                PCWSTR pszPixelShaderName
                  Key of the pixel shader

      Modifies: [m_Scenes].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::SetPixelShaderOfScene(_In_ PCWSTR pszSceneName, _In_ PCWSTR pszPixelShaderName)
    {
        HRESULT hr = S_OK;
        std::unordered_map<std::wstring, std::shared_ptr<Scene>>::const_iterator scene = m_scenes.find(pszSceneName);
        std::unordered_map<std::wstring, std::shared_ptr<PixelShader>>::const_iterator pixelShader = m_pixelShaders.find(pszPixelShaderName);
        if (scene != m_scenes.end() && pixelShader != m_pixelShaders.end())
        {
            for (auto i : scene->second->GetVoxels()) {
                i->SetPixelShader(pixelShader->second);
            }
        }
        else
        {
            return E_FAIL;
        }
        return hr;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::SetVertexShaderOfModel

      Summary:  Sets the pixel shader for a model

      Args:     PCWSTR pszModelName
                  Key of the model
                PCWSTR pszVertexShaderName
                  Key of the vertex shader

      Modifies: [m_renderables].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/

    HRESULT Renderer::SetVertexShaderOfModel(_In_ PCWSTR pszModelName, _In_ PCWSTR pszVertexShaderName) 
    {
        HRESULT hr = S_OK;

        std::unordered_map<std::wstring, std::shared_ptr<Model>>::const_iterator model = m_models.find(pszModelName);
        std::unordered_map<std::wstring, std::shared_ptr<VertexShader>>::const_iterator vertexShader = m_vertexShaders.find(pszVertexShaderName);
        if (model != m_models.end() && vertexShader != m_vertexShaders.end())
        {
            model->second->SetVertexShader(vertexShader->second);
        }
        else
        {
            return E_FAIL;
        }
        return hr;
    }
    

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderer::SetPixelShaderOfModel

      Summary:  Sets the pixel shader for a model

      Args:     PCWSTR pszModelName
                  Key of the model
                PCWSTR pszPixelShaderName
                  Key of the pixel shader

      Modifies: [m_renderables].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderer::SetPixelShaderOfModel(_In_ PCWSTR pszModelName, _In_ PCWSTR pszPixelShaderName) {
        HRESULT hr = S_OK;

        std::unordered_map<std::wstring, std::shared_ptr<Model>>::const_iterator model = m_models.find(pszModelName);
        std::unordered_map<std::wstring, std::shared_ptr<PixelShader>>::const_iterator pixelShader = m_pixelShaders.find(pszPixelShaderName);
        if (model != m_models.end() && pixelShader != m_pixelShaders.end())
        {
            model->second->SetPixelShader(pixelShader->second);
        }
        else
        {
            return E_FAIL;
        }
        return hr;
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
}