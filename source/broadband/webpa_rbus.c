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
    {WEBPA_SUBSCRIBE_LIST, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, SubscribeNotifyParamMethodHandler}}
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
 * Register data elements for data model and methods implementation using rbus.
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

    char** params = getGlobalNotifyParams();
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

static void eventReceiveHandler(
    rbusHandle_t rbus_handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)rbus_handle;
    (void)subscription;

    rbusValue_t newValue, oldValue, writeId;
    char newValueStr[256] = "NULL";
    char oldValueStr[256] = "NULL";
    char writeIdStr[256]  = "NULL";

    if (!event || !event->data || !event->name)
    {
        WalError("Received null event or event data\n");
        return;
    }

    WalInfo("Consumer received Value Change event for param %s\n", event->name ? event->name : "NULL");

    newValue = rbusObject_GetValue(event->data, "value");
    oldValue = rbusObject_GetValue(event->data, "oldValue");
    writeId = rbusObject_GetValue(event->data, "by");

    if(newValue)
        rbusValue_ToString(newValue, newValueStr, sizeof(newValueStr));
    if(oldValue)
        rbusValue_ToString(oldValue, oldValueStr, sizeof(oldValueStr));
    if(writeId)
        rbusValue_ToString(writeId, writeIdStr, sizeof(writeIdStr));

    WalInfo("New Value: %s, Old Value: %s, writeID: %s", newValueStr ? newValueStr : "NULL", oldValueStr ? oldValueStr : "NULL", writeIdStr ? writeIdStr : "NULL");
    return;
}

static void subscribeAsyncHandler(
    rbusHandle_t rbus_handle,
    rbusEventSubscription_t* subscription,
    rbusError_t error)
{
    (void)rbus_handle;
    WalInfo("subscribeAsyncHandler event %s, error %d - %s\n", subscription->eventName, error, rbusError_ToString(error));
}

rbusError_t subscribeToNotifyParams(
    const char* params[], int paramCount,
    char*** succeededParams, int* successCount,
    char*** failedParams, int* failureCount)
{
    WalInfo("Inside subscribeToNotifyParams");

    *succeededParams = calloc(paramCount, sizeof(char*));
    *failedParams    = calloc(paramCount, sizeof(char*));
    *successCount = 0;
    *failureCount = 0;

    if (!*succeededParams || !*failedParams)
    {
        WalError("Memory allocation failed\n");
        free(*succeededParams);
        free(*failedParams);
        return RBUS_ERROR_OUT_OF_RESOURCES;
    }

    for (int i = 0; i < paramCount; i++)
    {
        const char* param = params[i];
        if (!param || !*param)
        {
            WalError("Invalid parameter: %s\n", param);
            (*failedParams)[(*failureCount)++] = strdup("InvalidParam");
            continue;
        }

        WalInfo("Subscribing to %s", param);
        rbusError_t err = rbusEvent_SubscribeAsync(
            rbus_handle,
            param,
            eventReceiveHandler,
            subscribeAsyncHandler,
            "webpa_notify",
            0);

        if (err != RBUS_ERROR_SUCCESS)
        {
            WalError("Subscribe failed for %s : %s", param, rbusError_ToString(err));
            (*failedParams)[(*failureCount)++] = strdup(param);
        }
        else
        {
            (*succeededParams)[(*successCount)++] = strdup(param);
        }
    }
    return RBUS_ERROR_SUCCESS;
}

rbusError_t SubscribeNotifyParamMethodHandler(
    rbusHandle_t handle,
    const char* methodName,
    rbusObject_t inParams,
    rbusObject_t outParams,
    rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)methodName;
    (void)asyncHandle;

    WalInfo("SubscribeNotifyParamMethodHandler invoked\n");

    char* msgBuf = NULL;
    rbusValue_t message, statusCode;
    rbusValue_Init(&message);
    rbusValue_Init(&statusCode);

/* Extract inParams */
    rbusProperty_t props = rbusObject_GetProperties(inParams);
    int count = props ? rbusProperty_Count(props) : 0;

    if (count == 0)
    {
        WalError("No parameters provided");
        rbusValue_SetString(message, "No parameters provided");
        rbusValue_SetInt32(statusCode, RBUS_ERROR_INVALID_INPUT);
        goto set_response_and_return;
    }

    WalInfo("Total params to subscribe: %d", count);

    const char** dynamicNotifyParams = calloc(count, sizeof(char*));
    if (!dynamicNotifyParams)
    {
        WalError("Memory allocation failed for param array\n");
        rbusValue_SetString(message, "Memory allocation failed");
        rbusValue_SetInt32(statusCode, RBUS_ERROR_OUT_OF_RESOURCES);
        goto set_response_and_return;
    }

    int validCount = 0;
    for (int i = 0; i < count; i++)
    {
        char keyName[MAX_PARAM_LEN];
        snprintf(keyName, sizeof(keyName), "param%d", i);
        rbusValue_t val = rbusObject_GetValue(inParams, keyName);
        const char* paramName = NULL;
        if (val && rbusValue_GetType(val) == RBUS_STRING)
            paramName = rbusValue_GetString(val, NULL);

        if (!paramName || !*paramName)
        {
            WalError("Invalid or missing value for %s", keyName);
            continue;
        }
        dynamicNotifyParams[validCount++] = paramName;
    }

    if (validCount == 0)
    {
        WalError("No valid parameters found\n");
        free(dynamicNotifyParams);
        rbusValue_SetString(message, "No valid parameters provided");
        rbusValue_SetInt32(statusCode, RBUS_ERROR_INVALID_INPUT);
        goto set_response_and_return;
    }

