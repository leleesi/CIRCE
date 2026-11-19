// Window-scoped Windows Graphics Capture. Timestamps use the same QPC clock as PresentMon.
#include "capture.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include <roapi.h>
#include <winstring.h>
#ifdef __MINGW32__
// MinGW-w64 14 aliases boolean and BYTE. Avoid the unused duplicate boxed specialization.
#define ____FIReference_1_boolean_INTERFACE_DEFINED__
#endif
#include <windows.graphics.capture.h>
#include <windows.graphics.capture.interop.h>
#include <shellapi.h>
#include <fcntl.h>
#include <io.h>
#include <memory>
#include <algorithm>
#include "scale.hpp"

namespace graphics = ABI::Windows::Graphics;
namespace capture = graphics::Capture;
namespace dx = graphics::DirectX;
namespace rt = dx::Direct3D11;
namespace foundation = ABI::Windows::Foundation;

extern "C" HRESULT WINAPI CreateDirect3D11DeviceFromDXGIDevice(IDXGIDevice*, IInspectable**);
// The SDK exposes this interface in windows.graphics.directx.direct3d11.interop.h.
// Its ABI is also used by MinGW, which ships the WinRT types but not this interop header.
struct SurfaceAccess : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID, void**) = 0;
};
constexpr GUID surfaceAccessId{0xa9b3d012, 0x3df2, 0x4ee3, {0xb8, 0xd1, 0x86, 0x95, 0xf4, 0x57, 0xd3, 0xc1}};

template <class T> static ComPtr<T> factory(const wchar_t* name) {
    HSTRING id{};
    check(WindowsCreateString(name, static_cast<UINT32>(wcslen(name)), &id));
    ComPtr<T> result;
    HRESULT hr = RoGetActivationFactory(id, IID_PPV_ARGS(result.GetAddressOf()));
    WindowsDeleteString(id);
    check(hr);
    return result;
}
template <class T> struct Closable {
    ComPtr<T> obj;
    ~Closable() {
        ComPtr<foundation::IClosable> close;
        if (obj && SUCCEEDED(obj.As(&close)))
            close->Close();
    }
};

bool Capture::write(const unsigned char* pixels, unsigned w, unsigned h, double stamp) {
    if (!hasImage) {
        for (unsigned y = 3; y < h && !hasImage; y += 13) {
            for (unsigned x = 3; x < w; x += 13) {
                const auto p = pixels + (size_t(y) * w + x) * 4;
                if ((p[0] | p[1] | p[2]) > 3) {
                    hasImage = true;
                    break;
                }
            }
        }
    }
    if (!journal &&
        (fwrite(&stamp, 8, 1, stdout) != 1 || fwrite(&w, 4, 1, stdout) != 1 || fwrite(&h, 4, 1, stdout) != 1))
        return false;
    if (fwrite(pixels, 4, size_t(w) * h, stdout) != size_t(w) * h || fflush(stdout))
        return false;
    if (journal && fwrite(&stamp, sizeof(stamp), 1, journal) != 1)
        return false;
    ++frames;
    return true;
}

