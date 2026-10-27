#include "graphics/d3d11_renderer.hpp"

#include <d3dcompiler.h>
#include <wincodec.h>

#include <array>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace reality {
namespace {

void checked(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        std::ostringstream message;
        message << operation << " HRESULT=0x" << std::hex << static_cast<unsigned long>(result);
        throw std::runtime_error(message.str());
    }
}

std::string utf8(const std::wstring& text) {
    const auto size =
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) {
        throw std::runtime_error("adapter name conversion failed");
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), size, nullptr, nullptr)) {
        throw std::runtime_error("adapter name conversion failed");
    }
    result.pop_back();
    return result;
}

constexpr char shader[] = R"(
struct Vertex { float3 position : POSITION; float3 color : COLOR; };
struct Pixel { float4 position : SV_POSITION; float3 color : COLOR; };
Pixel vs_main(Vertex input) {
    Pixel output; output.position=float4(input.position,1); output.color=input.color; return output;
}
float4 ps_main(Pixel input) : SV_TARGET { return float4(input.color,1); }
)";
constexpr std::array<float, 4> background{0.055F, 0.075F, 0.10F, 1.0F};

} // namespace

D3D11Renderer::D3D11Renderer(HWND window, std::ostream& log, int adapter_index, bool debug)
    : log_(log) {
    ComPtr<IDXGIFactory6> factory;
    checked(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())), "CreateDXGIFactory1");
    ComPtr<IDXGIAdapter1> selected;
    for (unsigned index = 0;; ++index) {
        ComPtr<IDXGIAdapter1> candidate;
        const auto result = factory->EnumAdapterByGpuPreference(
            index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(candidate.GetAddressOf()));
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        checked(result, "EnumAdapterByGpuPreference");
        DXGI_ADAPTER_DESC1 description{};
        checked(candidate->GetDesc1(&description), "GetDesc1");
        log_ << "adapter_index=" << index << " name=" << utf8(description.Description)
             << " vendor=" << description.VendorId << " device=" << description.DeviceId
             << " software=" << ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) << '\n';
        const bool requested =
            adapter_index < 0 ? !selected : index == static_cast<unsigned>(adapter_index);
        if (requested && !(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            selected = candidate;
        }
    }
    if (!selected) {
        throw std::runtime_error("requested hardware adapter is unavailable; no software fallback");
    }
    DXGI_ADAPTER_DESC1 description{};
    checked(selected->GetDesc1(&description), "selected GetDesc1");
    adapter_name_ = description.Description;
    const D3D_FEATURE_LEVEL requested = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL actual{};
    checked(D3D11CreateDevice(selected.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                              debug ? D3D11_CREATE_DEVICE_DEBUG : 0, &requested, 1,
                              D3D11_SDK_VERSION, device_.GetAddressOf(), &actual,
                              context_.GetAddressOf()),
            "D3D11CreateDevice");
    log_ << "selected_adapter=" << utf8(adapter_name_) << " feature_level=0x" << std::hex << actual
         << std::dec << " hardware=1 gpu_debug=" << debug << '\n';
    log_.flush();
    if (debug) {
        checked(device_.As(&diagnostics_), "ID3D11InfoQueue");
    }
    DXGI_SWAP_CHAIN_DESC1 swap{};
    swap.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap.SampleDesc.Count = 1;
    swap.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap.BufferCount = 2;
    swap.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    checked(factory->CreateSwapChainForHwnd(device_.Get(), window, &swap, nullptr, nullptr,
                                            swap_chain_.GetAddressOf()),
            "CreateSwapChainForHwnd");
    checked(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");

    ComPtr<ID3DBlob> vertex_code;
    ComPtr<ID3DBlob> pixel_code;
    ComPtr<ID3DBlob> errors;
    auto compile = [&](const char* entry, const char* profile, ComPtr<ID3DBlob>& code) {
        const auto result =
            D3DCompile(shader, sizeof(shader) - 1, "lab_shader", nullptr, nullptr, entry, profile,
                       D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
                       code.GetAddressOf(), errors.ReleaseAndGetAddressOf());
        if (errors) {
            log_ << static_cast<const char*>(errors->GetBufferPointer()) << '\n';
        }
        checked(result, "D3DCompile");
    };
    compile("vs_main", "vs_5_0", vertex_code);
    compile("ps_main", "ps_5_0", pixel_code);
    checked(device_->CreateVertexShader(vertex_code->GetBufferPointer(),
                                        vertex_code->GetBufferSize(), nullptr,
                                        vertex_shader_.GetAddressOf()),
            "CreateVertexShader");
    checked(device_->CreatePixelShader(pixel_code->GetBufferPointer(), pixel_code->GetBufferSize(),
                                       nullptr, pixel_shader_.GetAddressOf()),
            "CreatePixelShader");
    const std::array<D3D11_INPUT_ELEMENT_DESC, 2> elements{
        {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
         {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}}};
    static_assert(sizeof(SceneVertex) == 24);
    checked(device_->CreateInputLayout(elements.data(), static_cast<UINT>(elements.size()),
                                       vertex_code->GetBufferPointer(),
                                       vertex_code->GetBufferSize(), layout_.GetAddressOf()),
            "CreateInputLayout");
    D3D11_BUFFER_DESC buffer{};
    buffer.ByteWidth = laboratory_vertex_capacity * sizeof(SceneVertex);
    buffer.Usage = D3D11_USAGE_DYNAMIC;
    buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    checked(device_->CreateBuffer(&buffer, nullptr, vertices_.GetAddressOf()), "CreateBuffer");
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    checked(device_->CreateRasterizerState(&rasterizer, rasterizer_.GetAddressOf()),
            "CreateRasterizerState");
    RECT area{};
    if (!GetClientRect(window, &area)) {
        throw std::runtime_error("GetClientRect failed");
    }
    resize(static_cast<unsigned>(area.right), static_cast<unsigned>(area.bottom));
}

