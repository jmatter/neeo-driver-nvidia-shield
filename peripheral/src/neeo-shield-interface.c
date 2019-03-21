#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>



//--------------------------------------------------
// BLE includes
//--------------------------------------------------
#include "neeo-shield-interface.h"
#include "neeo-shield-interface-services.h"  // Include pre-generated services header

#include "btstack.h"
#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"



//--------------------------------------------------
// Websocket includes
//--------------------------------------------------
#include "mongoose.h"



//--------------------------------------------------
// BLE variables
//--------------------------------------------------
//static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;

static const uint8_t adv_data[] = {
  // Flags general discoverable, BR/EDR not supported
  0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
  // Name
  0x13, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'N','e', 'e', 'o', ' ', 'S', 'h', 'i', 'e', 'l', 'd', ' ', 'd', 'r', 'i', 'v', 'e', 'r',
  // 16-bit Service UUIDs
  0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
  // Appearance (remote),
  0x03, 0x19, 0x80, 0x01,
};
static const uint8_t adv_data_len = sizeof(adv_data);

// from USB HID Specification 1.1, Appendix B.1
static const uint8_t hid_descriptor_keyboard_boot_mode[] = {
  /*
     Keyboard
   */
  0x05, 0x01,                    // Usage Page (Generic Desktop)
  0x09, 0x06,                    // Usage (Keyboard)
  0xa1, 0x01,                    // Collection (Application)

  0x85,  0x01,                   // Report ID 1

  // Modifier byte

  0x75, 0x01,                    //   Report Size (1)
  0x95, 0x08,                    //   Report Count (8)
  0x05, 0x07,                    //   Usage Page (Key codes)
  0x19, 0xE0,                    //   Usage Minimum (Keyboard LeftControl)
  0x29, 0xE7,                    //   Usage Maxium (Keyboard Right GUI)
  0x15, 0x00,                    //   Logical Minimum (0)
  0x25, 0x01,                    //   Logical Maximum (1)
  0x81, 0x02,                    //   Input (Data, Variable, Absolute)

  // Reserved byte

  0x75, 0x01,                    //   Report Size (1)
  0x95, 0x08,                    //   Report Count (8)
  0x81, 0x03,                    //   Input (Constant, Variable, Absolute)

  // LED report + padding

  0x95, 0x05,                    //   Report Count (5)
  0x75, 0x01,                    //   Report Size (1)
  0x05, 0x08,                    //   Usage Page (LEDs)
  0x19, 0x01,                    //   Usage Minimum (Num Lock)
  0x29, 0x05,                    //   Usage Maxium (Kana)
  0x91, 0x02,                    //   Output (Data, Variable, Absolute)

  0x95, 0x01,                    //   Report Count (1)
  0x75, 0x03,                    //   Report Size (3)
  0x91, 0x03,                    //   Output (Constant, Variable, Absolute)

  // Keycodes

  0x95, 0x06,                    //   Report Count (6)
  0x75, 0x08,                    //   Report Size (8)
  0x15, 0x00,                    //   Logical Minimum (0)
  0x25, 0xff,                    //   Logical Maximum (1)
  0x05, 0x07,                    //   Usage Page (Key codes)
  0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
  0x29, 0xff,                    //   Usage Maxium (Reserved)
  0x81, 0x00,                    //   Input (Data, Array)

  0xc0,                          // End collection
};
static const uint8_t hid_descriptor_keyboard_boot_mode_len = sizeof(hid_descriptor_keyboard_boot_mode);



//--------------------------------------------------
// Websocket variables
//--------------------------------------------------
#define WS_SETUP_DELAY 2500

static bool ws_initialized = false;
static const char *ws_http_port = "8000";
static struct mg_serve_http_opts ws_http_server_opts;
static struct mg_mgr ws_mgr;
static struct mg_connection *ws_nc;

static btstack_timer_source_t ws_setup_timer;
static btstack_timer_source_t ws_poll_timer;

static enum {
    IDLE,
    SENDING_FROM_BUFFER,
    SENDING_KEY_UP,
} ws_state;


//--------------------------------------------------
// Support functions
//--------------------------------------------------

/**
 * Returns if the needle is in the haystack
 * @param needle The integer we are looking for
 * @param haystack The array of integers to search in
 * @param haystack_length The length of the integer array
 */
static bool in_array(uint8_t needle, uint8_t haystack[], uint8_t haystack_length) {
  uint8_t idx;
  for (idx=0; idx<haystack_length; idx++) {
    // it was found
    if (haystack[idx] == needle)
      return true;
  }
  return false;
}



//--------------------------------------------------
// BLE methods
//--------------------------------------------------

/**
 * Setup method for the peripheral.
 */
