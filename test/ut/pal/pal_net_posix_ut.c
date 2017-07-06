// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#define UNIT_UNDER_TEST pal_net_posix
#include "util_ut.h"

//
// 1. Required mocks
//
#include "os.h"
#include "prx_types.h"

// socket/winsock.h
MOCKABLE_FUNCTION(, fd_t, socket, 
    int, family, int, type, int, protocol);
MOCKABLE_FUNCTION(, int, getsockname, 
    fd_t, s, struct sockaddr*, name, socklen_t*, namelen);
MOCKABLE_FUNCTION(, int, connect,
    fd_t, s, const struct sockaddr*, name, socklen_t, namelen);
MOCKABLE_FUNCTION(, int, bind, 
    fd_t, s, const struct sockaddr*, name, socklen_t, namelen);
MOCKABLE_FUNCTION(, int, listen,
    fd_t, s, int, backlog);
MOCKABLE_FUNCTION(, fd_t, accept,
    fd_t, s, struct sockaddr*, name, socklen_t*, namelen);
MOCKABLE_FUNCTION(, sockssize_t, sendto, 
    fd_t, s, const sockbuf_t*, buf, socksize_t, len, int, flags, const struct sockaddr*, name, socklen_t, namelen);
MOCKABLE_FUNCTION(, sockssize_t, send, 
    fd_t, s, const sockbuf_t*, buf, socksize_t, len, int, flags);
MOCKABLE_FUNCTION(, sockssize_t, recvfrom, 
    fd_t, s, sockbuf_t*, buf, socksize_t, len, int, flags, struct sockaddr*, name, socklen_t*, namelen);
MOCKABLE_FUNCTION(, sockssize_t, recv, 
    fd_t, s, sockbuf_t*, buf, socksize_t, len, int, flags);
MOCKABLE_FUNCTION(, int, shutdown, 
    fd_t, s, int, how);
MOCKABLE_FUNCTION(, int, getsockopt, 
    fd_t, s, int, optlevel, int, optname, sockbuf_t*, optval, socklen_t*, optlen);
MOCKABLE_FUNCTION(, int, setsockopt, 
    fd_t, s, int, optlevel, int, optname, const sockbuf_t*, optval, socklen_t, optlen);
MOCKABLE_FUNCTION(, int, getaddrinfo,
    const char*, name, const char*, svc, const struct addrinfo*, hints, struct addrinfo**, ai);
MOCKABLE_FUNCTION(, void, freeaddrinfo,
    struct addrinfo*, ai);
MOCKABLE_FUNCTION(, int, getnameinfo,
    const struct sockaddr*, address, socklen_t, addr_len, char*, buffer, socklen_t, buf_size, 
    char*, svcbuffer, socklen_t, svc_buf_size, int, flags);
MOCKABLE_FUNCTION(, int, getpeername,
    fd_t, s, struct sockaddr*, name, socklen_t*, namelen);

//
// 2. Include unit under test
//
#undef ENABLE_MOCKS
#include "pal_net.h"
#include "pal_types.h"
#define ENABLE_MOCKS
#include UNIT_C

// pal_socket.h - platform specific
MOCKABLE_FUNCTION(, int32_t, pal_os_to_prx_net_error,
    int, error);
MOCKABLE_FUNCTION(, int32_t, pal_os_last_net_error_as_prx_error);

//
// 3. Setup test suite
//
BEGIN_DECLARE_TEST_SUITE()
REGISTER_UMOCK_ALIAS_TYPE(socklen_t, int);
REGISTER_UMOCK_ALIAS_TYPE(socksize_t, int);
REGISTER_UMOCK_ALIAS_TYPE(sockssize_t, int);
REGISTER_UMOCK_ALIAS_TYPE(fd_t, int);
REGISTER_UMOCK_ALIAS_TYPE(intptr_t, void*);
END_DECLARE_TEST_SUITE()
// -or- DECLARE_TEST_SUITE()

//
// 4. Setup and run tests
//
DECLARE_TEST_SETUP()

// 
// Test pal_os_to_prx_gai_error happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_gai_error__success)
{
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, EAI_RANGE_BEGIN, EAI_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_to_prx_gai_error(input);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, result, er_unknown,
        er_retry, er_bad_flags, er_address_family, er_host_unknown, er_fatal, er_address_family);
}

// 
// Test pal_os_from_prx_gai_error happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_gai_error__success)
{
    int result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int32_t, input, er_unknown, er_ok);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_gai_error(input);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, result, EAI_NONAME, 206, 201, 203, 202, 0);
}

// 
// Test pal_os_to_prx_h_error happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_h_error__success)
{
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, H_ERRNO_RANGE_BEGIN, H_ERRNO_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_to_prx_h_error(input);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, result, er_unknown,
        er_no_host, er_retry, er_fatal, er_no_address);
}

// 
// Test pal_os_from_prx_h_error happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_h_error__success)
{
    int result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int32_t, input, er_unknown, er_ok);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_h_error(input);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, result, HOST_NOT_FOUND, 303, 302, 304);
}

// 
// Test pal_os_from_prx_client_getaddrinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getaddrinfo_flags__success_1)
{
    static const int32_t k_flags_valid = prx_ai_passive;
    int plat_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getaddrinfo_flags(k_flags_valid, &plat_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, AI_PASSIVE, plat_flags_valid);
}

// 
// Test pal_os_from_prx_client_getaddrinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getaddrinfo_flags__success_2)
{
    static const int32_t k_flags_valid = 0;
    int plat_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getaddrinfo_flags(k_flags_valid, &plat_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, 0, plat_flags_valid);
}

