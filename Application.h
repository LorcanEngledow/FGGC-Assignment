#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"

using namespace DirectX;

struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
	XMFLOAT2 Tex;
};

struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;

	XMFLOAT4 DiffuseMtrl;
	XMFLOAT4 DiffuseLight;
	XMFLOAT3 LightVecW;
	float gTime;

	XMFLOAT4 AmbientMtrl;
	XMFLOAT4 AmbientLight;

	XMFLOAT4 SpecularMtrl;
	XMFLOAT4 SpecularLight;
	float SpecularPower;
	XMFLOAT3 EyePosW;
};

class Application
{
private:
	HINSTANCE               _hInst;
	HWND                    _hWnd;
	D3D_DRIVER_TYPE         _driverType;
	D3D_FEATURE_LEVEL       _featureLevel;
	ID3D11Device*           _pd3dDevice;
	ID3D11DeviceContext*    _pImmediateContext;
	IDXGISwapChain*         _pSwapChain;
	ID3D11RenderTargetView* _pRenderTargetView;
	ID3D11VertexShader*     _pVertexShader;
	ID3D11PixelShader*      _pPixelShader;
	ID3D11InputLayout*      _pVertexLayout;
	ID3D11Buffer*           _pVertexBuffer;
	ID3D11Buffer*           _pPyVertexBuffer;
	ID3D11Buffer*           _pIndexBuffer;
	ID3D11Buffer*           _pPyIndexBuffer;
	ID3D11Buffer*           _pConstantBuffer;
	XMFLOAT4X4              _world, _world2, _world3, _moon, _moon2, _pyramid;
	XMFLOAT4X4              _view;
	XMFLOAT4X4              _projection;

	ID3D11DepthStencilView* _depthStencilView;
	ID3D11Texture2D* _depthStencilBuffer;

	ID3D11RasterizerState* _wireFrame;
	ID3D11RasterizerState* _solid;

	ID3D11ShaderResourceView* _pTextureRV = nullptr;
	ID3D11SamplerState* _pSamplerLinear = nullptr;

	bool wf;
private:
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	HRESULT InitDevice();
	void Cleanup();
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitShadersAndInputLayout();
	HRESULT InitCubeVertexBuffer();
	//HRESULT InitPyramidVertexBuffer();
	HRESULT InitCubeIndexBuffer();
	HRESULT InitPyramidIndexBuffer();

	UINT _WindowHeight;
	UINT _WindowWidth;

	XMFLOAT3 lightDirection;
	XMFLOAT4 diffuseMaterial;
	XMFLOAT4 diffuseLight;

	XMFLOAT4 AmbientMaterial;
	XMFLOAT4 AmbientLight;

	XMFLOAT4 SpecularMaterial;
	XMFLOAT4 SpecularLight;
	float SpecularPower;
	XMFLOAT3 EyePosW;
public:
	ConstantBuffer cb;
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	void Update();
	void Draw();
};