static void peripheral_setup(void)
{
  l2cap_init();
  l2cap_register_packet_handler(&peripheral_packet_handler);

  // setup le device db
  le_device_db_init();

  // setup SM: Display only
  sm_init();
  sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
  sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

  // setup ATT server
  att_server_init(profile_data, NULL, NULL);

  // setup battery service
  battery_service_server_init(100);

  // setup device information service
  device_information_service_server_init();

  // setup HID Device service
  hids_device_init(0, hid_descriptor_keyboard_boot_mode, hid_descriptor_keyboard_boot_mode_len);

  // setup advertisements
  uint16_t adv_int_min = 0x0030;
  uint16_t adv_int_max = 0x0030;
  uint8_t adv_type = 0;
  bd_addr_t null_addr;
  memset(null_addr, 0, 6);
  gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
  gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
  gap_advertisements_enable(1);

  // register for HCI events
  //hci_event_callback_registration.callback = &peripheral_packet_handler;
  //hci_add_event_handler(&hci_event_callback_registration);

  // register for SM events
  sm_event_callback_registration.callback = &peripheral_packet_handler;
  sm_add_event_handler(&sm_event_callback_registration);

  // register for HIDS
  //hids_device_register_packet_handler(peripheral_packet_handler);

  // register for ATT
  att_server_register_packet_handler(peripheral_packet_handler);
}

/**
 * Used to send data to the peripheral.
 * @param modifier The modifier to send (ALT/CTRL, etc.)
 * @param keycode The keycode in ASCII to send to the peripheral.
 */
static void peripheral_send(uint8_t modifier, uint8_t keycode)
{
  uint8_t report[] = {modifier, 0, keycode, 0, 0, 0, 0, 0};
  hids_device_send_input_report(con_handle, report, sizeof(report));
}

/**
 * Handles received bluetooth packets (callback function).
 * @param packet_type The type of packet we received.
 * @param channel On which channeld did we receive it.
 * @param packet The actual packet.
 * @param size The size of the received packet.
 */