// 
// Test pal_os_from_prx_client_getaddrinfo_flags passing as flags argument an invalid int32_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getaddrinfo_flags__arg_flags_invalid)
{
    static const int32_t k_flags_invalid = 0x100000;
    int plat_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getaddrinfo_flags(k_flags_invalid, &plat_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
    ASSERT_ARE_EQUAL(int, 0, plat_flags_valid);
}

// 
// Test pal_os_from_prx_client_getaddrinfo_flags passing as platform_flags argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getaddrinfo_flags__arg_platform_flags_null)
{
    static const int32_t k_flags_valid = prx_ai_passive;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getaddrinfo_flags(k_flags_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_client_getaddrinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getaddrinfo_flags__success_1)
{
    static const int k_flags_valid = AI_PASSIVE;
    int32_t prx_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getaddrinfo_flags(k_flags_valid, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int32_t, prx_ai_passive, prx_flags_valid);
}

// 
// Test pal_os_to_prx_client_getaddrinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getaddrinfo_flags__success_2)
{
    static const int k_flags_valid = 0;
    int32_t prx_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getaddrinfo_flags(k_flags_valid, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int32_t, 0, prx_flags_valid);
}

// 
// Test pal_os_to_prx_client_getaddrinfo_flags passing as flags argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getaddrinfo_flags__arg_flags_invalid)
{
    static const int k_flags_invalid = -1;
    int32_t prx_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getaddrinfo_flags(k_flags_invalid, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_client_getaddrinfo_flags passing as prx_flags argument an invalid int32_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getaddrinfo_flags__arg_prx_flags_null)
{
    static const int k_flags_valid = AI_PASSIVE;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getaddrinfo_flags(k_flags_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_addrinfo happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__success_1)
{
    static const char* k_canon_name_valid = "test";
    struct sockaddr_in6 sock_addr_valid;
    struct addrinfo ai_valid;
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    sock_addr_valid.sin6_family = AF_INET6;
    ai_valid.ai_canonname = (char*)k_canon_name_valid;
    ai_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 
    STRICT_EXPECTED_CALL(string_clone(k_canon_name_valid, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_copy(&k_canon_name_valid, sizeof(k_canon_name_valid))
        .SetReturn(er_ok);

    // act 
    result = pal_os_to_prx_addrinfo(&ai_valid, &prx_ai_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_os_to_prx_addrinfo happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__success_2)
{
    struct sockaddr_in6 sock_addr_valid;
    struct addrinfo ai_valid;
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    sock_addr_valid.sin6_family = AF_INET6;
    ai_valid.ai_canonname = NULL;
    ai_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 

    // act 
    result = pal_os_to_prx_addrinfo(&ai_valid, &prx_ai_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_IS_NULL(prx_ai_valid.name);
}

// 
// Test pal_os_to_prx_addrinfo happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__success_3)
{
    struct sockaddr_in sock_addr_valid;
    struct addrinfo ai_valid;
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    sock_addr_valid.sin_family = AF_INET;
    ai_valid.ai_canonname = NULL;
    ai_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 

    // act 
    result = pal_os_to_prx_addrinfo(&ai_valid, &prx_ai_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_os_to_prx_addrinfo passing as ai argument an invalid struct addrinfo* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__arg_ai_invalid_2)
{
    static const char* k_canon_name_valid = "test";
    struct addrinfo ai_valid;
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    ai_valid.ai_canonname = (char*)k_canon_name_valid;
    ai_valid.ai_addrlen = sizeof(struct sockaddr_in6);
    ai_valid.ai_addr = NULL;

    // arrange 

    // act 
    result = pal_os_to_prx_addrinfo(&ai_valid, &prx_ai_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_addrinfo passing as ai argument an invalid struct addrinfo* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__arg_ai_invalid_3)
{
    struct sockaddr_in sock_addr_valid;
    struct addrinfo ai_valid;
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    sock_addr_valid.sin_family = 0;
    ai_valid.ai_canonname = NULL;
    ai_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 

    // act 
    result = pal_os_to_prx_addrinfo(&ai_valid, &prx_ai_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_addrinfo passing as ai argument an invalid struct addrinfo* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__arg_ai_null)
{
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_addrinfo(NULL, &prx_ai_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_addrinfo passing as prx_ai argument an invalid prx_addrinfo_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__arg_prx_ai_invalid)
{
    struct sockaddr_in sock_addr_valid;
    struct addrinfo ai_valid;
    int32_t result;

    sock_addr_valid.sin_family = AF_INET;
    ai_valid.ai_canonname = NULL;
    ai_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 

    // act 
    result = pal_os_to_prx_addrinfo(&ai_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_addrinfo unhappy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_addrinfo__neg)
{
    static const char* k_canon_name_valid = "test";
    struct sockaddr_in6 sock_addr_valid;
    struct addrinfo ai_valid;
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    sock_addr_valid.sin6_family = AF_INET6;
    ai_valid.ai_canonname = (char*)k_canon_name_valid;
    ai_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 
    STRICT_EXPECTED_CALL(string_clone(k_canon_name_valid, IGNORED_PTR_ARG))
        .IgnoreArgument(2)
        .SetReturn(er_out_of_memory);

    // act 
    result = pal_os_to_prx_addrinfo(&ai_valid, &prx_ai_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_out_of_memory, result);
}

#if 0

// 
// Test pal_os_from_prx_addrinfo happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_addrinfo__success)
{
    static const char* k_canon_name_valid = "test";
    addrinfo ai_valid;
    prx_addrinfo_t prx_ai_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_os_from_prx_addrinfo(k_prx_ai_valid, k_ai_valid);

    // assert 
    // ... 
    ASSERT_EXPECTED_CALLS();
}

// 
// Test pal_os_from_prx_addrinfo passing as prx_ai argument an invalid prx_addrinfo_t* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_addrinfo__arg_prx_ai_invalid)
{
    // ... 

    // arrange 
    // ... 

    // act 
    result = pal_os_from_prx_addrinfo();

    // assert 
    // ... 
    ASSERT_EXPECTED_CALLS();
}

// 
// Test pal_os_from_prx_addrinfo passing as ai argument an invalid struct addrinfo* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_addrinfo__arg_ai_invalid)
{
    // ... 

    // arrange 
    // ... 

    // act 
    result = pal_os_from_prx_addrinfo();

    // assert 
    // ... 
    ASSERT_EXPECTED_CALLS();
}

// 
// Test pal_os_from_prx_addrinfo unhappy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_addrinfo__neg)
{
    const prx_addrinfo_t* k_prx_ai_valid;
    const struct addrinfo* k_ai_valid;

    // arrange

    // act
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_os_from_prx_addrinfo(k_prx_ai_valid, k_ai_valid);

    // assert
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif

// 
// Test pal_os_from_prx_client_getnameinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getnameinfo_flags__success_1)
{
    static const int32_t k_flags_valid = prx_ni_flag_namereqd;
    int plat_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getnameinfo_flags(k_flags_valid, &plat_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, NI_NAMEREQD, plat_flags_valid);
}

// 
// Test pal_os_from_prx_client_getnameinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getnameinfo_flags__success_2)
{
    static const int32_t k_flags_valid = 0;
    int plat_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getnameinfo_flags(k_flags_valid, &plat_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, 0, plat_flags_valid);
}
// 
// Test pal_os_from_prx_client_getnameinfo_flags passing as flags argument an invalid int32_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getnameinfo_flags__arg_flags_invalid)
{
    static const int32_t k_flags_invalid = 0x100000;
    int plat_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getnameinfo_flags(k_flags_invalid, &plat_flags_valid);

    // assert 
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_os_from_prx_client_getnameinfo_flags passing as plat_flags argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_client_getnameinfo_flags__arg_plat_flags_null)
{
    static const int32_t k_flags_valid = prx_ni_flag_namereqd;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_client_getnameinfo_flags(k_flags_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_client_getnameinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getnameinfo_flags__success_1)
{
    static const int k_flags_valid = NI_NAMEREQD;
    int32_t prx_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getnameinfo_flags(k_flags_valid, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int32_t, prx_ni_flag_namereqd, prx_flags_valid);
}

// 
// Test pal_os_to_prx_client_getnameinfo_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getnameinfo_flags__success_2)
{
    static const int k_flags_valid = 0;
    int32_t prx_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getnameinfo_flags(k_flags_valid, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int32_t, 0, prx_flags_valid);
}

// 
// Test pal_os_to_prx_client_getnameinfo_flags passing as flags argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getnameinfo_flags__arg_flags_invalid)
{
    static const int k_flags_invalid = -1;
    int32_t prx_flags_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getnameinfo_flags(k_flags_invalid, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_client_getnameinfo_flags passing as prx_flags argument an invalid int32_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_client_getnameinfo_flags__arg_prx_flags_null)
{
    static const int k_flags_valid = NI_NAMEREQD;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_client_getnameinfo_flags(k_flags_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_socket_address happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_address__success_1)
{
    prx_socket_address_t prx_address_valid;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    socklen_t sa_len_valid = 64;
    int32_t result;

    prx_address_valid.un.family = prx_address_family_inet6;
    
    // arrange 

    // act 
    result = pal_os_from_prx_socket_address(&prx_address_valid, sa_valid, &sa_len_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, sizeof(struct sockaddr_in6), sa_len_valid);
    ASSERT_ARE_EQUAL(int, AF_INET6, sa_valid->sa_family);
}

// 
// Test pal_os_from_prx_socket_address happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_address__success_2)
{
    prx_socket_address_t prx_address_valid;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    socklen_t sa_len_valid = 64;
    int32_t result;

    prx_address_valid.un.family = prx_address_family_inet;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_address(&prx_address_valid, sa_valid, &sa_len_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, sizeof(struct sockaddr_in), sa_len_valid);
    ASSERT_ARE_EQUAL(int, AF_INET, sa_valid->sa_family);
}

// 
// Test pal_os_from_prx_socket_address passing as prx_address argument an invalid const prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_address__arg_prx_address_invalid)
{
    prx_socket_address_t prx_address_valid;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    socklen_t sa_len_valid = sizeof(UT_MEM);
    int32_t result;

    prx_address_valid.un.family = prx_address_family_proxy;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_address(&prx_address_valid, sa_valid, &sa_len_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_from_prx_socket_address passing as sa argument an invalid struct sockaddr* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_address__arg_sa_null)
{
    prx_socket_address_t prx_address_valid;
    socklen_t sa_len_valid = 64;
    int32_t result;

    prx_address_valid.un.family = prx_address_family_inet;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_address(&prx_address_valid, NULL, &sa_len_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_socket_address passing as sa_len argument an invalid socklen_t* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_address__arg_sa_len_null)
{
    prx_socket_address_t prx_address_valid;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    int32_t result;

    prx_address_valid.un.family = prx_address_family_inet;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_address(&prx_address_valid, sa_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_socket_address passing as sa_len argument an invalid socklen_t* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_address__arg_sa_len_invalid_1)
{
    prx_socket_address_t prx_address_valid;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    socklen_t sa_len_valid = sizeof(struct sockaddr_in) - 1;
    int32_t result;

    prx_address_valid.un.family = prx_address_family_inet;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_address(&prx_address_valid, sa_valid, &sa_len_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_socket_address passing as sa_len argument an invalid socklen_t* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_address__arg_sa_len_invalid_2)
{
    prx_socket_address_t prx_address_valid;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    socklen_t sa_len_valid = sizeof(struct sockaddr_in);
    int32_t result;

    prx_address_valid.un.family = prx_address_family_inet6;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_address(&prx_address_valid, sa_valid, &sa_len_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_socket_address happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__success_1)
{
    static const socklen_t k_sa_len_valid = sizeof(struct sockaddr_in6);
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    prx_socket_address_t prx_address_valid;
    int32_t result;

    sa_valid->sa_family = AF_INET6;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(sa_valid, k_sa_len_valid, &prx_address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int32_t, prx_address_family_inet6, prx_address_valid.un.family);
}

// 
// Test pal_os_to_prx_socket_address happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__success_2)
{
    static const socklen_t k_sa_len_valid = sizeof(struct sockaddr_in);
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    prx_socket_address_t prx_address_valid;
    int32_t result;

    sa_valid->sa_family = AF_INET;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(sa_valid, k_sa_len_valid, &prx_address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int32_t, prx_address_family_inet, prx_address_valid.un.family);
}

// 
// Test pal_os_to_prx_socket_address passing as sa argument an invalid const struct sockaddr* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__arg_sa_null)
{
    static const socklen_t k_sa_len_valid = sizeof(struct sockaddr_in);
    prx_socket_address_t prx_address_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(NULL, k_sa_len_valid, &prx_address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_socket_address passing as sa argument an invalid const struct sockaddr* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__arg_sa_invalid)
{
    static const socklen_t k_sa_len_valid = sizeof(struct sockaddr_in);
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    prx_socket_address_t prx_address_valid;
    int32_t result;

    sa_valid->sa_family = 0;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(sa_valid, k_sa_len_valid, &prx_address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_socket_address passing as sa_len argument an invalid socklen_t value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__arg_sa_len_invalid_1)
{
    static const socklen_t k_sa_len_invalid = sizeof(struct sockaddr) - 1;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    prx_socket_address_t prx_address_valid;
    int32_t result;

    sa_valid->sa_family = AF_INET;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(sa_valid, k_sa_len_invalid, &prx_address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_socket_address passing as sa_len argument an invalid socklen_t value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__arg_sa_len_invalid_2)
{
    static const socklen_t k_sa_len_invalid = sizeof(struct sockaddr_in) - 1;
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    prx_socket_address_t prx_address_valid;
    int32_t result;

    sa_valid->sa_family = AF_INET;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(sa_valid, k_sa_len_invalid, &prx_address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_socket_address passing as sa_len argument an invalid socklen_t value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__arg_sa_len_invalid_3)
{
    static const socklen_t k_sa_len_invalid = sizeof(struct sockaddr_in);
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    prx_socket_address_t prx_address_valid;
    int32_t result;

    sa_valid->sa_family = AF_INET6;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(sa_valid, k_sa_len_invalid, &prx_address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_socket_address passing as prx_address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_address__arg_prx_address_null)
{
    static const socklen_t k_sa_len_valid = sizeof(struct sockaddr_in);
    struct sockaddr* sa_valid = (struct sockaddr*)UT_MEM;
    int32_t result;

    sa_valid->sa_family = AF_INET;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_address(sa_valid, k_sa_len_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_message_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_message_flags__success_1)
{
    int32_t result;
    int plat_flags_valid;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(int32_t, input,
        prx_msg_flag_oob, prx_msg_flag_peek, prx_msg_flag_dontroute, prx_msg_flag_trunc, prx_msg_flag_ctrunc);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_message_flags(input, &plat_flags_valid);

    // assert 
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    UMOCK_C_RANGE_TESTS_ASSERT(int, plat_flags_valid, 0,
        MSG_OOB, MSG_PEEK, MSG_DONTROUTE, MSG_TRUNC, MSG_CTRUNC);
}

// 
// Test pal_os_from_prx_message_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_message_flags__success_2)
{
    int32_t result;
    int plat_flags_valid;

    // arrange 

    // act 
    result = pal_os_from_prx_message_flags(prx_msg_flag_oob | prx_msg_flag_peek | prx_msg_flag_dontroute | 
        prx_msg_flag_trunc | prx_msg_flag_ctrunc, &plat_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, MSG_OOB | MSG_PEEK | MSG_DONTROUTE |
        MSG_TRUNC | MSG_CTRUNC, plat_flags_valid);
}

// 
// Test pal_os_from_prx_message_flags passing as flags argument an invalid int32_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_message_flags__arg_flags_invalid)
{
    int32_t result;
    int plat_flags_valid;

    // arrange 

    // act 
    result = pal_os_from_prx_message_flags(-1, &plat_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_os_from_prx_message_flags passing as platf_flags argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_message_flags__arg_platf_flags_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_message_flags(prx_msg_flag_oob, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_message_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_message_flags__success_1)
{
    int32_t result;
    int32_t prx_flags_valid;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(int, input, 
        MSG_OOB, MSG_PEEK, MSG_DONTROUTE, MSG_TRUNC, MSG_CTRUNC);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_to_prx_message_flags(input, &prx_flags_valid);

    // assert 
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, prx_flags_valid, 0,
        prx_msg_flag_oob, prx_msg_flag_peek, prx_msg_flag_dontroute, prx_msg_flag_trunc, prx_msg_flag_ctrunc);
}

// 
// Test pal_os_to_prx_message_flags happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_message_flags__success_2)
{
    int32_t result;
    int32_t prx_flags_valid;

    // arrange 

    // act 
    result = pal_os_to_prx_message_flags(
        MSG_OOB | MSG_PEEK | MSG_DONTROUTE | MSG_TRUNC | MSG_CTRUNC, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int32_t,
        prx_msg_flag_oob | prx_msg_flag_peek | prx_msg_flag_dontroute | prx_msg_flag_trunc | prx_msg_flag_ctrunc,
        prx_flags_valid);
}

// 
// Test pal_os_to_prx_message_flags passing as flags argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_message_flags__arg_flags_invalid)
{
    int32_t result;
    int32_t prx_flags_valid;

    // arrange 

    // act 
    result = pal_os_to_prx_message_flags(-1, &prx_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_message_flags passing as prx_flags argument an invalid int32_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_message_flags__arg_prx_flags_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_message_flags(MSG_PEEK, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__success)
{
    static const int k_opt_lvl_valid = SOL_SOCKET;
    static const int k_opt_name_valid = SO_DEBUG;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_option(k_opt_lvl_valid, k_opt_name_valid, &socket_option_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_os_to_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__success_1)
{
    static const int k_opt_lvl_valid = SOL_SOCKET;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, SO_RANGE_BEGIN, SO_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    socket_option_valid = (prx_socket_option_t)-1;
    result = pal_os_to_prx_socket_option(k_opt_lvl_valid, input, &socket_option_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, socket_option_valid, (prx_socket_option_t)-1,
        prx_so_debug,     prx_so_acceptconn, prx_so_reuseaddr,  prx_so_keepalive,  prx_so_dontroute,
        prx_so_broadcast, prx_so_linger,     prx_so_oobinline,  prx_so_sndbuf,     prx_so_rcvbuf, 
        prx_so_sndlowat,  prx_so_rcvlowat,   prx_so_sndtimeo,   prx_so_rcvtimeo,   prx_so_error, 
        prx_so_type);
}

// 
// Test pal_os_to_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__success_2)
{
    static const int k_opt_lvl_valid = IPPROTO_IP;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, SO_RANGE_BEGIN, SO_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    socket_option_valid = (prx_socket_option_t)-1;
    result = pal_os_to_prx_socket_option(k_opt_lvl_valid, input, &socket_option_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, socket_option_valid, (prx_socket_option_t)-1,
        prx_so_ip_options, prx_so_ip_hdrincl, prx_so_ip_tos, prx_so_ip_ttl, prx_so_ip_multicast_ttl,
        prx_so_ip_multicast_loop, prx_so_ip_pktinfo);
}

// 
// Test pal_os_to_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__success_3)
{
    static const int k_opt_lvl_valid = IPPROTO_IPV6;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, SO_RANGE_BEGIN, SO_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    socket_option_valid = (prx_socket_option_t)-1;
    result = pal_os_to_prx_socket_option(k_opt_lvl_valid, input, &socket_option_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, socket_option_valid, (prx_socket_option_t)-1,
        prx_so_ipv6_hoplimit, prx_so_ipv6_v6only);
}

// 
// Test pal_os_to_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__success_4)
{
    static const int k_opt_lvl_valid = IPPROTO_TCP;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, SO_RANGE_BEGIN, SO_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    socket_option_valid = (prx_socket_option_t)-1;
    result = pal_os_to_prx_socket_option(k_opt_lvl_valid, input, &socket_option_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, socket_option_valid, (prx_socket_option_t)-1,
        prx_so_tcp_nodelay);
}

// 
// Test pal_os_to_prx_socket_option passing as opt_lvl argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__arg_opt_lvl_invalid_1)
{
    static const int k_opt_lvl_invalid = -1;
    static const int k_opt_name_valid = SO_DEBUG;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_option(k_opt_lvl_invalid, k_opt_name_valid, &socket_option_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_socket_option passing as opt_lvl argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__arg_opt_lvl_invalid_2)
{
    static const int k_opt_lvl_invalid = IPPROTO_UDP;
    static const int k_opt_name_valid = SO_DEBUG;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_option(k_opt_lvl_invalid, k_opt_name_valid, &socket_option_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_socket_option passing as opt_name argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__arg_opt_name_invalid)
{
    static const int k_opt_lvl_valid = SOL_SOCKET;
    static const int k_opt_name_invalid = IP_OPTIONS;
    prx_socket_option_t socket_option_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_option(k_opt_lvl_valid, k_opt_name_invalid, &socket_option_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_socket_option passing as socket_option argument an invalid prx_socket_option_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_option__arg_socket_option_null)
{
    static const int k_opt_lvl_valid = SOL_SOCKET;
    static const int k_opt_name_valid = SO_DEBUG;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_option(k_opt_lvl_valid, k_opt_name_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__success)
{
    static const prx_socket_option_t k_socket_option_valid = prx_so_debug;
    int opt_name_valid;
    int opt_lvl_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_option(k_socket_option_valid, &opt_lvl_valid, &opt_name_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, SO_DEBUG, opt_name_valid);
    ASSERT_ARE_EQUAL(int, SOL_SOCKET, opt_lvl_valid);
}

// 
// Test pal_os_from_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__success_1)
{
    int opt_name_valid;
    int opt_lvl_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_socket_option_t, input,
        prx_so_debug, prx_so_acceptconn, prx_so_reuseaddr, prx_so_keepalive, prx_so_dontroute,
        prx_so_broadcast, prx_so_linger, prx_so_oobinline, prx_so_sndbuf, prx_so_rcvbuf,
        prx_so_sndlowat, prx_so_rcvlowat, prx_so_sndtimeo, prx_so_rcvtimeo, prx_so_error,
        prx_so_type);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_socket_option(input, &opt_lvl_valid, &opt_name_valid);

    // assert 
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, SOL_SOCKET, opt_lvl_valid);
    UMOCK_C_RANGE_TESTS_ASSERT(int, opt_name_valid, -1,
        SO_DEBUG, SO_ACCEPTCONN, SO_REUSEADDR, SO_KEEPALIVE, SO_DONTROUTE,
        SO_BROADCAST, SO_LINGER, SO_OOBINLINE, SO_SNDBUF, SO_RCVBUF,
        SO_SNDLOWAT, SO_RCVLOWAT, SO_SNDTIMEO, SO_RCVTIMEO, SO_ERROR,
        SO_TYPE);
}

// 
// Test pal_os_from_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__success_2)
{
    int opt_name_valid;
    int opt_lvl_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_socket_option_t, input,
        prx_so_ip_options, prx_so_ip_hdrincl, prx_so_ip_tos, prx_so_ip_ttl, prx_so_ip_multicast_ttl,
        prx_so_ip_multicast_loop, prx_so_ip_pktinfo);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_socket_option(input, &opt_lvl_valid, &opt_name_valid);

    // assert 
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, IPPROTO_IP, opt_lvl_valid);
    UMOCK_C_RANGE_TESTS_ASSERT(int, opt_name_valid, -1,
        IP_OPTIONS, IP_HDRINCL, IP_TOS, IP_TTL, IP_MULTICAST_TTL, IP_MULTICAST_LOOP, IP_PKTINFO);
}

// 
// Test pal_os_from_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__success_3)
{
    int opt_name_valid;
    int opt_lvl_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_socket_option_t, input,
        prx_so_ipv6_hoplimit, prx_so_ipv6_v6only);

    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_socket_option(input, &opt_lvl_valid, &opt_name_valid);

    // assert 
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, IPPROTO_IPV6, opt_lvl_valid);
    UMOCK_C_RANGE_TESTS_ASSERT(int, opt_name_valid, -1, IPV6_HOPLIMIT, IPV6_V6ONLY);
}

// 
// Test pal_os_from_prx_socket_option happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__success_4)
{
    int opt_name_valid;
    int opt_lvl_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_socket_option_t, input, prx_so_tcp_nodelay);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_socket_option(input, &opt_lvl_valid, &opt_name_valid);

    // assert 
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(int, IPPROTO_TCP, opt_lvl_valid);
    UMOCK_C_RANGE_TESTS_ASSERT(int, opt_name_valid, -1, TCP_NODELAY);
}

// 
// Test pal_os_from_prx_socket_option passing as socket_option argument an invalid prx_socket_option_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__arg_socket_option_invalid)
{
    static const prx_socket_option_t k_socket_option_invalid = prx_so_available;
    int opt_name_valid;
    int opt_lvl_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_option(k_socket_option_invalid, &opt_lvl_valid, &opt_name_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_from_prx_socket_option passing as opt_lvl argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__arg_opt_lvl_null)
{
    static const prx_socket_option_t k_socket_option_valid = prx_so_debug;
    int opt_name_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_option(k_socket_option_valid, NULL, &opt_name_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_socket_option passing as opt_name argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_option__arg_opt_name_null)
{
    static const prx_socket_option_t k_socket_option_valid = prx_so_debug;
    int opt_lvl_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_option(k_socket_option_valid, &opt_lvl_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_shutdown_op happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_shutdown_op__success_1)
{
    prx_shutdown_op_t prx_shutdown_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_shutdown_op(SHUT_RD, &prx_shutdown_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_os_to_prx_shutdown_op happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_shutdown_op__success_2)
{
    prx_shutdown_op_t prx_shutdown_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, SHUT_RANGE_BEGIN, SHUT_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    prx_shutdown_valid = (prx_shutdown_op_t)-1;
    result = pal_os_to_prx_shutdown_op(input, &prx_shutdown_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, prx_shutdown_valid, -1,
        prx_shutdown_op_read, prx_shutdown_op_write, prx_shutdown_op_both);
}

// 
// Test pal_os_to_prx_shutdown_op passing as platform_shutdown argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_shutdown_op__arg_platform_shutdown_invalid)
{
    prx_shutdown_op_t prx_shutdown_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_shutdown_op(-1, &prx_shutdown_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_os_to_prx_shutdown_op passing as prx_shutdown argument an invalid prx_shutdown_op_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_shutdown_op__arg_prx_shutdown_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_shutdown_op(SHUT_RD, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_shutdown_op happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_shutdown_op__success)
{
    int plat_shutdown_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_shutdown_op_t, input, 
        prx_shutdown_op_read, prx_shutdown_op_write, prx_shutdown_op_both);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_shutdown_op(input, &plat_shutdown_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    UMOCK_C_RANGE_TESTS_ASSERT(int, plat_shutdown_valid, -1, SHUT_RD, SHUT_WR, SHUT_RDWR);
}

// 
// Test pal_os_from_prx_shutdown_op passing as prx_shutdown argument an invalid prx_shutdown_op_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_shutdown_op__arg_prx_shutdown_invalid)
{
    int plat_shutdown_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_shutdown_op((prx_shutdown_op_t)-1, &plat_shutdown_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_os_from_prx_shutdown_op passing as platform_shutdown argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_shutdown_op__arg_platform_shutdown_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_shutdown_op(prx_shutdown_op_write, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_address_family happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_address_family__success_1)
{
    prx_address_family_t prx_af_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_address_family(AF_INET, &prx_af_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_os_to_prx_address_family happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_address_family__success_2)
{
    prx_address_family_t prx_af_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, AF_RANGE_BEGIN, AF_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    prx_af_valid = (prx_address_family_t)-1;
    result = pal_os_to_prx_address_family(input, &prx_af_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, prx_af_valid, -1,
        prx_address_family_unspec, prx_address_family_inet, prx_address_family_inet6, prx_address_family_unix);
}

// 
// Test pal_os_to_prx_address_family passing as platform_af argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_address_family__arg_platform_af_invalid)
{
    prx_address_family_t prx_af_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_address_family(-1, &prx_af_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_address_family passing as prx_af argument an invalid prx_address_family_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_address_family__arg_prx_af_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_address_family(AF_UNIX, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_address_family happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_address_family__success)
{
    int platform_af_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_address_family_t, input,
        prx_address_family_unspec, prx_address_family_unix, prx_address_family_inet, prx_address_family_inet6);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_address_family(input, &platform_af_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    UMOCK_C_RANGE_TESTS_ASSERT(int, platform_af_valid, -1, AF_UNSPEC, AF_UNIX, AF_INET, AF_INET6);
}

// 
// Test pal_os_from_prx_address_family passing as prx_af argument an invalid prx_address_family_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_address_family__arg_prx_af_invalid)
{
    int platform_af_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_address_family(prx_address_family_proxy, &platform_af_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_os_from_prx_address_family passing as platform_af argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_address_family__arg_platform_af_invalid)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_address_family(prx_address_family_inet, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_protocol_type happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_protocol_type__success_1)
{
    prx_protocol_type_t prx_proto_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_protocol_type(IPPROTO_TCP, &prx_proto_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_os_to_prx_protocol_type happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_protocol_type__success_2)
{
    prx_protocol_type_t prx_proto_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, IPPROTO_RANGE_BEGIN, IPPROTO_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    prx_proto_valid = (prx_protocol_type_t)-1;
    result = pal_os_to_prx_protocol_type(input, &prx_proto_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, prx_proto_valid, -1,
        prx_protocol_type_unspecified, prx_protocol_type_udp, prx_protocol_type_tcp, 
        prx_protocol_type_icmp, prx_protocol_type_icmpv6);
}

// 
// Test pal_os_to_prx_protocol_type passing as platform_proto argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_protocol_type__arg_platform_proto_invalid)
{
    prx_protocol_type_t prx_proto_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_protocol_type(IPPROTO_IP, &prx_proto_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_protocol_type passing as prx_proto argument an invalid prx_protocol_type_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_protocol_type__arg_prx_proto_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_protocol_type(IPPROTO_UDP, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_protocol_type happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_protocol_type__success)
{
    int platform_proto_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_protocol_type_t, input, 
        prx_protocol_type_udp, prx_protocol_type_tcp, prx_protocol_type_icmp, 
        prx_protocol_type_icmpv6, prx_protocol_type_unspecified);

    // act 
    result = pal_os_from_prx_protocol_type(input, &platform_proto_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    UMOCK_C_RANGE_TESTS_ASSERT(int, platform_proto_valid, -1, 
        IPPROTO_UDP, IPPROTO_TCP, IPPROTO_ICMP, IPPROTO_ICMPV6, 0);
}

// 
// Test pal_os_from_prx_protocol_type passing as prx_proto argument an invalid prx_protocol_type_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_protocol_type__arg_prx_proto_invalid)
{
    int platform_proto_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_protocol_type((prx_protocol_type_t)-1, &platform_proto_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_os_from_prx_protocol_type passing as platform_proto argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_protocol_type__arg_platform_proto_invalid)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_protocol_type(prx_protocol_type_tcp, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_to_prx_socket_type happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_type__success_1)
{
    prx_socket_type_t prx_socktype_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_type(SOCK_DGRAM, &prx_socktype_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_os_to_prx_socket_type happy path 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_type__success_2)
{
    prx_socket_type_t prx_socktype_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE_FROM_TO(int, input, SOCK_RANGE_BEGIN, SOCK_RANGE_END);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    prx_socktype_valid = (prx_socket_type_t)-1;
    result = pal_os_to_prx_socket_type(input, &prx_socktype_valid);

    // assert 
    UMOCK_C_RANGE_TESTS_ASSERT(int32_t, prx_socktype_valid, -1,
        prx_socket_type_dgram, prx_socket_type_stream, prx_socket_type_raw,
        prx_socket_type_seqpacket, prx_socket_type_rdm);
}

// 
// Test pal_os_to_prx_socket_type passing as platform_socktype argument an invalid int value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_type__arg_platform_socktype_invalid)
{
    prx_socket_type_t prx_socktype_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_type(-1, &prx_socktype_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_os_to_prx_socket_type passing as prx_socktype argument an invalid prx_socket_type_t* value 
// 
TEST_FUNCTION(pal_posix_os_to_prx_socket_type__arg_prx_socktype_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_to_prx_socket_type(SOCK_RAW, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_os_from_prx_socket_type happy path 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_type__success)
{
    int platform_socktype_valid;
    int32_t result;

    // arrange 
    UMOCK_C_RANGE_TESTS_ARRANGE(prx_socket_type_t, input,
        prx_socket_type_stream, prx_socket_type_dgram, prx_socket_type_raw,
        prx_socket_type_rdm, prx_socket_type_seqpacket);

    // act 
    UMOCK_C_RANGE_TESTS_ACT();
    result = pal_os_from_prx_socket_type(input, &platform_socktype_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    UMOCK_C_RANGE_TESTS_ASSERT(int, platform_socktype_valid, -1,
        SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_RDM, SOCK_SEQPACKET);
}

// 
// Test pal_os_from_prx_socket_type passing as prx_socktype argument an invalid prx_socket_type_t value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_type__arg_prx_socktype_invalid)
{
    int platform_socktype_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_type((prx_socket_type_t)-1, &platform_socktype_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_os_from_prx_socket_type passing as platform_socktype argument an invalid int* value 
// 
TEST_FUNCTION(pal_posix_os_from_prx_socket_type__arg_platform_socktype_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_os_from_prx_socket_type(prx_socket_type_stream, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_pton happy path 
// 
TEST_FUNCTION(pal_posix_pton__success_1)
{
    static const char* k_addr_string_valid = "some_address";
    struct addrinfo ai_info_valid, *ai_info_ptr_valid = &ai_info_valid;
    struct sockaddr_in6 sock_addr_valid;
    prx_socket_address_t address_valid;
    int32_t result;

    sock_addr_valid.sin6_family = AF_INET6;

    ai_info_valid.ai_next = NULL;
    ai_info_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 
    STRICT_EXPECTED_CALL(getaddrinfo(k_addr_string_valid, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));

    // act 
    result = pal_pton(k_addr_string_valid, &address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_pton happy path 
// 
TEST_FUNCTION(pal_posix_pton__success_2)
{
    static const char* k_addr_string_valid = "some_other_address";
    struct addrinfo ai_info_valid, *ai_info_ptr_valid = &ai_info_valid;
    struct sockaddr_in sock_addr_valid;
    prx_socket_address_t address_valid;
    int32_t result;

    sock_addr_valid.sin_family = AF_INET;

    ai_info_valid.ai_next = NULL;
    ai_info_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 
    STRICT_EXPECTED_CALL(getaddrinfo(k_addr_string_valid, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));

    // act 
    result = pal_pton(k_addr_string_valid, &address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_pton passing as addr_string argument an invalid const char* value 
// 
TEST_FUNCTION(pal_posix_pton__arg_addr_string_null)
{
    prx_socket_address_t address_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_pton(NULL, &address_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_pton passing as address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_posix_pton__arg_address_invalid)
{
    static const char* k_addr_string_valid = "some_other_address";
    int32_t result;

    // arrange 

    // act 
    result = pal_pton(k_addr_string_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_pton unhappy path 
// 
TEST_FUNCTION(pal_posix_pton__neg)
{
    static const char* k_addr_string_valid = "some_other_address";
    struct addrinfo ai_info_valid, *ai_info_ptr_valid = &ai_info_valid;
    struct sockaddr_in sock_addr_valid;
    prx_socket_address_t address_valid;
    int32_t result;

    sock_addr_valid.sin_family = AF_INET;

    ai_info_valid.ai_next = NULL;
    ai_info_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    STRICT_EXPECTED_CALL(getaddrinfo(k_addr_string_valid, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .IgnoreArgument(3)
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .SetReturn(0)
        .SetFailReturn(EAI_NONAME);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_pton(k_addr_string_valid, &address_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_host_unknown, er_ok);
}

// 
// Test pal_ntop passing as address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_posix_ntop__arg_address_invalid)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_ntop(NULL, UT_MEM, 256);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_ntop passing as addr_string argument an invalid char* value 
// 
TEST_FUNCTION(pal_posix_ntop__arg_addr_string_invalid)
{
    prx_socket_address_t sock_addr_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_ntop(&sock_addr_valid, NULL, 256);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getaddrinfo happy path 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__success)
{
    static const char* k_host_name_valid = "some_host_name";
    static const char* k_service_valid = "666";
    static const char* k_canon_name_valid = "";
    static const prx_address_family_t k_family_valid = prx_address_family_unspec;
    static const uint32_t k_flags_valid = prx_ai_passive;
    struct addrinfo ai_info_valid[3], *ai_info_ptr_valid = &ai_info_valid[0];
    struct sockaddr_in6 sock_addr_valid;
    prx_addrinfo_t* info_valid;
    size_t info_count_valid;
    int32_t result;

    sock_addr_valid.sin6_family = AF_INET6;
    ai_info_valid[0].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[0].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[0].ai_canonname = NULL;
    ai_info_valid[0].ai_next = &ai_info_valid[1];
    ai_info_valid[1].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[1].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[1].ai_canonname = (char*)k_canon_name_valid;
    ai_info_valid[1].ai_next = &ai_info_valid[2];
    ai_info_valid[2].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[2].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[2].ai_canonname = NULL;
    ai_info_valid[2].ai_next = NULL;

    UT_MEM_ALLOCED = malloc(20000);

    // arrange 
    STRICT_EXPECTED_CALL(getaddrinfo(k_host_name_valid, k_service_valid, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .IgnoreArgument(3)
        .SetReturn(0);
    STRICT_EXPECTED_CALL(h_realloc(4 * sizeof(prx_addrinfo_t), NULL, true, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(4).IgnoreArgument(5).IgnoreArgument(6)
        .SetReturn((void*)UT_MEM_ALLOCED);
    STRICT_EXPECTED_CALL(string_clone(k_canon_name_valid, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_copy(&k_canon_name_valid, sizeof(k_canon_name_valid))
        .SetReturn(er_ok);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));

    // act 
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_valid, &info_valid, &info_count_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_IS_TRUE(info_count_valid == 3);
}

// 
// Test pal_getaddrinfo passing as host_name argument an invalid const char* value 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__arg_host_name_invalid)
{
    static const char* k_service_valid = "11";
    static const prx_address_family_t k_family_valid = prx_address_family_inet;
    static const uint32_t k_flags_valid = 0;
    struct addrinfo ai_info_valid, *ai_info_ptr_valid = &ai_info_valid;
    struct sockaddr_in sock_addr_valid;
    prx_addrinfo_t* info_valid;
    size_t info_count_valid;
    int32_t result;

    sock_addr_valid.sin_family = AF_INET;
    ai_info_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid.ai_canonname = NULL;
    ai_info_valid.ai_next = NULL;

    UT_MEM_ALLOCED = malloc(5000);

    // arrange 
    STRICT_EXPECTED_CALL(getaddrinfo(NULL, k_service_valid, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .IgnoreArgument(3)
        .SetReturn(0);
    STRICT_EXPECTED_CALL(h_realloc(2 * sizeof(prx_addrinfo_t), NULL, true, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(4).IgnoreArgument(5).IgnoreArgument(6)
        .SetReturn((void*)UT_MEM_ALLOCED);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));

    // act 
    result = pal_getaddrinfo(NULL, k_service_valid, k_family_valid, k_flags_valid, &info_valid, &info_count_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_IS_TRUE(info_count_valid == 1);
}

// 
// Test pal_getaddrinfo passing as service argument an invalid const char* value 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__arg_service_null)
{
    static const char* k_host_name_valid = "some_host_name";
    static const prx_address_family_t k_family_valid = prx_address_family_inet;
    static const uint32_t k_flags_valid = 0;
    struct addrinfo ai_info_valid[2], *ai_info_ptr_valid = &ai_info_valid[0];
    struct sockaddr_in6 sock_addr_valid;
    prx_addrinfo_t* info_valid;
    size_t info_count_valid;
    int32_t result;

    sock_addr_valid.sin6_family = AF_INET6;
    ai_info_valid[0].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[0].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[0].ai_canonname = NULL;
    ai_info_valid[0].ai_next = &ai_info_valid[1];
    ai_info_valid[1].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[1].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[1].ai_canonname = NULL;
    ai_info_valid[1].ai_next = NULL;

    UT_MEM_ALLOCED = malloc(20000);

    // arrange 
    STRICT_EXPECTED_CALL(getaddrinfo(k_host_name_valid, NULL, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .IgnoreArgument(3)
        .SetReturn(0);
    STRICT_EXPECTED_CALL(h_realloc(3 * sizeof(prx_addrinfo_t), NULL, true, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(4).IgnoreArgument(5).IgnoreArgument(6)
        .SetReturn((void*)UT_MEM_ALLOCED);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));

    // act 
    result = pal_getaddrinfo(k_host_name_valid, NULL, k_family_valid, k_flags_valid, &info_valid, &info_count_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_IS_TRUE(info_count_valid == 2);
}

// 
// Test pal_getaddrinfo passing as family argument an invalid prx_address_family_t value 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__arg_family_invalid)
{
    static const char* k_host_name_valid = "some_host_name";
    static const char* k_service_valid = "666";
    static const char* k_canon_name_valid = "";
    static const prx_address_family_t k_family_invalid = prx_address_family_proxy;
    static const uint32_t k_flags_valid = 0;
    prx_addrinfo_t* info_valid;
    size_t info_count_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_invalid, k_flags_valid, &info_valid, &info_count_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_getaddrinfo passing as flags argument an invalid uint32_t value 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__arg_flags_invalid)
{
    static const char* k_host_name_valid = "some_host_name";
    static const char* k_service_valid = "666";
    static const char* k_canon_name_valid = "";
    static const prx_address_family_t k_family_valid = prx_address_family_unspec;
    static const uint32_t k_flags_invalid = (uint32_t)-1;
    prx_addrinfo_t* info_valid;
    size_t info_count_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_invalid, &info_valid, &info_count_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_getaddrinfo passing as info argument an invalid prx_addrinfo_t** value 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__arg_info_null)
{
    static const char* k_host_name_valid = "some_host_name";
    static const char* k_service_valid = "666";
    static const char* k_canon_name_valid = "";
    static const prx_address_family_t k_family_valid = prx_address_family_unspec;
    static const uint32_t k_flags_valid = 0;
    size_t info_count_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_valid, NULL, &info_count_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getaddrinfo passing as info_count argument an invalid size_t* value 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__arg_info_count_invalid)
{
    static const char* k_host_name_valid = "some_host_name";
    static const char* k_service_valid = "666";
    static const char* k_canon_name_valid = "";
    static const prx_address_family_t k_family_valid = prx_address_family_unspec;
    static const uint32_t k_flags_valid = 0;
    prx_addrinfo_t* info_valid;
    int32_t result;

    // arrange 

    // act 
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_valid, &info_valid, NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getaddrinfo unhappy path 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__neg_1)
{
    static const char* k_host_name_valid = "some_host_name";
    static const char* k_service_valid = "234";
    static const char* k_canon_name_valid = "peipipeip";
    static const prx_address_family_t k_family_valid = prx_address_family_inet;
    static const uint32_t k_flags_valid = 0;
    struct addrinfo ai_info_valid, *ai_info_ptr_valid = &ai_info_valid;
    struct sockaddr_in sock_addr_valid;
    prx_addrinfo_t* info_valid;
    size_t info_count_valid;
    int32_t result;

    sock_addr_valid.sin_family = AF_UNSPEC;
    ai_info_valid.ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid.ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid.ai_canonname = (char*)k_canon_name_valid;
    ai_info_valid.ai_next = NULL;

    UT_MEM_ALLOCED = malloc(5000);

    // arrange 
    STRICT_EXPECTED_CALL(getaddrinfo(k_host_name_valid, k_service_valid, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .IgnoreArgument(3)
        .SetReturn(0);
    STRICT_EXPECTED_CALL(h_realloc(2 * sizeof(prx_addrinfo_t), NULL, true, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(4).IgnoreArgument(5).IgnoreArgument(6)
        .SetReturn((void*)UT_MEM_ALLOCED);
    STRICT_EXPECTED_CALL(h_free(UT_MEM_ALLOCED, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2).IgnoreArgument(3).IgnoreArgument(4);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));
    STRICT_EXPECTED_CALL(pal_caps())
        .SetReturn(0);

    // act 
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_valid, &info_valid, &info_count_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_getaddrinfo unhappy path 
// 
TEST_FUNCTION(pal_posix_getaddrinfo__neg_2)
{
    static const char* k_host_name_valid = "some_host_name";
    static const char* k_service_valid = "666";
    static const char* k_canon_name_valid = "";
    static const prx_address_family_t k_family_valid = prx_address_family_unspec;
    static const uint32_t k_flags_valid = prx_ai_passive;
    struct addrinfo ai_info_valid[3], *ai_info_ptr_valid = &ai_info_valid[0];
    struct sockaddr_in6 sock_addr_valid;
    prx_addrinfo_t* info_valid;
    size_t info_count_valid;
    int32_t result;

    sock_addr_valid.sin6_family = AF_INET6;
    ai_info_valid[0].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[0].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[0].ai_canonname = NULL;
    ai_info_valid[0].ai_next = &ai_info_valid[1];
    ai_info_valid[1].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[1].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[1].ai_canonname = (char*)k_canon_name_valid;
    ai_info_valid[1].ai_next = &ai_info_valid[2];
    ai_info_valid[2].ai_addrlen = sizeof(sock_addr_valid);
    ai_info_valid[2].ai_addr = (struct sockaddr*)&sock_addr_valid;
    ai_info_valid[2].ai_canonname = NULL;
    ai_info_valid[2].ai_next = NULL;

    UT_MEM_ALLOCED = malloc(20000);

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    STRICT_EXPECTED_CALL(getaddrinfo(k_host_name_valid, k_service_valid, IGNORED_PTR_ARG, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_ai(&ai_info_ptr_valid, sizeof(ai_info_ptr_valid))
        .IgnoreArgument(3)
        .SetReturn(0)
        .SetFailReturn(EAI_FAIL);
    STRICT_EXPECTED_CALL(h_realloc(4 * sizeof(prx_addrinfo_t), NULL, true, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(4).IgnoreArgument(5).IgnoreArgument(6)
        .SetReturn((void*)UT_MEM_ALLOCED)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(string_clone(k_canon_name_valid, IGNORED_PTR_ARG))
        .CopyOutArgumentBuffer_copy(&k_canon_name_valid, sizeof(k_canon_name_valid))
        .SetReturn(er_ok)
        .SetFailReturn(er_out_of_memory);
    STRICT_EXPECTED_CALL(freeaddrinfo(ai_info_ptr_valid));
    STRICT_EXPECTED_CALL(pal_caps())
        .SetReturn(0);

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_valid, &info_valid, &info_count_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_fatal, er_out_of_memory, er_ok);
}

// 
// Test pal_freeaddrinfo happy path 
// 
TEST_FUNCTION(pal_posix_freeaddrinfo__success_1)
{
    prx_addrinfo_t addr_info_valid[6];
    int32_t result;

    for (size_t i = 0; i < _countof(addr_info_valid); i++)
    {
        addr_info_valid[i].reserved = (i == _countof(addr_info_valid) - 1) ? 1 : 0;
        addr_info_valid[i].name = (char*)i+1;
    }

    // arrange 
    for (size_t i = 1; i < _countof(addr_info_valid); i++)
    {
        STRICT_EXPECTED_CALL(h_free((void*)i, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
            .IgnoreArgument(2).IgnoreArgument(3).IgnoreArgument(4);
    }
    STRICT_EXPECTED_CALL(h_free(&addr_info_valid, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2).IgnoreArgument(3).IgnoreArgument(4);

    // act 
    result = pal_freeaddrinfo(addr_info_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_freeaddrinfo happy path 
// 
TEST_FUNCTION(pal_posix_freeaddrinfo__success_2)
{
    prx_addrinfo_t addr_info_valid[6];
    int32_t result;

    for (size_t i = 0; i < _countof(addr_info_valid); i++)
    {
        addr_info_valid[i].reserved = (i == _countof(addr_info_valid) - 1) ? 0 : 1;
        addr_info_valid[i].name = NULL;
    }

    // arrange 
    STRICT_EXPECTED_CALL(h_free(&addr_info_valid, IGNORED_PTR_ARG, IGNORED_NUM_ARG, IGNORED_NUM_ARG))
        .IgnoreArgument(2).IgnoreArgument(3).IgnoreArgument(4);

    // act 
    result = pal_freeaddrinfo(addr_info_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
}

// 
// Test pal_freeaddrinfo passing as info argument an invalid prx_addrinfo_t* value 
// 
TEST_FUNCTION(pal_posix_freeaddrinfo__arg_info_null)
{
    int32_t result;

    // arrange 

    // act 
    result = pal_freeaddrinfo(NULL);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getnameinfo happy path 
// 
TEST_FUNCTION(pal_posix_getnameinfo__success_1)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    static const char* k_host_valid = "some_host";
    static const char* k_service_valid = "some_service";
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM + k_host_length_valid;
    const int32_t k_flags_valid = prx_ni_flag_numeric;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet6;

    // arrange 
    STRICT_EXPECTED_CALL(getnameinfo(IGNORED_PTR_ARG, sizeof(struct sockaddr_in6), IGNORED_PTR_ARG, (socklen_t)k_host_length_valid,
        IGNORED_PTR_ARG, (socklen_t)k_service_length_valid, NI_NUMERICHOST | NI_NUMERICSERV))
        .CopyOutArgumentBuffer_buffer(k_host_valid, strlen(k_host_valid)+1)
        .CopyOutArgumentBuffer_svcbuffer(k_service_valid, strlen(k_service_valid)+1)
        .IgnoreArgument(1)
        .SetReturn(0);

    // act 
    result = pal_getnameinfo(
        &address_valid, host_valid, k_host_length_valid, service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(char_ptr, k_host_valid, host_valid);
    ASSERT_ARE_EQUAL(char_ptr, k_service_valid, service_valid);
}

// 
// Test pal_getnameinfo happy path 
// 
TEST_FUNCTION(pal_posix_getnameinfo__success_2)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    static const char* k_host_valid = "some_other";
    static const char* k_service_valid = "twofitty";
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM + k_host_length_valid;
    const int32_t k_flags_valid = prx_ni_flag_namereqd;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet;

    // arrange 
    STRICT_EXPECTED_CALL(getnameinfo(IGNORED_PTR_ARG, sizeof(struct sockaddr_in), IGNORED_PTR_ARG, (socklen_t)k_host_length_valid,
        IGNORED_PTR_ARG, (socklen_t)k_service_length_valid, NI_NAMEREQD))
        .CopyOutArgumentBuffer_buffer(k_host_valid, strlen(k_host_valid) + 1)
        .CopyOutArgumentBuffer_svcbuffer(k_service_valid, strlen(k_service_valid) + 1)
        .IgnoreArgument(1)
        .SetReturn(0);

    // act 
    result = pal_getnameinfo(
        &address_valid, host_valid, k_host_length_valid, service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    ASSERT_ARE_EQUAL(char_ptr, k_host_valid, host_valid);
    ASSERT_ARE_EQUAL(char_ptr, k_service_valid, service_valid);
}

// 
// Test pal_getnameinfo passing as address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_posix_getnameinfo__arg_address_invalid)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM + k_host_length_valid;
    const int32_t k_flags_valid = prx_ni_flag_namereqd;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_proxy;

    // arrange 

    // act 
    result = pal_getnameinfo(
        &address_valid, host_valid, k_host_length_valid, service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_not_supported, result);
}

// 
// Test pal_getnameinfo passing as address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_posix_getnameinfo__arg_address_null)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM + k_host_length_valid;
    const int32_t k_flags_valid = prx_ni_flag_namereqd;
    int32_t result;

    // arrange 

    // act 
    result = pal_getnameinfo(
        NULL, host_valid, k_host_length_valid, service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getnameinfo passing as host argument an invalid char* value 
// 
TEST_FUNCTION(pal_posix_getnameinfo__arg_host_null)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    char* service_valid = UT_MEM;
    const int32_t k_flags_valid = 0;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet6;

    // arrange 

    // act 
    result = pal_getnameinfo(
        &address_valid, NULL, k_host_length_valid, service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getnameinfo passing as host_length argument an invalid size_t value 
// 
TEST_FUNCTION(pal_posix_getnameinfo__arg_host_length_invalid)
{
    static const size_t k_service_length_valid = 32;
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM;
    const int32_t k_flags_valid = 0;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet;

    // arrange 

    // act 
    result = pal_getnameinfo(
        &address_valid, host_valid, 0, service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getnameinfo passing as service argument an invalid char* value 
// 
TEST_FUNCTION(pal_posix_getnameinfo__arg_service_null)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    char* host_valid = UT_MEM;
    const int32_t k_flags_valid = prx_ni_flag_namereqd;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet;

    // arrange 

    // act 
    result = pal_getnameinfo(
        &address_valid, host_valid, k_host_length_valid, NULL, k_service_length_valid, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getnameinfo passing as service_length argument an invalid size_t value 
// 
TEST_FUNCTION(pal_posix_getnameinfo__arg_service_length_invalid)
{
    static const size_t k_host_length_valid = 256;
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM + k_host_length_valid;
    const int32_t k_flags_valid = 0;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet6;

    // arrange 

    // act 
    result = pal_getnameinfo(
        &address_valid, host_valid, k_host_length_valid, service_valid, 0, k_flags_valid);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
}

// 
// Test pal_getnameinfo passing as flags argument an invalid int32_t value 
// 
TEST_FUNCTION(pal_posix_getnameinfo__arg_flags_invalid)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM + k_host_length_valid;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet;

    // arrange 

    // act 
    result = pal_getnameinfo(
        &address_valid, host_valid, k_host_length_valid, service_valid, k_service_length_valid, -1);

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_arg, result);
}

// 
// Test pal_getnameinfo unhappy path 
// 
TEST_FUNCTION(pal_posix_getnameinfo__neg)
{
    static const size_t k_host_length_valid = 256;
    static const size_t k_service_length_valid = 32;
    static const char* k_host_valid = "some_host";
    static const char* k_service_valid = "some_service";
    char* host_valid = UT_MEM;
    char* service_valid = UT_MEM + k_host_length_valid;
    const int32_t k_flags_valid = prx_ni_flag_numeric;
    prx_socket_address_t address_valid;
    int32_t result;

    address_valid.un.family = prx_address_family_inet6;

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    STRICT_EXPECTED_CALL(getnameinfo(IGNORED_PTR_ARG, sizeof(struct sockaddr_in6), IGNORED_PTR_ARG, (socklen_t)k_host_length_valid,
        IGNORED_PTR_ARG, (socklen_t)k_service_length_valid, NI_NUMERICHOST | NI_NUMERICSERV))
        .CopyOutArgumentBuffer_buffer(k_host_valid, strlen(k_host_valid) + 1)
        .CopyOutArgumentBuffer_svcbuffer(k_service_valid, strlen(k_service_valid) + 1)
        .IgnoreArgument(1)
        .SetReturn(0)
        .SetFailReturn(EAI_FAIL);

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_getnameinfo(
        &address_valid, host_valid, k_host_length_valid, service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_fatal);
}


#ifdef pal_pton

//
//Test pal_pton happy path 
// 
TEST_FUNCTION(pal_pton__success)
{
    static const const char* k_addr_string_valid;
    static const prx_socket_address_t* k_address_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_pton(k_addr_string_valid, k_address_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_pton passing as addr_string argument an invalid const char* value 
// 
TEST_FUNCTION(pal_pton__arg_addr_string_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_pton();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_pton passing as address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_pton__arg_address_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_pton();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_pton unhappy path 
// 
TEST_FUNCTION(pal_pton__neg)
{
    static const const char* k_addr_string_valid;
    static const prx_socket_address_t* k_address_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_pton(k_addr_string_valid, k_address_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_pton;

#ifdef pal_ntop

//
//Test pal_ntop happy path 
// 
TEST_FUNCTION(pal_ntop__success)
{
    static const prx_socket_address_t* k_address_valid;
    static const char* k_addr_string_valid;
    static const size_t k_addr_string_size_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_ntop(k_address_valid, k_addr_string_valid, k_addr_string_size_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_ntop passing as address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_ntop__arg_address_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_ntop();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_ntop passing as addr_string argument an invalid char* value 
// 
TEST_FUNCTION(pal_ntop__arg_addr_string_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_ntop();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_ntop passing as addr_string_size argument an invalid size_t value 
// 
TEST_FUNCTION(pal_ntop__arg_addr_string_size_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_ntop();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_ntop unhappy path 
// 
TEST_FUNCTION(pal_ntop__neg)
{
    static const prx_socket_address_t* k_address_valid;
    static const char* k_addr_string_valid;
    static const size_t k_addr_string_size_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_ntop(k_address_valid, k_addr_string_valid, k_addr_string_size_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_ntop;

#ifdef pal_getifaddrinfo

//
//Test pal_getifaddrinfo happy path 
// 
TEST_FUNCTION(pal_getifaddrinfo__success)
{
    static const const char* k_if_name_valid;
    static const uint32_t k_flags_valid;
    static const prx_ifaddrinfo_t** k_info_valid;
    static const size_t* k_info_count_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_getifaddrinfo(k_if_name_valid, k_flags_valid, k_info_valid, k_info_count_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_getifaddrinfo passing as if_name argument an invalid const char* value 
// 
TEST_FUNCTION(pal_getifaddrinfo__arg_if_name_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifaddrinfo passing as flags argument an invalid uint32_t value 
// 
TEST_FUNCTION(pal_getifaddrinfo__arg_flags_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifaddrinfo passing as info argument an invalid prx_ifaddrinfo_t** value 
// 
TEST_FUNCTION(pal_getifaddrinfo__arg_info_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifaddrinfo passing as info_count argument an invalid size_t* value 
// 
TEST_FUNCTION(pal_getifaddrinfo__arg_info_count_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifaddrinfo unhappy path 
// 
TEST_FUNCTION(pal_getifaddrinfo__neg)
{
    static const const char* k_if_name_valid;
    static const uint32_t k_flags_valid;
    static const prx_ifaddrinfo_t** k_info_valid;
    static const size_t* k_info_count_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_getifaddrinfo(k_if_name_valid, k_flags_valid, k_info_valid, k_info_count_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_getifaddrinfo;

#ifdef pal_freeifaddrinfo

//
//Test pal_freeifaddrinfo happy path 
// 
TEST_FUNCTION(pal_freeifaddrinfo__success)
{
    static const prx_ifaddrinfo_t* k_info_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_freeifaddrinfo(k_info_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_freeifaddrinfo passing as info argument an invalid prx_ifaddrinfo_t* value 
// 
TEST_FUNCTION(pal_freeifaddrinfo__arg_info_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_freeifaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_freeifaddrinfo unhappy path 
// 
TEST_FUNCTION(pal_freeifaddrinfo__neg)
{
    static const prx_ifaddrinfo_t* k_info_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_freeifaddrinfo(k_info_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_freeifaddrinfo;

#ifdef pal_getifnameinfo

//
//Test pal_getifnameinfo happy path 
// 
TEST_FUNCTION(pal_getifnameinfo__success)
{
    static const prx_socket_address_t* k_if_address_valid;
    static const char* k_if_name_valid;
    static const size_t k_if_name_length_valid;
    static const uint64_t* k_if_index_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_getifnameinfo(k_if_address_valid, k_if_name_valid, k_if_name_length_valid, k_if_index_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_getifnameinfo passing as if_address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_getifnameinfo__arg_if_address_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifnameinfo passing as if_name argument an invalid char* value 
// 
TEST_FUNCTION(pal_getifnameinfo__arg_if_name_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifnameinfo passing as if_name_length argument an invalid size_t value 
// 
TEST_FUNCTION(pal_getifnameinfo__arg_if_name_length_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifnameinfo passing as if_index argument an invalid uint64_t* value 
// 
TEST_FUNCTION(pal_getifnameinfo__arg_if_index_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getifnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getifnameinfo unhappy path 
// 
TEST_FUNCTION(pal_getifnameinfo__neg)
{
    static const prx_socket_address_t* k_if_address_valid;
    static const char* k_if_name_valid;
    static const size_t k_if_name_length_valid;
    static const uint64_t* k_if_index_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_getifnameinfo(k_if_address_valid, k_if_name_valid, k_if_name_length_valid, k_if_index_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_getifnameinfo;

#ifdef pal_getaddrinfo

//
//Test pal_getaddrinfo happy path 
// 
TEST_FUNCTION(pal_getaddrinfo__success)
{
    static const const char* k_host_name_valid;
    static const const char* k_service_valid;
    static const prx_address_family_t k_family_valid;
    static const uint32_t k_flags_valid;
    static const prx_addrinfo_t** k_info_valid;
    static const size_t* k_info_count_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_valid, k_info_valid, k_info_count_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_getaddrinfo passing as host_name argument an invalid const char* value 
// 
TEST_FUNCTION(pal_getaddrinfo__arg_host_name_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getaddrinfo passing as service argument an invalid const char* value 
// 
TEST_FUNCTION(pal_getaddrinfo__arg_service_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getaddrinfo passing as family argument an invalid prx_address_family_t value 
// 
TEST_FUNCTION(pal_getaddrinfo__arg_family_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getaddrinfo passing as flags argument an invalid uint32_t value 
// 
TEST_FUNCTION(pal_getaddrinfo__arg_flags_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getaddrinfo passing as info argument an invalid prx_addrinfo_t** value 
// 
TEST_FUNCTION(pal_getaddrinfo__arg_info_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getaddrinfo passing as info_count argument an invalid size_t* value 
// 
TEST_FUNCTION(pal_getaddrinfo__arg_info_count_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getaddrinfo unhappy path 
// 
TEST_FUNCTION(pal_getaddrinfo__neg)
{
    static const const char* k_host_name_valid;
    static const const char* k_service_valid;
    static const prx_address_family_t k_family_valid;
    static const uint32_t k_flags_valid;
    static const prx_addrinfo_t** k_info_valid;
    static const size_t* k_info_count_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_getaddrinfo(k_host_name_valid, k_service_valid, k_family_valid, k_flags_valid, k_info_valid, k_info_count_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_getaddrinfo;

#ifdef pal_freeaddrinfo

//
//Test pal_freeaddrinfo happy path 
// 
TEST_FUNCTION(pal_freeaddrinfo__success)
{
    static const prx_addrinfo_t* k_info_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_freeaddrinfo(k_info_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_freeaddrinfo passing as info argument an invalid prx_addrinfo_t* value 
// 
TEST_FUNCTION(pal_freeaddrinfo__arg_info_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_freeaddrinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_freeaddrinfo unhappy path 
// 
TEST_FUNCTION(pal_freeaddrinfo__neg)
{
    static const prx_addrinfo_t* k_info_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_freeaddrinfo(k_info_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_freeaddrinfo;

#ifdef pal_getnameinfo

//
//Test pal_getnameinfo happy path 
// 
TEST_FUNCTION(pal_getnameinfo__success)
{
    static const prx_socket_address_t* k_address_valid;
    static const char* k_host_valid;
    static const size_t k_host_length_valid;
    static const char* k_service_valid;
    static const size_t k_service_length_valid;
    static const int32_t k_flags_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_getnameinfo(k_address_valid, k_host_valid, k_host_length_valid, k_service_valid, k_service_length_valid, k_flags_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_getnameinfo passing as address argument an invalid prx_socket_address_t* value 
// 
TEST_FUNCTION(pal_getnameinfo__arg_address_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getnameinfo passing as host argument an invalid char* value 
// 
TEST_FUNCTION(pal_getnameinfo__arg_host_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getnameinfo passing as host_length argument an invalid size_t value 
// 
TEST_FUNCTION(pal_getnameinfo__arg_host_length_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getnameinfo passing as service argument an invalid char* value 
// 
TEST_FUNCTION(pal_getnameinfo__arg_service_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getnameinfo passing as service_length argument an invalid size_t value 
// 
TEST_FUNCTION(pal_getnameinfo__arg_service_length_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getnameinfo passing as flags argument an invalid int32_t value 
// 
TEST_FUNCTION(pal_getnameinfo__arg_flags_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_getnameinfo();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_getnameinfo unhappy path 
// 
TEST_FUNCTION(pal_getnameinfo__neg)
{
    static const prx_socket_address_t* k_address_valid;
    static const char* k_host_valid;
    static const size_t k_host_length_valid;
    static const char* k_service_valid;
    static const size_t k_service_length_valid;
    static const int32_t k_flags_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_getnameinfo(k_address_valid, k_host_valid, k_host_length_valid, k_service_valid, k_service_length_valid, k_flags_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_getnameinfo;

#ifdef pal_gethostname

//
//Test pal_gethostname happy path 
// 
TEST_FUNCTION(pal_gethostname__success)
{
    static const char* k_name_valid;
    static const size_t k_name_length_valid;
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_gethostname(k_name_valid, k_name_length_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_gethostname passing as name argument an invalid char* value 
// 
TEST_FUNCTION(pal_gethostname__arg_name_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_gethostname();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_gethostname passing as name_length argument an invalid size_t value 
// 
TEST_FUNCTION(pal_gethostname__arg_name_length_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_gethostname();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_gethostname unhappy path 
// 
TEST_FUNCTION(pal_gethostname__neg)
{
    static const char* k_name_valid;
    static const size_t k_name_length_valid;
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_gethostname(k_name_valid, k_name_length_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_gethostname;

#ifdef pal_os_last_net_error_as_prx_error

//
//Test pal_os_last_net_error_as_prx_error happy path 
// 
TEST_FUNCTION(pal_os_last_net_error_as_prx_error__success)
{
    int32_t result;

    // arrange 
    // ... 

    // act 
    result = pal_os_last_net_error_as_prx_error();

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_ok, result);
    // ... 
}

// 
// Test pal_os_last_net_error_as_prx_error unhappy path 
// 
TEST_FUNCTION(pal_os_last_net_error_as_prx_error__neg)
{
    int32_t result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_os_last_net_error_as_prx_error();

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(int32_t, result, er_ok);
}

#endif // pal_os_last_net_error_as_prx_error;

#ifdef pal_os_set_net_error_as_prx_error

//
//Test pal_os_set_net_error_as_prx_error happy path 
// 
TEST_FUNCTION(pal_os_set_net_error_as_prx_error__success)
{
    static const int32_t k_error_valid;
    void result;

    // arrange 
    // ... 

    // act 
    result = pal_os_set_net_error_as_prx_error(k_error_valid);

    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(void, er_ok, result);
    // ... 
}

// 
// Test pal_os_set_net_error_as_prx_error passing as error argument an invalid int32_t value 
// 
TEST_FUNCTION(pal_os_set_net_error_as_prx_error__arg_error_invalid)
{
    // ... 
    int32_t result;

    // arrange 
    // ... 

    // act 
    handle = pal_os_set_net_error_as_prx_error();

    // assert 
    ASSERT_EXPECTED_CALLS();
    ASSERT_ARE_EQUAL(int32_t, er_fault, result);
    // ... 
}

// 
// Test pal_os_set_net_error_as_prx_error unhappy path 
// 
TEST_FUNCTION(pal_os_set_net_error_as_prx_error__neg)
{
    static const int32_t k_error_valid;
    void result;

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    // arrange 
    UMOCK_C_NEGATIVE_TESTS_ARRANGE();
    // ... 

    // act 
    UMOCK_C_NEGATIVE_TESTS_ACT();
    result = pal_os_set_net_error_as_prx_error(k_error_valid);

    // assert 
    UMOCK_C_NEGATIVE_TESTS_ASSERT(void, result, er_ok);
}

#endif // pal_os_set_net_error_as_prx_error;


//
// 5. Teardown tests and test suite
//
DECLARE_TEST_COMPLETE()

