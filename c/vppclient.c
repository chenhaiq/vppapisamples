#define _GNU_SOURCE /* for strcasestr(3) */

#include <stdio.h>

#include <vnet/vnet.h>
#include <vppinfra/types.h>
#include <vlibapi/api.h>
#include <vlibmemory/api.h>

#define vl_api_version(n,v) static u32 vpe_api_version = (v);
#include <vpp/api/vpe.api.h>
#undef vl_api_version

#include <vpp/api/vpe_msg_enum.h>
#define vl_typedefs
#include <vpp/api/vpe_all_api_h.h>
#undef vl_typedefs

// Automatically generate endian handlers for messages
#define vl_endianfun
#include <vpp/api/vpe_all_api_h.h>
#undef vl_endianfun

// Automatically generate print handlers for messages
#define vl_print(handle, ...)
#define vl_printfun
#include <vpp/api/vpe_all_api_h.h>
#undef vl_printfun

vlib_main_t vlib_global_main;
vlib_main_t **vlib_mains;

// Keep pointers to API endpoint
typedef struct {
    unix_shared_memory_queue_t * vl_input_queue;
    u32 my_client_index;
} vppclient_main_t;

/* shared vppclient main structure */
vppclient_main_t vppclient_main __attribute__((aligned (64)));

#define INTERFACE_EVENT "sw_interface_event"
#define WANT_INTERFACE_EVENTS "want_interface_events"
#define WANT_INTERFACE_EVENTS_REPLY "want_interface_events_reply"

#define VXLAN_TUNNEL_DUMP "vxlan_tunnel_dump"
#define VXLAN_TUNNEL_DETAILS "vxlan_tunnel_details"




static u32 find_msg_id(char* msg) {
    api_main_t * am = &api_main;
    hash_pair_t *hp;

    hash_foreach_pair (hp, am->msg_index_by_name_and_crc,
    ({
        char *key = (char *)hp->key; // key format: name_crc
        int msg_name_len = strlen(key) - 9; // ignore crc
        if (strlen(msg) == msg_name_len &&
        strncmp(msg, (char *)hp->key, msg_name_len) == 0) {
            return (u32)hp->value[0];
        }
    }));
}

static int vxlan_tunnel_dump(u32 msg_id, u32 message_id, u32 sw_if_index) {
    vl_api_vxlan_tunnel_dump_t * mp;
    vppclient_main_t * jm = &vppclient_main;

    // Allocate control ping message
    mp = vl_msg_api_alloc(sizeof(*mp));
    memset(mp, 0, sizeof(*mp));
    // Set the message ID to control_ping ID reported by VPP
    mp->_vl_msg_id = ntohs(msg_id);
    // Set our client index
    mp->client_index = jm->my_client_index;
    // Set context (context is an arbitrary number that can help matching request/reply pairs)
    mp->context = clib_host_to_net_u32(message_id);

    mp->sw_if_index = ntohl(sw_if_index);

    // send messagee to VPP
    vl_msg_api_send_shmem(jm->vl_input_queue, (u8 *) &mp);
    printf("Sending vxlan_tunnel_dump. msg_id: %d, message id: %d\n", msg_id, message_id);
}

static int want_interface_events(u32 msg_id, u32 message_id) {
    vl_api_want_interface_events_t * mp;
    vppclient_main_t * jm = &vppclient_main;

    // Allocate control ping message
    mp = vl_msg_api_alloc(sizeof(*mp));
    memset(mp, 0, sizeof(*mp));
    // Set the message ID to control_ping ID reported by VPP
    mp->_vl_msg_id = ntohs(msg_id);
    // Set our client index
    mp->client_index = jm->my_client_index;
    // Set context (context is an arbitrary number that can help matching request/reply pairs)
    mp->context = clib_host_to_net_u32(message_id);

    mp->enable_disable = ntohl(1);

    // send messagee to VPP
    vl_msg_api_send_shmem(jm->vl_input_queue, (u8 *) &mp);
    printf("Sending want_interface_events. msg_id: %d, message id: %d\n", msg_id, message_id);
}

static void vl_api_vxlan_tunnel_details_t_handler(
    vl_api_vxlan_tunnel_details_t * mp) {
    printf("\tvxlan_tunnel_details received, message id: %d, vni: %d\n",
    		clib_net_to_host_u32(mp->context),
			clib_net_to_host_u32(mp->vni));
}

static void vl_api_sw_interface_event_t_handler(
    vl_api_sw_interface_event_t * mp) {
    printf("\tsw_interface_event received, sw_if_index: %d\n",
			clib_net_to_host_u32(mp->sw_if_index));

    vxlan_tunnel_dump(find_msg_id(VXLAN_TUNNEL_DUMP), 1, clib_net_to_host_u32(mp->sw_if_index));
}

static void vl_api_want_interface_events_reply_t_handler(
		vl_api_want_interface_events_reply_t * mp) {

    printf("\twant_interface_events_reply received, message id: %d\n",
    		clib_net_to_host_u32(mp->context));
}



int main()
{
    vppclient_main_t * jm = &vppclient_main;
    api_main_t * am = &api_main;

    clib_mem_init (0, 128 << 20);
    // Open VPP management session under name vpp-manager
    if (vl_client_connect_to_vlib("vpe-api", "vpp-manager", 32) < 0)
        return -1;

    printf("Connected to VPP! as client: %d\n", jm->my_client_index);


    jm->my_client_index = am->my_client_index;
    jm->vl_input_queue = am->shmem_hdr->vl_input_queue;

    vl_msg_api_set_handlers(find_msg_id(VXLAN_TUNNEL_DETAILS), VXLAN_TUNNEL_DETAILS,
                            vl_api_vxlan_tunnel_details_t_handler, vl_noop_handler,
 							vl_api_vxlan_tunnel_details_t_endian,
 							vl_api_vxlan_tunnel_details_t_print,
	                        sizeof(vl_api_vxlan_tunnel_details_t), 1);

    printf("INTERFACE_EVENT %d\n", find_msg_id(INTERFACE_EVENT));
    vl_msg_api_set_handlers(find_msg_id(INTERFACE_EVENT), INTERFACE_EVENT,
                            vl_api_sw_interface_event_t_handler, vl_noop_handler,
 							vl_api_sw_interface_event_t_endian,
 							vl_api_sw_interface_event_t_print,
	                        sizeof(vl_api_sw_interface_event_t), 1);

    vl_msg_api_set_handlers(find_msg_id(WANT_INTERFACE_EVENTS_REPLY), WANT_INTERFACE_EVENTS_REPLY,
                            vl_api_want_interface_events_reply_t_handler, vl_noop_handler,
 							vl_api_want_interface_events_reply_t_endian,
 							vl_api_want_interface_events_reply_t_print,
	                        sizeof(vl_api_want_interface_events_reply_t), 1);

    want_interface_events(find_msg_id(WANT_INTERFACE_EVENTS), 1);
    sleep(100);

    return 0;
}
