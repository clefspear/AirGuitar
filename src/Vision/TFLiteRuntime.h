#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace AirGuitar {

class TFLiteRuntime
{
public:
    enum class Error
    {
        None,
        ModelNotFound,
        ModelLoadFailed,
        InterpreterInitFailed,
        InputSetupFailed,
        InvokeFailed,
        OutputReadFailed,
        MismatchedDimensions,
    };

    TFLiteRuntime() noexcept;
    ~TFLiteRuntime();

    TFLiteRuntime(const TFLiteRuntime&) = delete;
    TFLiteRuntime& operator=(const TFLiteRuntime&) = delete;
    TFLiteRuntime(TFLiteRuntime&&) noexcept;
    TFLiteRuntime& operator=(TFLiteRuntime&&) noexcept;

    Error load(const std::string& modelPath, int numThreads = 2);
    bool isLoaded() const;
    void unload();

    Error resizeInput(int index, const std::vector<int>& dims);
    Error runInference();

    float* getInputFloatPtr(int index = 0);
    uint8_t* getInputUint8Ptr(int index = 0);
    float* getOutputFloatPtr(int index = 0);
    uint8_t* getOutputUint8Ptr(int index = 0);

    std::vector<int> getInputDims(int index = 0) const;
    std::vector<int> getOutputDims(int index = 0) const;
    int getInputCount() const;
    int getOutputCount() const;

    Error getInputTensorInfo(int index, std::string& name,
                             std::vector<int>& dims, bool& isFloat) const;
    Error getOutputTensorInfo(int index, std::string& name,
                              std::vector<int>& dims, bool& isFloat) const;

    std::string getLastError() const { return lastError; }

    static std::string errorToString(Error err);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::string lastError;

    Error fromTfLiteStatus(int status, Error errCode);
};

} // namespace AirGuitar
