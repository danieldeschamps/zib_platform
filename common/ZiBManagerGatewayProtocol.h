#ifndef _ZIB_MANAGER_GATEWAY_PROTOCOL_H_
#define _ZIB_MANAGER_GATEWAY_PROTOCOL_H_

/***********************************************************************************
* ZiB Platform - Manager-Gateway Protocol Specification 
*
* - General guidelines:
* Coordinator <--(rs232)--> Gateway <--(USB + Virtual COM)--> Manager
* Binary data from Coordinator to Manager, defined by the ZiBMessage struct
*
* - Protocol:
* Asynchronous communication.
* Manager originates a REQUEST. Coordinator responds with a CONFIRMATION.
* Coordinator originates an INDICATION. Manager rsponds with a RESPONSE.
*
***********************************************************************************/

#ifndef MCHP_C18 // C18 User Guide pg 93: Members of structures and unions are aligned on byte boundaries.
#pragma pack(1) // alignment of 1 byte for structs and unions (needed for MSVC6)
#endif

// The start byte shall be transmited before any message (array of bytes)
#define ZIB_COMM_START_BYTE						'+'

// Timeout to receive a complete message after the start byte has ben signaled
// If the timer expires the receive buffer shall be discarded and the receive entity should wait for the start byte again
#define ZIB_COMM_TIMEOUT_AFTER_START_BYTE_MS	200

// Available Messages
typedef enum _EZiBMessageCode
{
	ZIB_MSG_CODE_RESET_REQ,
	ZIB_MSG_CODE_RESET_CONF,
	ZIB_MSG_CODE_RESET_IND,
	ZIB_MSG_CODE_RESET_RESP,
	ZIB_MSG_CODE_PAN_ID_REQ,
	ZIB_MSG_CODE_PAN_ID_CONF,
	ZIB_MSG_CODE_NODE_LIST_REQ,
	ZIB_MSG_CODE_NODE_LIST_CONF,
	ZIB_MSG_CODE_EP_LIST_REQ,
	ZIB_MSG_CODE_EP_LIST_CONF,
	ZIB_MSG_CODE_CLUSTER_LIST_REQ,
	ZIB_MSG_CODE_CLUSTER_LIST_CONF,
	ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_REQ,
	ZIB_MSG_CODE_GET_ATTRIBUTE_VALUE_CONF,
	ZIB_MSG_CODE_SET_ATTRIBUTE_VALUE_REQ,
	ZIB_MSG_CODE_SET_ATTRIBUTE_VALUE_CONF
} EZiBMessageCode;

// Error codes
typedef enum _EZiBError
{
	ZIB_ERROR_SUCCESS, // no error
	ZIB_ERROR_FAILURE, // general flaw
	ZIB_ERROR_INVALID_NODE, // the target node does not exist
	ZIB_ERROR_NODE_NO_ANSWER // the target node does not answer
} EZiBError;

// Data structure which describes a ZigBee node
typedef struct _ZiBNode
{
	BYTE mac_addr[8];
	WORD nwk_addr;
	WORD profile_id;
	WORD device_id;
} ZiBNode;

// General message format
#define ZIB_MSG_HEADER_SIZE	2
typedef struct _ZiBMessage
{
	BYTE length; // Whole message length, including this byte (do not count the start byte)
	BYTE code; // One of EZiBMessageCode
	union // Available messages
	{
		struct _reset_req
		{
			BYTE reserved;
		} reset_req;

		struct _reset_conf
		{
			BYTE status; // EZiBError
		} reset_conf;

		struct _reset_ind
		{
			BYTE reserved;
		} reset_ind;

		struct _reset_resp
		{
			BYTE status; // EZiBError
		} reset_resp;

		struct _pan_id_req
		{
			BYTE reserved;
		} pan_id_req;

		struct _pan_id_conf
		{
			BYTE status; // EZiBError
			WORD pan_id;
		} pan_id_conf;

		struct _node_list_req
		{
			BYTE reserved;
		} node_list_req;

		struct _node_list_conf
		{
			BYTE status; // EZiBError
			WORD count;
			BYTE node_list; // ZiBNode array
		} node_list_conf;

		struct _ep_list_req
		{
			WORD nwk_addr;
		} ep_list_req;

		struct _ep_list_conf
		{
			BYTE status; // EZiBError
			WORD nwk_addr;
			BYTE count;
			BYTE ep_list; // BYTE array
		} ep_list_conf;

		struct _cluster_list_req
		{
			WORD nwk_addr;
			BYTE ep;
		} cluster_list_req;

		struct _cluster_list_conf
		{
			BYTE status; // EZiBError
			WORD nwk_addr;
			BYTE ep;
			BYTE count;
			BYTE cluster_list; // BYTE array
		} cluster_list_conf;

		struct _get_attribute_value_req
		{
			WORD nwk_addr;
			BYTE ep;
			BYTE cluster;
			WORD attribute_id;
		} get_attribute_value_req;

		struct _get_attribute_value_conf
		{
			BYTE status; // EZiBError
			WORD nwk_addr;
			BYTE ep;
			BYTE cluster;
			WORD attribute_id;
			BYTE size;
			BYTE value; // Variable data type
		} get_attribute_value_conf;

		struct _set_attribute_value_req
		{
			WORD nwk_addr;
			BYTE ep;
			BYTE cluster;
			WORD attribute_id;
			BYTE size;
			BYTE value; // Variable data type
		} set_attribute_value_req;

		struct _set_attribute_value_conf
		{
			BYTE status; // EZiBError
		} set_attribute_value_conf;
	} type;
} ZiBMessage;

#endif // _ZIB_MANAGER_GATEWAY_PROTOCOL_H_
