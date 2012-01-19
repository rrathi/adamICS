#ifndef U300_RIL_INFORMATION_H
#define U300_RIL_INFORMATION_H 1

#include <telephony/ril.h>

void requestGetIMSI(void *data, size_t datalen, RIL_Token t);
void requestDeviceIdentity(void *data, size_t datalen, RIL_Token t);
void requestGetIMEI(void *data, size_t datalen, RIL_Token t);
void requestGetIMEISV(void *data, size_t datalen, RIL_Token t);
void requestBasebandVersion(void *data, size_t datalen, RIL_Token t);

int retryRadioPower(void);
int isRadioOn(void);
void setRadioState(RIL_RadioState newState);
RIL_RadioState getRadioState(void);
void onSIMReady(void *p);

#endif
