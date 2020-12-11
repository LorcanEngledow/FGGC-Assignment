#include "Application.h"
#include "DDSTextureLoader.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

Application::Application()
{
	_hInst = nullptr;
	_hWnd = nullptr;
	_driverType = D3D_DRIVER_TYPE_NULL;
	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	_pd3dDevice = nullptr;
	_pImmediateContext = nullptr;
	_pSwapChain = nullptr;
	_pRenderTargetView = nullptr;
	_pVertexShader = nullptr;
	_pPixelShader = nullptr;
	_pVertexLayout = nullptr;
	_pVertexBuffer = nullptr;
	_pPyVertexBuffer = nullptr;
	_pIndexBuffer = nullptr;
	_pPyIndexBuffer = nullptr;
	_pConstantBuffer = nullptr;
}

Application::~Application()
{
	Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
        return E_FAIL;
	}

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _WindowWidth = rc.right - rc.left;
    _WindowHeight = rc.bottom - rc.top;

    if (FAILED(InitDevice()))
    {
        Cleanup();

        return E_FAIL;
    }

	// Initialize the world matrix
	XMStoreFloat4x4(&_world, XMMatrixIdentity());

    // Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(5.0f, 5.0f, 2.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMStoreFloat4x4(&_view, XMMatrixLookAtLH(Eye, At, Up));

    // Initialize the projection matrix
	XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, _WindowWidth / (FLOAT) _WindowHeight, 0.01f, 100.0f));

    // Light direction from surface (XYZ)
    lightDirection = XMFLOAT3(0.25f, 0.5f, -1.0f);
    // Diffuse material properties (RGBA)
    diffuseMaterial = XMFLOAT4(0.8f, 0.5f, 0.5f, 1.0f);
    // Diffuse light colour (RGBA)
    diffuseLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    // Ambient material properties (RGBA)
    AmbientMaterial = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    // Ambient light properties (RGBA)
    AmbientLight = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);

    // Specular material properties (RGBA)
    SpecularMaterial = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    // Spuclar light properties (RGBA)
    SpecularLight = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    // Specular Power
    SpecularPower = 3.0f;


    // Creates DDS Texture Object and assigns it to texture register 0
    CreateDDSTextureFromFile(_pd3dDevice, L"Crate_COLOR.dds", nullptr, &_pTextureRV);
    _pImmediateContext->PSSetShaderResources(0, 1, &_pTextureRV);

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    _pd3dDevice->CreateSamplerState(&sampDesc, &_pSamplerLinear);

	return S_OK;
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{	
		pVSBlob->Release();
        return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

    if (FAILED(hr))
        return hr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
        return hr;

    // Set the input layout
    _pImmediateContext->IASetInputLayout(_pVertexLayout);

	return hr;
}

HRESULT Application::InitCubeVertexBuffer()
{
    HRESULT hr;

    // Create vertex buffer
    SimpleVertex vertices[] =
    {
        // Top
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 3 // 0
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 1 // 1
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 0 // 2

        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // 2 // 3
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 1 // 4
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 3 // 5

        // Bottom
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 6 // 6
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 4 // 7
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 5 // 8

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // 7 // 9
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 4 // 10
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 6 // 11

        // Left
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 11 // 12
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 9 // 13
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // 8 // 14

        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 10 // 15
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 9 // 16
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 11 // 17

        // Right
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 14 // 18
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 12 // 19
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 13 // 20

        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }, // 18 // 27
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // 17 // 28
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // 19 // 29

        // Front
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }, // 19 // 24
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) }, // 17 // 25
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) }, // 16 // 26

        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) }, // 19 // 24
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) }, // 19 // 24
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) }, // 19 // 24

        // Back
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 19 // 24
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) }, // 17 // 25
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) }, // 16 // 26

        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) }, // 19 // 24
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) }, // 19 // 24
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 19 // 24

    };

    SimpleVertex vertices2[] =
    {
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.6f, 1.6f, 0.8f), XMFLOAT2(1.0f, 1.0f) }, //B
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.8f, 0.8f, -1.6f), XMFLOAT2(1.0f, 1.0f) }, //G
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-0.8f, 0.8f, 1.6f), XMFLOAT2(1.0f, 1.0f) }, //Y
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.6f, 1.6f, 0.8f), XMFLOAT2(1.0f, 1.0f) }, //R

        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.6f, -1.6f, -0.8f), XMFLOAT2(1.0f, 1.0f) }, //P
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-0.8f, -0.8f, -1.6f), XMFLOAT2(1.0f, 1.0f) }, //BL
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.8f, -0.8f, 1.6f), XMFLOAT2(1.0f, 1.0f) }, //GR
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.6f, -1.6f, 0.8f), XMFLOAT2(1.0f, 1.0f) }, //W
    };

    //CalculateModelVectors(vertices, 36);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 36;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pVertexBuffer);

    if (FAILED(hr))
        return hr;
}

