// Scale and letterbox the WGC surface on the GPU before the staging read.
class VideoScaler {
    ComPtr<ID3D11VideoDevice> video;
    ComPtr<ID3D11VideoContext> context;
    ComPtr<ID3D11VideoProcessorEnumerator> enumerator;
    ComPtr<ID3D11VideoProcessor> processor;
    ComPtr<ID3D11VideoProcessorOutputView> view;
    ComPtr<ID3D11Texture2D> output;

  public:
    VideoScaler(ID3D11Device* device, ID3D11DeviceContext* immediate, UINT width, UINT height, UINT outWidth,
                UINT outHeight, UINT fps) {
        check(device->QueryInterface(IID_PPV_ARGS(video.GetAddressOf())));
        check(immediate->QueryInterface(IID_PPV_ARGS(context.GetAddressOf())));
        D3D11_VIDEO_PROCESSOR_CONTENT_DESC desc{};
        desc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
        desc.InputFrameRate = desc.OutputFrameRate = {fps, 1};
        desc.InputWidth = width;
        desc.InputHeight = height;
        desc.OutputWidth = outWidth;
        desc.OutputHeight = outHeight;
        desc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;
        check(video->CreateVideoProcessorEnumerator(&desc, enumerator.GetAddressOf()));
        check(video->CreateVideoProcessor(enumerator.Get(), 0, processor.GetAddressOf()));
        D3D11_TEXTURE2D_DESC tex{};
        tex.Width = outWidth;
        tex.Height = outHeight;
        tex.MipLevels = tex.ArraySize = 1;
        tex.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        tex.SampleDesc.Count = 1;
        tex.BindFlags = D3D11_BIND_RENDER_TARGET;
        check(device->CreateTexture2D(&tex, nullptr, output.GetAddressOf()));
        D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC dest{};
        dest.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
        check(video->CreateVideoProcessorOutputView(output.Get(), enumerator.Get(), &dest,
                                                    view.GetAddressOf()));
        const double ratio = (std::min)(double(outWidth) / width, double(outHeight) / height);
        LONG w = LONG(width * ratio + .5), h = LONG(height * ratio + .5);
        RECT rect{(LONG(outWidth) - w) / 2, (LONG(outHeight) - h) / 2, 0, 0};
        rect.right = rect.left + w;
        rect.bottom = rect.top + h;
        RECT sourceRect{0, 0, LONG(width), LONG(height)};
        context->VideoProcessorSetStreamSourceRect(processor.Get(), 0, TRUE, &sourceRect);
        context->VideoProcessorSetStreamFrameFormat(processor.Get(), 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);
        context->VideoProcessorSetStreamAutoProcessingMode(processor.Get(), 0, FALSE);
        context->VideoProcessorSetStreamDestRect(processor.Get(), 0, TRUE, &rect);
        D3D11_VIDEO_COLOR black{};
        black.RGBA.A = 1;
        context->VideoProcessorSetOutputBackgroundColor(processor.Get(), FALSE, &black);
    }
    ID3D11Texture2D* scale(ID3D11Texture2D* frame) {
        D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC desc{};
        desc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
        ComPtr<ID3D11VideoProcessorInputView> input;
        check(video->CreateVideoProcessorInputView(frame, enumerator.Get(), &desc, input.GetAddressOf()));
        D3D11_VIDEO_PROCESSOR_STREAM stream{};
        stream.Enable = TRUE;
        stream.pInputSurface = input.Get();
        check(context->VideoProcessorBlt(processor.Get(), view.Get(), 0, 1, &stream));
        return output.Get();
    }
};
