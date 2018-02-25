#pragma once
#include "stdafx.h"
struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

class CRenderManager;

class CPluginRender
{
public:
    CPluginRender(CRenderManager* RdrMgr);
    ~CPluginRender();
    void Render();
    void UpdateBeforeRender(); 
    void UpdateAfterRender();
    void InitIndex();
    void InitVetex();
private:
    CComPtr<ID3D11Device> D3DDevice;
    CComPtr<ID3D11DeviceContext> D3DDeviceContext;            //D3D11设备上下文指针 
    ID2D1DeviceContext1* D2DDeviceContext;            //D2D设备上下文指针
private:
    //顶点缓存，下标缓存 
    ID3D11Buffer * m_pVertexBuffer;
    ID3D11Buffer *m_pIndexBuffer;//新增
    ID3D11InputLayout *m_pInputLayout;
    //渲染模式(类似DX9的SetRenderState函数)
    ID3D11RasterizerState* frameRS;
    //着色器配置
    ID3DX11EffectTechnique *m_pTechnique;
    //输入着色器管线
    ID3DX11EffectMatrixVariable *m_pFxWorld;
    ID3DX11EffectMatrixVariable *m_pFxView;
    ID3DX11EffectMatrixVariable *m_pFxProj;

    XMFLOAT4X4 m_world;
    XMFLOAT4X4 m_view;
    XMFLOAT4X4 m_proj;
    ID3DX11Effect*          HLSLEffect;          //着色器

    float m_theta;	//
    float m_phi;	//
    float m_radius;	//半径

    CComPtr<ID2D1GradientStopCollection> pGradientStops;
    CComPtr<ID2D1SolidColorBrush> SolidColorBrush;
    D2D1_GRADIENT_STOP gradientStops[2];
};



class CRenderManager
{
public:
    CRenderManager();
    void UpdateAndRenderAll();                                       //计算和渲染所有对象
    ID2D1Factory2* DirectXInit(HWND hWindow);
    void PluginRenderInit();
    void SetD2DFactory(ID2D1Factory2* Fac);
    ID2D1DeviceContext1 * GetD2DDeviceContext();
    ID3D11DeviceContext* GetD3DDeviceContext();
    ID3D11Device* GetD3DDevice();
    ~CRenderManager();
private:
    D3D_FEATURE_LEVEL m_featureLevel;
    CComPtr<ID3D11Device> D3DDevice;                  //D3D11设备指针
    CComPtr<ID3D11DeviceContext> D3DDeviceContext;            //D3D11设备上下文指针  
    ID3D11RenderTargetView*  RenderTargetView;
    ID3D11Texture2D* DepthStencilBuffer;
    ID3D11DepthStencilView* DepthStencilView;
    ID3D11Texture2D* backBuffer;
private:
    CComPtr<ID2D1Device1> D2DDevice;                          //D2D设备指针
    ID2D1DeviceContext1* D2DDeviceContext;            //D2D设备上下文指针
    IDXGISwapChain1* SwapChain;                               //DXGI交换链，用于渲染
    CPluginRender* PluginRender;
    CComPtr<ID2D1Factory2> D2D1Factory;
};