/*HRESULT Application::InitPyramidVertexBuffer()
{
	HRESULT phr;

    // Create vertex buffer
    SimpleVertex pyramidVertices[] =
    {
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.5f, 0.2f, 1.0f) }, //Base
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.5f, 0.2f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.5f, 0.2f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.5f, 0.2f, 1.0f) },

        { XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.78f, 0.0f, 0.0f, 1.0f) }, // Tip
    };

    D3D11_BUFFER_DESC pbd;
    ZeroMemory(&pbd, sizeof(pbd));
    pbd.Usage = D3D11_USAGE_DEFAULT;
    pbd.ByteWidth = sizeof(SimpleVertex) * 9;
    pbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    pbd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA pInitData;
    ZeroMemory(&pInitData, sizeof(pInitData));
    pInitData.pSysMem = pyramidVertices;

    phr = _pd3dDevice->CreateBuffer(&pbd, &pInitData, &_pPyVertexBuffer);

    if (FAILED(phr))
        return phr;

	return S_OK;
}*/

HRESULT Application::InitCubeIndexBuffer()
{
    HRESULT hr;

    /*WORD indices[] =
  {
      4,1,3, //+x 1
      4,3,6,

      3,1,0, //+y 3
      3,0,2,

      6,3,2, //+z 5
      6,2,7,

      7,2,0, //-x 7
      7,0,5,

      7,5,4, //-y 9
      7,4,6,

      5,0,1, //-z 11
      5,1,4
  };*/

    // Create index buffer
    WORD indices[] =
    {
        0,1,2,
        3,4,5,

        6,7,8,
        9,10,11,

        12,13,14,
        15,16,17,

        18,19,20,
        21,22,23,

        24,25,26,
        27,28,29,

        30,31,32,
        33,34,35
    };

   D3D11_BUFFER_DESC bd;
   ZeroMemory(&bd, sizeof(bd));

   bd.Usage = D3D11_USAGE_DEFAULT;
   bd.ByteWidth = sizeof(WORD) * 36;
   bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
   bd.CPUAccessFlags = 0;

   D3D11_SUBRESOURCE_DATA InitData;
   ZeroMemory(&InitData, sizeof(InitData));
   InitData.pSysMem = indices;
   hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pIndexBuffer);

   if (FAILED(hr))
       return hr;
}