D3D11Renderer::~D3D11Renderer() {
    if (context_) {
        context_->ClearState();
        context_->Flush();
    }
}

void D3D11Renderer::resize(unsigned width, unsigned height) {
    if (width == 0 || height == 0 || (width == width_ && height == height_)) {
        return;
    }
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    target_.Reset();
    depth_.Reset();
    checked(swap_chain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers");
    ComPtr<ID3D11Texture2D> back_buffer;
    checked(swap_chain_->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf())), "GetBuffer");
    checked(device_->CreateRenderTargetView(back_buffer.Get(), nullptr, target_.GetAddressOf()),
            "CreateRenderTargetView");
    D3D11_TEXTURE2D_DESC descriptor{};
    descriptor.Width = width;
    descriptor.Height = height;
    descriptor.MipLevels = 1;
    descriptor.ArraySize = 1;
    descriptor.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descriptor.SampleDesc.Count = 1;
    descriptor.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ComPtr<ID3D11Texture2D> texture;
    checked(device_->CreateTexture2D(&descriptor, nullptr, texture.GetAddressOf()),
            "CreateTexture2D depth");
    checked(device_->CreateDepthStencilView(texture.Get(), nullptr, depth_.GetAddressOf()),
            "CreateDepthStencilView");
    width_ = width;
    height_ = height;
    log_ << "resize=" << width << 'x' << height << '\n';
    check_diagnostics();
}

void D3D11Renderer::draw(const std::vector<SceneTriangle>& scene) {
    if (scene.size() > laboratory_vertex_capacity / 3) {
        throw std::length_error("laboratory vertex capacity exceeded");
    }
    D3D11_MAPPED_SUBRESOURCE mapped{};
    checked(context_->Map(vertices_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map vertices");
    auto* destination = static_cast<SceneVertex*>(mapped.pData);
    for (const auto& triangle : scene) {
        std::memcpy(destination, triangle.vertices.data(), sizeof(SceneVertex) * 3);
        destination += 3;
    }
    context_->Unmap(vertices_.Get(), 0);
    const UINT stride = sizeof(SceneVertex), offset = 0;
    const D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width_), static_cast<float>(height_),
                                  0, 1};
    context_->RSSetViewports(1, &viewport);
    context_->RSSetState(rasterizer_.Get());
    context_->OMSetRenderTargets(1, target_.GetAddressOf(), depth_.Get());
    context_->ClearRenderTargetView(target_.Get(), background.data());
    context_->ClearDepthStencilView(depth_.Get(), D3D11_CLEAR_DEPTH, 1.0F, 0);
    context_->IASetInputLayout(layout_.Get());
    context_->IASetVertexBuffers(0, 1, vertices_.GetAddressOf(), &stride, &offset);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertex_shader_.Get(), nullptr, 0);
    context_->PSSetShader(pixel_shader_.Get(), nullptr, 0);
    context_->Draw(static_cast<UINT>(scene.size() * 3), 0);
    check_diagnostics();
}

