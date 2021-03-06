#pragma once

#include "controllers/controller.h"
#include "controllers/hid/hiddevice.h"
#include "controllers/hid/legacyhidcontrollermapping.h"
#include "util/duration.h"

/// HID controller backend
class HidController final : public Controller {
    Q_OBJECT
  public:
    explicit HidController(
            mixxx::hid::DeviceInfo&& deviceInfo);
    ~HidController() override;

    ControllerJSProxy* jsProxy() override;

    QString mappingExtension() override;

    virtual std::shared_ptr<LegacyControllerMapping> cloneMapping() override;
    void setMapping(std::shared_ptr<LegacyControllerMapping> pMapping) override;

    bool isMappable() const override {
        if (!m_pMapping) {
            return false;
        }
        return m_pMapping->isMappable();
    }

    bool matchMapping(const MappingInfo& mapping) override;

  protected:
    void sendReport(QList<int> data, unsigned int length, unsigned int reportID);

  private slots:
    int open() override;
    int close() override;

    bool poll() override;

  private:
    bool isPolling() const override;

    // For devices which only support a single report, reportID must be set to
    // 0x0.
    void sendBytes(const QByteArray& data) override;
    void sendBytesReport(QByteArray data, unsigned int reportID);
    void sendFeatureReport(const QList<int>& dataList, unsigned int reportID);

    const mixxx::hid::DeviceInfo m_deviceInfo;

    hid_device* m_pHidDevice;
    std::shared_ptr<LegacyHidControllerMapping> m_pMapping;

    static constexpr int kNumBuffers = 2;
    static constexpr int kBufferSize = 255;
    unsigned char m_pPollData[kNumBuffers][kBufferSize];
    int m_iLastPollSize;
    int m_iPollingBufferIndex;

    friend class HidControllerJSProxy;
};

class HidControllerJSProxy : public ControllerJSProxy {
    Q_OBJECT
  public:
    HidControllerJSProxy(HidController* m_pController)
            : ControllerJSProxy(m_pController),
              m_pHidController(m_pController) {
    }

    Q_INVOKABLE void send(const QList<int>& data, unsigned int length = 0) override {
        m_pHidController->send(data, length);
    }

    Q_INVOKABLE void send(const QList<int>& data, unsigned int length, unsigned int reportID) {
        m_pHidController->sendReport(data, length, reportID);
    }

    Q_INVOKABLE void sendFeatureReport(
            const QList<int>& dataList, unsigned int reportID) {
        m_pHidController->sendFeatureReport(dataList, reportID);
    }

  private:
    HidController* m_pHidController;
};
