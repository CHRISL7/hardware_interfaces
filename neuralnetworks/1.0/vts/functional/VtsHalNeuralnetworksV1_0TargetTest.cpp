/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "VtsHalNeuralnetworksV1_0TargetTest.h"
#include "Event.h"
#include "Models.h"
#include "TestHarness.h"

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace vts {
namespace functional {

using ::android::hardware::neuralnetworks::V1_0::implementation::Event;
using ::generated_tests::MixedTypedExampleType;
namespace generated_tests {
extern void Execute(const sp<IDevice>&, std::function<Model(void)>, std::function<bool(int)>,
                    const std::vector<MixedTypedExampleType>&);
}

// A class for test environment setup
NeuralnetworksHidlEnvironment::NeuralnetworksHidlEnvironment() {}

NeuralnetworksHidlEnvironment::~NeuralnetworksHidlEnvironment() {}

NeuralnetworksHidlEnvironment* NeuralnetworksHidlEnvironment::getInstance() {
    // This has to return a "new" object because it is freed inside
    // ::testing::AddGlobalTestEnvironment when the gtest is being torn down
    static NeuralnetworksHidlEnvironment* instance = new NeuralnetworksHidlEnvironment();
    return instance;
}

void NeuralnetworksHidlEnvironment::registerTestServices() {
    registerTestService<IDevice>();
}

// The main test class for NEURALNETWORK HIDL HAL.
NeuralnetworksHidlTest::~NeuralnetworksHidlTest() {}

void NeuralnetworksHidlTest::SetUp() {
    device = ::testing::VtsHalHidlTargetTestBase::getService<IDevice>(
        NeuralnetworksHidlEnvironment::getInstance());
    ASSERT_NE(nullptr, device.get());
}

void NeuralnetworksHidlTest::TearDown() {}

sp<IPreparedModel> NeuralnetworksHidlTest::doPrepareModelShortcut(const Model& model) {
    sp<IPreparedModel> preparedModel;
    ErrorStatus prepareStatus;
    sp<Event> preparationEvent = new Event();
    if (preparationEvent.get() == nullptr) {
        return nullptr;
    }

    Return<void> prepareRet = device->prepareModel(
        model, preparationEvent, [&](ErrorStatus status, const sp<IPreparedModel>& prepared) {
            prepareStatus = status;
            preparedModel = prepared;
        });

    if (!prepareRet.isOk() || prepareStatus != ErrorStatus::NONE ||
        preparedModel.get() == nullptr) {
        return nullptr;
    }
    Event::Status eventStatus = preparationEvent->wait();
    if (eventStatus != Event::Status::SUCCESS) {
        return nullptr;
    }

    return preparedModel;
}

// create device test
TEST_F(NeuralnetworksHidlTest, CreateDevice) {}

// status test
TEST_F(NeuralnetworksHidlTest, StatusTest) {
    Return<DeviceStatus> status = device->getStatus();
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(DeviceStatus::AVAILABLE, static_cast<DeviceStatus>(status));
}

// initialization
TEST_F(NeuralnetworksHidlTest, GetCapabilitiesTest) {
    Return<void> ret =
        device->getCapabilities([](ErrorStatus status, const Capabilities& capabilities) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            EXPECT_NE(nullptr, capabilities.supportedOperationTuples.data());
            EXPECT_NE(0ull, capabilities.supportedOperationTuples.size());
            EXPECT_EQ(0u, static_cast<uint32_t>(capabilities.cachesCompilation) & ~0x1);
            EXPECT_LT(0.0f, capabilities.float32Performance.execTime);
            EXPECT_LT(0.0f, capabilities.float32Performance.powerUsage);
            EXPECT_LT(0.0f, capabilities.quantized8Performance.execTime);
            EXPECT_LT(0.0f, capabilities.quantized8Performance.powerUsage);
        });
    EXPECT_TRUE(ret.isOk());
}

// supported operations positive test
TEST_F(NeuralnetworksHidlTest, SupportedOperationsPositiveTest) {
    Model model = createValidTestModel();
    Return<void> ret = device->getSupportedOperations(
        model, [&](ErrorStatus status, const hidl_vec<bool>& supported) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            EXPECT_EQ(model.operations.size(), supported.size());
        });
    EXPECT_TRUE(ret.isOk());
}

// supported operations negative test 1
TEST_F(NeuralnetworksHidlTest, SupportedOperationsNegativeTest1) {
    Model model = createInvalidTestModel1();
    Return<void> ret = device->getSupportedOperations(
        model, [&](ErrorStatus status, const hidl_vec<bool>& supported) {
            EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            (void)supported;
        });
    EXPECT_TRUE(ret.isOk());
}

// supported operations negative test 2
TEST_F(NeuralnetworksHidlTest, SupportedOperationsNegativeTest2) {
    Model model = createInvalidTestModel2();
    Return<void> ret = device->getSupportedOperations(
        model, [&](ErrorStatus status, const hidl_vec<bool>& supported) {
            EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            (void)supported;
        });
    EXPECT_TRUE(ret.isOk());
}

