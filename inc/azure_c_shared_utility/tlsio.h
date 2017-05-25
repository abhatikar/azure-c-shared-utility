// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TLSIO_H
#define TLSIO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "xio.h"

MOCKABLE_FUNCTION(, const IO_INTERFACE_DESCRIPTION*, tlsio_get_interface_description);

typedef struct TLSIO_CONFIG_TAG
{
    const char* hostname;
    int port;
    const IO_INTERFACE_DESCRIPTION* underlying_io_interface;
    void* underlying_io_parameters;
} TLSIO_CONFIG;

#ifdef TLSIO_STATE_VERIFICATION_ENABLE

// The tlsio external state values are the states of the tlsio adapter
// as seen by the caller on the basis of calls made and callbacks received.
#define TLSIO_STATE_EXT_VALUES \
    TLSIO_STATE_EXT_CLOSED, \
    TLSIO_STATE_EXT_OPENING, \
    TLSIO_STATE_EXT_OPEN, \
    TLSIO_STATE_EXT_CLOSING, \
    TLSIO_STATE_EXT_ERROR

DEFINE_ENUM(TLSIO_STATE_EXT, TLSIO_STATE_EXT_VALUES);

// tlsio_verify_internal_state compares the supplied expected_state with the internal state
// of the tlsio adapter, and the expected_message_queue_length with the actual
// message queue length, and uses xlogging to log any discrepancies. It returns 0 if there
// are no discrepancies, and non-zero otherwise. This function is not guaranteed to be 
// accurate during tlsio callbacks.
// This function exists only for unit testing builds and must never be
// called in production code.
int tlsio_verify_internal_state(const CONCRETE_IO_HANDLE tlsio,
	TLSIO_STATE_EX expected_state, uint32_t expected_message_queue_length);

#endif // TLSIO_STATE_VERIFICATION_ENABLE

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TLSIO_H */
