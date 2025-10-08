/**
 *  Copyright 2025 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <sys/time.h>

#include "../source/include/webpa_adapter.h"
#include "../source/broadband/include/webpa_internal.h"
#include "../source/broadband/include/webpa_notification.h"
#include "../source/broadband/include/webpa_eventing.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <wrp-c.h>
#include <libparodus.h>
#include <cJSON.h>
#include <ccsp_base_api.h>
#include "mock_stack.h"

#define MAX_PARAMETER_LEN			512
#define NOTIFY_PARAM_FILE        "test_notify_params.txt"

int getWebpaParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    UNUSED(parameterNames); UNUSED(paramCount); UNUSED(val_size); UNUSED(val);
    return (int) mock();
}

int setWebpaParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam )
{
    UNUSED(faultParam); UNUSED(paramCount); UNUSED(val);
    return (int) mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_bootupNotifyInitDone(void)
{
    assert_false(getBootupNotifyInitDone());

    setBootupNotifyInitDone(true);
    assert_true(getBootupNotifyInitDone());

    setBootupNotifyInitDone(false);
    assert_false(getBootupNotifyInitDone());
}

void test_addParamToGlobalList_null_param()
{
    addParamToGlobalList(NULL, true, true);
    assert_null(getGlobalNotifyHead());
}

void test_addParamToGlobalList_add_one_param()
{
    clearGlobalNotifyList();

    const char *paramName = "Device.Sample.Param1";
    bool paramType = true;
    bool paramSubscriptionStatus = false;
    addParamToGlobalList(paramName, paramType, paramSubscriptionStatus);

    g_NotifyParam *node = getGlobalNotifyHead();
    assert_non_null(node);

    assert_string_equal(node->paramName, paramName);
    assert_int_equal(node->paramType, paramType);
    assert_false(node->paramSubscriptionStatus);

    clearGlobalNotifyList();
}

void test_addParamToGlobalList_add_multiple_params()
{
    clearGlobalNotifyList();

    addParamToGlobalList("Device.Sample.Param1", true, false);
    addParamToGlobalList("Device.Sample.Param2", false, true);

    g_NotifyParam *head = getGlobalNotifyHead();
    assert_non_null(head);
    assert_string_equal(head->paramName, "Device.Sample.Param1");
    assert_true(head->paramType);
    assert_false(head->paramSubscriptionStatus);

    g_NotifyParam *tail = head->next;
    assert_non_null(tail);
    assert_string_equal(tail->paramName, "Device.Sample.Param2");
    assert_false(tail->paramType);
    assert_true(tail->paramSubscriptionStatus);
    assert_ptr_equal(head->next, tail);

    clearGlobalNotifyList();
}

void test_searchParaminGlobalList(void)
{
    clearGlobalNotifyList();

    g_NotifyParam *node = searchParaminGlobalList(NULL);
    assert_null(node);

    node = searchParaminGlobalList("Device.Sample.Param1");
    assert_null(node);

    addParamToGlobalList("Device.Sample.Param1", true, false);
    node = searchParaminGlobalList("Device.Sample.Param1");
    assert_non_null(node);
    assert_string_equal(node->paramName, "Device.Sample.Param1");
    assert_true(node->paramType);
    assert_false(node->paramSubscriptionStatus);

    node = searchParaminGlobalList("Device.Sample.Param2");
    assert_null(node);

    clearGlobalNotifyList();
}

void test_getParamStatus_and_updateParamStatus(void)
{
    clearGlobalNotifyList();

    addParamToGlobalList("Device.Sample.Param1", true, false);
    g_NotifyParam *node = searchParaminGlobalList("Device.Sample.Param1");
    assert_non_null(node);

    assert_false(getParamStatus(node));

    updateParamStatus(node, true);
    assert_true(getParamStatus(node));

    updateParamStatus(node, false);
    assert_false(getParamStatus(node));

    clearGlobalNotifyList();
}

void test_updateParamStatus_null_param(void)
{
    updateParamStatus(NULL, true);
}

void test_CreateJsonFromGlobalNotifyList_single_param(void)
{
    clearGlobalNotifyList();

    addParamToGlobalList("Device.Sample.Param1", true, false);

    char *jsonStr = CreateJsonFromGlobalNotifyList();
    assert_non_null(jsonStr);
    WalInfo("jsonstr=%s\n", jsonStr);

    cJSON *root = cJSON_Parse(jsonStr);
    assert_non_null(root);

    cJSON *item = cJSON_GetArrayItem(root, 0);
    assert_non_null(item);

    cJSON *paramName = cJSON_GetObjectItem(item, "ParamName");
    cJSON *type      = cJSON_GetObjectItem(item, "Type");
    cJSON *status    = cJSON_GetObjectItem(item, "Status");

    assert_string_equal(paramName->valuestring, "Device.Sample.Param1");
    assert_string_equal(type->valuestring, "Static");
    assert_string_equal(status->valuestring, "OFF"); 

    cJSON_Delete(root);
    free(jsonStr);
    clearGlobalNotifyList();
}

void test_writeDynamicParamToDBFile_null_filepath(void)
{
    int rc = writeDynamicParamToDBFile(NULL, "Device.Param.1");
    assert_int_equal(rc, 0);
}

void test_writeDynamicParamToDBFile_valid_param(void)
{
    remove(NOTIFY_PARAM_FILE);
    int rc = writeDynamicParamToDBFile(NOTIFY_PARAM_FILE, "Device.Param.1");
    assert_int_equal(rc, 1);

    FILE *fp = fopen(NOTIFY_PARAM_FILE, "r");
    assert_non_null(fp);

    char buf[128];
    assert_non_null(fgets(buf, sizeof(buf), fp));
    fclose(fp);

    buf[strcspn(buf, "\r\n")] = 0;
    assert_string_equal(buf, "Device.Param.1");
}

void test_writeDynamicParamToDBFile_null_param(void)
{
    remove(NOTIFY_PARAM_FILE);

    int rc = writeDynamicParamToDBFile(NOTIFY_PARAM_FILE, NULL);
    assert_int_equal(rc, 0);
}

void test_readDynamicParamsFromDBFile_null_filepath(void)
{
    int count = 0;
    readDynamicParamsFromDBFile(NULL, &count);
    assert_int_equal(count, 0);
}

void test_readDynamicParamsFromDBFile_missing(void)
{
    remove(NOTIFY_PARAM_FILE);

    int count = 0;
    readDynamicParamsFromDBFile(NOTIFY_PARAM_FILE, &count);
    assert_int_equal(count, 0);
}

void test_readDynamicParamsFromDBFile_multiple_params(void)
{
    clearGlobalNotifyList();
    remove(NOTIFY_PARAM_FILE);

    FILE *fp = fopen(NOTIFY_PARAM_FILE, "w");
    fprintf(fp, "Device.Param.1\nDevice.Param.2\nDevice.Param.3\n");
    fclose(fp);

    int count = 0;
    readDynamicParamsFromDBFile(NOTIFY_PARAM_FILE, &count);
    assert_int_equal(count, 3);

    g_NotifyParam *node = getGlobalNotifyHead();
    assert_non_null(node);
    assert_string_equal(node->paramName, "Device.Param.1");

    node = node->next;
    assert_non_null(node);
    assert_string_equal(node->paramName, "Device.Param.2");

    node = node->next;
    assert_non_null(node);
    assert_string_equal(node->paramName, "Device.Param.3");

    clearGlobalNotifyList();
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bootupNotifyInitDone),
        cmocka_unit_test(test_addParamToGlobalList_null_param),
        cmocka_unit_test(test_addParamToGlobalList_add_one_param),
        cmocka_unit_test(test_searchParaminGlobalList),
        cmocka_unit_test(test_getParamStatus_and_updateParamStatus),
        cmocka_unit_test(test_updateParamStatus_null_param),
        cmocka_unit_test(test_addParamToGlobalList_add_multiple_params),
        cmocka_unit_test(test_CreateJsonFromGlobalNotifyList_single_param),
        cmocka_unit_test(test_writeDynamicParamToDBFile_null_filepath),
        cmocka_unit_test(test_writeDynamicParamToDBFile_valid_param),
        cmocka_unit_test(test_writeDynamicParamToDBFile_null_param),
        cmocka_unit_test(test_readDynamicParamsFromDBFile_null_filepath),
        cmocka_unit_test(test_readDynamicParamsFromDBFile_missing),
        cmocka_unit_test(test_readDynamicParamsFromDBFile_multiple_params)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
