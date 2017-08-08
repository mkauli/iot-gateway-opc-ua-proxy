// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define UNIT_UNDER_TEST pal_sd_avahi
#include "util_ut.h"

//
// 1. Required mocks
//
#include "os.h"
#include "prx_types.h"

// client.h
MOCKABLE_FUNCTION(, AvahiClient*, avahi_client_new,
    const AvahiPoll*, poll_api, AvahiClientFlags, flags, AvahiClientCallback, callback, void*, userdata, int*, error);
MOCKABLE_FUNCTION(, int, avahi_client_errno,
    AvahiClient*, client);
MOCKABLE_FUNCTION(, const char*, avahi_client_get_domain_name,
    AvahiClient*, client);
MOCKABLE_FUNCTION(, void, avahi_client_free,
    AvahiClient*, client);
// watch.h
MOCKABLE_FUNCTION(, AvahiThreadedPoll*, avahi_threaded_poll_new);
MOCKABLE_FUNCTION(, void, avahi_threaded_poll_free,
    AvahiThreadedPoll*, p);
MOCKABLE_FUNCTION(, const AvahiPoll*, avahi_threaded_poll_get,
    AvahiThreadedPoll*, p);
MOCKABLE_FUNCTION(, int, avahi_threaded_poll_start,
    AvahiThreadedPoll*, s);
MOCKABLE_FUNCTION(, int, avahi_threaded_poll_stop,
    AvahiThreadedPoll*, p);
MOCKABLE_FUNCTION(, void, avahi_threaded_poll_lock,
    AvahiThreadedPoll*, p);
MOCKABLE_FUNCTION(, void, avahi_threaded_poll_unlock,
    AvahiThreadedPoll*, p);
// lookup.h
MOCKABLE_FUNCTION(, AvahiDomainBrowser*, avahi_domain_browser_new,
    AvahiClient*, client, AvahiIfIndex, interface, AvahiProtocol, protocol, const char*, domain,
    AvahiDomainBrowserType, btype, AvahiLookupFlags, flags, AvahiDomainBrowserCallback, callback, void*, userdata);
MOCKABLE_FUNCTION(, AvahiClient*, avahi_domain_browser_get_client,
    AvahiDomainBrowser*, browser);
MOCKABLE_FUNCTION(, int, avahi_domain_browser_free,
    AvahiDomainBrowser*, browser);
MOCKABLE_FUNCTION(, AvahiServiceTypeBrowser*, avahi_service_type_browser_new,
    AvahiClient*, client, AvahiIfIndex, interface, AvahiProtocol, protocol, const char*, domain,
    AvahiLookupFlags, flags, AvahiServiceTypeBrowserCallback, callback, void*, userdata);
MOCKABLE_FUNCTION(, AvahiClient*, avahi_service_type_browser_get_client,
    AvahiServiceTypeBrowser*, browser);
MOCKABLE_FUNCTION(, int, avahi_service_type_browser_free,
    AvahiServiceTypeBrowser*, browser);
MOCKABLE_FUNCTION(, AvahiServiceBrowser*, avahi_service_browser_new,
    AvahiClient*, client, AvahiIfIndex, interface, AvahiProtocol, protocol, const char*, type,
    const char*, domain, AvahiLookupFlags, flags, AvahiServiceBrowserCallback, callback, void*, userdata);
MOCKABLE_FUNCTION(, AvahiClient*, avahi_service_browser_get_client,
    AvahiServiceBrowser*, browser);
MOCKABLE_FUNCTION(, int, avahi_service_browser_free,
    AvahiServiceBrowser*, browser);
MOCKABLE_FUNCTION(, AvahiServiceResolver*, avahi_service_resolver_new,
    AvahiClient*, client, AvahiIfIndex, interface, AvahiProtocol, protocol,  const char*, name,
    const char*, type, const char*, domain, AvahiProtocol, aprotocol, AvahiLookupFlags, flags,
    AvahiServiceResolverCallback, callback, void*, userdata);
MOCKABLE_FUNCTION(, AvahiClient*, avahi_service_resolver_get_client,
    AvahiServiceResolver*, resolver);
MOCKABLE_FUNCTION(, int, avahi_service_resolver_free,
    AvahiServiceResolver*, resolver);
