#include<stdio.h>
#include <CUnit/Basic.h>
#include <rbus/rbus_property.h>

#include "../source/include/webpa_rbus.h"
#include "../source/broadband/include/webpa_eventing.h"
rbusHandle_t handle;

// Test case for isRbusEnabled
void test_isRbusEnabled_success()
{
    WalInfo("\n**************************************************\n");
	bool result = isRbusEnabled();
	CU_ASSERT_TRUE(result);
}

// Test case for isRbusInitialized success
void test_isRbusInitialized_success()
{
    WalInfo("\n**************************************************\n");
	webpaRbusInit("componentName");
	bool result = isRbusInitialized();
        printf("The bool value is %d\n", result);

	CU_ASSERT_TRUE(result);
	webpaRbus_Uninit();
}

void test_webpaRbusInit_success()
{
    WalInfo("\n**************************************************\n");
    int result = webpaRbusInit("component");
    CU_ASSERT_EQUAL(result, 0);
	webpaRbus_Uninit();
}

//Successcase for setTraceContext
void test_setTraceContext_success()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    //allocating moemory for each element
    traceContext[0] = strdup("randomvalueone");
    traceContext[1] = strdup("randomvaluetwo");
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);
    free(traceContext[0]);
    free(traceContext[1]);
}

//traceContext header NULL
void test_setTraceContext_header_NULL()
{   
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];
    traceContext[0] = NULL;
    traceContext[1] = NULL;
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

//traceContext header empty
void test_setTraceContext_header_empty()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];
    traceContext[0] = "";
    traceContext[1] = "";
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

void test_getTraceContext_success()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    //allocating moemory for each element
    traceContext[0] = strdup("randomvalueone");
    traceContext[1] = strdup("randomvaluetwo");
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);

    getTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);
    clearTraceContext();
}

void test_getTraceContext_empty()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    getTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

void test_NotifySubscriptionListGetHandler_unexpected_param(void)
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    rbusProperty_t mockProp = NULL;

    rbusProperty_Init(&mockProp, "Device.Wrong.Parameter", NULL);

    rc = NotifySubscriptionListGetHandler(NULL, mockProp, NULL);
    CU_ASSERT_EQUAL(rc, RBUS_ERROR_ELEMENT_DOES_NOT_EXIST);

    rbusProperty_Release(mockProp);
}

void test_NotifySubscriptionListGetHandler_NO_DATA()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    rbusProperty_t mockProp = NULL;
    clearGlobalNotifyList();

    rbusProperty_Init(&mockProp, WEBPA_NOTIFY_SUBSCRIPTION_PARAM, NULL);

    rc = NotifySubscriptionListGetHandler(NULL, mockProp, NULL);
    CU_ASSERT_EQUAL(rc, RBUS_ERROR_SUCCESS);

    const char *jsonStr = rbusValue_GetString(rbusProperty_GetValue(mockProp), NULL);
    CU_ASSERT_STRING_EQUAL(jsonStr, "[]");

    rbusProperty_Release(mockProp);
}

void test_NotifySubscriptionListGetHandler_one_param_success(void)
{
    clearGlobalNotifyList();

    // Add one parameter
    addParamToGlobalList("Device.Param.One", true, true);

    rbusProperty_t mockProp = NULL;
    rbusProperty_Init(&mockProp, WEBPA_NOTIFY_SUBSCRIPTION_PARAM, NULL);

    rbusError_t rc = NotifySubscriptionListGetHandler(NULL, mockProp, NULL);
    CU_ASSERT_EQUAL(rc, RBUS_ERROR_SUCCESS);

    rbusValue_t val = rbusProperty_GetValue(mockProp);
    CU_ASSERT_PTR_NOT_NULL(val);

    const char *jsonStr = rbusValue_GetString(val, NULL);
    CU_ASSERT_PTR_NOT_NULL(jsonStr);

    cJSON *root = cJSON_Parse(jsonStr);
    CU_ASSERT_PTR_NOT_NULL(root);
    CU_ASSERT_TRUE(cJSON_IsArray(root));
    CU_ASSERT_EQUAL(cJSON_GetArraySize(root), 1);

    cJSON *item = cJSON_GetArrayItem(root, 0);
    CU_ASSERT_PTR_NOT_NULL(item);

    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(item, "ParamName")->valuestring, "Device.Param.One");
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(item, "Type")->valuestring, "Static");
    CU_ASSERT_STRING_EQUAL(cJSON_GetObjectItem(item, "Status")->valuestring, "ON");

    cJSON_Delete(root);
    rbusProperty_Release(mockProp);
    clearGlobalNotifyList();
}

void add_suites( CU_pSuite *suite )
{
	*suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test isRbusEnabled_success", test_isRbusEnabled_success);
    CU_add_test( *suite, "test isRbusInitialized_success", test_isRbusInitialized_success);
    CU_add_test( *suite, "test webpaRbusInit_success", test_webpaRbusInit_success);
    CU_add_test( *suite, "test setTraceContext_success", test_setTraceContext_success);
    CU_add_test( *suite, "test setTraceContext_header_NULL", test_setTraceContext_header_NULL);
    CU_add_test( *suite, "test setTraceContext_header_empty", test_setTraceContext_header_empty);
    CU_add_test( *suite, "test getTraceContext_success", test_getTraceContext_success);
    CU_add_test( *suite, "test getTraceContext_empty", test_getTraceContext_empty);
    CU_add_test( *suite, "test NotifySubscriptionListGetHandler_unexpected_param", test_NotifySubscriptionListGetHandler_unexpected_param);
    CU_add_test( *suite, "test NotifySubscriptionListGetHandler_NO_DATA", test_NotifySubscriptionListGetHandler_NO_DATA);
    CU_add_test( *suite, "test NotifySubscriptionListGetHandler_one_param_success", test_NotifySubscriptionListGetHandler_one_param_success);
}


int main( int argc, char *argv[] )
{
	unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    (void ) argv;
    
    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }
        CU_cleanup_registry();
    }
    return rv;
}


