/*
 *  bugficks
 *	(c) 2013
 *
 *  sectroyer
 *	(c) 2014
 *
 *  zoelechat
 *	(c) 2015
 *
 *  tasshack
 *  (c) 2019 - 2021
 *
 *  License: GPLv3
 *
 */
 //////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h> 
#include <sys/stat.h>  
#include <fcntl.h>
#include <dlfcn.h>
#include <glob.h>
#include <stdarg.h>
#include <pthread.h>

//////////////////////////////////////////////////////////////////////////////

#define LIB_NAME "Sambilight"
#define LIB_VERSION "v1.0.7"
#define LIB_TV_MODELS "E/F/H"
#define LIB_HOOKS sambilight_hooks
#define hCTX sambilight_hook_ctx

#include "common.h"
#include "hook.h"
#include "led_manager.h"
#include "util.h"
#include "jsmn.h"


unsigned char osd_enabled = 1, black_border_state = 1, black_border_enabled = 1, external_led_state = 1, external_led_enabled = 0, tv_remote_enabled = 1, gfx_lib = 1;
unsigned long baudrate = 115200, fps_test_frames = 0, capture_frequency = 20;
int serial;
void* h;
led_manager_config_t led_config = { 35, 19, 7, 68, 480, 270, "RGB", 0, 1 };
char device[100] = "/dev/ttyUSB0";

STATIC int show_msg_box(const char* text);

//////////////////////////////////////////////////////////////////////////////

typedef union {
	const void* procs[34];
	struct {
		const int(*SdDisplay_CaptureScreenE)(int*, unsigned char*, int*);
		const int(*SdDisplay_CaptureScreenF)(int*, unsigned char*, int*, int);
		const int(*SdDisplay_CaptureScreenH)(int*, unsigned char*, int*, int*, int);

		int* g_IPanel;
		void* g_TaskManager;
		void** g_pTaskManager;
		void** g_TVViewerMgr;
		void** g_pTVViewerMgr;
		int** g_TVViewerWStringEng;
		int** g_WarningWStringEng;
		void** g_pCustomTextResMgr;
		int** g_CustomTextWStringEng;
		void* (*CResourceManager_GetWString)(void* this, int);
		const char* (*PCWString_Convert2)(unsigned short*, char*, int, int, int*);
		void* (*CTaskManager_GetApplication)(void* this, int);
		void* (*CViewerApp_GetViewerManager)(void* this);
		void* (*ALMOND0300_CDesktopManager_GetInstance)(int);
		void* (*ALMOND0300_CDesktopManager_GetApplication)(void* this, char*, int);
		const int(*CViewerManager_ShowSystemAlert)(void* this, int, int);
		const int(*CViewerManager_ShowSystemAlertB)(void* this, int);
		const int(*CViewerManager_ShowCustomText)(void* this, int);
		const int(*CViewerManager_ShowCustomTextF)(void* this, int, int);
		const int(*CViewerManager_ShowRCMode)(void* this, int);
		const int(*CCustomTextApp_t_OnActivated)(void* this, char*, int);

		void* (*MsOS_PA2KSEG0)(int);
		const int (*MsOS_Dcache_Flush)(void*, int);
		void* (*MApi_MMAP_GetInfo)(int, int);
		void (*MApi_XC_W2BYTEMSK)(int, int, int);
		const int (*gfx_InitNonGAPlane)(void*, unsigned int, unsigned int, int, int);
		const int (*gfx_ReleasePlane)(void*, int);
		const int (*gfx_CaptureFrame)(void*, int, int, unsigned int, unsigned int, unsigned int, unsigned int, int, int, int, int);
		const int (*gfx_BitBltScale)(void*, unsigned int, unsigned int, unsigned int, unsigned int, void*, int, int, unsigned int, unsigned int);
		const int (*MApi_GOP_DWIN_CaptureOneFrame)();
		const int (*MApi_GOP_DWIN_GetWinProperty)(void*);
	};
} samyGO_whacky_t;

