#pragma once
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>

namespace AirGuitar {

class Camera
{
public:
    enum class Error
    {
        None,
        NotFound,
        PermissionDenied,
        OpenFailed,
        CaptureFailed,
    };

    static constexpr int kDefaultWidth = 1280;
    static constexpr int kDefaultHeight = 720;
    static constexpr int kDefaultFps = 60;

    Camera() = default;
    ~Camera();

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    Error open(int deviceId = 0,
               int width = kDefaultWidth,
               int height = kDefaultHeight,
               int fps = kDefaultFps);
    void close();
    bool isOpen() const;

    void startCapture();
    void stopCapture();
    bool isCapturing() const;

    struct Frame
    {
        const unsigned char* data = nullptr;
        int cols = 0;
        int rows = 0;
        int step = 0;
        int64_t timestampMs = 0;
    };

    Frame getLatestFrame();
    cv::Mat getLatestFrameMat();
    double getFrameRate() const;
    int getWidth() const;
    int getHeight() const;

    using FrameCallback = std::function<void(const cv::Mat&, int64_t)>;
    void setFrameCallback(FrameCallback cb);

private:
    cv::VideoCapture capture;
    std::thread captureThread;
    std::mutex frameMutex;
    std::atomic<bool> capturing{false};
    std::atomic<bool> shouldStop{false};

    cv::Mat latestFrame;
    int64_t latestTimestampMs = 0;
    int width = kDefaultWidth;
    int height = kDefaultHeight;
    int targetFps = kDefaultFps;
    double measuredFps = 0.0;

    FrameCallback callback;

    void captureLoop();

    static Error fromCvErrorCode(int code);
};

} // namespace AirGuitar
