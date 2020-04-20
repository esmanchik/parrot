#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "JumpingSumo.h"

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <jmorecfg.h>

#define TAG "wsumo"

static JumpingSumo sumo{};
static ARCONTROLLER_Device_t *deviceController;
static std::vector<uint8_t> frames[2];
static volatile int activeFrame = 0;
static std::string command;

static boolean parseCommand(const std::string& command) 
{
    eARCONTROLLER_ERROR error;
    auto errorOccured = [&error]() {
        if (error != ARCONTROLLER_OK) {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "\n- command error :%s", ARCONTROLLER_Error_ToString(error));
        }
        return error != ARCONTROLLER_OK;
    };
    int force = 25, udelay = 300;
    const int start = 64; // {"action":"ping","type":"consumer","dest":"producer","command":"
    std::string header = command.substr(start, 1);
    if (command.empty() || command == "\n" || header == "q") return false;
    if (header == "p") {
        std::string posture = command.substr(start+2, 1);
        auto type = ARCOMMANDS_JUMPINGSUMO_PILOTING_POSTURE_TYPE_JUMPER;
        if (posture.substr(start, 1) == "k") {
            type = ARCOMMANDS_JUMPINGSUMO_PILOTING_POSTURE_TYPE_KICKER;
        }
        error = deviceController->jumpingSumo->sendPilotingPosture(deviceController->jumpingSumo, type);
        if (errorOccured()) return false;
    } else if (header == "l") {
        int intensity = 127; //command.substr(start+2);
        //cin >> intensity;
        error = deviceController->common->sendHeadlightsIntensity(deviceController->common, intensity, intensity);
        if (errorOccured()) return false;
    } else {
        if (command.substr(start+2, 1) == "-") force = -force;
        //cin >> force >> udelay;
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Doing %s with %d ...", header.c_str(), force);
        error = deviceController->jumpingSumo->setPilotingPCMDFlag(deviceController->jumpingSumo, 1);
        if (errorOccured()) return false;
        if (header == "t")
            error = deviceController->jumpingSumo->setPilotingPCMDTurn(deviceController->jumpingSumo, force);
        else
            error = deviceController->jumpingSumo->setPilotingPCMDSpeed(deviceController->jumpingSumo, force * 3);
        if (errorOccured()) return false;
        usleep(udelay * 1000);
        error = deviceController->jumpingSumo->setPilotingPCMDFlag(deviceController->jumpingSumo, 0);
        if (errorOccured()) return false;
        error = deviceController->jumpingSumo->setPilotingPCMDSpeed(deviceController->jumpingSumo, 0);
        if (errorOccured()) return false;
        error = deviceController->jumpingSumo->setPilotingPCMDTurn(deviceController->jumpingSumo, 0);
        if (errorOccured()) return false;
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, ".....done");
    }
    //command = "";
    //cin >> command;
    return true;
}

static struct lws_context *context;
static struct lws *client_wsi;
static int interrupted, zero_length_ping, port = 443, // 80, 
	   ssl_connection = LCCSCF_USE_SSL;
static const char *server_address = "libwebsockets.org", *server_path = "/", *pro = "lws-mirror-protocol";

struct pss {
	int send_a_ping;
        int number;
        int of;
};

static int
connect_client(void)
{
	struct lws_client_connect_info i;

	memset(&i, 0, sizeof(i));

	i.context = context;
	i.port = port;
	i.address = server_address; 
	i.path = server_path; 
	i.host = i.address;
	i.origin = i.address;
	i.ssl_connection = ssl_connection;
	i.protocol = pro;
	i.local_protocol_name = "lws-ping-test";
	i.pwsi = &client_wsi;

	return !lws_client_connect_via_info(&i);
}

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;
}

const int packsize = 10240 * 2;
const int MAX_PING = 1024 * 100;
uint8_t ping[MAX_PING];
std::string base64Frame;

