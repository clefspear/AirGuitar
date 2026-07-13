#include "Camera.h"
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <iostream>

namespace AirGuitar {

Camera::~Camera()
{
    stopCapture();
    close();
}

Camera::Error Camera::open(int deviceId, int w, int h, int fps)
{
    width = w;
    height = h;
    targetFps = fps;

    capture.open(deviceId);
    if (!capture.isOpened())
        return Error::NotFound;

    capture.set(cv::CAP_PROP_FRAME_WIDTH, width);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    capture.set(cv::CAP_PROP_FPS, targetFps);

    int actualWidth = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int actualHeight = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    if (actualWidth > 0) width = actualWidth;
    if (actualHeight > 0) height = actualHeight;

    measuredFps = capture.get(cv::CAP_PROP_FPS);

    capture.set(cv::CAP_PROP_BUFFERSIZE, 1);

    return Error::None;
}

void Camera::close()
{
    if (capture.isOpened())
        capture.release();
}

bool Camera::isOpen() const
{
    return capture.isOpened();
}

void Camera::startCapture()
{
    if (capturing.load())
        return;

    if (!capture.isOpened())
        return;

    shouldStop.store(false);
    capturing.store(true);
    captureThread = std::thread(&Camera::captureLoop, this);
}

void Camera::stopCapture()
{
    if (!capturing.load())
        return;

    shouldStop.store(true);
    if (captureThread.joinable())
        captureThread.join();
    capturing.store(false);
}

bool Camera::isCapturing() const
{
    return capturing.load();
}

Camera::Frame Camera::getLatestFrame()
{
    std::lock_guard<std::mutex> lock(frameMutex);
    if (latestFrame.empty())
        return {};

    return Frame{
        .image = latestFrame.clone(),
        .timestampMs = latestTimestampMs
    };
}

double Camera::getFrameRate() const
{
    return measuredFps;
}

int Camera::getWidth() const
{
    return width;
}

int Camera::getHeight() const
{
    return height;
}

void Camera::setFrameCallback(FrameCallback cb)
{
    callback = std::move(cb);
}

void Camera::captureLoop()
{
    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();
    int frameCount = 0;
    bool loggedResolution = false;

    while (!shouldStop.load())
    {
        cv::Mat frame;
        if (!capture.read(frame))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if (frame.empty())
            continue;

        // Log actual capture resolution on first frame
        if (!loggedResolution)
        {
            std::cout << "Camera: actual capture resolution = "
                      << frame.cols << "x" << frame.rows
                      << " (requested: " << width << "x" << height << ")"
                      << " step=" << frame.step << std::endl;
            loggedResolution = true;
        }

        cv::Mat rgb;
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

        auto now = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastTime).count();

        auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        {
            std::lock_guard<std::mutex> lock(frameMutex);
            latestFrame = rgb.clone();
            latestTimestampMs = timestampMs;
        }

        if (callback)
            callback(rgb, timestampMs);

        ++frameCount;
        if (elapsed >= 1000)
        {
            measuredFps = static_cast<double>(frameCount) * 1000.0
                          / static_cast<double>(elapsed);
            frameCount = 0;
            lastTime = now;
        }
    }
}

Camera::Error Camera::fromCvErrorCode(int)
{
    return Error::None;
}

} // namespace AirGuitar
