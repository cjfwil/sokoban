/* date = December 11th 2020 5:59 pm */

#include "newtypes.h"

#pragma warning(push, 0)
#include <windows.h>
#include <d3d11_1.h>
#pragma warning(pop)

#ifndef SHARED_STATE_H
#define SHARED_STATE_H

struct win32_state {
    HWND window;
    WNDCLASSEX wndClassEx = { sizeof(wndClassEx) };
};

struct d3d_state {
    ID3D11Device *BaseDevice;
    ID3D11DeviceContext *BaseDeviceContext;
    ID3D11Device1 *Device;
    ID3D11DeviceContext1 *DeviceContext;
    IDXGIDevice1 *DXGIDevice;
    IDXGIAdapter *DXGIAdapter;
    IDXGIFactory2 *DXGIFactory;
    IDXGISwapChain1 *SwapChain;
    ID3D11Texture2D *FrameBuffer;
    ID3D11RenderTargetView *FrameBufferView;
    ID3DBlob *VertexShaderBlob;
    ID3D11VertexShader *VertexShader;
    ID3DBlob *PixelShaderBlob;
    ID3D11PixelShader *PixelShader;
    ID3D11RasterizerState1 *RasteriserState;
    ID3D11InputLayout *InputLayout;
    ID3D11Buffer *VertexBuffer;
    ID3D11SamplerState *SamplerState;
    ID3D11Texture2D *Texture;
    ID3D11ShaderResourceView *TextureView;
    ID3D11Buffer *ConstantBuffer;
    ID3D11BlendState *BlendState;
};

struct input_state {
    b32 escape;
    b32 space;
    b32 decimal;
    b32 comma;
    b32 leftBrace;
    b32 rightBrace;
    b32 up;
    b32 down;
    b32 left;
    b32 right;
    b32 f1;
    b32 f2;
};

#pragma pack(push, 4)
struct shared_state {
    win32_state win32 = {};
    d3d_state d3d = {};
    input_state input = {};
    input_state prevInput = {};
    b32 quit = false;
    b32 reloadProgram = true;
    b32 releaseEverything = false;
};
#pragma pack(pop)

#endif //SHARED_STATE_H
