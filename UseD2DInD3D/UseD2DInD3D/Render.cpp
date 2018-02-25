#include "Render.h"

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}



void CPluginRender::InitVetex()
{
    Vertex vertices[] =
    {
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(255, 255, 255, 1) },//white
    { XMFLOAT3(-0.5f, +0.5f, -0.5f), XMFLOAT4(0, 0, 0, 1) },//black
    { XMFLOAT3(+0.5f, +0.5f, -0.5f), XMFLOAT4(255, 0, 0, 1) },//red
    { XMFLOAT3(+0.5f, -0.5f, -0.5f), XMFLOAT4(0, 255, 0, 1) },//green
    { XMFLOAT3(-0.5f, -0.5f, +0.5f), XMFLOAT4(0, 0, 255, 1) },//blue
    { XMFLOAT3(-0.5f, +0.5f, +0.5f), XMFLOAT4(255, 255, 0, 1) },//yellow
    { XMFLOAT3(+0.5f, +0.5f, +0.5f), XMFLOAT4(0, 255, 255, 1) },//cyan
    { XMFLOAT3(+0.5f, -0.5f, +0.5f), XMFLOAT4(255, 0, 255, 1) }//magenta
    };


    D3D11_BUFFER_DESC vertexDesc;
    ZeroMemory(&vertexDesc, sizeof(vertexDesc));
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(Vertex) * 8;

    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory(&resourceData, sizeof(resourceData));
    resourceData.pSysMem = vertices;
    HRESULT result = D3DDevice->CreateBuffer(&vertexDesc, &resourceData, &m_pVertexBuffer);
    if (FAILED(result))
    {
        return;
    }
}

void CPluginRender::InitIndex()
{
    UINT indices[] = {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    D3D11_BUFFER_DESC indexDesc;
    ZeroMemory(&indexDesc, sizeof(indexDesc));
    indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexDesc.ByteWidth = sizeof(UINT) * 36;

    D3D11_SUBRESOURCE_DATA indexData;
    ZeroMemory(&indexData, sizeof(indexData));
    indexData.pSysMem = indices;
    HRESULT result = D3DDevice->CreateBuffer(&indexDesc, &indexData, &m_pIndexBuffer);
    if (FAILED(result))
    {
        return;
    }
}


CPluginRender::CPluginRender(CRenderManager* RdrMgr) :m_theta(1.5f*XM_PI)
, m_phi(0.25f*XM_PI)
, m_radius(5.0f)
{
    this->D3DDevice = (ID3D11Device*)RdrMgr->GetD3DDevice();
    this->D3DDeviceContext = (ID3D11DeviceContext*)RdrMgr->GetD3DDeviceContext();
    this->D2DDeviceContext= (ID2D1DeviceContext1*)RdrMgr->GetD2DDeviceContext();


    D3D11_RASTERIZER_DESC wireframeDesc;
    ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
    wireframeDesc.FillMode = D3D11_FILL_SOLID;
    wireframeDesc.CullMode = D3D11_CULL_BACK;
    wireframeDesc.FrontCounterClockwise = false;
    wireframeDesc.DepthClipEnable = true;

    D3DDevice->CreateRasterizerState(&wireframeDesc, &frameRS);

    InitVetex();
    InitIndex();

    D3DX11CreateEffectFromFile(L"DxFx.cso", 0, D3DDevice, &this->HLSLEffect);
    D3D11_INPUT_ELEMENT_DESC solidColorLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numLayoutElements = ARRAYSIZE(solidColorLayout);
    D3DX11_PASS_DESC passDesc;
    m_pTechnique = this->HLSLEffect->GetTechniqueByName("BasicTech");
    m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
    m_pFxWorld = this->HLSLEffect->GetVariableByName("World")->AsMatrix();
    m_pFxView = this->HLSLEffect->GetVariableByName("View")->AsMatrix();
    m_pFxProj = this->HLSLEffect->GetVariableByName("Projection")->AsMatrix();

    HRESULT result = D3DDevice->CreateInputLayout(solidColorLayout, numLayoutElements, passDesc.pIAInputSignature,
        passDesc.IAInputSignatureSize, &m_pInputLayout);


    gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::Black, 1);
    gradientStops[0].position = 0.5f;
    gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::Black, 1);
    gradientStops[1].position = 1.0f;
    RdrMgr->GetD2DDeviceContext()->CreateGradientStopCollection(
        gradientStops,
        2,
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_CLAMP,
        &pGradientStops
    );
    RdrMgr->GetD2DDeviceContext()->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black),
        &SolidColorBrush
    );
}