static void peripheral_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  UNUSED(channel);
  UNUSED(size);

  if (packet_type != HCI_EVENT_PACKET)
    return;

  uint16_t conn_interval;

  switch (hci_event_packet_get_type(packet))
  {
    case SM_EVENT_JUST_WORKS_REQUEST:
      printf("[Bluetooth driver]: Pairing method 'Just Works' requested\n");
      sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
      break;
    case SM_EVENT_PAIRING_COMPLETE:
      switch (sm_event_pairing_complete_get_status(packet)) {
        case ERROR_CODE_SUCCESS:
          printf("[Bluetooth driver]: Pairing complete, success\n");

          if (!ws_initialized) {
            // set one-shot timer
            ws_setup_timer.process = &websocket_setup;
            btstack_run_loop_set_timer(&ws_setup_timer, WS_SETUP_DELAY);
            btstack_run_loop_add_timer(&ws_setup_timer);
          }
          break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
          printf("[Bluetooth driver]: Pairing failed, timeout\n");
          break;
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
          printf("[Bluetooth driver]: Pairing faileed, disconnected\n");
          break;
        case ERROR_CODE_AUTHENTICATION_FAILURE:
          printf("[Bluetooth driver]: Pairing failed, reason = %u\n", sm_event_pairing_complete_get_reason(packet));
          break;
        default:
          break;
      }
      break;
    case HCI_EVENT_LE_META:
      switch (hci_event_le_meta_get_subevent_code(packet)) {
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
          con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
          // print connection parameters (without using float operations)
          conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
          printf("[Bluetooth driver]: Client connection complete:\n");
          printf("- Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
          printf("- Connection Latency: %u\n", hci_subevent_le_connection_complete_get_conn_latency(packet));

          // request min con interval 15 ms for iOS 11+
          gap_request_connection_parameter_update(con_handle, 12, 12, 0, 0x0048);
          
          if (!ws_initialized) {
            // set one-shot timer
            ws_setup_timer.process = &websocket_setup;
            btstack_run_loop_set_timer(&ws_setup_timer, WS_SETUP_DELAY);
            btstack_run_loop_add_timer(&ws_setup_timer);
          }
          break;
        case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
          // print connection parameters (without using float operations)
          conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
          printf("[Bluetooth driver]: Client connection update:\n");
          printf("- Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
          printf("- Connection Latency: %u\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
          break;
        default:
          break;
      }
      break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
      con_handle = HCI_CON_HANDLE_INVALID;
      printf("[Bluetooth driver]: Client disconnected\n");

      if (ws_initialized) {
        websocket_teardown();
      }
      break;
    break;
  }
}



//--------------------------------------------------
// Websocket functions
//--------------------------------------------------

/**
 * Setup method for the WebSocket server
 */
static void websocket_setup(btstack_timer_source_t * ts) {
  ts = NULL; // prevent "unused parameter warning"

  mg_mgr_init(&ws_mgr, NULL);
  ws_nc = mg_bind(&ws_mgr, ws_http_port, websocket_event_handler);
  mg_set_protocol_http_websocket(ws_nc);
  ws_http_server_opts.document_root = ".";  // Serve current directory
  ws_http_server_opts.enable_directory_listing = "yes";

  printf("[WebSocket server]: Started on port %s\n", ws_http_port);

  // Fire once-shot polling timer
  ws_initialized = true;
  ws_poll_timer.process = &websocket_poll;
  btstack_run_loop_set_timer(&ws_poll_timer, 50);
  btstack_run_loop_add_timer(&ws_poll_timer);
}

/**
 * Method which handles the websocket connection polling and message handling
 */
static void websocket_poll(btstack_timer_source_t * ts) {
  mg_mgr_poll(&ws_mgr, 100);
  
  // Keep running this timer until we are ready
  if (ws_initialized) {
    btstack_run_loop_set_timer(ts, 50);
    btstack_run_loop_add_timer(ts);
  }
}

/**
 * Event handler function, is called when a static file is requested, a client joins, a client sends a message and when a client disconnects.
 * @param mg_connection The connection initiating the event
 * @param ev The event ID
 * @param ev_data The data of the event
 */
static void websocket_event_handler(struct mg_connection *nc, int ev, void *ev_data) {
  // In case we get an HTTP request, we always serve it
  if (ev == MG_EV_HTTP_REQUEST) {
    mg_serve_http(nc, (struct http_message *) ev_data, ws_http_server_opts);
    return;
  }

  // In all other cases it should be a websocket connection
  bool isWsMessage = (nc->flags & MG_F_IS_WEBSOCKET);
  if (!isWsMessage)
    return;

  switch (ev) {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      // Client connected, tell everybody.
      websocket_broadcast(nc, mg_mk_str("connected."));
      break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      char buffer[wm->size];
      sprintf(buffer, "%.*s", (uint8_t) wm->size, (char *) wm->data);

      // Broadcast what we recieved
      char debug[512];
      sprintf(debug, "Received: '%s'", buffer);
      websocket_broadcast(nc, mg_mk_str(debug));

      // We allow a max length of 7 (<modifier:3>,<keycode:3>)
      if (strlen(buffer) <= 7) {
        int modifier = 0;
        int keycode = 0;
        bool valid_input = false;

        // Is there a comma present?
        if (strchr(buffer, ',') != NULL) {
          // Parse modifier & keycode
          valid_input = (sscanf(buffer, "%d,%d", &modifier, &keycode) == 2);

          // Check if the modifier was valid
          uint8_t allowed_modifiers[9] = {0, 1, 2, 4, 8, 16, 32, 64, 128};
          if (valid_input && !in_array(modifier, allowed_modifiers, 9)) {
            valid_input = false;
          }
        } else {
          // Parse modifier & keycode
          valid_input = (sscanf(buffer, "%d", &keycode) == 1);
        }

        // Check keycode
        if (keycode > 255) {
          valid_input = false;
        }

        // Check to see if we have valid input
        if (!valid_input) {
          websocket_broadcast(nc, mg_mk_str("- Invalid message detected. Please check the documentation."));
        } else {
          sprintf(debug, "- Found modifier '%d' and keycode '%d'.", modifier, keycode);
          websocket_broadcast(nc, mg_mk_str(debug));
        }
      } else {
        websocket_broadcast(nc, mg_mk_str("- Invalid message detected. Please check the documentation."));
      }
      break;
    }
    case MG_EV_CLOSE: {
      // Client disconnected, tell everybody.
      websocket_broadcast(nc, mg_mk_str("disconnected."));
      break;
    }
  }
}

/**
 * Broadcast a message to all connected clients. This is mainly used for debug logging.
 * @param mg_connection The connection initiating the broadcast
 * @param mg_str The message we want to broadcast
 */
static void websocket_broadcast(struct mg_connection *nc, const struct mg_str msg) {
  char addr[32];
  mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

  // Create string to send
  char buf[1024];
  snprintf(buf, sizeof(buf), "%s %.*s", addr, (uint8_t) msg.len, msg.p);

  // Local echo
  printf("[WebSocket server]: %s\n", buf);

  // Send to all connected clients
  struct mg_connection *c;
  for (c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
    // Don't send to the sender.
    // if (c == nc) continue;
    mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
  }
}

/**
 * Destructor for the WebSocket server
 */
static void websocket_teardown(void) {
  ws_initialized = false;
  websocket_broadcast(ws_nc, mg_mk_str("Server going down!"));
  mg_mgr_free(&ws_mgr);
  printf("[WebSocket server]: Stopped.\n");
}



//--------------------------------------------------
// Main method
//--------------------------------------------------
uint8_t btstack_main(void)
{
  // Setup BLE peripheral
  peripheral_setup();

  // Turn on Bluetooth Radio
  hci_power_control(HCI_POWER_ON);

  return 0;
}