HRESULT Application::InitPyramidIndexBuffer()
{
	HRESULT phr;

    // Create pyramid index buffer
    WORD pindices[] =
    {
        0,1,4,
        2,4,0,
        3,4,2,
        1,4,3,
        3,1,2,
        2,1,0
    };

    D3D11_BUFFER_DESC pbd;
    ZeroMemory(&pbd, sizeof(pbd));

    pbd.Usage = D3D11_USAGE_DEFAULT;
    pbd.ByteWidth = sizeof(WORD) * 18;
    pbd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    pbd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA pInitData;
    ZeroMemory(&pInitData, sizeof(pInitData));
    pInitData.pSysMem = pindices;
    phr = _pd3dDevice->CreateBuffer(&pbd, &pInitData, &_pPyIndexBuffer);

    if (FAILED(phr))
        return phr;

	return S_OK;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    _hInst = hInstance;
    RECT rc = {0, 0, 640, 480};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindow(L"TutorialWindowClass", L"DX11 Framework", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                         nullptr);
    if (!_hWnd)
		return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);

    return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob != nullptr)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

        if (pErrorBlob) pErrorBlob->Release();

        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT Application::InitDevice()
{
    bool wf = true;

    HRESULT hr = S_OK;

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
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _WindowWidth;
    sd.BufferDesc.Height = _WindowHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;

    D3D11_TEXTURE2D_DESC depthStencilDesc;

    depthStencilDesc.Width = _WindowWidth;
    depthStencilDesc.Height = _WindowHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    _pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);
    _pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

    _pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_WindowWidth;
    vp.Height = (FLOAT)_WindowHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

    //InitPyramidVertexBuffer();
    InitCubeVertexBuffer();
    InitCubeIndexBuffer();
    InitPyramidIndexBuffer();

    //// Set vertex buffer
    //UINT stride = sizeof(SimpleVertex);
    //UINT offset = 0;
    //_pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBuffer, &stride, &offset);

    //InitCubeIndexBuffer();

    //// Set index buffer
    //_pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
    ConstantBuffer cb;

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &_pConstantBuffer);

    D3D11_RASTERIZER_DESC wfdesc;
    ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
    wfdesc.FillMode = D3D11_FILL_WIREFRAME;
    wfdesc.CullMode = D3D11_CULL_NONE;
    hr = _pd3dDevice->CreateRasterizerState(&wfdesc, &_wireFrame);

    D3D11_RASTERIZER_DESC sdesc;
    ZeroMemory(&sdesc, sizeof(D3D11_RASTERIZER_DESC));
    sdesc.FillMode = D3D11_FILL_SOLID;
    sdesc.CullMode = D3D11_CULL_NONE;
    hr = _pd3dDevice->CreateRasterizerState(&sdesc, &_solid);

    if (FAILED(hr))
        return hr;

    return S_OK;
}

void Application::Cleanup()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();

    if (_pConstantBuffer) _pConstantBuffer->Release();
    if (_pVertexBuffer) _pVertexBuffer->Release();
    if (_pIndexBuffer) _pIndexBuffer->Release();
    if (_pVertexLayout) _pVertexLayout->Release();
    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();
    if (_pRenderTargetView) _pRenderTargetView->Release();
    if (_pSwapChain) _pSwapChain->Release();
    if (_pImmediateContext) _pImmediateContext->Release();
    if (_pd3dDevice) _pd3dDevice->Release();
    if (_depthStencilView) _depthStencilView->Release();
    if (_depthStencilBuffer) _depthStencilBuffer->Release();
    if (_wireFrame) _wireFrame->Release();
    if (_solid) _solid->Release();
}

void Application::Update()
{
    // Update our time
    static float t = 0.0f;

    if (_driverType == D3D_DRIVER_TYPE_REFERENCE)
    {
        t += (float) XM_PI * 0.0125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();

        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;

        t = (dwTimeCur - dwTimeStart) / 1000.0f;
    }

    cb.gTime = t;

    if (GetAsyncKeyState(VK_UP))
    {
        wf = !wf;
    }

    //
    // Animate the cube
    //
    XMStoreFloat4x4(&_world, XMMatrixScaling(1.4, 1.4, 1.4) * XMMatrixRotationY(t)); //Sun

    XMStoreFloat4x4(&_world2, XMMatrixScaling(1.1, 1.1, 1.1) * XMMatrixTranslation(5, 0 ,0) * XMMatrixRotationY(t/500) * XMMatrixRotationY(t)); //Planet 1

	XMStoreFloat4x4(&_moon, XMMatrixScaling(0.7, 0.7, 0.7) * XMMatrixRotationY(t*2) * XMMatrixRotationZ(t) * (XMMatrixTranslation(4.0, 0.0, 0.0)  * XMMatrixRotationY(t))
                                                        * (XMMatrixRotationY(t*2)* XMMatrixTranslation(6.0, 0.0, 0.0) * XMMatrixRotationY(t))); //Moon 1


    XMStoreFloat4x4(&_world3, XMMatrixScaling(0.9, 0.9, 0.9) * XMMatrixTranslation(5, 0, 0) *  XMMatrixRotationY(t / 500) * XMMatrixRotationY(t*2)); //Planet 2

    XMStoreFloat4x4(&_moon2, XMMatrixScaling(0.4, 0.4, 0.4) * XMMatrixRotationY(t * 2) * XMMatrixRotationZ(t*2) * (XMMatrixTranslation(2.0, 0.0, 0.0) * XMMatrixRotationY(t))
                                                        * (XMMatrixRotationY(t * 1.5) * XMMatrixTranslation(8.0, 0.0, 0.0) * XMMatrixRotationY(t*2))); //Moon 2

    XMStoreFloat4x4(&_pyramid, XMMatrixRotationY(t * 2) * XMMatrixTranslation(3, 2, 3)); //Pyramid
}