samyGO_whacky_t hCTX =
{
	// libScreenShot (libSDAL.so)
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP23SdDisplay_CaptureInfo_t",
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP23SdDisplay_CaptureInfo_t12SdMainChip_k",
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP24SdVideoCommonFrameData_tP8SdRect_t12SdMainChip_k",

	// libAlert
	(const void*)"g_IPanel",
	(const void*)"g_TaskManager",
	(const void*)"g_pTaskManager",
	(const void*)"g_TVViewerMgr",
	(const void*)"g_pTVViewerMgr",
	(const void*)"g_TVViewerWStringEng",
	(const void*)"g_WarningWStringEng",
	(const void*)"g_pCustomTextResMgr",
	(const void*)"g_CustomTextWStringEng",
	(const void*)"_ZN16CResourceManager10GetWStringEi",
	(const void*)"_ZN9PCWString7ConvertEPtPKciiPi",
	(const void*)"_ZN12CTaskManager14GetApplicationE15DTV_APPLICATION",
	(const void*)"_ZN10CViewerApp16GetViewerManagerEv",
	(const void*)"_ZN10ALMOND030015CDesktopManager11GetInstanceENS_13EDesktopIndexE",
	(const void*)"_ZN10ALMOND030015CDesktopManager14GetApplicationEPKcS2_",
	(const void*)"_ZN14CViewerManager15ShowSystemAlertEii",
	(const void*)"_ZN14CViewerManager15ShowSystemAlertEi",
	(const void*)"_ZN14CViewerManager14ShowCustomTextEi",
	(const void*)"_ZN14CViewerManager14ShowCustomTextEib",
	(const void*)"_ZN14CViewerManager10ShowRCModeEi",
	(const void*)"_ZN14CCustomTextApp13t_OnActivatedEPKci",

	// libUTOPIA.so
	(const void*)"MsOS_PA2KSEG0",
	(const void*)"MsOS_Dcache_Flush",
	(const void*)"MApi_MMAP_GetInfo",
	(const void*)"MApi_XC_W2BYTEMSK",
	(const void*)"gfx_InitNonGAPlane",
	(const void*)"gfx_ReleasePlane",
	(const void*)"gfx_CaptureFrame",
	(const void*)"gfx_BitBltScale",
	(const void*)"MApi_GOP_DWIN_CaptureOneFrame",
	(const void*)"MApi_GOP_DWIN_GetWinProperty"
};

_HOOK_IMPL(int, CViewerApp_t_OnInputOccur, void* this, int* a2)
{
	char key = (char)a2[5];
	const char* msgHeader = "Ambient Lighting";
	char msg[100];
	unsigned int index;

	switch (key) {
	case 108: // KEY_RED
		if (led_manager_get_state()) {
			led_manager_set_state(0);
			sprintf(msg, "%s %s", msgHeader, "Off");
		}
		else {
			led_manager_set_state(1);
			sprintf(msg, "%s %s", msgHeader, "On");
		}
		show_msg_box(msg);
		return 1;
	case 20: // KEY_GREEN
		index = led_manager_get_profile_index();
		if (led_profiles[index].index > 0) {
			if (black_border_enabled && black_border_state) {
				black_border_state = 0;
			}
			else {
				index++;
			}
		}
		else {
			if (black_border_enabled) {
				black_border_state = !black_border_state;
			}
			index = 1;
		}

		if (black_border_state) {
			sprintf(msg, "%s Auto Mode", msgHeader);
		}
		else {
			sprintf(msg, "%s %s Mode", msgHeader, led_profiles[index - 1].name);
		}
		show_msg_box(msg);

		led_manager_set_profile(&led_profiles[index - 1]);
		return 1;
	case 21: // KEY_YELLOW
		switch (led_manager_get_intensity()) {
		case 100:
			led_manager_set_intensity(75, 0);
			break;
		case 75:
			led_manager_set_intensity(50, 0);
			break;
		case 50:
			led_manager_set_intensity(25, 0);
			break;
		default:;
			led_manager_set_intensity(100, 0);
			break;
		}
		sprintf(msg, "%s %%%d", msgHeader, led_manager_get_intensity());
		show_msg_box(msg);
		return 1;
	case 22: // KEY_BLUE
		if (external_led_enabled) {
			if (external_led_state) {
				external_led_state = 0;
			}
			else {
				external_led_state = 1;
			}
			sprintf(msg, "External %s %s", msgHeader, external_led_state ? "On" : "Off");
			show_msg_box(msg);
			return 1;
		}
	default:;
	}

	_HOOK_DISPATCH(CViewerApp_t_OnInputOccur, this, a2);
	return (int)h_ret;
}