/* Subscribe validated parameters to rbus */
    char** succeededParams = NULL;
    char** failedParams = NULL;
    int successCount = 0, failureCount = 0;

    rbusError_t err = subscribeToNotifyParams(dynamicNotifyParams, validCount, &succeededParams, &successCount, &failedParams, &failureCount);
    free(dynamicNotifyParams);
    
    
    if(err != RBUS_ERROR_SUCCESS)
    {
        rbusValue_SetString(message, "Subscription failed");
        rbusValue_SetInt32(statusCode,  err);
        goto set_response_and_return;
    }

/* Build comma-separated success/failure lists */
    size_t successBufLen = successCount ? successCount * 64 : 1;
    size_t failedBufLen  = failureCount ? failureCount * 64 : 1;
    char* successBuf = calloc(successBufLen, 1);
    char* failedBuf  = calloc(failedBufLen, 1);
    if (!successBuf || !failedBuf)
    {
        WalError("Failed to allocate successBuf or failedBuf");
        rbusValue_SetString(message, "Memory allocation failed");
        rbusValue_SetInt32(statusCode, RBUS_ERROR_OUT_OF_RESOURCES);
        goto set_response_and_return;
    }
    size_t successLen = 0, failedLen = 0;

/* Buffer for subscribed parameters */
    for (int i = 0; i < successCount; i++)
    {
        size_t rem = successBufLen - successLen;
        if (rem == 0) break;
        int wrote = snprintf(successBuf + successLen, rem, "%s%s",
                            (i > 0) ? ", " : "", succeededParams[i]);
        if (wrote < 0) break;
        if ((size_t)wrote >= rem) { successLen = successBufLen - 1; break; }
        successLen += (size_t)wrote;
    }
/* Buffer for subscribe failed parameters */
    for (int i = 0; i < failureCount; i++)
    {
        size_t rem = failedBufLen - failedLen;
        if (rem == 0) break;
        int wrote = snprintf(failedBuf + failedLen, rem, "%s%s",
                            (i > 0) ? ", " : "", failedParams[i]);
        if (wrote < 0) break;
        if ((size_t)wrote >= rem) { failedLen = failedBufLen - 1; break; }
        failedLen += (size_t)wrote;
    }
/* Format final response message */
    size_t msgBuf_len = strlen(successBuf) + strlen(failedBuf) + 256;
    if (msgBuf_len == 0) msgBuf_len = 256;
    
    msgBuf = malloc(msgBuf_len);
    if (!msgBuf) {
        WalError("Failed to allocate msgBuf");
        rbusValue_SetString(message, "Memory allocation failed");
        rbusValue_SetInt32(statusCode, RBUS_ERROR_OUT_OF_RESOURCES);
        goto set_response_and_return;
    }

    if (failureCount == 0)
        snprintf(msgBuf, msgBuf_len,
                "All %d subscription requests initiated successfully: %s",
                successCount, successBuf);
    else if (successCount == 0)
        snprintf(msgBuf, msgBuf_len,
                "All %d subscription requests failed: %s",
                failureCount, failedBuf);
    else
        snprintf(msgBuf, msgBuf_len,
                "Partial success: %d succeeded [%s], %d failed [%s]",
                successCount, successBuf, failureCount, failedBuf);

/* Set verbose output message to rbus outParams */
    rbusValue_SetString(message, msgBuf);
    rbusValue_SetInt32(statusCode, failureCount == 0 ? 0 : RBUS_ERROR_BUS_ERROR);

    WalInfo("SubscribeNotifyParamMethodHandler completed: %s", msgBuf);
    free(msgBuf); msgBuf = NULL;

set_response_and_return:
    rbusObject_SetValue(outParams, "message", message);
    rbusObject_SetValue(outParams, "statusCode", statusCode);

    if (message) rbusValue_Release(message);
    if (statusCode) rbusValue_Release(statusCode);

    if (succeededParams)
    {
        for (int i = 0; i < successCount; i++) free(succeededParams[i]);
        free(succeededParams);
    }
    if (failedParams)
    {
        for (int i = 0; i < failureCount; i++) free(failedParams[i]);
        free(failedParams);
    }

    if (successBuf) free(successBuf);
    if (failedBuf) free(failedBuf);

    WalInfo("SubscribeNotifyParamMethodHandler completed: %s", msgBuf);
    return RBUS_ERROR_SUCCESS;
}

