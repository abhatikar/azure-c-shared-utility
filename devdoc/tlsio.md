tlsio
=============

## Overview

This specification defines generic behavior for tlsio adapters, which operate primarily through the `xio` interface and provide communication to remote systems over a TLS or similar secure channel.  

### Retry Behavior
The `xio` specification allows retry sequences (open/fail/close/open) that must succeed. This means it must work to `close` and the re-`open` an adapter after an error has occurred during either an IO error or an error during the `open` sequence.

Here are some retry sequences defined for the use in requirements:
* **Failed open retry** means a retry (close/open) that occurs after `dowork` fails to establish its connection to the server.
* **Failed i/o retry** means a retry (close/open) that occurs after `dowork` has experienced a failure while sending or receiving messages through an established connection.

## References

[TLS Protocol (RFC2246)](https://www.ietf.org/rfc/rfc2246.txt)

[TLS Protocol (generic information)](https://en.wikipedia.org/wiki/Transport_Layer_Security)

[OpenSSL](https://www.openssl.org/)

[xio.h](https://github.com/Azure/azure-c-shared-utility/blob/master/inc/azure_c_shared_utility/xio.h)

[tlsio.h](https://github.com/Azure/azure-c-shared-utility/blob/master/inc/azure_c_shared_utility/xio.h)


## Exposed API

**SRS_TLSIO_30_001: [** The tlsio shall implement and export all the Concrete functions in the VTable IO_INTERFACE_DESCRIPTION defined in the `xio.h`.
```c
typedef OPTIONHANDLER_HANDLE (*IO_RETRIEVEOPTIONS)(CONCRETE_IO_HANDLE concrete_io);
typedef CONCRETE_IO_HANDLE(*IO_CREATE)(void* io_create_parameters);
typedef void(*IO_DESTROY)(CONCRETE_IO_HANDLE concrete_io);
typedef int(*IO_OPEN)(CONCRETE_IO_HANDLE concrete_io, ON_IO_OPEN_COMPLETE on_io_open_complete, void* on_io_open_complete_context, ON_BYTES_RECEIVED on_bytes_received, void* on_bytes_received_context, ON_IO_ERROR on_io_error, void* on_io_error_context);
typedef int(*IO_CLOSE)(CONCRETE_IO_HANDLE concrete_io, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context);
typedef int(*IO_SEND)(CONCRETE_IO_HANDLE concrete_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context);
typedef void(*IO_DOWORK)(CONCRETE_IO_HANDLE concrete_io);
typedef int(*IO_SETOPTION)(CONCRETE_IO_HANDLE concrete_io, const char* optionName, const void* value);


typedef struct IO_INTERFACE_DESCRIPTION_TAG
{
    IO_RETRIEVEOPTIONS concrete_io_retrieveoptions;
    IO_CREATE concrete_io_create;
    IO_DESTROY concrete_io_destroy;
    IO_OPEN concrete_io_open;
    IO_CLOSE concrete_io_close;
    IO_SEND concrete_io_send;
    IO_DOWORK concrete_io_dowork;
    IO_SETOPTION concrete_io_setoption;
} IO_INTERFACE_DESCRIPTION;
```
 **]**

**SRS_TLSIO_30_002: [** The tlsio shall report the open operation status using the IO_OPEN_RESULT enumerator defined in the `xio.h`:
```c
typedef enum IO_OPEN_RESULT_TAG
{
    IO_OPEN_OK,
    IO_OPEN_ERROR,
    IO_OPEN_CANCELLED
} IO_OPEN_RESULT;
```
 **]**

**SRS_TLSIO_30_003: [** The tlsio shall report the send operation status using the IO_SEND_RESULT enumerator defined in the `xio.h`:
```c
typedef enum IO_SEND_RESULT_TAG
{
    IO_SEND_OK,
    IO_SEND_ERROR,
    IO_SEND_CANCELLED
} IO_SEND_RESULT;
```
 **]**

**SRS_TLSIO_30_004: [** The tlsio shall call the callbacks functions defined in the `xio.h`:
```c
typedef void(*ON_BYTES_RECEIVED)(void* context, const unsigned char* buffer, size_t size);
typedef void(*ON_SEND_COMPLETE)(void* context, IO_SEND_RESULT send_result);
typedef void(*ON_IO_OPEN_COMPLETE)(void* context, IO_OPEN_RESULT open_result);
typedef void(*ON_IO_CLOSE_COMPLETE)(void* context);
typedef void(*ON_IO_ERROR)(void* context);
```
 **]**

 **SRS_TLSIO_30_005: [** The tlsio shall receive the connection information using the TLSIO_CONFIG structure defined in `tlsio.h`:
 ```c
 typedef struct TLSIO_CONFIG_TAG
 {
  	const char* hostname;
    int port;
 } TLSIO_CONFIG;
 ```
**]**

**SRS_TLSIO_30_006: [** Tlsio adapters shall observe this internally defined timeout for its opening and sending processes. This value is considered an emergency limit rather than a useful tuning parameter, so it is not adjustable via the more expensive get / set options system:
  ```c
#ifndef TLSIO_OPERATION_TIMEOUT_SECONDS
#define TLSIO_OPERATION_TIMEOUT_SECONDS 40
#endif // !TLSIO_OPERATION_TIMEOUT_SECONDS
  ```
**]**

**SRS_TLSIO_30_007: [** Tlsio adapters which use an internal buffer to pass data into the `on_bytes_received` callback shall size this buffer with this internally defined value.
  ```c
// The TLSIO_RECEIVE_BUFFER_SIZE has very little effect on performance, and is kept small
// to minimize memory consumption.
#ifndef TLSIO_RECEIVE_BUFFER_SIZE
#define TLSIO_RECEIVE_BUFFER_SIZE 64
#endif // !TLSIO_RECEIVE_BUFFER_SIZE
  ```
**]**

## External State
The external state of the tlsio adapter is determined by which of the adapter's interface functions have been called and which callbacks have been performed. The adapter's internal state should map cleanly to its external state, but the mapping is not necessarily 1:1. The external states are defined as follows:

* TLSIO_STATE_EXT_CLOSED means either that the module is newly constructed by `tlsio_create`, or that the `tlsio_close` complete callback has been received from a `tlsio_close` call.
* TLSIO_STATE_EXT_OPENING means that the `tlsio_open` call has completed successfully but the `on_tlsio_open_complete` callback has not been performed.
* TLSIO_STATE_EXT_OPEN means that the `on_tlsio_open_complete` callback has returned with `IO_OPEN_OK`.
* TLSIO_STATE_EXT_CLOSING means that the `tlsio_close` call has completed successfully but the `on_tlsio_close_complete` callback has not been performed.
* TLSIO_STATE_EXT_ERROR is the state entered (or maintained) after one or more of the following occurrences:
  * `tlsio_open` has returned an error
  * `on_tlsio_open_complete` has been called with `IO_OPEN_ERROR`
  * `on_ tlsio _error` has been called
  * `tlsio_open`, `tlsio_send`, or `tlsio_close` are called during TLSIO_STATE_EXT_OPENING (invalid usage)
  * `tlsio_open` is called during TLSIO_STATE_EXT_OPEN (invalid usage)
  * `tlsio_open`, `tlsio_send`, or `tlsio_close` are called during TLSIO_STATE_EXT_CLOSING (invalid usage)
  * `tlsio_open` or `tlsio_send` are called during TLSIO_STATE_EXT_ERROR (invalid usage)

## State Transitions
This list shows the effect of the calls as a function of state with happy internal functionality. Unhappy functionality is not shown because it always ends in TLSIO_STATE_EXT_ERROR. The `tlsio_setoption` and `tlsio_getoptions` calls are not shown because they have no effect on state and are always allowed. Calls to `tlsio_send` also do not affect state, but are allowed only during TLSIO_STATE_EXT_OPEN.

<table>
  <tr>From state **TLSIO_STATE_EXT_CLOSED**</tr>
  <tr>
    <td>tlsio_destroy</td>
    <td>ok (destroyed)</td>
  </tr>
  <tr>
    <td>tlsio_open</td>
    <td>ok, enter TLSIO_STATE_OPENING</td>
  </tr>
  <tr>
    <td>tlsio_close</td>
    <td>fail, log error, remain in TLSIO_STATE_CLOSED (invalid usage)</td>
  </tr>
  <tr>
    <td>tlsio_dowork</td>
    <td>ok (does nothing), remain in TLSIO_STATE_CLOSED</td>
  </tr>
</table>

<table>
  <tr>From state **TLSIO_STATE_EXT_OPENING**</tr>
  <tr>
    <td>tlsio_destroy</td>
    <td>log error, force immediate close, destroy (destroyed)</td>
  </tr>
  <tr>
    <td>tlsio_open</td>
    <td>fail, log error, enter TLSIO_STATE_ERROR (invalid usage)</td>
  </tr>
  <tr>
    <td>tlsio_close</td>
    <td>fail, log error, enter TLSIO_STATE_ERROR (invalid usage)</td>
  </tr>
  <tr>
    <td>tlsio_dowork</td>
    <td>ok (continue opening), remain in TLSIO_STATE_OPENING or enter TLSIO_STATE_OPEN</td>
  </tr>
</table>

<table>
  <tr>From state **TLSIO_STATE_EXT_OPEN**</tr>
  <tr>
    <td>tlsio_destroy</td>
    <td>log error, force immediate close, destroy (destroyed)</td>
  </tr>
  <tr>
    <td>tlsio_open</td>
    <td>fail, log error, enter TLSIO_STATE_ERROR (invalid usage)</td>
  </tr>
  <tr>
    <td>tlsio_close</td>
    <td>ok, enter TLSIO _STATE_ CLOSING</td>
  </tr>
  <tr>
    <td>tlsio_dowork</td>
    <td>ok (send and receive as necessary), remain in TLSIO_STATE_OPEN</td>
  </tr>
</table>

<table>
  <tr>From state **TLSIO_STATE_EXT_CLOSING**</tr>
  <tr>
    <td>tlsio_destroy</td>
    <td>log error, force immediate close, destroy (destroyed)</td>
  </tr>
  <tr>
    <td>tlsio_open</td>
    <td>fail, log error, enter TLSIO_STATE_ERROR (invalid usage)</td>
  </tr>
  <tr>
    <td>tlsio_close</td>
    <td>fail, log error, enter TLSIO_STATE_ERROR (invalid usage)</td>
  </tr>
  <tr>
    <td>tlsio_dowork</td>
    <td>ok (continue graceful closing) , remain in TLSIO_STATE_CLOSING or enter TLSIO_STATE_CLOSED</td>
  </tr>
</table>

<table>
  <tr>From state **TLSIO_STATE_EXT_ERROR**</tr>
  <tr>
    <td>tlsio_destroy</td>
    <td>log error, force immediate close, destroy (destroyed)</td>
  </tr>
  <tr>
    <td>tlsio_open</td>
    <td>fail, log error, remain in TLSIO_STATE_ERROR (invalid usage)</td>
  </tr>
  <tr>
    <td>tlsio_close</td>
    <td>ok, force immediate close, enter TLSIO_STATE_CLOSED</td>
  </tr>
  <tr>
    <td>tlsio_dowork</td>
    <td>ok (does nothing), remain in TLSIO_STATE_ERROR</td>
  </tr>
</table>

## Design Decisions

* frog
* tlsio_openssl_compact_dowork




* **Retry decision**: the tlsio adapter shall not initiate error recovery of any sort.
* **Reason for retry decision**: Decisions such as how, when, and whether to retry after error are high level policy decisions that are deliberately deferred to the higher level modules that own the tilso adapter. The tlsio adapter is not competent to make any guesses about what the correct retry behavior should be.

* hello adf
* world adf

* **Unsent messages decision**: the tlsio adapter shall discard any unsent messages when it receives a “close” command.
* parakeet


* **Reasons for unsent messages decision**


  1.       This is consistent with existing tlsio adapters.
  2.       Decisions about which messages should be re-sent after a (possibly lengthy) recovery must be deferred to higher level modules. Discarding all unsent messages upon “close” puts that responsibility where it belongs.

No fake sends decision: although it would be possible to enqueue messages no matter what state the tlsio is in, the tlsio shall only accept messages for transmission when it is in the TLSIO_STATE_OPEN state.
Reason for no fake sends decision:
1.       This is consistent with existing tlsio adapters.
2.       Accepting messages in other states amounts to poorly designed message queuing, and message queuing is already being implemented properly at higher levels.
3.       Accepting messages in other states would require non-trivial design and unit test work and force the redesign of higher levels without adding any real functionality.


## API Calls

###   tlsio_get_interface_description
```c
const IO_INTERFACE_DESCRIPTION* tlsio_get_interface_description(void);
```

**SRS_TLSIO_30_008: [** The tlsio_get_interface_description shall return the VTable IO_INTERFACE_DESCRIPTION. **]**


###   tlsio_create
Implementation of `IO_CREATE concrete_io_create`
```c
CONCRETE_IO_HANDLE tlsio_create(void* io_create_parameters);
```

**SRS_TLSIO_30_010: [** The `tlsio_create` shall allocate and initialize all necessary resources and return an instance of the `tlsio`. **]**

**SRS_TLSIO_30_011: [** If any resource allocation fails, `tlsio_create` shall return NULL. **]**

**SRS_TLSIO_30_012: [** The `tlsio_create` shall receive the connection configuration as a `TLSIO_CONFIG*` in `io_create_parameters`. **]**

**SRS_TLSIO_30_013: [** If the `io_create_parameters` value is NULL, `tlsio_create` shall log an error and return NULL. **]**

**SRS_TLSIO_30_014: [** If the `hostname` member of `io_create_parameters` value is NULL, `tlsio_create` shall log an error and return NULL. **]**

**SRS_TLSIO_30_015: [** If the `port` member of `io_create_parameters` value is less than 0 or greater than 0xffff, `tlsio_create` shall log an error and return NULL. **]**

**SRS_TLSIO_30_016: [** `tlsio_create` shall make a copy of the `hostname` member of `io_create_parameters` to allow deletion of `hostname` immediately after the call. **]**


###   tlsio_destroy
Implementation of `IO_DESTROY concrete_io_destroy`
```c
void tlsio_destroy(CONCRETE_IO_HANDLE tlsio_handle);
```

**SRS_TLSIO_30_020: [** If `tlsio_handle` is NULL, `tlsio_destroy` shall do nothing. **]**

**SRS_TLSIO_30_021: [** The `tlsio_destroy` shall release all allocated resources and then release `tlsio_handle`. **]**

**SRS_TLSIO_30_022: [** If `tlsio_close` has not been called before `tlsio_destroy`, `tlsio_destroy` shall additionally log an error. **]**


###   tlsio_open
Implementation of `IO_OPEN concrete_io_open`

```c
int tlsio_open(
    CONCRETE_IO_HANDLE tlsio_handle,
    ON_IO_OPEN_COMPLETE on_io_open_complete,
    void* on_io_open_complete_context,
    ON_BYTES_RECEIVED on_bytes_received,
    void* on_bytes_received_context,
    ON_IO_ERROR on_io_error,
    void* on_io_error_context);
```

**SRS_TLSIO_30_030: [** If the `tlsio_handle` parameter is NULL, `tlsio_open` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_031: [** If the on_io_open_complete parameter is NULL, `tlsio_open` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_032: [** If the on_bytes_received parameter is NULL, `tlsio_open` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_033: [** If the on_io_error parameter is NULL, `tlsio_open` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_034: [** The `tlsio_open` shall store the provided `on_bytes_received`,  `on_bytes_received_context`, `on_io_error`, `on_io_error_context`, `on_io_open_complete`,  and `on_io_open_complete_context` parameters for later use as specified and tested per other line entries in this document. **]**

**SRS_TLSIO_30_035: [** The `tlsio_open` shall begin the process of opening the ssl connection with the host provided in the `tlsio_create` call. **]**

**SRS_TLSIO_30_036: [** If `tlsio_open` successfully begins opening the OpenSSL connection, it shall return 0. **]**

**SRS_TLSIO_30_037: [** If `tlsio_open` has already been called, it shall log an error, and return _FAILURE_. **]**

**SRS_TLSIO_30_038: [** If the `tlsio_open` fails to begin opening the OpenSSL connection it shall return _FAILURE_. **]**

**SRS_TLSIO_30_039: [** If the `tlsio_open` returns _FAILURE_  it shall call `on_io_open_complete` with the provided `on_io_open_complete_context` and IO_OPEN_ERROR. **]**

**SRS_TLSIO_30_040: [** `tlsio_open` shall succeed during a 'Failed open retry' as defined at the top of this document. **]**

**SRS_TLSIO_30_041: [** `tlsio_open` shall succeed during a 'Failed i/o retry' as defined at the top of this document. **]**


###   tlsio_close
Implementation of `IO_CLOSE concrete_io_close`
```c
int tlsio_close(CONCRETE_IO_HANDLE tlsio_handle, ON_IO_CLOSE_COMPLETE on_io_close_complete, void* callback_context);
```

**SRS_TLSIO_30_050: [** If the `tlsio_handle` parameter is NULL, `tlsio_close` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_055: [** If the `on_io_close_complete` parameter is NULL, `tlsio_close` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_051: [** The `tlsio_close` shall forcibly close any existing ssl connection. **]**

**SRS_TLSIO_30_052: [** The `tlsio_close` return value shall be 0 unless  `tlsio_open` has not been called previously. **]**

**SRS_TLSIO_30_053: [** If `tlsio_open` has not been called previously then `tlsio_close` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_054: [** If `tlsio_open` has been called  but the process of opening has not been completed, then the `on_io_open_complete` callback shall be made with `IO_OPEN_CANCELLED`. **]**

**SRS_TLSIO_30_056: [** If `tlsio_close` is called while there are unsent messages in the queue, the `tlsio_close` shall call each message's  `on_send_complete`, passing its associated `callback_context` and `IO_SEND_CANCELLED`. **]**

**SRS_TLSIO_30_057: [** When the closing process is complete, `tlsio_close`  shall call `on_io_close_complete` and pass the `callback_context` as a parameter. **]**


###   tlsio_send
Implementation of `IO_SEND concrete_io_send`
```c
int tlsio_send(CONCRETE_IO_HANDLE tlsio_handle, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context);
```

**SRS_TLSIO_30_060: [** If the `tlsio_handle` parameter is NULL, `tlsio_send` shall log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_061: [** If the `buffer` is NULL, `tlsio_send` shall log the error and return _FAILURE_. **]**

**SRS_TLSIO_30_062: [** If the `on_send_complete` is NULL, `tlsio_send` shall log the error and return _FAILURE_. **]**

**SRS_TLSIO_30_063: [** The `tlsio_send` shall enqueue for transmission the `on_send_complete`, the `callback_context`, the `size`, and the contents of `buffer`. **]**

**SRS_TLSIO_30_064: [** If the supplied message cannot be enqueued for transmission, `tlsio_send` shall return _FAILURE_. **]**

**SRS_TLSIO_30_065: [** If `tlsio_open` has not been called or the opening process has not been completed, `tlsio_send` shall log an error and return _FAILURE_. **]**

###   tlsio_dowork
Implementation of `IO_DOWORK concrete_io_dowork`
```c
void tlsio_dowork(CONCRETE_IO_HANDLE tlsio_handle);
```
The `tlsio_dowork` call executes async jobs for the tlsio. This includes connection completion, sending to the SSL connection, and checking the SSL connection for available bytes to read.

The underlying OpenSSL `SSL_read` call does not return errors if the connection has been lost, so the tlsio will not know if the connection has been lost until it attempts a write operation.

**SRS_TLSIO_30_070: [** If the `tlsio_handle` parameter is NULL, `tlsio_dowork` shall do nothing except log an error. **]**

#### Post-error behavior

These post-error behavior specifications take precedence over all other behavior specifications.

**SRS_TLSIO_30_071: [** If the `tlsio` adapter has returned a failure from `tlsio_open` or if `on_io_open_complete` has been called with `IO_OPEN_ERROR` or `IO_OPEN_CANCELLED`, then `tlsio_dowork` shall do nothing. **]**

**SRS_TLSIO_30_072: [** If `tlsio_dowork` ever calls `on_io_error`, then subsequent calls to `tlsio_dowork` shall do nothing. **]**

#### Behavior selection

**SRS_TLSIO_30_075: [** If `tlsio_dowork` is called before `tlsio_open`, `tlsio_dowork` shall do nothing. **]**

**SRS_TLSIO_30_076: [** If `tlsio_dowork` is called after `tlsio_close`, `tlsio_dowork` shall do nothing. **]**

**SRS_TLSIO_30_077: [** If  `tlsio_dowork` called after `tlsio_open` but before the connection process completes, `tlsio_dowork` shall perform the [Connection completion behaviors](#connection-completion-behaviors) but not the [Data transmission behaviors](#data-transmission-behaviors) or the [Data reception behaviors](#data-reception-behaviors). **]**

**SRS_TLSIO_30_078: [** If `tlsio_dowork` is called after the connection process completes successfully, `tlsio_dowork` shall perform the [Data transmission behaviors](#data-transmission-behaviors) and then the [Data reception behaviors](#data-reception-behaviors) but not the [Connection completion behaviors](#connection-completion-behaviors). **]**

#### Connection completion behaviors

Connection completion may require multiple calls to `tlsio_dowork`. The number of calls required is not specified.

**SRS_TLSIO_30_080: [** The `tlsio_dowork` shall establish an OpenSSL connection using the `hostName` and `port` provided during `tlsio_open`. **]**

**SRS_TLSIO_30_081: [** If the connection process takes longer than the internally defined `TLSIO_OPERATION_TIMEOUT_SECONDS` it shall log an error and call `on_io_open_complete` with the `on_io_open_complete_context` parameter provided in `tlsio_open` and `IO_OPEN_ERROR`. **]**

**SRS_TLSIO_30_082: [** If the connection process fails for any reason, `tlsio_dowork` shall log an error and call `on_io_open_complete` with the `on_io_open_complete_context` parameter provided in `tlsio_open` and `IO_OPEN_ERROR`. **]**

**SRS_TLSIO_30_083: [** If `tlsio_dowork` successfully opens the ssl connection it shall call `on_io_open_complete` with the `on_io_open_complete_context` parameter provided in `tlsio_open` and `IO_OPEN_OK`. **]**

#### Data transmission behaviors

**SRS_TLSIO_30_090: [** If an enqueued message size is 0, the `tlsio_dowork` shall just call the `on_send_complete` with the `callback_context` and `IO_SEND_OK`. **]**

**SRS_TLSIO_30_091: [** If `tlsio_dowork` is able to send all the bytes in an enqueued message, it shall call the messages's `on_send_complete` along with its associated `callback_context` and `IO_SEND_OK`. **]**

**SRS_TLSIO_30_092: [** If the send process for any given message takes longer than the internally defined `TLSIO_OPERATION_TIMEOUT_SECONDS` it call the message's `on_send_complete` along with its associated `callback_context` and `IO_SEND_ERROR`. **]**

**SRS_TLSIO_30_093: [** If the OpenSSL send was not able to send an entire enqueued message at once, subsequent calls to `tlsio_dowork` shall repeat OpenSSL send for the remaining bytes. **]**

**SRS_TLSIO_30_094: [** If the send process encounters an internal error or calls `on_send_complete` with `IO_SEND_ERROR` due to either failure or timeout, it shall also call `on_io_error` and pass in the associated `on_io_error_context`. **]**

**SRS_TLSIO_30_095: [** If the send process fails before sending all of the bytes in an enqueued message, the `tlsio_dowork` shall call the message's `on_send_complete` along with its associated `callback_context` and `IO_SEND_ERROR`. **]**

**SRS_TLSIO_30_096: [** If there are no enqueued messages available, `tlsio_dowork` shall do nothing. **]**

#### Data reception behaviors

The underlying API for OpenSSL is not capable of detecting useful errors when attempting to receive, so it will never call the `on_io_error` callback supplied during `tlsio_open`.

**SRS_TLSIO_30_100: [** If the OpenSSL client is able to provide received data, the `tlsio_dowork` shall read this data and call `on_bytes_received` with the pointer to the buffer containing the data, the number of bytes received, and the  `on_bytes_received_context`. **]**

**SRS_TLSIO_30_102: [** If the OpenSSL client gets no data from its `SSL_recv` call then it shall not call the  `on_bytes_received` callback. **]**


###   tlsio_setoption
Implementation of `IO_SETOPTION concrete_io_setoption`
```c
int tlsio_setoption(CONCRETE_IO_HANDLE tlsio_handle, const char* optionName, const void* value);
```
**SRS_TLSIO_30_120: [** If the `tlsio_handle` parameter is NULL, `tlsio_setoption` shall do nothing except log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_121: [** If the `optionName` parameter is NULL, `tlsio_setoption` shall do nothing except log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_122: [** If the `value` parameter is NULL, `tlsio_setoption` shall do nothing except log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_123 [** The `tlsio_setoption` shall do nothing and return 0. **]**


###   tlsio_retrieveoptions
Implementation of `IO_RETRIEVEOPTIONS concrete_io_retrieveoptions`
```c
OPTIONHANDLER_HANDLE tlsio_retrieveoptions(CONCRETE_IO_HANDLE tlsio_handle);
```

**SRS_TLSIO_30_160: [** If the `tlsio_handle` parameter is NULL, `tlsio_retrieveoptions` shall do nothing except log an error and return _FAILURE_. **]**

**SRS_TLSIO_30_161: [** The `tlsio_retrieveoptions` shall do nothing and return NULL. **]**