MOCKABLE_FUNCTION(, AvahiHostNameResolver*, avahi_host_name_resolver_new,
    AvahiClient*, client, AvahiIfIndex, interface, AvahiProtocol, protocol, const char*, name,
    AvahiProtocol, aprotocol, AvahiLookupFlags, flags, AvahiHostNameResolverCallback, callback, void*, userdata);
MOCKABLE_FUNCTION(, AvahiClient*, avahi_host_name_resolver_get_client,
    AvahiHostNameResolver*, resolver);
MOCKABLE_FUNCTION(, int, avahi_host_name_resolver_free,
    AvahiHostNameResolver*, resolver);

//
// 2. Include unit under test
//
#undef ENABLE_MOCKS
#include "pal_sd.h"
#define ENABLE_MOCKS
#include UNIT_C

//
// 3. Setup test suite
//
BEGIN_DECLARE_TEST_SUITE()
END_DECLARE_TEST_SUITE()
// -or- DECLARE_TEST_SUITE()

//
// 4. Setup and run tests
//
DECLARE_TEST_SETUP()


#ifdef pal_sd_init
//
// Test pal_sd_init happy path
//
TEST_FUNCTION(pal_avahi_sd_init__success)
{
    int32_t result;

    // arrange
    // ...

    // act
    result = pal_sd_init();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ...
}