CPluginRender::~CPluginRender()
{
}

void CPluginRender::Render()
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    //设置数据信息格式控制信息
    D3DDeviceContext->IASetInputLayout(m_pInputLayout);
    //设置要绘制的几何体信息
    D3DDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    D3DDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    //指明如何绘制
    D3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //设置渲染模式
    D3DDeviceContext->RSSetState(frameRS);
    //设置常量 
    XMMATRIX world = XMLoadFloat4x4(&m_world);
    XMMATRIX view = XMLoadFloat4x4(&m_view);
    XMMATRIX proj = XMLoadFloat4x4(&m_proj);
    //XMMATRIX worldViewProj = world*view*proj;
    m_pFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
    m_pFxView->SetMatrix(reinterpret_cast<float*>(&view));
    m_pFxProj->SetMatrix(reinterpret_cast<float*>(&proj));

    D3DX11_TECHNIQUE_DESC techDesc;
    m_pTechnique->GetDesc(&techDesc);
    for (UINT i = 0; i < techDesc.Passes; ++i)
    {
        m_pTechnique->GetPassByIndex(i)->Apply(0, D3DDeviceContext);
        D3DDeviceContext->DrawIndexed(36, 0, 0);
    }


    HRESULT hr = 0;
    /*D2D1_GRADIENT_STOP gradientStops[2];*/
    CComPtr<ID2D1RadialGradientBrush> m_pRadialGradientBrush;
    hr = D2DDeviceContext->CreateRadialGradientBrush(
        D2D1::RadialGradientBrushProperties(
            D2D1::Point2F(20, 20),
            D2D1::Point2F(0, 0),
            10,
            10),
        pGradientStops,
        &m_pRadialGradientBrush
    );
    if (SUCCEEDED(hr))
    {
        D2D1_ELLIPSE Ellipse;
        Ellipse.point = D2D1::Point2F(200, 200);
        Ellipse.radiusX = 100;
        Ellipse.radiusY = 100;
        D2DDeviceContext->FillEllipse(Ellipse, m_pRadialGradientBrush);
        //SafeRelease<ID2D1RadialGradientBrush>(&m_pRadialGradientBrush);
    }
}

void CPluginRender::UpdateBeforeRender()
{
    static float t = 0.0f;
    t += (float)XM_PI * 0.0125f;
    static DWORD dwTimeStart = 0;
    DWORD dwTimeCur = GetTickCount();
    if (dwTimeStart == 0)
        dwTimeStart = dwTimeCur;
    t = (dwTimeCur - dwTimeStart) / 1000.0f;
#ifdef _DEBUG
    // 将球面坐标转换为笛卡尔坐标
    float x = m_radius * sinf(m_phi)*cosf(m_theta);
    float z = m_radius * sinf(m_phi)*sinf(m_theta);
    float y = m_radius * cosf(m_phi);
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
#else
    XMVECTOR pos = XMVectorSet(2.0f, 0.0f, 0.0f, 1.0f);
#endif 
    XMVECTOR target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_view, V);
    XMMATRIX T = XMMatrixPerspectiveFovLH(XM_PIDIV2, 100 / static_cast<float>(100),
        0.01f, 100.0f);
    XMStoreFloat4x4(&m_proj, T);
    //根据时间旋转world矩阵
    XMMATRIX P = XMMatrixRotationY(t);
    XMStoreFloat4x4(&m_world, P);
}

void CPluginRender::UpdateAfterRender()
{

}

CRenderManager::CRenderManager()
{

}

namespace Colors
{
    FLOAT White[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    FLOAT Black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    //FLOAT Red = { 1.0f, 0.0f, 0.0f, 1.0f };
    //FLOAT  Green[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    /*FLOAT  Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
    FLOAT  Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
    FLOAT  Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
    FLOAT  Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

    FLOAT  Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
    FLOAT  LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };*/
}

