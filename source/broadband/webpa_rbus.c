#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include "webpa_rbus.h"
#include "webpa_notification.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;

rbusDataElement_t dataElements[2] = {
            {WEBPA_NOTIFY_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NotifyParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
            {WEBPA_SUBSCRIBE_LIST, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, SubscribeToNotifyParamHandler}} /* API to subscribe 
                                                                                                            to notify parameters from components */
    };

bool isRbusEnabled()
{
        if(RBUS_ENABLED == rbus_checkStatus())
        {
                isRbus = true;
        }
        else
        {
                isRbus = false;
        }
        WalInfo("Webpa RBUS mode active status = %s\n", isRbus ? "true":"false");
        return isRbus;
}

bool isRbusInitialized()
{
    return rbus_handle != NULL ? true : false;
}

WDMP_STATUS webpaRbusInit(const char *pComponentName)
{
        int ret = RBUS_ERROR_SUCCESS;

        WalInfo("rbus_open for component %s\n", pComponentName);
        ret = rbus_open(&rbus_handle, pComponentName);
        if(ret != RBUS_ERROR_SUCCESS)
        {
                WalError("webpaRbusInit failed with error code %d\n", ret);
                return WDMP_FAILURE;
        }
        WalInfo("webpaRbusInit is success. ret is %d\n", ret);
        return WDMP_SUCCESS;
}

void webpaRbus_Uninit()
{
    rbus_close(rbus_handle);
}

rbusError_t setTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        if(isRbusInitialized)
        {
                if(traceContext[0] != NULL && traceContext[1] != NULL) {
                       if(strlen(traceContext[0]) > 0 && strlen(traceContext[1]) > 0) {
			    WalInfo("Invoked setTraceContext function with value traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);    
                            ret = rbusHandle_SetTraceContextFromString(rbus_handle, traceContext[0], traceContext[1]);
                            if(ret == RBUS_ERROR_SUCCESS) {
                                  WalPrint("SetTraceContext request success\n");
                            }
                             else {
                                   WalError("SetTraceContext request failed with error code - %d\n", ret);
                             }
                        }
                        else {
                              WalError("Header is empty\n");
                        }
                  }
                  else {
                        WalError("Header is NULL\n");
                  }
        }
        else {
                WalError("Rbus not initialzed in setTraceContext function\n");
        }	
        return ret;
}

rbusError_t getTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        char traceParent[512] = {'\0'};
        char traceState[512] = {'\0'};
	if(isRbusInitialized)
        {
	      ret =  rbusHandle_GetTraceContextAsString(rbus_handle, traceParent, sizeof(traceParent), traceState, sizeof(traceState));
	      if( ret == RBUS_ERROR_SUCCESS) {
		      if(strlen(traceParent) > 0 && strlen(traceState) > 0) {
			      WalPrint("GetTraceContext request success\n");
		              traceContext[0] = strdup(traceParent);
	                      traceContext[1] = strdup(traceState);
			      WalInfo("traceContext value, traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);
	               }
		       else {
			       WalPrint("traceParent & traceState are empty\n");
		       }	       
	      }
	      else {
		      WalError("GetTraceContext request failed with error code - %d\n", ret);
	      }	      
	}
        else { 
              WalError("Rbus not initialzed in getTraceContext function\n");
	}
        return ret;
}

rbusError_t clearTraceContext()
{
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;
	if(isRbusInitialized)
	{
		ret = rbusHandle_ClearTraceContext(rbus_handle);
		if(ret == RBUS_ERROR_SUCCESS) {
			WalInfo("ClearTraceContext request success\n");
		}
		else {
			WalError("ClearTraceContext request failed with error code - %d\n", ret);
		}
	}
	else {
		WalError("Rbus not initialized in clearTraceContext funcion\n");
        }
}

/**
 * Register data elements for dataModel implementation using rbus.
 */
int regWebPaDataModel()
{
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    if(!rbus_handle)
    {
        WalError("regWebPaDataModel failed in getting bus handles\n");
        return rc;
    }

	rc = rbus_regDataElements(rbus_handle, 2, dataElements);

    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data element %s with rbus \n ", WEBPA_NOTIFY_PARAM);
    }
    else
	{
		WalError("Failed in registering data element %s \n", WEBPA_NOTIFY_PARAM);
	}
	return rc;
}

/**
 * Un-Register data elements for dataModel implementation using rbus.
 */
int UnregWebPaDataModel()
{
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    if(!rbus_handle)
    {
        WalError("regWebPaDataModel failed in getting bus handles\n");
        return rc;
    }

	rc = rbus_unregDataElements(rbus_handle, 2, dataElements);
    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data element %s with rbus \n ", WEBPA_NOTIFY_PARAM);
    }
    else
	{
		WalError("Failed in registering data element %s \n", WEBPA_NOTIFY_PARAM);
	}
	return rc;
}