//
// Test pal_sd_init unhappy path
//
TEST_FUNCTION(pal_avahi_sd_init__neg)
{
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ...

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_sd_init();

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_sd_init

#ifdef pal_sdclient_create
//
// Test pal_sdclient_create happy path
//
TEST_FUNCTION(pal_avahi_sdclient_create__success)
{
    static const pal_sdclient_t** k_client_valid;
    int32_t result;

    // arrange
    // ...

    // act
    result = pal_sdclient_create(k_client_valid);

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ...
}

//
// Test pal_sdclient_create passing as client argument an invalid pal_sdclient_t** value
//
TEST_FUNCTION(pal_avahi_sdclient_create__arg_client_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdclient_create();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdclient_create unhappy path
//
TEST_FUNCTION(pal_avahi_sdclient_create__neg)
{
    static const pal_sdclient_t** k_client_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ...

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_sdclient_create(k_client_valid);

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_sdclient_create

#ifdef pal_sdbrowser_create
//
// Test pal_sdbrowser_create happy path
//
TEST_FUNCTION(pal_avahi_sdbrowser_create__success)
{
    static const pal_sdclient_t* k_client_valid;
    static const pal_sd_result_cb_t k_cb_valid;
    static const void* k_context_valid;
    static const pal_sdbrowser_t** k_browser_valid;
    int32_t result;

    // arrange
    // ...

    // act
    result = pal_sdbrowser_create(k_client_valid, k_cb_valid, k_context_valid, k_browser_valid);

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ...
}

//
// Test pal_sdbrowser_create passing as client argument an invalid pal_sdclient_t* value
//
TEST_FUNCTION(pal_avahi_sdbrowser_create__arg_client_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_create();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_create passing as cb argument an invalid pal_sd_result_cb_t value
//
TEST_FUNCTION(pal_avahi_sdbrowser_create__arg_cb_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_create();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_create passing as context argument an invalid void* value
//
TEST_FUNCTION(pal_avahi_sdbrowser_create__arg_context_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_create();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_create passing as browser argument an invalid pal_sdbrowser_t** value
//
TEST_FUNCTION(pal_avahi_sdbrowser_create__arg_browser_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_create();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_create unhappy path
//
TEST_FUNCTION(pal_avahi_sdbrowser_create__neg)
{
    static const pal_sdclient_t* k_client_valid;
    static const pal_sd_result_cb_t k_cb_valid;
    static const void* k_context_valid;
    static const pal_sdbrowser_t** k_browser_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ...

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_sdbrowser_create(k_client_valid, k_cb_valid, k_context_valid, k_browser_valid);

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_sdbrowser_create

#ifdef pal_sdbrowser_browse
//
// Test pal_sdbrowser_browse happy path
//
TEST_FUNCTION(pal_avahi_sdbrowser_browse__success)
{
    static const pal_sdbrowser_t* k_browser_valid;
    static const const char* k_service_name_valid;
    static const const char* k_service_type_valid;
    static const const char* k_domain_valid;
    static const int32_t k_itf_index_valid;
    int32_t result;

    // arrange
    // ...

    // act
    result = pal_sdbrowser_browse(k_browser_valid, k_service_name_valid, k_service_type_valid, k_domain_valid, k_itf_index_valid);

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ...
}

//
// Test pal_sdbrowser_browse passing as browser argument an invalid pal_sdbrowser_t* value
//
TEST_FUNCTION(pal_avahi_sdbrowser_browse__arg_browser_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_browse();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_browse passing as service_name argument an invalid const char* value
//
TEST_FUNCTION(pal_avahi_sdbrowser_browse__arg_service_name_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_browse();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_browse passing as service_type argument an invalid const char* value
//
TEST_FUNCTION(pal_avahi_sdbrowser_browse__arg_service_type_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_browse();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_browse passing as domain argument an invalid const char* value
//
TEST_FUNCTION(pal_avahi_sdbrowser_browse__arg_domain_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_browse();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_browse passing as itf_index argument an invalid int32_t value
//
TEST_FUNCTION(pal_avahi_sdbrowser_browse__arg_itf_index_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_browse();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_browse unhappy path
//
TEST_FUNCTION(pal_avahi_sdbrowser_browse__neg)
{
    static const pal_sdbrowser_t* k_browser_valid;
    static const const char* k_service_name_valid;
    static const const char* k_service_type_valid;
    static const const char* k_domain_valid;
    static const int32_t k_itf_index_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ...

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_sdbrowser_browse(k_browser_valid, k_service_name_valid, k_service_type_valid, k_domain_valid, k_itf_index_valid);

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_sdbrowser_browse

#ifdef pal_sdbrowser_free
//
// Test pal_sdbrowser_free happy path
//
TEST_FUNCTION(pal_avahi_sdbrowser_free__success)
{
    static const pal_sdbrowser_t* k_browser_valid;
    void result;

    // arrange
    // ...

    // act
    result = pal_sdbrowser_free(k_browser_valid);

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(void, er_ok, result);
    // ...
}

//
// Test pal_sdbrowser_free passing as browser argument an invalid pal_sdbrowser_t* value
//
TEST_FUNCTION(pal_avahi_sdbrowser_free__arg_browser_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdbrowser_free();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdbrowser_free unhappy path
//
TEST_FUNCTION(pal_avahi_sdbrowser_free__neg)
{
    static const pal_sdbrowser_t* k_browser_valid;
    void result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ...

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_sdbrowser_free(k_browser_valid);

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(void, result, er_ok);
}

#endif // pal_sdbrowser_free

#ifdef pal_sdclient_free
//
// Test pal_sdclient_free happy path
//
TEST_FUNCTION(pal_avahi_sdclient_release__success)
{
    static const pal_sdclient_t* k_client_valid;
    void result;

    // arrange
    // ...

    // act
    result = pal_sdclient_free(k_client_valid);

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(void, er_ok, result);
    // ...
}

//
// Test pal_sdclient_free passing as client argument an invalid pal_sdclient_t* value
//
TEST_FUNCTION(pal_avahi_sdclient_release__arg_client_invalid)
{
    // ...
    int32_t result;

    // arrange
    // ...

    // act
    handle = pal_sdclient_free();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ...
}

//
// Test pal_sdclient_free unhappy path
//
TEST_FUNCTION(pal_avahi_sdclient_release__neg)
{
    static const pal_sdclient_t* k_client_valid;
    void result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ...

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_sdclient_free(k_client_valid);

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(void, result, er_ok);
}

#endif // pal_sdclient_free

#ifdef pal_sd_deinit
//
// Test pal_sd_deinit happy path
//
TEST_FUNCTION(pal_avahi_sd_deinit__success)
{
    void result;

    // arrange
    // ...

    // act
    result = pal_sd_deinit();

    // assert
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(void, er_ok, result);
    // ...
}

//
// Test pal_sd_deinit unhappy path
//
TEST_FUNCTION(pal_avahi_sd_deinit__neg)
{
    void result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ...

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_sd_deinit();

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(void, result, er_ok);
}

#endif // pal_sd_deinit

//
// 5. Teardown tests and test suite
//
DECLARE_TEST_COMPLETE()

