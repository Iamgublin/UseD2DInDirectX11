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
    CComPtr<ID3D11DeviceContext> D3DDeviceContext;            //D3D11�豸������ָ�� 
    ID2D1DeviceContext1* D2DDeviceContext;            //D2D�豸������ָ��
private:
    //���㻺�棬�±껺�� 
    ID3D11Buffer * m_pVertexBuffer;
    ID3D11Buffer *m_pIndexBuffer;//����
    ID3D11InputLayout *m_pInputLayout;
    //��Ⱦģʽ(����DX9��SetRenderState����)
    ID3D11RasterizerState* frameRS;
    //��ɫ������
    ID3DX11EffectTechnique *m_pTechnique;
    //������ɫ������
    ID3DX11EffectMatrixVariable *m_pFxWorld;
    ID3DX11EffectMatrixVariable *m_pFxView;
    ID3DX11EffectMatrixVariable *m_pFxProj;

    XMFLOAT4X4 m_world;
    XMFLOAT4X4 m_view;
    XMFLOAT4X4 m_proj;
    ID3DX11Effect*          HLSLEffect;          //��ɫ��

    float m_theta;	//
    float m_phi;	//
    float m_radius;	//�뾶

    CComPtr<ID2D1GradientStopCollection> pGradientStops;
    CComPtr<ID2D1SolidColorBrush> SolidColorBrush;
    D2D1_GRADIENT_STOP gradientStops[2];
};



class CRenderManager
{
public:
    CRenderManager();
    void UpdateAndRenderAll();                                       //�������Ⱦ���ж���
    ID2D1Factory2* DirectXInit(HWND hWindow);
    void PluginRenderInit();
    void SetD2DFactory(ID2D1Factory2* Fac);
    ID2D1DeviceContext1 * GetD2DDeviceContext();
    ID3D11DeviceContext* GetD3DDeviceContext();
    ID3D11Device* GetD3DDevice();
    ~CRenderManager();
private:
    D3D_FEATURE_LEVEL m_featureLevel;
    CComPtr<ID3D11Device> D3DDevice;                  //D3D11�豸ָ��
    CComPtr<ID3D11DeviceContext> D3DDeviceContext;            //D3D11�豸������ָ��  
    ID3D11RenderTargetView*  RenderTargetView;
    ID3D11Texture2D* DepthStencilBuffer;
    ID3D11DepthStencilView* DepthStencilView;
    ID3D11Texture2D* backBuffer;
private:
    CComPtr<ID2D1Device1> D2DDevice;                          //D2D�豸ָ��
    ID2D1DeviceContext1* D2DDeviceContext;            //D2D�豸������ָ��
    IDXGISwapChain1* SwapChain;                               //DXGI��������������Ⱦ
    CPluginRender* PluginRender;
    CComPtr<ID2D1Factory2> D2D1Factory;
};