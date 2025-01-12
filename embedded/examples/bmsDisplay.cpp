#include <stdio.h>
#include <connStat.h>
#include <pthread.h>
#include "HVTelemetry_Loop.h"
extern "C" {
    #include "data.h"
#include "NCD9830DBR2G.h"
    #include "imu.h"
    #include "can_devices.h"
    #include "bms.h"
    extern pthread_t CANThread;
}

int main() {
    initData();
    initPressureSensors();
    SetupCANDevices();
    SetupHVTelemetry((char *) DASHBOARD_IP, DASHBOARD_PORT);
    pthread_join(CANThread, NULL);
}
