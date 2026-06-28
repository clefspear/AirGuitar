#include "TFLiteRuntime.h"

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/c/c_api_types.h"

#include <fstream>
#include <cstring>

namespace AirGuitar {

struct TFLiteRuntime::Impl
{
    std::unique_ptr<tflite::FlatBufferModel> model;
    std::unique_ptr<tflite::Interpreter> interpreter;
    std::string modelPath;
    bool loaded = false;
};

TFLiteRuntime::TFLiteRuntime() noexcept = default;
TFLiteRuntime::TFLiteRuntime(TFLiteRuntime&&) noexcept = default;
TFLiteRuntime& TFLiteRuntime::operator=(TFLiteRuntime&&) noexcept = default;

TFLiteRuntime::~TFLiteRuntime()
{
    unload();
}

TFLiteRuntime::Error TFLiteRuntime::load(const std::string& modelPath,
                                          int numThreads)
{
    impl = std::make_unique<Impl>();
    impl->modelPath = modelPath;

    std::ifstream f(modelPath);
    if (!f.good())
    {
        lastError = "Model file not found: " + modelPath;
        return Error::ModelNotFound;
    }
    f.close();

    impl->model = tflite::FlatBufferModel::BuildFromFile(modelPath.c_str());
    if (!impl->model)
    {
        lastError = "Failed to build model from: " + modelPath;
        return Error::ModelLoadFailed;
    }

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*impl->model, resolver);

    if (builder(&impl->interpreter) != kTfLiteOk)
    {
        lastError = "Failed to create interpreter for: " + modelPath;
        return Error::InterpreterInitFailed;
    }

    if (!impl->interpreter)
    {
        lastError = "Interpreter creation returned null";
        return Error::InterpreterInitFailed;
    }

    if (numThreads > 1)
        impl->interpreter->SetNumThreads(numThreads);

    if (impl->interpreter->AllocateTensors() != kTfLiteOk)
    {
        lastError = "Failed to allocate tensors";
        return Error::InputSetupFailed;
    }

    impl->loaded = true;

    auto inputDims = getInputDims();
    auto outputDims = getOutputDims();

    return Error::None;
}

bool TFLiteRuntime::isLoaded() const
{
    return impl && impl->loaded;
}

void TFLiteRuntime::unload()
{
    impl.reset();
}

TFLiteRuntime::Error TFLiteRuntime::resizeInput(int index,
                                                 const std::vector<int>& dims)
{
    if (!isLoaded())
        return Error::InputSetupFailed;

    auto status = impl->interpreter->ResizeInputTensor(index, dims);
    if (status != kTfLiteOk)
        return Error::InputSetupFailed;

    if (impl->interpreter->AllocateTensors() != kTfLiteOk)
        return Error::InputSetupFailed;

    return Error::None;
}

TFLiteRuntime::Error TFLiteRuntime::runInference()
{
    if (!isLoaded())
        return Error::InvokeFailed;

    auto status = impl->interpreter->Invoke();
    if (status != kTfLiteOk)
    {
        lastError = "Inference failed with status: " + std::to_string(status);
        return Error::InvokeFailed;
    }

    return Error::None;
}

float* TFLiteRuntime::getInputFloatPtr(int index)
{
    if (!isLoaded())
        return nullptr;

    auto* tensor = impl->interpreter->input_tensor(index);
    if (tensor == nullptr || tensor->type != kTfLiteFloat32)
        return nullptr;

    return impl->interpreter->typed_input_tensor<float>(index);
}

uint8_t* TFLiteRuntime::getInputUint8Ptr(int index)
{
    if (!isLoaded())
        return nullptr;

    auto* tensor = impl->interpreter->input_tensor(index);
    if (tensor == nullptr || tensor->type != kTfLiteUInt8)
        return nullptr;

    return impl->interpreter->typed_input_tensor<uint8_t>(index);
}

float* TFLiteRuntime::getOutputFloatPtr(int index)
{
    if (!isLoaded())
        return nullptr;

    auto* tensor = impl->interpreter->output_tensor(index);
    if (tensor == nullptr || tensor->type != kTfLiteFloat32)
        return nullptr;

    return impl->interpreter->typed_output_tensor<float>(index);
}

uint8_t* TFLiteRuntime::getOutputUint8Ptr(int index)
{
    if (!isLoaded())
        return nullptr;

    auto* tensor = impl->interpreter->output_tensor(index);
    if (tensor == nullptr || tensor->type != kTfLiteUInt8)
        return nullptr;

    return impl->interpreter->typed_output_tensor<uint8_t>(index);
}

std::vector<int> TFLiteRuntime::getInputDims(int index) const
{
    if (!isLoaded())
        return {};

    const auto& dims = impl->interpreter->input_tensor(index)->dims;
    if (dims == nullptr)
        return {};

    return std::vector<int>(dims->data, dims->data + dims->size);
}

std::vector<int> TFLiteRuntime::getOutputDims(int index) const
{
    if (!isLoaded())
        return {};

    const auto& dims = impl->interpreter->output_tensor(index)->dims;
    if (dims == nullptr)
        return {};

    return std::vector<int>(dims->data, dims->data + dims->size);
}

int TFLiteRuntime::getInputCount() const
{
    if (!isLoaded())
        return 0;
    return impl->interpreter->inputs().size();
}

int TFLiteRuntime::getOutputCount() const
{
    if (!isLoaded())
        return 0;
    return impl->interpreter->outputs().size();
}

TFLiteRuntime::Error TFLiteRuntime::getInputTensorInfo(
    int index, std::string& name, std::vector<int>& dims, bool& isFloat) const
{
    if (!isLoaded() || index >= getInputCount())
        return Error::MismatchedDimensions;

    auto* tensor = impl->interpreter->input_tensor(index);
    if (tensor == nullptr)
        return Error::OutputReadFailed;

    name = tensor->name ? std::string(tensor->name) : "";
    isFloat = tensor->type == kTfLiteFloat32;
    dims = getInputDims(index);
    return Error::None;
}

TFLiteRuntime::Error TFLiteRuntime::getOutputTensorInfo(
    int index, std::string& name, std::vector<int>& dims, bool& isFloat) const
{
    if (!isLoaded() || index >= getOutputCount())
        return Error::MismatchedDimensions;

    auto* tensor = impl->interpreter->output_tensor(index);
    if (tensor == nullptr)
        return Error::OutputReadFailed;

    name = tensor->name ? std::string(tensor->name) : "";
    isFloat = tensor->type == kTfLiteFloat32;
    dims = getOutputDims(index);
    return Error::None;
}

std::string TFLiteRuntime::errorToString(Error err)
{
    switch (err)
    {
        case Error::None: return "Success";
        case Error::ModelNotFound: return "Model file not found";
        case Error::ModelLoadFailed: return "Failed to load model";
        case Error::InterpreterInitFailed: return "Interpreter initialization failed";
        case Error::InputSetupFailed: return "Input setup failed";
        case Error::InvokeFailed: return "Inference invocation failed";
        case Error::OutputReadFailed: return "Failed to read output";
        case Error::MismatchedDimensions: return "Mismatched tensor dimensions";
        default: return "Unknown error";
    }
}

} // namespace AirGuitar