rbusError_t NotifyParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    WalInfo("NotifyParamGetHandler is called\n");
    const char* paramName = rbusProperty_GetName(property);
    if(strncmp(paramName, WEBPA_NOTIFY_PARAM, strlen(WEBPA_NOTIFY_PARAM)) != 0)
    {
        WalError("Unexpected parameter = %s\n", paramName);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    const char * const *params = getGlobalNotifyParams();
    if(!params)
        return RBUS_ERROR_BUS_ERROR;

    size_t totalLen = 1;
    for (int i = 0; params[i] != NULL; i++)
    {
        totalLen += strlen(params[i]);
        if (params[i+1] != NULL)
            totalLen += 1;
    }

    char* buffer = malloc(totalLen);
    if(!buffer)
        return RBUS_ERROR_BUS_ERROR;

    char* ptr = buffer;
    for(int i = 0; params[i] != NULL; i++)
    {
        size_t paramLen = strlen(params[i]);
        memcpy(ptr, params[i], paramLen);
        ptr += paramLen;
        
        if(params[i+1] != NULL)
        {
            *ptr++ = '\n';
        }
    }
    *ptr = '\0';

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, buffer);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    free(buffer);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t SubscribeToNotifyParamHandler(rbusHandle_t handle, const char* methodName,
                                rbusObject_t inParams, rbusObject_t outParams,
                                rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)methodName;
    (void)asyncHandle;

    int count = 0;
    rbusProperty_t prop = NULL;

    prop = rbusObject_GetProperties(inParams);
    count = rbusProperty_Count(prop);

    WalInfo("SubscribeToNotifyParamHandler invoked\n");
    WalInfo("Total params to subscribe: %d", count);

    if (count == 0)
    {
        WalError("No parameters provided in inParams object");
        return RBUS_ERROR_INVALID_INPUT;
    }

    // Taking manual count for debugging
    int manualCnt = 0;
    char keyName[MAX_PARAM_LEN];
    rbusValue_t val;

    for (int i = 0; ; i++)
    {
        snprintf(keyName, sizeof(keyName), "param%d", i);
        val = rbusObject_GetValue(inParams, keyName);
        if (!val)
            break;
        manualCnt++;
    }

    WalInfo("Manual count: %d\n", manualCnt);

    if (manualCnt == count)
    {
        WalInfo("Manual count and getcount(prop) is same: %d\n", count);
    }

    char** dynamicParamsList = calloc(manualCnt, sizeof(char*));
    if(!dynamicParamsList)
    {
        WalError("Memory allocation failed for dynamicParamsList\n");
        return RBUS_ERROR_OUT_OF_RESOURCES;
    }

    for (int i = 0; i < count; i++)
    {
        char paramName[MAX_PARAM_LEN];
        snprintf(paramName, sizeof(paramName), "param%d", i);

        rbusValue_t val = rbusObject_GetValue(inParams, paramName);
        if (val && rbusValue_GetType(val) == RBUS_STRING)
        {
            const char* strVal = rbusValue_GetString(val, NULL);
            if (strVal)
            {
                dynamicParamsList[i] = strdup(strVal);
                WalInfo("Added dynamic param[%d]: %s\n", i, dynamicParamsList[i]);
            }
        }
    }

    subscribeDynamicNotifyList(dynamicParamsList, count);

    for (int i = 0; i < count; i++)
        free(dynamicParamsList[i]);
    if(dynamicParamsList) free(dynamicParamsList);

    return RBUS_ERROR_SUCCESS;
}

static void eventReceiveHandler(
    rbusHandle_t rbus_handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)rbus_handle;
    (void)subscription;

    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");

    WebcfgInfo("Consumer received ValueChange event for param %s\n", event->name);

    if(newValue)
    { 
        const char* newValueStr = rbusValue_GetString(newValue, NULL);

        if(oldValue)
        {
            const char* oldValueStr = rbusValue_GetString(oldValue, NULL);
            WalInfo("New Value: %s Old Value: %s\n",
                    newValueStr ? newValueStr : "NULL",
                    oldValueStr ? oldValueStr : "NULL");
        }
        else
        {
            WalInfo("New Value: %s Old Value: <none>\n",
                    newValueStr ? newValueStr : "NULL");
        }
    }
    else
    {
        WalInfo("Received event, but no new value\n");
    }
}

static void subscribeAsyncHandler(
    rbusHandle_t rbus_handle,
    rbusEventSubscription_t* subscription,
    rbusError_t error)
{
    (void)rbus_handle;
    WalInfo("subscribeAsyncHandler event %s, error %d - %s\n", subscription->eventName, error, rbusError_ToString(error));
}

static void subscribeDynamicNotifyList(char** dynamicParamsList, int count)
{
	if(!rbus_handle)
	{
		WebcfgError("registerRBUSEventElement failed as rbus_handle is not initialized\n");
		return;
	}

    rbusError_t err = 0;

    for(int i=0; i<count; i++)
    {
        if(dynamicParamsList[i])
        {
            WalInfo("Subscribing to %s Event\n", dynamicParamsList[i]);
            err = rbusEvent_SubscribeAsync(rbus_handle, dynamicParamsList[i], eventReceiveHandler, subscribeAsyncHandler, "webpa_notify", 0);
            if(err != RBUS_ERROR_SUCCESS)
            {
                WalError("%s subscribe failed : %d - %s\n", WEBPA_SUBSCRIBE_LIST, err, rbusError_ToString(err));
            }
        }
    }
    return;
}


