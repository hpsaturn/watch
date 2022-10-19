#ifndef __LUNOKIOT__PROVISIONING_APP__
#define __LUNOKIOT__PROVISIONING_APP__

#include "../system/Application.hpp"
#include "../UI/widgets/ButtonWidget.hpp"
#include "../UI/widgets/ButtonImageXBMWidget.hpp"

#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_SOFTAP   "softap"
#define PROV_TRANSPORT_BLE      "ble"

class ProvisioningApplication: public LunokIoTApplication {
    private:
        unsigned long nextRedraw;
        ButtonImageXBMWidget * clearProvBtn=nullptr;
        ButtonImageXBMWidget * backBtn=nullptr;
        ButtonImageXBMWidget * startProvBtn=nullptr;
        //@TODO button for stop provisioning
        //wifi_prov_mgr_stop_provisioning
    public:
        ProvisioningApplication();
        ~ProvisioningApplication();
        bool Tick();
};

#endif