STATIC dyn_fn_t dyn_hook_fn_tab[] =
{
	{ 0, "_ZN10CViewerApp14t_OnInputOccurEPK7PTEvent" },
};

STATIC hook_entry_t LIB_HOOKS[] =
{
#define _HOOK_ENTRY(F, I) \
    &hook_##F, &dyn_hook_fn_tab[I], &x_##F
	{ _HOOK_ENTRY(CViewerApp_t_OnInputOccur, __COUNTER__) }
#undef _HOOK_ENTRY
};

void* sambiligth_thread(void* params) {

	int capture_info[20] = { 0 }, panel_size[4] = { 0, 0, 1920, 1080 }, cd_size[2] = { 0, 0 }, win_property[20];
	unsigned char* buffer, * data;
	unsigned long fps_counter = 0, counter = 0, c, data_size, leds_count, fps_test_remaining_frames = 0, bytesWritten, bytesRemaining;
	unsigned short header_size = 6, h_border = 0, v_border = 0, h_new_border = 0, v_new_border = 0, scale = 2;
	clock_t capture_begin, capture_end, fps_begin, fps_end, elapsed;
	void* frame_buffer, * out_buffer;
	int* out_buffer_info, * frame_buffer_info;
	typedef int func(void);

	if (fps_test_frames != 1) {
		pthread_detach(pthread_self());
	}

	leds_count = led_manager_init(&led_config, &led_profiles[0]);

	data_size = (leds_count * 3) + header_size;
	if (external_led_enabled) {
		data_size++;
	}

	data = malloc(data_size);
	memcpy(data, "Ada", 3);
	data[3] = ((leds_count - 1) >> 8) & 0xff;
	data[4] = (leds_count - 1) & 0xff;
	data[5] = data[3] ^ data[4] ^ 0x55;
	memset(data + header_size, 0, data_size - header_size);

	if (external_led_enabled) {
		data[data_size - 1] = external_led_state;
	}
	//if (tv_remote_enabled) {
	//	show_msg_box("Ambient Lighting On");
	//}
	capture_frequency *= 1000;

	cd_size[0] = led_config.image_width;
	cd_size[1] = led_config.image_height;

	if (hCTX.g_IPanel) {
		panel_size[2] = ((func*)hCTX.g_IPanel[3])();
		panel_size[3] = ((func*)hCTX.g_IPanel[4])();
	}

	gfx_lib = gfx_lib && fps_test_frames != 1 && hCTX.gfx_InitNonGAPlane && hCTX.MsOS_PA2KSEG0 && hCTX.MApi_MMAP_GetInfo && hCTX.gfx_CaptureFrame && hCTX.MApi_GOP_DWIN_CaptureOneFrame && hCTX.MApi_GOP_DWIN_GetWinProperty;

	if (gfx_lib) {
		log("gfx_lib enabled\n");
	}

	scale = panel_size[2] / led_config.image_width;

	if (gfx_lib) {
		frame_buffer_info = hCTX.MApi_MMAP_GetInfo(59, 0);
		frame_buffer = hCTX.MsOS_PA2KSEG0(*frame_buffer_info);
	}

	if (gfx_lib && scale > 1) {
		out_buffer_info = hCTX.MApi_MMAP_GetInfo(60, 0);
		out_buffer = hCTX.MsOS_PA2KSEG0(*out_buffer_info);
		buffer = out_buffer;
	}
	else {
		buffer = malloc(4 * led_config.image_width * led_config.image_height);
	}

	if (fps_test_frames > 0) {
		led_manager_set_intensity(100, 1);
		//char buff[4096] = "";
		//led_manager_print_area(buff);
		//LOG(buff);

		log("Test Started for %ld frames\n", fps_test_frames);
		fps_test_remaining_frames = fps_test_frames;
		fps_begin = clock();
	}

	log("Grabbing started\n");

	capture_begin = clock();

	while (1) {
		if (led_manager_get_state()) {
			if (gfx_lib) {
				if (osd_enabled) {
					hCTX.MApi_XC_W2BYTEMSK(4166, 0x8000, 0x8000);
					hCTX.MApi_XC_W2BYTEMSK(4310, 0x2000, 0x2000);
				}

				hCTX.gfx_InitNonGAPlane(frame_buffer, panel_size[2], panel_size[3], 32, 0);

				hCTX.MApi_GOP_DWIN_GetWinProperty(win_property);
				if (win_property[4] != panel_size[2] / 2) {
					hCTX.gfx_CaptureFrame(frame_buffer, 0, 2, 0, 0, panel_size[2], panel_size[3], 0, 1, 1, 0);
					usleep(10000);
					hCTX.gfx_ReleasePlane(frame_buffer, 0);
					continue;
				}

				hCTX.MApi_GOP_DWIN_CaptureOneFrame();

				if (scale > 1) {
					hCTX.gfx_InitNonGAPlane(out_buffer, led_config.image_width, led_config.image_height, 32, 0);
					hCTX.gfx_BitBltScale(frame_buffer, 0, 0, panel_size[2], panel_size[3], out_buffer, 0, 0, led_config.image_width, led_config.image_height);
					hCTX.gfx_ReleasePlane(out_buffer, 0);
				}
				else {
					memcpy(buffer, frame_buffer, 4 * led_config.image_width * led_config.image_height);
				}
				hCTX.gfx_ReleasePlane(frame_buffer, 0);

				if (osd_enabled) {
					hCTX.MApi_XC_W2BYTEMSK(4166, 0, 0x8000);
					hCTX.MApi_XC_W2BYTEMSK(4310, 0, 0x2000);
				}
			}
			else if (hCTX.SdDisplay_CaptureScreenF) {
				hCTX.SdDisplay_CaptureScreenF(cd_size, buffer, capture_info, 0);
			}
			else if (hCTX.SdDisplay_CaptureScreenE) {
				hCTX.SdDisplay_CaptureScreenE(cd_size, buffer, capture_info);
			}
			else if (hCTX.SdDisplay_CaptureScreenH) {
				hCTX.SdDisplay_CaptureScreenH(cd_size, buffer, capture_info, panel_size, 0);
			}

			if (led_manager_argb8888_to_leds(buffer, &data[header_size])) {
				if (external_led_enabled) {
					data[data_size - 1] = external_led_state;
				}

				bytesWritten = 0;
				bytesRemaining = data_size;
				while (bytesRemaining > 0) {
					bytesWritten = write(serial, data + bytesWritten, bytesRemaining);
					bytesRemaining -= bytesWritten;
				}

				fps_counter++;
				if (black_border_enabled && fps_counter >= 30) {
					fps_counter = 0;
					if (black_border_state) {
						if (led_manager_get_borders(buffer, &h_new_border, &v_new_border)) {
							if (h_border != h_new_border || v_border != v_new_border) {
								counter = 0;
								h_border = h_new_border;
								v_border = v_new_border;
							}
							else if (counter == 5) {
								led_manager_profile_t profile;
								profile.index = 1;
								c = 0;
								while (profile.index) {
									profile = led_profiles[c];
									if (abs(profile.h_padding_percent - h_border) <= 2 && abs(profile.v_padding_percent - v_border) <= 2) {
										if (led_manager_get_profile_index() != profile.index) {
											led_manager_set_profile(&profile);
											log("Switched to %s Mode\n", profile.name);
										}
										break;
									}
									c++;
								}
							}
							counter++;
						}
					}
					else {
						counter = 0;
						h_border = 0;
						v_border = 0;
					}
				}
			}

			capture_end = clock();
			elapsed = capture_end - capture_begin;
			if (elapsed < capture_frequency && fps_test_frames != 1) {
				usleep(capture_frequency - elapsed);
			}

			if (fps_test_frames > 0) {
				fps_test_remaining_frames--;
				if (fps_test_remaining_frames == 0) {
					if (fps_test_frames == 1) {
						break;
					}
					fps_end = clock();
					log("FPS: %d\n", fps_test_frames * 1000 / ((fps_end - fps_begin) / 1000));
					fps_test_remaining_frames = fps_test_frames;
					fps_begin = clock();
				}
			}

			capture_begin = clock();
		}
		else {
			usleep(100000);
		}
	}

	if (gfx_lib) {
		hCTX.MsOS_Dcache_Flush(frame_buffer, frame_buffer_info[1]);
		if (scale > 1) {
			hCTX.MsOS_Dcache_Flush(out_buffer, out_buffer_info[1]);
		}
	}

	if (!gfx_lib || scale <= 1) {
		free(buffer);
	}

	free(data);
	close(serial);

	led_manager_deinit();
	dlclose(h);

	log("Sambilight ended\n");
	if (fps_test_frames != 1) {
		pthread_exit(NULL);
	}
	return NULL;
}