static int
callback_minimal_broker(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
        std::string command;
	struct pss *pss = (struct pss *)user;
	int n;

	switch (reason) {

	case LWS_CALLBACK_PROTOCOL_INIT:
		goto _try;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
			 in ? (char *)in : "(null)");
		client_wsi = NULL;
		lws_timed_callback_vh_protocol(lws_get_vhost(wsi),
				lws_get_protocol(wsi), LWS_CALLBACK_USER, 1);
		break;

	/* --- client callbacks --- */

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		lwsl_user("%s: established\n", __func__);
		lws_set_timer_usecs(wsi, 5 * LWS_USEC_PER_SEC);
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
            if (pss->send_a_ping > 0) {
                pss->send_a_ping = 0;
                auto& frame = frames[activeFrame];
                cv::Mat jpeg = cv::imdecode(cv::Mat(frame), cv::IMREAD_COLOR); // cv::IMREAD_UNCHANGED
                std::cout << jpeg.size << std::endl;
                std::vector<uchar> packet;//buffer for coding
                if (jpeg.empty()) packet = frame;
                else {
                    cv::Mat dst = jpeg;
                    //cv::cvtColor(dst, dst, cv::COLOR_BGR2GRAY);
                    cv::resize(dst, dst, cv::Size(320, 240)); // 160, 120
                    std::cout << dst.size << std::endl;
                    std::vector<int> param(2);
                    param[0] = cv::IMWRITE_JPEG_QUALITY;
                    param[1] = 80;//default(95) 0-100

                    cv::imencode(".jpg", dst, packet, param);
                }
                base64Frame = base64_encode(packet.data(),  packet.size());
                int packets = base64Frame.length() / packsize + ((base64Frame.length() % packsize > 0) ? 1 : 0);
                pss->number = 0;
                pss->of = packets;
            } 
            if (pss->number < pss->of) {
                    //std::stringstream ss;
                    //ss << "{\"action\":\"ping\",\"type\":\"producer\",\"dest\":\"consumer\",\"frame\":\"" << base64Frame << "\"}";
                    //std::string message = ss.str();
			int m;

                        int packets = pss->of;
			n = 0;
       			lwsl_user("%d packets to send %d\n", packets, (int)base64Frame.length());
                        //for (int i = 0; i < packets; ++i)
                        int i = pss->number;
                        {
                            eARCONTROLLER_ERROR error = ARCONTROLLER_OK;
                            eARCONTROLLER_DEVICE_STATE deviceState = ARCONTROLLER_Device_GetState (deviceController, &error);
                            if ((error != ARCONTROLLER_OK) || (deviceState != ARCONTROLLER_DEVICE_STATE_RUNNING))
                            {
                                interrupted = 1;
                            }

                            std::string submsg = base64Frame.substr(i * packsize, packsize);
                            //if (!zero_length_ping)
                            n = lws_snprintf((char *)ping, // + LWS_PRE, 
                                    MAX_PING, //"ping body!"
                                    "{\"action\":\"ping\",\"type\":\"producer\",\"dest\":\"consumer\",\"battery\":\"%d\",\"deviceState\":\"%d\",\"error\":\"%s\",\"number\":\"%d\",\"of\":\"%d\",\"frame\":\"%s\"}", 
                                    (int)sumo.getBatteryPercent(), deviceState, ARCONTROLLER_Error_ToString(error), i, packets, submsg.c_str()); // (int)base64Frame.length() ); 
                            //n = message.length();
//                          lwsl_user("Sending PING %d %s... \n", n, submsg.c_str());
        			lwsl_user("Sending PING %d... \n", n);
                            //flags = lws_write_ws_flags(LWS_WRITE_TEXT, 1, 1);
                            m = lws_write(wsi, ping, n, LWS_WRITE_TEXT);
                            //m = lws_write(wsi, (unsigned char *)message.c_str(), n, LWS_WRITE_TEXT);
                            if (m < n) {
                                    lwsl_err("sending ping failed: %d\n", m);

                                    return -1;
                            }
                            
                        }
                        pss->number = i + 1;
			
                        lwsl_user("Sent PING %d\n", m);
                        activeFrame = (activeFrame + 1) % 2;
			lws_callback_on_writable(wsi);
		}
		break;

	case LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL:
		client_wsi = NULL;
		lws_timed_callback_vh_protocol(lws_get_vhost(wsi),
					       lws_get_protocol(wsi),
					       LWS_CALLBACK_USER, 1);
		break;


	case LWS_CALLBACK_CLIENT_RECEIVE:
                command = (const char *)in;
		lwsl_user("RX: %s\n", command.c_str());
                if (!parseCommand(command)) interrupted = 1;
		pss->send_a_ping = 1;
		lws_callback_on_writable(wsi);
		break;
            
        case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
		lwsl_user("LWS_CALLBACK_CLIENT_RECEIVE_PONG\n");
                std::cout << (char *)in << std::endl;
		lwsl_hexdump_notice(in, len);
		break;

	case LWS_CALLBACK_TIMER:
		/* we want to send a ws PING every few seconds */
		pss->send_a_ping = 1;
		lws_callback_on_writable(wsi);
		lws_set_timer_usecs(wsi, 5 * LWS_USEC_PER_SEC);
		break;

	/* rate-limited client connect retries */

	case LWS_CALLBACK_USER:
		lwsl_notice("%s: LWS_CALLBACK_USER\n", __func__);
_try:
		if (connect_client())
			lws_timed_callback_vh_protocol(lws_get_vhost(wsi),
						       lws_get_protocol(wsi),
						       LWS_CALLBACK_USER, 1);
		break;

	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static const struct lws_protocols protocols[] = {
	{
		"lws-ping-test",
		callback_minimal_broker,
		sizeof(struct pss),
		0,
	},
	{ NULL, NULL, 0, 0 }
};

//static void
//sigint_handler(int sig)
//{
//	interrupted = 1;
//}

int main(int argc, const char **argv) {
    frames[0].push_back(0x0ca);
    frames[0].push_back(0x0fe);
    frames[1].push_back(0x0ba);
    frames[1].push_back(0x0be);
    
    struct lws_context_creation_info info;
    const char *p;
    int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
                    /* for LLL_ verbosity above NOTICE to be built into lws,
                     * lws must have been configured and built with
                     * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
                    /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
                    /* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
                    /* | LLL_DEBUG */;

    //signal(SIGINT, sigint_handler);

    if ((p = lws_cmdline_option(argc, argv, "-d")))
            logs = atoi(p);

    lws_set_log_level(logs, NULL);
    lwsl_user("LWS minimal ws client PING\n");

    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
#if defined(LWS_WITH_MBEDTLS)
    /*
     * OpenSSL uses the system trust store.  mbedTLS has to be told which
     * CA to trust explicitly.
     */
    info.client_ssl_ca_filepath = "./libwebsockets.org.cer";
#endif

    if (lws_cmdline_option(argc, argv, "-z"))
            zero_length_ping = 1;

    if ((p = lws_cmdline_option(argc, argv, "--protocol")))
            pro = p;

    if ((p = lws_cmdline_option(argc, argv, "--server"))) {
            server_address = p;
            //pro = "lws-minimal";
            //ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;
    }
    
    if ((p = lws_cmdline_option(argc, argv, "--path"))) {
            server_path = p;
    }

    if ((p = lws_cmdline_option(argc, argv, "--port")))
            port = atoi(p);

    /*
     * since we know this lws context is only ever going to be used with
     * one client wsis / fds / sockets at a time, let lws know it doesn't
     * have to use the default allocations for fd tables up to ulimit -n.
     * It will just allocate for 1 internal and 1 (+ 1 http2 nwsi) that we
     * will use.
     */
    info.fd_limit_per_thread = 1 + 1 + 1;

    context = lws_create_context(&info);
    if (!context) {
            lwsl_err("lws init failed\n");
            return 1;
    }

    using namespace std;

    int frameNb = 0;
    JumpingSumo::FrameLambda onFrame = [&frameNb](const JumpingSumo::Bytes &bytes) {
        frameNb++;
        std::vector<uint8_t> v(bytes.data(), bytes.data() + bytes.size());
        int anotherFrame = (activeFrame + 1) % 2;
        frames[anotherFrame] = v;
        //return ARCONTROLLER_OK;
    };

    JumpingSumo::LoopLambda loop = [](ARCONTROLLER_Device_t *localDeviceController) {
        deviceController = localDeviceController;
        interrupted = 0;
        while (!interrupted) {
            int rest = 50;
            int zeros = rest; int n = 1;
            while (n > 0 || zeros > 0) {
                n = lws_service(context, 0);
                zeros = n == 0 ? zeros - 1 : rest;
            }
        }
    };

    sumo.onFrame(onFrame).doLoop(loop);
    
    for (int i = 0; i < 24 * 60 * 3; ++i) {
        sumo.start();
        std::cout << "retry " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    lws_context_destroy(context);
    lwsl_user("Completed\n");
    
    

    return 0;
}