bool D3D11Renderer::present() {
    const auto result = swap_chain_->Present(1, 0);
    checked(result, "Present");
    return result != DXGI_STATUS_OCCLUDED;
}
const std::wstring& D3D11Renderer::adapter_name() const noexcept { return adapter_name_; }
unsigned D3D11Renderer::width() const noexcept { return width_; }
unsigned D3D11Renderer::height() const noexcept { return height_; }

void D3D11Renderer::check_diagnostics() {
    if (!diagnostics_) {
        return;
    }
    const auto count = diagnostics_->GetNumStoredMessagesAllowedByRetrievalFilter();
    bool failed = false;
    for (std::uint64_t index = 0; index < count; ++index) {
        SIZE_T size = 0;
        checked(diagnostics_->GetMessage(index, nullptr, &size), "GetMessage size");
        std::vector<std::byte> bytes(size);
        auto* message = reinterpret_cast<D3D11_MESSAGE*>(bytes.data());
        checked(diagnostics_->GetMessage(index, message, &size), "GetMessage");
        if (message->Severity <= D3D11_MESSAGE_SEVERITY_WARNING) {
            log_ << "gpu_diagnostic=" << message->pDescription << '\n';
            failed = true;
        }
    }
    diagnostics_->ClearStoredMessages();
    if (failed) {
        throw std::runtime_error("D3D11 debug layer reported a warning or error; see log");
    }
}

std::size_t D3D11Renderer::capture(const std::filesystem::path& path) {
    ComPtr<ID3D11Texture2D> source;
    checked(swap_chain_->GetBuffer(0, IID_PPV_ARGS(source.GetAddressOf())), "capture GetBuffer");
    D3D11_TEXTURE2D_DESC descriptor{};
    source->GetDesc(&descriptor);
    descriptor.Usage = D3D11_USAGE_STAGING;
    descriptor.BindFlags = 0;
    descriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    checked(device_->CreateTexture2D(&descriptor, nullptr, staging.GetAddressOf()),
            "capture staging");
    context_->CopyResource(staging.Get(), source.Get());
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width_) * height_ * 4);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    checked(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "capture Map");
    for (unsigned row = 0; row < height_; ++row) {
        std::memcpy(pixels.data() + static_cast<std::size_t>(row) * width_ * 4,
                    static_cast<const std::byte*>(mapped.pData) +
                        static_cast<std::size_t>(row) * mapped.RowPitch,
                    static_cast<std::size_t>(width_) * 4);
    }
    context_->Unmap(staging.Get(), 0);
    std::size_t colored = 0;
    for (std::size_t pixel = 0; pixel < pixels.size(); pixel += 4) {
        if (pixels[pixel] > 30 || pixels[pixel + 1] > 35 || pixels[pixel + 2] > 40) {
            ++colored;
        }
        std::swap(pixels[pixel], pixels[pixel + 2]);
    }
    ComPtr<IWICImagingFactory> factory;
    checked(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                             IID_PPV_ARGS(factory.GetAddressOf())),
            "WIC factory");
    ComPtr<IWICStream> stream;
    checked(factory->CreateStream(stream.GetAddressOf()), "WIC stream");
    checked(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE), "WIC file");
    ComPtr<IWICBitmapEncoder> encoder;
    checked(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf()),
            "PNG encoder");
    checked(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache), "PNG initialize");
    ComPtr<IWICBitmapFrameEncode> frame;
    checked(encoder->CreateNewFrame(frame.GetAddressOf(), nullptr), "PNG frame");
    checked(frame->Initialize(nullptr), "PNG frame initialize");
    checked(frame->SetSize(width_, height_), "PNG size");
    auto format = GUID_WICPixelFormat32bppBGRA;
    checked(frame->SetPixelFormat(&format), "PNG format");
    if (format != GUID_WICPixelFormat32bppBGRA) {
        throw std::runtime_error("PNG pixel format mismatch");
    }
    checked(
        frame->WritePixels(height_, width_ * 4, static_cast<UINT>(pixels.size()), pixels.data()),
        "PNG pixels");
    checked(frame->Commit(), "PNG frame commit");
    checked(encoder->Commit(), "PNG encoder commit");
    log_ << "capture=" << utf8(path.filename().wstring()) << " colored_pixels=" << colored << '\n';
    return colored;
}

} // namespace reality
