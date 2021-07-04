/*
TODO list
----Set up debug layer to break on D3D errors
*/

#include "shared_state.h"
#include "sokoban.h"
#include "engine.cpp"

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma warning(push, 0)
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <math.h>
#pragma warning(pop)

struct cbuffer_constants {
    float tilePos[2];
    float cameraPos[2];
    float zoom;
    int textureIndex;
    float spriteSelected;
};

static void PlatformSprintfInteger(char* string, char* buffer, int value)
{
    wsprintf(buffer, string, value);
}

static inline void DrawQuad(shared_state state, vec2 pos, camera cam, int index, b32 selected)
{
    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    state.d3d.DeviceContext->Map(state.d3d.ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    cbuffer_constants* constants = (cbuffer_constants*)(mappedSubresource.pData);
    constants->tilePos[0] = pos.x;
    constants->tilePos[1] = pos.y;
    constants->zoom = cam.zoom;
    constants->cameraPos[0] = cam.pos.x;
    constants->cameraPos[1] = cam.pos.y;
    
    constants->textureIndex = index;
    constants->spriteSelected = (float)selected;
    state.d3d.DeviceContext->Draw(4, 0);
    state.d3d.DeviceContext->Unmap(state.d3d.ConstantBuffer, 0);
}

static inline void FileWrite(char* filename, void* data, u32 size)
{
    HANDLE hFile = CreateFileA(
                               filename,
                               GENERIC_WRITE,
                               FILE_SHARE_WRITE | FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );
    if (hFile) {
        DWORD NumberOfBytesWritten;
        WriteFile(
                  hFile,
                  data,
                  size ,
                  &NumberOfBytesWritten,
                  NULL
                  );
    }
    CloseHandle(hFile);
}

static inline void* FileRead(char* filename)
{
    HANDLE hFile = CreateFileA(
                               filename,
                               GENERIC_READ,
                               FILE_SHARE_WRITE | FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );
    
    size_t size = GetFileSize(hFile, NULL);
    void* result = AllocateMemory(size);
    DWORD NumberOfBytesRead = 0;
    if (hFile) {
        ReadFile(hFile, result, (DWORD)size, &NumberOfBytesRead, NULL);
    }
    CloseHandle(hFile);
    return(result);
}

static inline void Win32D3DReleaseEverything(shared_state *state)
{
    if (state->d3d.BlendState) state->d3d.BlendState->Release();
    if (state->d3d.ConstantBuffer) state->d3d.ConstantBuffer->Release();
    if (state->d3d.TextureView) state->d3d.TextureView->Release();
    if (state->d3d.Texture) state->d3d.Texture->Release();
    if (state->d3d.SamplerState) state->d3d.SamplerState->Release();
    if (state->d3d.VertexBuffer) state->d3d.VertexBuffer->Release();
    if (state->d3d.InputLayout) state->d3d.InputLayout->Release();
    if (state->d3d.RasteriserState) state->d3d.RasteriserState->Release();
    if (state->d3d.PixelShaderBlob) state->d3d.PixelShaderBlob->Release();
    if (state->d3d.PixelShader) state->d3d.PixelShader->Release();
    if (state->d3d.VertexShaderBlob) state->d3d.VertexShaderBlob->Release();
    if (state->d3d.VertexShader) state->d3d.VertexShader->Release();
    if (state->d3d.FrameBuffer) state->d3d.FrameBuffer->Release();
    if (state->d3d.FrameBufferView) state->d3d.FrameBufferView->Release();
    if (state->d3d.SwapChain) state->d3d.SwapChain->Release();
    if (state->d3d.DXGIFactory) state->d3d.DXGIFactory->Release();
    if (state->d3d.DXGIAdapter) state->d3d.DXGIAdapter->Release();
    if (state->d3d.DXGIDevice) state->d3d.DXGIDevice->Release();
    if (state->d3d.Device) state->d3d.Device->Release();
    if (state->d3d.DeviceContext) state->d3d.DeviceContext->Release();
    if (state->d3d.BaseDevice) state->d3d.BaseDevice->Release();
    if (state->d3d.BaseDeviceContext) state->d3d.BaseDeviceContext->Release();
    state->d3d = {};
    DestroyWindow(state->win32.window);
    UnregisterClassA(state->win32.wndClassEx.lpszClassName, GetModuleHandle(0));
    state->win32 = {};
}

static inline void* AllocateMemory(size_t size)
{
    void* result = (u32*)VirtualAlloc(NULL, (SIZE_T)size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return(result);
}

static inline void FreeMemory(void* p)
{
    VirtualFree(p, MEM_RELEASE, NULL);
}

static engine_state globalEngineState = {};

extern "C" shared_state UpdateDraw(shared_state sharedState)
{
    shared_state state = sharedState;
    state.prevInput = state.input;
    if (state.reloadProgram) {
        Win32D3DReleaseEverything(&state);
        
        state.win32.wndClassEx.cbSize = sizeof(state.win32.wndClassEx);
        state.win32.wndClassEx.lpfnWndProc = DefWindowProcA;
        state.win32.wndClassEx.lpszClassName = "WindowClassName";
        state.win32.wndClassEx.hCursor = LoadCursor(0, IDC_CROSS);
        
        RegisterClassExA(&state.win32.wndClassEx);
        
        state.win32.window = CreateWindowExA(0,
                                             state.win32.wndClassEx.lpszClassName,
                                             "Sokoban",
                                             WS_POPUP | WS_MAXIMIZE | WS_VISIBLE,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             NULL,
                                             NULL,
                                             GetModuleHandle(0), 
                                             NULL);
        D3D11CreateDevice(NULL,
                          D3D_DRIVER_TYPE_HARDWARE,
                          NULL,
                          D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                          NULL, //TODO: feature levels
                          0, //TODO: num feature levels
                          D3D11_SDK_VERSION,
                          &state.d3d.BaseDevice,
                          NULL,
                          &state.d3d.BaseDeviceContext);
        state.d3d.BaseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&state.d3d.Device);
        state.d3d.BaseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&state.d3d.DeviceContext);
        state.d3d.Device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&state.d3d.DXGIDevice);
        state.d3d.DXGIDevice->GetAdapter(&state.d3d.DXGIAdapter);
        state.d3d.DXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&state.d3d.DXGIFactory);
        
        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
        SwapChainDesc.Width = 0;
        SwapChainDesc.Height = 0;
        SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        SwapChainDesc.Stereo = FALSE;
        SwapChainDesc.SampleDesc.Count = 1;
        SwapChainDesc.SampleDesc.Quality = 0;
        SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.BufferCount = 2;
        SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        SwapChainDesc.Flags = 0;
        state.d3d.DXGIFactory->CreateSwapChainForHwnd(state.d3d.Device, 
                                                      state.win32.window,
                                                      &SwapChainDesc,
                                                      NULL,
                                                      NULL,
                                                      &state.d3d.SwapChain);
        state.d3d.SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&state.d3d.FrameBuffer);
        state.d3d.Device->CreateRenderTargetView(state.d3d.FrameBuffer, NULL, &state.d3d.FrameBufferView);
        
        ID3DBlob* ErrorMsgs = 0;
        HRESULT hr = D3DCompileFromFile(L"../release/shaders/sprite.hlsl", NULL, NULL, "VertexShaderMain", "vs_5_0", 0, 0, &(state.d3d.VertexShaderBlob), &ErrorMsgs);
        if (!SUCCEEDED(hr)) {
            MessageBoxA(0, (char*)ErrorMsgs->GetBufferPointer(), "Vertex Shader Error", MB_ICONERROR);
            ErrorMsgs->Release();
        }
        state.d3d.Device->CreateVertexShader(state.d3d.VertexShaderBlob->GetBufferPointer(), state.d3d.VertexShaderBlob->GetBufferSize(), NULL, &state.d3d.VertexShader);
        hr = D3DCompileFromFile(L"../release/shaders/sprite.hlsl", NULL, NULL, "PixelShaderMain", "ps_5_0", 0, 0, &(state.d3d.PixelShaderBlob), &ErrorMsgs);
        if (!SUCCEEDED(hr)) {
            MessageBoxA(0, (char*)ErrorMsgs->GetBufferPointer(), "Pixel Shader Error", MB_ICONERROR);
            ErrorMsgs->Release();
        }
        state.d3d.Device->CreatePixelShader(state.d3d.PixelShaderBlob->GetBufferPointer(), state.d3d.PixelShaderBlob->GetBufferSize(), NULL, &state.d3d.PixelShader);
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        state.d3d.Device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), state.d3d.VertexShaderBlob->GetBufferPointer(),
                                            state.d3d.VertexShaderBlob->GetBufferSize(), &state.d3d.InputLayout);
        
        D3D11_RASTERIZER_DESC1 rasteriserDesc = {};
        rasteriserDesc.FillMode = D3D11_FILL_SOLID;
        rasteriserDesc.CullMode = D3D11_CULL_NONE;
        state.d3d.Device->CreateRasterizerState1(&rasteriserDesc, &state.d3d.RasteriserState);
        
        float data[] = {
            -0.5f, -0.5f, 0.0f, 1.0f,
            0.5f, -0.5f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.0f, 0.0f,
            0.5f, 0.5f, 1.0f, 0.0f
        };
        
        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = sizeof(data);
        vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vertexData = {data};
        state.d3d.Device->CreateBuffer(&vertexBufferDesc, &vertexData, &state.d3d.VertexBuffer);
        
        D3D11_SAMPLER_DESC samplerDesc = {};
        //samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        
        state.d3d.Device->CreateSamplerState(&samplerDesc, &state.d3d.SamplerState);
        
        //TODO: Replace this with file read (add file header with dimensions)
        //FileRead("../release/data/sprites.spritestack");
        HANDLE hFile = CreateFileA("../release/data/sprites.spritestack",
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        int size = (int)GetFileSize(hFile, NULL);
        
        //TODO: Parse file header (and generate it on asset_builder.exe)
        
        const int depth = Level::NUM_TILES;
        const int byteDepth = sizeof(u32);
        const u32 width = (const u32)sqrt((size/depth)/byteDepth);
        const u32 height = width;
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = depth; //NOTE
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        
        u32* texture = (u32*)AllocateMemory((size_t)size);
        DWORD NumberOfBytesRead;
        ReadFile(hFile, texture, (DWORD)size, &NumberOfBytesRead, NULL);
        CloseHandle(hFile);
        
        D3D11_SUBRESOURCE_DATA textureData[depth] = {};
        for (int i = 0; i < depth; ++i) {
            textureData[i].pSysMem = &texture[width*height*i];
            textureData[i].SysMemPitch = width * sizeof(u32);
            textureData[i].SysMemSlicePitch = width * height * sizeof(float);
        }
        
        hr = state.d3d.Device->CreateTexture2D(&textureDesc, textureData, &state.d3d.Texture);
        state.d3d.Device->CreateShaderResourceView(state.d3d.Texture, NULL, &state.d3d.TextureView);
        FreeMemory(texture);
        
        
        D3D11_BUFFER_DESC constantBufferDesc = {};
        constantBufferDesc.ByteWidth = (sizeof(cbuffer_constants) + 0xf) & 0xfffffff0;
        constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        state.d3d.Device->CreateBuffer(&constantBufferDesc, NULL, &state.d3d.ConstantBuffer);
        
        D3D11_BLEND_DESC BlendStateDesc = {};
        BlendStateDesc.AlphaToCoverageEnable = FALSE;
        BlendStateDesc.IndependentBlendEnable = FALSE;
        BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
        BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        state.d3d.Device->CreateBlendState(&BlendStateDesc, &state.d3d.BlendState);
        
        globalEngineState.loadLevel = true;
        
        state.reloadProgram = false;
    }
    
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) {
            b32 btnState = !(msg.lParam & (1 << 31));
            switch(msg.wParam) {
                case VK_F1: {
                    state.input.f1 = btnState;
                } break;
                case VK_F2: {
                    state.input.f2 = btnState;
                } break;
                case VK_SPACE: {
                    state.input.space = btnState;
                } break;
                case VK_OEM_PERIOD: {
                    state.input.decimal = btnState;
                } break;
                case VK_OEM_COMMA: {
                    state.input.comma = btnState;
                } break;
                case VK_OEM_4: {
                    state.input.leftBrace = btnState;
                } break;
                case VK_OEM_6: {
                    state.input.rightBrace = btnState;
                } break;
                case VK_ESCAPE: { 
                    state.input.escape = btnState;
                    state.quit = true;
                } break;
                case VK_UP: {
                    state.input.up = btnState;
                } break;
                case VK_DOWN: {
                    state.input.down = btnState;
                } break;
                case VK_LEFT: {
                    state.input.left = btnState;
                } break;
                case VK_RIGHT: {
                    state.input.right = btnState;
                } break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 1.0f };
    UINT stride = 4 * sizeof(float);
    UINT offset = 0;
    float clearColour[4] = { 0.045f, 0.045f, 0.05f, 1.0f };
    state.d3d.DeviceContext->ClearRenderTargetView(state.d3d.FrameBufferView, clearColour);
    state.d3d.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    state.d3d.DeviceContext->IASetInputLayout(state.d3d.InputLayout);
    state.d3d.DeviceContext->IASetVertexBuffers(0, 1, &state.d3d.VertexBuffer, &stride, &offset);
    state.d3d.DeviceContext->VSSetShader(state.d3d.VertexShader, NULL, 0);
    state.d3d.DeviceContext->VSSetConstantBuffers(0, 1, &state.d3d.ConstantBuffer);
    state.d3d.DeviceContext->RSSetViewports(1, &viewport);
    state.d3d.DeviceContext->RSSetState(state.d3d.RasteriserState);
    state.d3d.DeviceContext->PSSetShader(state.d3d.PixelShader, NULL, 0);
    state.d3d.DeviceContext->PSSetShaderResources(0, 1, &state.d3d.TextureView);
    state.d3d.DeviceContext->PSSetSamplers(0, 1, &state.d3d.SamplerState);
    state.d3d.DeviceContext->PSSetConstantBuffers(0, 1, &state.d3d.ConstantBuffer);
    state.d3d.DeviceContext->OMSetRenderTargets(1, &state.d3d.FrameBufferView, 0);
    float blendFactor[4] = {};
    state.d3d.DeviceContext->OMSetBlendState(state.d3d.BlendState, blendFactor, 0xffffffff);
    
    globalEngineState = EngineInit(state, globalEngineState);
    globalEngineState = EngineUpdateDraw(state, globalEngineState);
    
    state.d3d.SwapChain->Present(1, 0);
    
    if (state.quit || state.releaseEverything) {
        Win32D3DReleaseEverything(&state);
        state.releaseEverything = false;
        state.reloadProgram = true;
    }
    return(state);
}