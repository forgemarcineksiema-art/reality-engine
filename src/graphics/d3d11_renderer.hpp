#pragma once

#include "graphics/lab_scene.hpp"

#include <d3d11.h>
#include <d3d11sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <filesystem>
#include <ostream>
#include <string>

namespace reality {

class D3D11Renderer final {
  public:
    D3D11Renderer(HWND window, std::ostream& log, int adapter_index, bool debug);
    ~D3D11Renderer();
    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;
    void resize(unsigned width, unsigned height);
    void draw(const std::vector<SceneTriangle>& scene);
    [[nodiscard]] bool present();
    [[nodiscard]] std::size_t capture(const std::filesystem::path& path);
    [[nodiscard]] const std::wstring& adapter_name() const noexcept;
    [[nodiscard]] unsigned width() const noexcept;
    [[nodiscard]] unsigned height() const noexcept;
    void check_diagnostics();

  private:
    template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    std::ostream& log_;
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain1> swap_chain_;
    ComPtr<ID3D11RenderTargetView> target_;
    ComPtr<ID3D11DepthStencilView> depth_;
    ComPtr<ID3D11VertexShader> vertex_shader_;
    ComPtr<ID3D11PixelShader> pixel_shader_;
    ComPtr<ID3D11InputLayout> layout_;
    ComPtr<ID3D11Buffer> vertices_;
    ComPtr<ID3D11RasterizerState> rasterizer_;
    ComPtr<ID3D11InfoQueue> diagnostics_;
    std::wstring adapter_name_;
    unsigned width_ = 0;
    unsigned height_ = 0;
};

} // namespace reality
