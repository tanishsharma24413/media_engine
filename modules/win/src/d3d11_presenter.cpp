#include "media/win/d3d11_presenter.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include <iterator>

namespace media::win {

namespace {

template <class T>
void com_release(T*& ptr) {
  if (ptr) {
    ptr->Release();
    ptr = nullptr;
  }
}

} // namespace

static const char* kShaderSrc = R"(
Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct VSOut {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VSOut vs_main(uint id : SV_VertexID) {
    VSOut output;
    output.uv = float2((id << 1) & 2, id & 2);
    output.pos = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}

float4 ps_main(VSOut input) : SV_Target {
    return tex.Sample(sam, input.uv);
}
)";

D3d11Presenter::D3d11Presenter() = default;

D3d11Presenter::~D3d11Presenter() { shutdown(); }

bool D3d11Presenter::initialize(HWND hwnd) {
  shutdown();
  hwnd_ = hwnd;

  DXGI_SWAP_CHAIN_DESC desc{};
  desc.BufferCount = 2;
  desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.BufferDesc.RefreshRate.Numerator = 0;
  desc.BufferDesc.RefreshRate.Denominator = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.OutputWindow = hwnd_;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Windowed = TRUE;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
  D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_11_0;

  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      flags,
      levels,
      static_cast<UINT>(std::size(levels)),
      D3D11_SDK_VERSION,
      &desc,
      &swap_,
      &device_,
      &obtained,
      &ctx_);

  if (FAILED(hr) || !swap_ || !device_ || !ctx_) {
    shutdown();
    return false;
  }

  if (!create_rtv()) {
    shutdown();
    return false;
  }

  if (!create_shaders()) {
    shutdown();
    return false;
  }

  RECT rect;
  GetClientRect(hwnd, &rect);
  client_width_ = rect.right - rect.left;
  client_height_ = rect.bottom - rect.top;

  return true;
}

void D3d11Presenter::shutdown() {
  release_shaders();
  release_rtv();
  com_release(swap_);
  com_release(ctx_);
  com_release(device_);
  hwnd_ = nullptr;
}

void D3d11Presenter::release_rtv() { com_release(rtv_); }

bool D3d11Presenter::create_rtv() {
  release_rtv();
  if (!swap_ || !device_) {
    return false;
  }

  ID3D11Texture2D* backbuffer = nullptr;
  HRESULT hr = swap_->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
  if (FAILED(hr) || !backbuffer) {
    return false;
  }

  hr = device_->CreateRenderTargetView(backbuffer, nullptr, &rtv_);
  backbuffer->Release();

  return SUCCEEDED(hr) && rtv_;
}

void D3d11Presenter::resize(std::uint32_t client_width, std::uint32_t client_height) {
  if (!swap_ || !ctx_) {
    return;
  }

  if (client_width == 0 || client_height == 0) {
    return;
  }

  release_rtv();
  ctx_->ClearState();
  ctx_->Flush();

  HRESULT hr = swap_->ResizeBuffers(0, client_width, client_height, DXGI_FORMAT_UNKNOWN, 0);
  if (FAILED(hr)) {
    shutdown();
    return;
  }

  if (!create_rtv()) {
    shutdown();
  }
}

void D3d11Presenter::present_clear(float r, float g, float b, float a) {
  if (!ctx_ || !swap_ || !rtv_) {
    return;
  }

  ID3D11RenderTargetView* views[] = {rtv_};
  ctx_->OMSetRenderTargets(1, views, nullptr);

  const float color[4] = {r, g, b, a};
  ctx_->ClearRenderTargetView(rtv_, color);

  UINT sync = 1; // DXGI_PRESENT_SYNC_INTERVAL_ONE vs tearing; v1 keeps vsync on
  UINT flags = 0;
  swap_->Present(sync, flags);
}

bool D3d11Presenter::create_shaders() {
  release_shaders();

  ID3DBlob* vs_blob = nullptr;
  ID3DBlob* ps_blob = nullptr;

  if (FAILED(D3DCompile(kShaderSrc, strlen(kShaderSrc), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vs_blob, nullptr))) return false;
  if (FAILED(D3DCompile(kShaderSrc, strlen(kShaderSrc), nullptr, nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &ps_blob, nullptr))) {
      vs_blob->Release();
      return false;
  }

  device_->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vs_);
  device_->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &ps_);

  vs_blob->Release();
  ps_blob->Release();

  D3D11_SAMPLER_DESC samp_desc{};
  samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  device_->CreateSamplerState(&samp_desc, &sampler_);

  return vs_ && ps_ && sampler_;
}

void D3d11Presenter::release_shaders() {
  com_release(video_srv_);
  com_release(video_tex_);
  com_release(sampler_);
  com_release(ps_);
  com_release(vs_);
  tex_width_ = 0;
  tex_height_ = 0;
}

bool D3d11Presenter::create_texture(int width, int height) {
  com_release(video_srv_);
  com_release(video_tex_);

  D3D11_TEXTURE2D_DESC desc{};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  if (FAILED(device_->CreateTexture2D(&desc, nullptr, &video_tex_))) return false;
  if (FAILED(device_->CreateShaderResourceView(video_tex_, nullptr, &video_srv_))) return false;

  tex_width_ = width;
  tex_height_ = height;
  return true;
}

void D3d11Presenter::render_frame(MediaFrame const& frame) {
  if (!ready() || !frame.data) return;

  if (tex_width_ != frame.width || tex_height_ != frame.height) {
      if (!create_texture(frame.width, frame.height)) return;
  }

  D3D11_MAPPED_SUBRESOURCE mapped;
  if (SUCCEEDED(ctx_->Map(video_tex_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      for (int y = 0; y < frame.height; ++y) {
          memcpy(static_cast<uint8_t*>(mapped.pData) + y * mapped.RowPitch,
                 frame.data + y * frame.pitch,
                 frame.width * 4);
      }
      ctx_->Unmap(video_tex_, 0);
  }

  ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
  D3D11_VIEWPORT vp = { 0.0f, 0.0f, static_cast<float>(client_width_), static_cast<float>(client_height_), 0.0f, 1.0f };
  ctx_->RSSetViewports(1, &vp);

  ctx_->VSSetShader(vs_, nullptr, 0);
  ctx_->PSSetShader(ps_, nullptr, 0);
  ctx_->PSSetShaderResources(0, 1, &video_srv_);
  ctx_->PSSetSamplers(0, 1, &sampler_);
  ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx_->Draw(3, 0); // Fullscreen triangle

  swap_->Present(1, 0);
}

} // namespace media::win