static int captureWindow(Capture& target) {
    auto items = factory<IGraphicsCaptureItemInterop>(L"Windows.Graphics.Capture.GraphicsCaptureItem");
    ComPtr<capture::IGraphicsCaptureItem> item;
    check(items->CreateForWindow(target.window, IID_PPV_ARGS(item.GetAddressOf())));
    graphics::SizeInt32 size{};
    check(item->get_Size(&size));
    if (size.Width <= 0 || size.Height <= 0)
        return 3;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                            D3D11_CREATE_DEVICE_BGRA_SUPPORT |
                                (target.journal ? D3D11_CREATE_DEVICE_VIDEO_SUPPORT : 0),
                            nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context));
    ComPtr<IDXGIDevice> dxgi;
    ComPtr<IInspectable> inspect;
    ComPtr<rt::IDirect3DDevice> direct;
    check(device.As(&dxgi));
    check(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(), &inspect));
    check(inspect.As(&direct));
    auto pools = factory<capture::IDirect3D11CaptureFramePoolStatics2>(
        L"Windows.Graphics.Capture.Direct3D11CaptureFramePool");
    constexpr auto format = dx::DirectXPixelFormat_B8G8R8A8UIntNormalized;
    Closable<capture::IDirect3D11CaptureFramePool> pool;
    check(pools->CreateFreeThreaded(direct.Get(), format, 2, size, &pool.obj));
    Closable<capture::IGraphicsCaptureSession> session;
    check(pool.obj->CreateCaptureSession(item.Get(), &session.obj));
    ComPtr<capture::IGraphicsCaptureSession2> cursor;
    if (SUCCEEDED(session.obj.As(&cursor)))
        cursor->put_IsCursorCaptureEnabled(FALSE);
    check(session.obj->StartCapture());

    ComPtr<ID3D11Texture2D> staging;
    std::unique_ptr<VideoScaler> scaler;
    std::vector<unsigned char> pixels;
    double first = qpcSeconds(), next = 0;
    unsigned outW = target.width, outH = target.height;
    while (qpcSeconds() < target.end) {
        if (!target.validWindow())
            return 3;
        Closable<capture::IDirect3D11CaptureFrame> frame;
        check(pool.obj->TryGetNextFrame(&frame.obj));
        if (!frame.obj) {
            if (!target.frames && qpcSeconds() - first > 1.0)
                return 6;
            Sleep(3);
            continue;
        }
        foundation::TimeSpan clock{};
        check(frame.obj->get_SystemRelativeTime(&clock));
        const double stamp = clock.Duration / 1e7;
        if (stamp < first || stamp + .0005 < next || stamp >= target.end)
            continue;
        graphics::SizeInt32 current{};
        check(frame.obj->get_ContentSize(&current));
        if (current.Width <= 0 || current.Height <= 0)
            return 3;
        if (current.Width != size.Width || current.Height != size.Height) {
            ComPtr<foundation::IClosable> close;
            if (SUCCEEDED(frame.obj.As(&close)))
                close->Close();
            frame.obj.Reset();
            size = current;
            scaler.reset();
            staging.Reset();
            check(pool.obj->Recreate(direct.Get(), format, 2, size));
            continue;
        }
        ComPtr<rt::IDirect3DSurface> surface;
        ComPtr<SurfaceAccess> access;
        ComPtr<ID3D11Texture2D> source;
        check(frame.obj->get_Surface(&surface));
        check(surface->QueryInterface(surfaceAccessId, reinterpret_cast<void**>(access.GetAddressOf())));
        check(access->GetInterface(IID_PPV_ARGS(source.GetAddressOf())));
        if (target.journal && !scaler)
            scaler = std::make_unique<VideoScaler>(device.Get(), context.Get(), size.Width, size.Height, outW,
                                                   outH, target.rate);
        if (!target.journal) {
            outW = size.Width;
            outH = size.Height;
        }
        ID3D11Texture2D* image = scaler ? scaler->scale(source.Get()) : source.Get();
        if (!staging) {
            D3D11_TEXTURE2D_DESC desc{};
            image->GetDesc(&desc);
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = desc.MiscFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            check(device->CreateTexture2D(&desc, nullptr, &staging));
            pixels.resize(size_t(outW) * outH * 4);
        }
        context->CopyResource(staging.Get(), image);
        D3D11_MAPPED_SUBRESOURCE map{};
        check(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map));
        for (unsigned y = 0; y < outH; ++y)
            memcpy(pixels.data() + size_t(y) * outW * 4,
                   static_cast<const BYTE*>(map.pData) + size_t(y) * map.RowPitch, outW * 4);
        context->Unmap(staging.Get(), 0);
        if (!target.write(pixels.data(), outW, outH, stamp))
            return 9;
        if (!next || stamp - next > 1. / target.rate)
            next = stamp;
        next += 1. / target.rate;
    }
    return target.frames ? (target.hasImage ? 0 : 8) : 6;
}

static int record(int argc, wchar_t** argv) {
    if (argc != 5 && argc != 8)
        return 2;
    Capture target;
    target.window = reinterpret_cast<HWND>(wcstoull(argv[1], nullptr, 10));
    target.pid = wcstoul(argv[2], nullptr, 10);
    target.rate = wcstoul(argv[3], nullptr, 10);
    target.end = wcstod(argv[4], nullptr);
    if (!target.pid || !target.validWindow())
        return 3;
    if ((target.rate != 30 && target.rate != 60) ||
        !(target.end > qpcSeconds() && target.end < qpcSeconds() + 65))
        return 2;
    if (argc == 8) {
        target.width = wcstoul(argv[5], nullptr, 10);
        target.height = wcstoul(argv[6], nullptr, 10);
        if (!((target.width == 1280 && target.height == 720) ||
              (target.width == 1920 && target.height == 1080)))
            return 2;
        target.journal = _wfopen(argv[7], L"wb");
        if (!target.journal)
            return 4;
    }
    _setmode(_fileno(stdout), _O_BINARY);
    HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
    int result = 5;
    if (SUCCEEDED(hr)) {
        try {
            result = captureWindow(target);
        } catch (HRESULT error) {
            if (error == E_ACCESSDENIED)
                result = 10;
            fprintf(stderr, "Windows Graphics Capture: 0x%08lX\n", (unsigned long)error);
        } catch (const std::exception& ex) {
            fprintf(stderr, "Windows Graphics Capture: %s\n", ex.what());
        }
        RoUninitialize();
    }
    if ((result == 5 || result == 6) && !target.frames && target.journal && qpcSeconds() < target.end) {
        fputs("Windows Graphics Capture -> GDI (HWND)\n", stderr);
        result = captureGdi(target);
    }
    if (target.journal && fclose(target.journal))
        result = 9;
    return result;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    SetProcessDPIAware();
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int result = argv ? record(argc, argv) : 2;
    if (argv)
        LocalFree(argv);
    return result;
}
