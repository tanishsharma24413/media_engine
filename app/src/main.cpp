#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "media/player.hpp"
#include "media/win/d3d11_presenter.hpp"

#include <shellapi.h>

#include <cstdio>
#include <memory>
#include <string>

namespace {

constexpr wchar_t kWindowClassName[] = L"libtanish_shell";

std::string utf8_from_wide(std::wstring_view wide) {
  if (wide.empty()) {
    return {};
  }

  int required = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
  if (required <= 0) {
    return {};
  }

  std::string out(static_cast<std::size_t>(required), '\0');
  WideCharToMultiByte(
      CP_UTF8,
      0,
      wide.data(),
      static_cast<int>(wide.size()),
      out.data(),
      required,
      nullptr,
      nullptr);
  return out;
}

struct AppState {
  media::Player player{};
  media::win::D3d11Presenter presenter{};
};

AppState* app_from_hwnd(HWND hwnd) { return reinterpret_cast<AppState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)); }

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
  case WM_CREATE: {
    auto* create = reinterpret_cast<CREATESTRUCT*>(lparam);
    auto* app = reinterpret_cast<AppState*>(create->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));

    if (!app->presenter.initialize(hwnd)) {
      return -1;
    }

    app->player.set_on_state([](media::PlayerState s) {
      wchar_t buf[128]{};
      swprintf_s(buf, L"[media] state=%u\n", static_cast<unsigned>(s));
      OutputDebugStringW(buf);
    });

    app->player.set_on_error([](std::string const& e) {
      std::wstring wide(e.begin(), e.end());
      OutputDebugStringW(L"[media] error: ");
      OutputDebugStringW(wide.c_str());
      OutputDebugStringW(L"\n");
    });

    return 0;
  }
  case WM_DESTROY: {
    if (auto* app = app_from_hwnd(hwnd)) {
      app->presenter.shutdown();
    }
    PostQuitMessage(0);
    return 0;
  }
  case WM_SIZE: {
    if (auto* app = app_from_hwnd(hwnd)) {
      UINT w = LOWORD(lparam);
      UINT h = HIWORD(lparam);
      app->presenter.resize(w, h);
    }
    return 0;
  }
  case WM_KEYDOWN: {
    if (auto* app = app_from_hwnd(hwnd)) {
      if (wparam == VK_SPACE) {
        if (app->player.state() == media::PlayerState::Playing) {
          app->player.pause();
        } else {
          app->player.play();
        }
      } else if (wparam == VK_ESCAPE) {
        app->player.stop();
      }
    }
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  default:
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

} // namespace

int WINAPI wWinMain(_In_ HINSTANCE hinst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int show) {
  auto app = std::make_unique<AppState>();

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv && argc >= 2) {
    app->player.open(media::MediaUri{utf8_from_wide(argv[1])});
  }
  LocalFree(argv);

  WNDCLASSW wc{};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hinst;
  wc.lpszClassName = kWindowClassName;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassW(&wc);

  HWND hwnd = CreateWindowExW(
      0,
      kWindowClassName,
      L"libtanish (prototype)",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      1280,
      720,
      nullptr,
      nullptr,
      hinst,
      app.get());

  if (!hwnd) {
    return 1;
  }

  ShowWindow(hwnd, show);
  UpdateWindow(hwnd);

  MSG msg{};
  for (;;) {
    BOOL has_msg = PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE);
    if (!has_msg) {
      if (app->presenter.ready()) {
        app->presenter.present_clear(0.05f, 0.06f, 0.08f, 1.0f);
      }
      Sleep(1);
      continue;
    }

    if (msg.message == WM_QUIT) {
      break;
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  app->presenter.shutdown();
  return static_cast<int>(msg.wParam);
}
