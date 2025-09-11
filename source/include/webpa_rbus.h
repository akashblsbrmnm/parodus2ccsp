#ifndef _WEBPA_RBUS_H_
#define _WEBPA_RBUS_H_

#include <stdio.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>

#include "webpa_adapter.h"
#include <wdmp-c.h>
#include <cimplog.h>

#define MAX_PARAM_LEN 256

#define WEBPA_NOTIFY_PARAM "Device.Webpa.NotifyParameters"
#define WEBPA_SUBSCRIBE_LIST "Device.Webpa.SubscribeNotifyList()"

bool isRbusEnabled();
bool isRbusInitialized();
WDMP_STATUS webpaRbusInit(const char *pComponentName);
void webpaRbus_Uninit();
rbusError_t setTraceContext(char* traceContext[]);
rbusError_t getTraceContext(char* traceContext[]);
rbusError_t clearTraceContext();
int regWebPaDataModel();
rbusError_t NotifyParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t SubscribeToNotifyParamHandler(rbusHandle_t handle, const char* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);
void subscribeDynamicNotifyList(char** dynamicParamsList, int count);

#endif
