#include <stdio.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <setjmp.h>
#include <check.h>
#include <vapi/vapi.h>
#include <vapi/vpe.api.vapi.h>
#include <vapi/interface.api.vapi.h>
#include <vapi/vxlan.api.vapi.h>

DEFINE_VAPI_MSG_IDS_VPE_API_JSON;
DEFINE_VAPI_MSG_IDS_INTERFACE_API_JSON
DEFINE_VAPI_MSG_IDS_VXLAN_API_JSON;

static char *app_name = "test";
static char *api_prefix = NULL;
static const int max_outstanding_requests = 64;
static const int response_queue_size = 32;


vapi_error_e vxlan_tunnel_dump_cb(struct vapi_ctx_s *ctx, void *callback_ctx,
		vapi_error_e rv, bool is_last,
		vapi_payload_vxlan_tunnel_details * reply) {

	if (!is_last) {

		printf("VXLAN dump entry: vni %d: idx %d\n", reply->vni,
				reply->sw_if_index);

	}
	return VAPI_OK;
}


vapi_error_e
want_interface_events_cb (vapi_ctx_t ctx, void *callback_ctx,
			   vapi_error_e rv, bool is_last,
			   vapi_payload_want_interface_events_reply *
			   payload)
{
  printf("interface_events_cb reply ok");
  return VAPI_OK;
}

vapi_error_e
sw_interface_event_cb (vapi_ctx_t ctx, void *callback_ctx,
		    vapi_payload_sw_interface_event * payload)
{
  int *called = callback_ctx;
  ++*called;
  printf ("sw_interface_event_cb: sw_if_index=%u\n",
	  payload->sw_if_index);

  vapi_msg_vxlan_tunnel_dump *dump;

  dump = vapi_alloc_vxlan_tunnel_dump (ctx);
  dump->payload.sw_if_index = payload->sw_if_index;

  vapi_vxlan_tunnel_dump(ctx, dump, vxlan_tunnel_dump_cb,
  				      NULL);

  return VAPI_OK;
}

int main()
{

  vapi_ctx_t ctx;
  vapi_error_e rv = vapi_ctx_alloc (&ctx);


  rv = vapi_connect (ctx, app_name, api_prefix, max_outstanding_requests,
		     response_queue_size, VAPI_MODE_BLOCKING, true);
  printf("vapi connect success!\n");


  vapi_msg_want_interface_events *ws =
    vapi_alloc_want_interface_events (ctx);
  ws->payload.enable_disable = 1;
  ws->payload.pid = getpid ();
  rv = vapi_want_interface_events (ctx, ws, want_interface_events_cb,
					 NULL);



  int called = 0;
  vapi_set_event_cb (ctx, vapi_msg_id_sw_interface_event,
		     (vapi_event_cb) sw_interface_event_cb, &called);

  rv = vapi_dispatch_one (ctx);

  rv = vapi_disconnect (ctx);
  printf("rv = %d\n", rv);
  vapi_ctx_free (ctx);
}