// prepare simple model positive test
TEST_F(NeuralnetworksHidlTest, SimplePrepareModelPositiveTest) {
    Model model = createValidTestModel();
    sp<Event> preparationEvent = new Event();
    ASSERT_NE(nullptr, preparationEvent.get());
    Return<void> prepareRet = device->prepareModel(
        model, preparationEvent, [&](ErrorStatus status, const sp<IPreparedModel>& prepared) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            (void)prepared;
        });
    ASSERT_TRUE(prepareRet.isOk());
}

// prepare simple model negative test 1
TEST_F(NeuralnetworksHidlTest, SimplePrepareModelNegativeTest1) {
    Model model = createInvalidTestModel1();
    sp<Event> preparationEvent = new Event();
    ASSERT_NE(nullptr, preparationEvent.get());
    Return<void> prepareRet = device->prepareModel(
        model, preparationEvent, [&](ErrorStatus status, const sp<IPreparedModel>& prepared) {
            EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            (void)prepared;
        });
    ASSERT_TRUE(prepareRet.isOk());
}

// prepare simple model negative test 2
TEST_F(NeuralnetworksHidlTest, SimplePrepareModelNegativeTest2) {
    Model model = createInvalidTestModel2();
    sp<Event> preparationEvent = new Event();
    ASSERT_NE(nullptr, preparationEvent.get());
    Return<void> prepareRet = device->prepareModel(
        model, preparationEvent, [&](ErrorStatus status, const sp<IPreparedModel>& prepared) {
            EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            (void)prepared;
        });
    ASSERT_TRUE(prepareRet.isOk());
}

// execute simple graph positive test
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphPositiveTest) {
    Model model = createValidTestModel();
    sp<IPreparedModel> preparedModel = doPrepareModelShortcut(model);
    ASSERT_NE(nullptr, preparedModel.get());
    Request request = createValidTestRequest();

    sp<Event> executionEvent = new Event();
    ASSERT_NE(nullptr, executionEvent.get());
    Return<ErrorStatus> executeStatus = preparedModel->execute(request, executionEvent);
    ASSERT_TRUE(executeStatus.isOk());
    EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executeStatus));
    Event::Status eventStatus = executionEvent->wait();
    EXPECT_EQ(Event::Status::SUCCESS, eventStatus);

    std::vector<float> outputData = {-1.0f, -1.0f, -1.0f, -1.0f};
    std::vector<float> expectedData = {6.0f, 8.0f, 10.0f, 12.0f};
    const uint32_t OUTPUT = 1;

    sp<IMemory> outputMemory = mapMemory(request.pools[OUTPUT]);
    ASSERT_NE(nullptr, outputMemory.get());
    float* outputPtr = reinterpret_cast<float*>(static_cast<void*>(outputMemory->getPointer()));
    ASSERT_NE(nullptr, outputPtr);
    outputMemory->read();
    std::copy(outputPtr, outputPtr + outputData.size(), outputData.begin());
    outputMemory->commit();
    EXPECT_EQ(expectedData, outputData);
}

// execute simple graph negative test 1
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphNegativeTest1) {
    Model model = createValidTestModel();
    sp<IPreparedModel> preparedModel = doPrepareModelShortcut(model);
    ASSERT_NE(nullptr, preparedModel.get());
    Request request = createInvalidTestRequest1();

    sp<Event> executionEvent = new Event();
    ASSERT_NE(nullptr, executionEvent.get());
    Return<ErrorStatus> executeStatus = preparedModel->execute(request, executionEvent);
    ASSERT_TRUE(executeStatus.isOk());
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(executeStatus));
    executionEvent->wait();
}

// execute simple graph negative test 2
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphNegativeTest2) {
    Model model = createValidTestModel();
    sp<IPreparedModel> preparedModel = doPrepareModelShortcut(model);
    ASSERT_NE(nullptr, preparedModel.get());
    Request request = createInvalidTestRequest2();

    sp<Event> executionEvent = new Event();
    ASSERT_NE(nullptr, executionEvent.get());
    Return<ErrorStatus> executeStatus = preparedModel->execute(request, executionEvent);
    ASSERT_TRUE(executeStatus.isOk());
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(executeStatus));
    executionEvent->wait();
}

// Mixed-typed examples
typedef MixedTypedExampleType MixedTypedExample;

// in frameworks/ml/nn/runtime/tests/generated/
#include "all_generated_vts_tests.cpp"

// TODO: Add tests for execution failure, or wait_for/wait_until timeout.
//       Discussion:
//       https://googleplex-android-review.git.corp.google.com/#/c/platform/hardware/interfaces/+/2654636/5/neuralnetworks/1.0/vts/functional/VtsHalNeuralnetworksV1_0TargetTest.cpp@222

}  // namespace functional
}  // namespace vts
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

using android::hardware::neuralnetworks::V1_0::vts::functional::NeuralnetworksHidlEnvironment;

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(NeuralnetworksHidlEnvironment::getInstance());
    ::testing::InitGoogleTest(&argc, argv);
    NeuralnetworksHidlEnvironment::getInstance()->init(&argc, argv);

    int status = RUN_ALL_TESTS();
    return status;
}