void Application::Draw()
{
    _pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    
    if (!wf)
    {
        _pImmediateContext->RSSetState(_wireFrame);
    }
    else
    {
        _pImmediateContext->RSSetState(_solid);
    }

    //
    // Clear the back buffer
    //
    float ClearColor[4] = {0.15f, 0.0f, 0.3f, 1.0f}; // red,green,blue,alpha
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);

	XMMATRIX world = XMLoadFloat4x4(&_world);
	XMMATRIX view = XMLoadFloat4x4(&_view);
	XMMATRIX projection = XMLoadFloat4x4(&_projection);

    //
    // Update variables
    //
	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(view);
	cb.mProjection = XMMatrixTranspose(projection);

    cb.LightVecW = lightDirection;
    cb.DiffuseMtrl = diffuseMaterial;
    cb.DiffuseLight = diffuseLight;
    cb.AmbientMtrl = AmbientMaterial;
    cb.AmbientLight = AmbientLight;
    cb.SpecularMtrl = SpecularMaterial;
    cb.SpecularLight = SpecularLight;
    cb.SpecularPower = SpecularPower;
    cb.EyePosW = EyePosW;

    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);

    // Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    _pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBuffer, &stride, &offset);

    // Set index buffer
    _pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);


    //
    // Renders a triangle
    //
	_pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
	_pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
	_pImmediateContext->DrawIndexed(36, 0, 0);        

    world = XMLoadFloat4x4(&_world);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(36, 0, 0);

    //XMMATRIX world2 = XMLoadFloat4x4(&_world2);
    //cb.mWorld = XMMatrixTranspose(world2);
    //_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    //_pImmediateContext->DrawIndexed(36, 0, 0);

    //XMMATRIX world3 = XMLoadFloat4x4(&_world3);
    //cb.mWorld = XMMatrixTranspose(world3);
    //_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    //_pImmediateContext->DrawIndexed(36, 0, 0);

    // /*Set vertex buffer
    //_pImmediateContext->IASetVertexBuffers(0, 1, &_pPyVertexBuffer, &stride, &offset);
    // Set index buffer
    //_pImmediateContext->IASetIndexBuffer(_pPyIndexBuffer, DXGI_FORMAT_R16_UINT, 0);*/

    //XMMATRIX moon = XMLoadFloat4x4(&_moon);
    //cb.mWorld = XMMatrixTranspose(moon);
    //_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    //_pImmediateContext->DrawIndexed(36, 0, 0);

    //XMMATRIX moon2 = XMLoadFloat4x4(&_moon2);
    //cb.mWorld = XMMatrixTranspose(moon2);
    //_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    //_pImmediateContext->DrawIndexed(36, 0, 0);

    /*XMMATRIX pyramid = XMLoadFloat4x4(&_pyramid);
    cb.mWorld = XMMatrixTranspose(pyramid);
    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    _pImmediateContext->DrawIndexed(18, 0, 0);*/

    //
    // Present our back buffer to our front buffer
    //
    _pSwapChain->Present(0, 0);
}