#pragma once

#include "media/pipeline.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;

namespace media::win {

// Minimal GPU presenter: owns D3D11 swapchain + RTV for an HWND.
class D3d11Presenter final : public IVideoOutput {
public:
  D3d11Presenter();
  ~D3d11Presenter() override;

  D3d11Presenter(D3d11Presenter const&) = delete;
  D3d11Presenter& operator=(D3d11Presenter const&) = delete;

  bool initialize(HWND hwnd);
  void shutdown();

  void resize(std::uint32_t client_width, std::uint32_t client_height);
  void present_clear(float r, float g, float b, float a);

  HWND hwnd() const { return hwnd_; }
  bool ready() const { return swap_ != nullptr && rtv_ != nullptr; }

private:
  void release_rtv();
  bool create_rtv();

  HWND hwnd_{};
  IDXGISwapChain* swap_{nullptr};
  ID3D11Device* device_{nullptr};
  ID3D11DeviceContext* ctx_{nullptr};
  ID3D11RenderTargetView* rtv_{nullptr};
};

} // namespace media::win