static int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

void load_profiles_config(const char* path) {
	int length, index = 0, s;
	char* ptr, * json_buffer, tmp[50];
	jsmn_parser parser;
	jsmntok_t tokens[128];
	FILE* config_file;

	config_file = fopen(path, "r");
	if (config_file) {
		fseek(config_file, 0, SEEK_END);
		length = ftell(config_file);
		fseek(config_file, 0, SEEK_SET);
		json_buffer = malloc(length);
		s = fread(json_buffer, 1, length, config_file);
		fclose(config_file);

		jsmn_init(&parser);
		int r = jsmn_parse(&parser, json_buffer, length, tokens, sizeof(tokens) / sizeof(tokens[0]));
		if (r >= 0 && tokens[0].type == JSMN_ARRAY) {
			for (int i = 1; i < r; i++) {
				if (tokens[i].type == JSMN_OBJECT) {
					index++;
					if (index == 1) {
						memset(led_profiles, 0, sizeof(led_profiles));
					}
					led_profiles[index - 1].index = index;

					log("-------------%d-------------\n", index);
				}
				else if (index) {
					if (jsoneq(json_buffer, &tokens[i], "name") == 0) {
						s = tokens[i + 1].end - tokens[i + 1].start;
						if (s > sizeof(led_profiles[index - 1].name) - 1) {
							s = sizeof(led_profiles[index - 1].name) - 1;
						}
						strncpy(led_profiles[index - 1].name, json_buffer + tokens[i + 1].start, s);

						log("Name: %s\n", led_profiles[index - 1].name);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "saturation_gain_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 1) {
							s = 1;
						}
						led_profiles[index - 1].saturation_gain_percent = s;

						log("Saturation Gain: %d%%\n", led_profiles[index - 1].saturation_gain_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "value_gain_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 1) {
							s = 1;
						}
						led_profiles[index - 1].value_gain_percent = s;

						log("Value Gain: %d%%\n", led_profiles[index - 1].value_gain_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "brightness_correction") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < -255) {
							s = -255;
						}
						else if (s > 255) {
							s = 255;
						}
						led_profiles[index - 1].brightness_correction = s;

						log("Brightness Correction: %d\n", led_profiles[index - 1].brightness_correction);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "horizontal_depth_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].horizontal_depth_percent = s;

						log("Horizontal Depth: %d%%\n", led_profiles[index - 1].horizontal_depth_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "vertical_depth_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].vertical_depth_percent = s;

						log("Vertical Depth: %d%%\n", led_profiles[index - 1].vertical_depth_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "overlap_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].overlap_percent = s;

						log("Overlap: %d%%\n", led_profiles[index - 1].overlap_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "h_padding_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].h_padding_percent = strtol(tmp, &ptr, 10);

						log("Horizontal Padding: %d%%\n", led_profiles[index - 1].h_padding_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "v_padding_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].v_padding_percent = s;

						log("Vertical Padding: %d%%\n", led_profiles[index - 1].v_padding_percent);
						i++;
					}
				}
			}
			log("%d Profile(s) Loaded From Config\n", index);
		}
		else {
			log("Failed To Parse Config!\n");
		}

		free(json_buffer);
	}
}

//////////////////////////////////////////////////////////////////////////////
STATIC int _hooked = 0;
EXTERN_C void lib_init(void* _h, const char* libpath)
{
	if (_hooked) {
		log("Injecting once is enough!\n");
		return;
	}

	int argc;
	char* argv[512], * optstr, path[PATH_MAX];
	pthread_t thread;
	size_t len;

	unlink(LOG_FILE);
	log("SamyGO "LIB_TV_MODELS" lib"LIB_NAME" "LIB_VERSION" - (c) tasshack 2019 - 2021\n");
	h = dlopen(0, RTLD_LAZY);
	if (!h) {
		char* serr = dlerror();
		log("%s", serr);
		return;
	}

	patch_adbg_CheckSystem(h);

	samyGO_whacky_t_init(h, &hCTX, ARRAYSIZE(hCTX.procs));

	argc = getArgCArgV(libpath, argv);

	optstr = getOptArg(argv, argc, "H_LEDS:");
	if (optstr && strlen(optstr))
		led_config.h_leds_count = atol(optstr);

	optstr = getOptArg(argv, argc, "V_LEDS:");
	if (optstr && strlen(optstr))
		led_config.v_leds_count = atol(optstr);

	optstr = getOptArg(argv, argc, "BOTTOM_GAP:");
	if (optstr && strlen(optstr))
		led_config.bottom_gap = atol(optstr);

	optstr = getOptArg(argv, argc, "START_OFFSET:");
	if (optstr && strlen(optstr))
		led_config.start_offset = atol(optstr);

	optstr = getOptArg(argv, argc, "CLOCKWISE:");
	if (optstr)
		led_config.led_order = atoi(optstr);

	optstr = getOptArg(argv, argc, "COLOR_ORDER:");
	if (optstr && strlen(optstr) == 3)
		strncpy(led_config.color_order, optstr, 3);

	optstr = getOptArg(argv, argc, "CAPTURE_POS:");
	if (optstr && strlen(optstr))
		led_config.capture_pos = atoi(optstr);

	optstr = getOptArg(argv, argc, "BLACK_BORDER:");
	if (optstr && strlen(optstr))
		black_border_enabled = atoi(optstr);

	optstr = getOptArg(argv, argc, "CAPTURE_FREQ:");
	if (optstr && strlen(optstr))
		capture_frequency = atoi(optstr);

	optstr = getOptArg(argv, argc, "TV_REMOTE:");
	if (optstr)
		tv_remote_enabled = atoi(optstr);

	optstr = getOptArg(argv, argc, "EXTERNAL:");
	if (optstr)
		external_led_enabled = atoi(optstr);

	optstr = getOptArg(argv, argc, "CAPTURE_SIZE:");
	if (optstr) {
		switch (atoi(optstr)) {
		case 1:
			led_config.image_width = 96;
			led_config.image_height = 54;
			break;
		case 2:
			led_config.image_width = 160;
			led_config.image_height = 90;
			break;
		case 3:
			led_config.image_width = 240;
			led_config.image_height = 135;
			break;
		case 4:
			led_config.image_width = 320;
			led_config.image_height = 180;
			break;
		case 6:
			led_config.image_width = 960;
			led_config.image_height = 540;
			break;
		case 7:
			led_config.image_width = 1920;
			led_config.image_height = 1080;
			break;
		case 5:
		default:
			led_config.image_width = 480;
			led_config.image_height = 270;
			break;
		}
	}

	optstr = getOptArg(argv, argc, "OSD_CAPTURE:");
	if (optstr)
		osd_enabled = atol(optstr);

	optstr = getOptArg(argv, argc, "GFX_LIB:");
	if (optstr && strlen(optstr))
		gfx_lib = atoi(optstr);

	optstr = getOptArg(argv, argc, "DEVICE:");
	if (optstr && strlen(optstr))
		strncpy(device, optstr, strlen(device));

	optstr = getOptArg(argv, argc, "BAUDRATE:");
	if (optstr && strlen(optstr))
		baudrate = atol(optstr);

	optstr = getOptArg(argv, argc, "TEST_FRAMES:");
	if (optstr)
		fps_test_frames = atol(optstr);

	if (fps_test_frames == 1) {
		tv_remote_enabled = 0;
	}

	if (tv_remote_enabled) {
		if (dyn_sym_tab_init(h, dyn_hook_fn_tab, ARRAYSIZE(dyn_hook_fn_tab)) >= 0) {
			set_hooks(LIB_HOOKS, ARRAYSIZE(LIB_HOOKS));
			_hooked = 1;
		}
	}

	log("Sambilight started\n");
	log("H_LEDS: %d, V_LEDS: %d, BOTTOM_GAP: %d, START_OFFSET: %d, COLOR_ORDER: %s, CAPTURE_POS: %d, CLOCKWISE: %d\n", led_config.h_leds_count, led_config.v_leds_count, led_config.bottom_gap, led_config.start_offset, led_config.color_order, led_config.capture_pos, led_config.led_order);
	log("%s %d %dx%d %dms\n", device, baudrate, led_config.image_width, led_config.image_height, capture_frequency);

	if (black_border_enabled) {
		log("Black border detection enabled\n");
	}

	if (tv_remote_enabled) {
		log("TV Remote control support enabed\n");
	}

	if (external_led_enabled) {
		log("External led control enabed\n");
	}

	strncpy(path, libpath, PATH_MAX);
	len = strlen(libpath) - 1;
	while (path[len] && path[len] != '.') {
		path[len] = 0;
		len--;
	}
	strncat(path, "config", PATH_MAX);
	load_profiles_config(path);

	serial = open(device, O_WRONLY | O_NOCTTY);
	if (serial >= 0) {
		struct termios SerialPortSettings;
		tcgetattr(serial, &SerialPortSettings);

		switch (baudrate) {
		case 2000000:
			cfsetispeed(&SerialPortSettings, B2000000);
			cfsetospeed(&SerialPortSettings, B2000000);
			break;
		case 1000000:
			cfsetispeed(&SerialPortSettings, B1000000);
			cfsetospeed(&SerialPortSettings, B1000000);
			break;
		case 921600:
			cfsetispeed(&SerialPortSettings, B921600);
			cfsetospeed(&SerialPortSettings, B921600);
			break;
		case 576000:
			cfsetispeed(&SerialPortSettings, B576000);
			cfsetospeed(&SerialPortSettings, B576000);
			break;
		case 500000:
			cfsetispeed(&SerialPortSettings, B500000);
			cfsetospeed(&SerialPortSettings, B500000);
			break;
		case 460800:
			cfsetispeed(&SerialPortSettings, B460800);
			cfsetospeed(&SerialPortSettings, B460800);
			break;
		case 230400:
			cfsetispeed(&SerialPortSettings, B230400);
			cfsetospeed(&SerialPortSettings, B230400);
			break;
		default:;
			cfsetispeed(&SerialPortSettings, B115200);
			cfsetospeed(&SerialPortSettings, B115200);
		}

		SerialPortSettings.c_cflag &= ~PARENB;
		SerialPortSettings.c_cflag &= ~CSTOPB;
		SerialPortSettings.c_cflag |= CS8;
		SerialPortSettings.c_cflag &= ~CRTSCTS;
		SerialPortSettings.c_cflag |= CREAD | CLOCAL;
		SerialPortSettings.c_cflag &= ~HUPCL;
		SerialPortSettings.c_lflag &= ~ICANON;
		SerialPortSettings.c_lflag &= ~ECHO;
		SerialPortSettings.c_lflag &= ~ECHOE;
		SerialPortSettings.c_lflag &= ~ECHONL;
		SerialPortSettings.c_lflag &= ~ISIG;
		SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);
		SerialPortSettings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
		SerialPortSettings.c_oflag &= ~OPOST;
		SerialPortSettings.c_oflag &= ~ONLCR;
		SerialPortSettings.c_cc[VTIME] = 1;
		SerialPortSettings.c_cc[VMIN] = 0;
		tcsetattr(serial, TCSANOW, &SerialPortSettings);

		if (fps_test_frames == 1) {
			sambiligth_thread(NULL);
			return;
		}

		pthread_create(&thread, NULL, &sambiligth_thread, NULL);
	}
	else {
		log("Could not open serial port!\n");
		dlclose(h);
		log("Sambilight ended\n");
	}
}

EXTERN_C void lib_deinit(void* _h)
{
	log(">>> %s\n", __func__);

	if (_hooked)
		remove_hooks(LIB_HOOKS, ARRAYSIZE(LIB_HOOKS));

	_hooked = 0;
	log("<<< %s\n", __func__);
}

//////////////////////////////////////////////////////////////////////////////
STATIC int show_msg_box(const char* text)
{
	void* CViewerManager, * CViewerInstance, * CViewerApp, * CCustomTextApp;

	int newLength, strPos, i, seekLen = 13, strLen = 0;
	unsigned short* tvString = NULL;

	unsigned char stringUtf[512];
	memset(stringUtf, 0, 512 * sizeof(char));

	strncpy(stringUtf, text, 511);

	unsigned short notAvail[] = { 0x004E, 0x006F, 0x0074, 0x0020, 0x0041, 0x0076, 0x0061, 0x0069, 0x006C, 0x0061, 0x0062, 0x006C, 0x0065, 0x0000,	// "Not available\0"
							   0x0050, 0x006C, 0x0065, 0x0061, 0x0073, 0x0065, 0x0020, 0x0067, 0x006F, 0x0020, 0x0069, 0x006E, 0x0074, 0x006F,	// "Please go into"
							   0x0055, 0x0053, 0x0042, 0x0020, 0x0054, 0x0065, 0x0073, 0x0074, 0x0020, 0x003A, 0x0020, 0x004F, 0x004B, 0x0000,	// "USB Test : OK\0"
							   0x004E, 0x006F, 0x0074, 0x0020, 0x0046, 0x006F, 0x0072, 0x0020, 0x0053, 0x0061, 0x006C, 0x0065, 0x0000, 0x0000,	// "Not For Sale\0\0"
							   0x0055, 0x0053, 0x0042, 0x0020, 0x0053, 0x0075, 0x0063, 0x0063, 0x0065, 0x0073, 0x0073, 0x0000, 0x0000, 0x0000 };	// "USB Success\0\0\0"

	if (hCTX.ALMOND0300_CDesktopManager_GetInstance) {
		CViewerInstance = hCTX.ALMOND0300_CDesktopManager_GetInstance(1);
		CViewerApp = hCTX.ALMOND0300_CDesktopManager_GetApplication(CViewerInstance, "samsung.native.tvviewer", 0);
		int* CSystemAlertApp = hCTX.ALMOND0300_CDesktopManager_GetApplication(CViewerInstance, "samsung.native.systemalert", 0);
		CSystemAlertApp[0x4b14 / 4] = 1;
		CCustomTextApp = hCTX.ALMOND0300_CDesktopManager_GetApplication(CViewerInstance, "samsung.native.rcmode", 0);
	}
	else if (hCTX.g_pTaskManager)
		CViewerApp = hCTX.CTaskManager_GetApplication(*hCTX.g_pTaskManager, 2);
	else if (hCTX.g_TaskManager)
		CViewerApp = hCTX.CTaskManager_GetApplication(hCTX.g_TaskManager, 2);
	else
		return -1;

	CViewerManager = hCTX.CViewerApp_GetViewerManager(CViewerApp);

	for (i = 0; i < seekLen; i++)
		notAvail[i] = notAvail[i + 42];

	if (hCTX.g_WarningWStringEng) {
		hCTX.g_TVViewerWStringEng = hCTX.g_CustomTextWStringEng;
		hCTX.g_pTVViewerMgr = hCTX.g_pCustomTextResMgr;
	}
	for (i = 0; i < 600; i++) {
		if (!memcmp(notAvail, hCTX.g_TVViewerWStringEng[i], seekLen * 2))
			break;
	}
	if (i == 600)
		return -1;
	strPos = i;

	if (hCTX.g_TVViewerMgr)
		tvString = hCTX.CResourceManager_GetWString(*hCTX.g_TVViewerMgr, strPos);
	else if (hCTX.g_pTVViewerMgr)
		tvString = hCTX.CResourceManager_GetWString(*hCTX.g_pTVViewerMgr, strPos);

	unsigned short oldString[512];
	memset(oldString, 0, 512 * sizeof(short));

	if (!tvString) {
		return -1;
	}

	uint32_t paligned = (uint32_t)tvString & ~0x0FFF;
	mprotect((uint32_t*)paligned, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

	for (i = 0; i <= 255; i++)
		oldString[i] = tvString[i];

	i = 0;
	while (stringUtf[i]) {
		if (stringUtf[i] >= 0 && stringUtf[i] <= 127) strLen += 1;
		else if ((stringUtf[i] & 0xE0) == 0xC0) strLen += 2;
		else if ((stringUtf[i] & 0xF0) == 0xE0) strLen += 3;
		else if ((stringUtf[i] & 0xF8) == 0xF0) strLen += 4;
		i++;
	}

	if (strLen < 255)
		newLength = strLen;
	else
		newLength = 255;

	for (i = 0; i <= newLength; i++)
		tvString[i] = 0;

	hCTX.PCWString_Convert2(tvString, stringUtf, newLength, 1, 0);
	tvString[newLength] = 0;

	if (hCTX.CCustomTextApp_t_OnActivated)
		hCTX.CCustomTextApp_t_OnActivated(CCustomTextApp, 0, 0);
	else if (hCTX.CViewerManager_ShowSystemAlertB)
		hCTX.CViewerManager_ShowRCMode(CViewerManager, 5);
	else if (hCTX.CViewerManager_ShowCustomText)
		hCTX.CViewerManager_ShowCustomText(CViewerManager, 5);
	else
		hCTX.CViewerManager_ShowCustomTextF(CViewerManager, 5, 1);

	usleep(200000);

	for (i = 0; i < 256; i++)
		tvString[i] = oldString[i];

	return 0;
}