void CRenderManager::UpdateAndRenderAll()
{
    D3DDeviceContext->ClearRenderTargetView(RenderTargetView, reinterpret_cast<const float*>(&Colors::White));
    D3DDeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    D2DDeviceContext->BeginDraw();

    PluginRender->UpdateBeforeRender();
    PluginRender->Render();
    PluginRender->UpdateAfterRender();

    D2DDeviceContext->EndDraw();
    SwapChain->Present(0, 0);
}
ID2D1Factory2* CRenderManager::DirectXInit(HWND hWindow)
{
    //初始化过程请参考
    //https://msdn.microsoft.com/en-us/library/windows/desktop/hh780339(v=vs.85).aspx

    // DXGI 适配器
    IDXGIAdapter*						pDxgiAdapter = nullptr;
    // DXGI 工厂
    IDXGIFactory2*						pDxgiFactory = nullptr;
    // DXGI Surface 后台缓冲
    IDXGISurface*						pDxgiBackBuffer = nullptr;
    // DXGI 设备
    IDXGIDevice1*						pDxgiDevice = nullptr;

    HRESULT hr = S_FALSE;

    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    /*#ifdef _DEBUG
    // Debug状态 有D3D DebugLayer就可以取消注释
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif*/
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    // 创建设备
    hr = D3D11CreateDevice(
        nullptr,					// 设为空指针选择默认设备
        D3D_DRIVER_TYPE_HARDWARE,	// 强行指定硬件渲染
        nullptr,					// 强行指定WARP渲染 D3D_DRIVER_TYPE_WARP 没有软件接口
        creationFlags,				// 创建flag
        featureLevels,				// 欲使用的特性等级列表
        ARRAYSIZE(featureLevels),	// 特性等级列表长度
        D3D11_SDK_VERSION,			// SDK 版本
        &D3DDevice,				// 返回的D3D11设备指针
        &m_featureLevel,			// 返回的特性等级
        &D3DDeviceContext);		// 返回的D3D11设备上下文指针

    if (hr)
    {
        return NULL;
    }


    ID2D1Factory2* D2D1Factory = NULL;
    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory2),
        reinterpret_cast<void**>(&D2D1Factory));
    if (hr)
    {
        return NULL;
    }
    RECT rc = { 0 };
    GetClientRect(hWindow, &rc);

    /*hr = D2D1Factory->CreateHwndRenderTarget(
    D2D1::RenderTargetProperties(),
    D2D1::HwndRenderTargetProperties(
    hWindow,
    D2D1::SizeU(
    rc.right - rc.left,
    rc.bottom - rc.top)
    ),
    &D2DDeviceContext
    );*/

    if (SUCCEEDED(hr))
        hr = D3DDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
    // 创建D2D设备
    if (SUCCEEDED(hr))
        hr = D2D1Factory->CreateDevice(pDxgiDevice, &D2DDevice);
    // 创建D2D设备上下文
    if (SUCCEEDED(hr))
        hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2DDeviceContext);

    /*RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = this->ScreenX;
    rc.bottom = this->ScreenY;*/
    /*this->RenderHwnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Static"), NULL,
    WS_CHILD | WS_VISIBLE | ES_MULTILINE,
    60, 30, this->ScreenX, this->ScreenY, hWindow, NULL, WindowManager->GetInstance(), NULL);

    hr = D2D1Factory->CreateHwndRenderTarget(
    D2D1::RenderTargetProperties(),
    D2D1::HwndRenderTargetProperties(
    this->RenderHwnd,
    D2D1::SizeU(
    rc.right - rc.left,
    rc.bottom - rc.top)
    ),
    &GameRenderTarget
    );*/
    if (hr)
    {
        return NULL;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = lround(rc.right - rc.left);
    swapChainDesc.BufferDesc.Height = lround(rc.bottom - rc.top);
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    //	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.OutputWindow = hWindow;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    //	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // 获取 IDXGIDevice
    if (SUCCEEDED(hr))
    {
        hr = D3DDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
    }
    // 获取Dxgi适配器 可以获取该适配器信息
    if (SUCCEEDED(hr))
    {
        hr = pDxgiDevice->GetAdapter(&pDxgiAdapter);
    }
    // 获取Dxgi工厂
    if (SUCCEEDED(hr))
    {
        hr = pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory));
    }
    // 创建交换链
    if (SUCCEEDED(hr))
    {
        hr = pDxgiFactory->CreateSwapChain(D3DDevice, &swapChainDesc, (IDXGISwapChain**)&SwapChain);
        /*hr = pDxgiFactory->CreateSwapChainForHwnd(
        D3DDevice,
        hWindow,
        &swapChainDescs,
        nullptr,                            //TODO:全屏相关
        nullptr,
        &SwapChain);*/
    }

    // 利用交换链获取Dxgi表面
    if (SUCCEEDED(hr))
    {
        hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(&pDxgiBackBuffer));
    }
    // 利用Dxgi表面创建位图
    if (SUCCEEDED(hr))
    {
        //暂时删除Direct2D通过表面初始化的方式
        /*D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f,
        96.0f);
        hr = D2DDeviceContext->CreateBitmapFromDxgiSurface(
        pDxgiBackBuffer,
        &bitmapProperties,
        &D2DTargetBimtap);*/

        D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE
        );

        D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT, pixelFormat, (0.0F), (0.0F),
            D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT
        );

        static D2D1_RENDER_TARGET_PROPERTIES props = {
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96,
            96
        };

        D2D1Factory->CreateDxgiSurfaceRenderTarget(
            pDxgiBackBuffer,
            &props,
            (ID2D1RenderTarget**)&D2DDeviceContext);
    }
    // 设置
    /*if (SUCCEEDED(hr))
    {
    // 设置 Direct2D 渲染目标
    D2DDeviceContext->SetTarget(D2DTargetBimtap);
    }*/

    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&backBuffer));

    //Create depth texture.
    D3D11_TEXTURE2D_DESC	depthTexDesc;
    ZeroMemory(&depthTexDesc, sizeof(depthTexDesc));
    depthTexDesc.Width = lround(rc.right - rc.left);
    depthTexDesc.Height = lround(rc.bottom - rc.top);
    depthTexDesc.MipLevels = 1;
    depthTexDesc.ArraySize = 1;
    depthTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthTexDesc.SampleDesc.Count = 1;
    depthTexDesc.SampleDesc.Quality = 0;
    depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
    depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthTexDesc.CPUAccessFlags = 0;
    depthTexDesc.MiscFlags = 0;

    D3DDevice->CreateRenderTargetView(backBuffer, 0, &RenderTargetView);

    D3DDevice->CreateTexture2D(
        &depthTexDesc, 0, &DepthStencilBuffer);

    D3D11_DEPTH_STENCIL_VIEW_DESC desvDesc;
    ZeroMemory(&desvDesc, sizeof(desvDesc));
    desvDesc.Format = depthTexDesc.Format;
    desvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    desvDesc.Texture2D.MipSlice = 0;

    D3DDevice->CreateDepthStencilView(
        DepthStencilBuffer, &desvDesc, &DepthStencilView);

    D3DDeviceContext->OMSetRenderTargets(
        1, &RenderTargetView, DepthStencilView);

    // Set the viewport transform.
    D3D11_VIEWPORT mScreenViewport;
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = (FLOAT)round(rc.right - rc.left);
    mScreenViewport.Height = (FLOAT)round(rc.bottom - rc.top);
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    D3DDeviceContext->RSSetViewports(1, &mScreenViewport);

    SafeRelease<IDXGIDevice1>(&pDxgiDevice);
    SafeRelease<IDXGIAdapter>(&pDxgiAdapter);
    SafeRelease<IDXGIFactory2>(&pDxgiFactory);
    SafeRelease<IDXGISurface>(&pDxgiBackBuffer);

    return D2D1Factory;
}

void CRenderManager::PluginRenderInit()
{
    PluginRender = new CPluginRender(this);
}

void CRenderManager::SetD2DFactory(ID2D1Factory2 * Fac)
{
    this->D2D1Factory = Fac;
}

ID2D1DeviceContext1 * CRenderManager::GetD2DDeviceContext()
{
    return this->D2DDeviceContext;
}

ID3D11DeviceContext * CRenderManager::GetD3DDeviceContext()
{
    return this->D3DDeviceContext;
}

ID3D11Device * CRenderManager::GetD3DDevice()
{
    return this->D3DDevice;
}

CRenderManager::~CRenderManager()
{
}
