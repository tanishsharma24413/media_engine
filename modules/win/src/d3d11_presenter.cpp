#include "media/win/d3d11_presenter.hpp"

#include <d3d11.h>
#include <dxgi.h>

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

  return true;
}

void D3d11Presenter::shutdown() {
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

} // namespace media::win
