// NI Adam GPS library
// Uses an Open Source NMEA parsing lib
// Written by MrGuy

#include <hardware/gps.h>
#include <stdio.h>
#include <android/log.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include "nmea/nmea/nmea.h"

//#define  GPS_DEBUG  1

#define  LOG_TAG  "gps_adam"
#define GPS_TTYPORT "/dev/ttyHS3"
#define MAX_NMEA_CHARS 85


#define MASK_GSV_MSG1 0x0001
#define MASK_GSV_MSG2 0x0002
#define MASK_GSV_MSG3 0x0004

//Mirror the define in nmea/nmea/info.h
#define NMEA_MAXSATS (12)

#if GPS_DEBUG
#define LOGV(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGV(...) ((void)0)
#endif

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static GpsInterface adamGpsInterface;
static GpsCallbacks* adamGpsCallbacks;
static pthread_t NMEAThread;
char NMEA[MAX_NMEA_CHARS];
pthread_mutex_t mutGPS = PTHREAD_MUTEX_INITIALIZER;
char gpsOn = 0;
GpsSvStatus *storeSV = NULL;
int svMask = 0;
pthread_mutex_t mutGSV = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutUseMask = PTHREAD_MUTEX_INITIALIZER;
uint32_t useMask = 0;

typedef struct _argsCarrier
{
	char* 		NMEA;
	nmeaINFO*	info;

} argsCarrier;

typedef struct _nmeaArgs
{
	char* NMEA;
	GpsUtcTime time;
} nmeaArgs;
/////////////////////////////////////////////////////////
//			GPS THREAD		       //
/////////////////////////////////////////////////////////

static void updateStatus(void* arg) {
	if (adamGpsCallbacks != NULL) {
		adamGpsCallbacks->status_cb((GpsStatus*)arg);
	}
	free(arg);
}


static GpsUtcTime getUTCTime(nmeaTIME *time) {
	GpsUtcTime ret = time->year*(31556926);
	ret += time->mon*(2629743);
	ret += time->day*(86400);
	ret += time->hour*(3600);
	ret += time->min*(60);
	ret += time->sec*(1);
	ret *= 1000; // Scale up to milliseconds
	ret += time->hsec*(10);
	return ret;
}

static double convertCoord(double coord) {
	// dddmm.mmm -> dddd.ddddd, etc
	double degrees;
	double mins = modf(coord/100.0f, &degrees);
	degrees += mins*100.0/60.0f;
	return degrees;
}

static void updateNMEA(void* arg) {
	nmeaArgs *Args = (nmeaArgs*)arg;
	GpsUtcTime time = Args->time;
	char* NMEA2 = Args->NMEA;
	//LOGV("Debug GPS: %s", NMEA2);
	if (adamGpsCallbacks != NULL) {
		adamGpsCallbacks->nmea_cb(time, NMEA2, strlen(NMEA2));
	}
	free(NMEA2);
	free(arg);	
}

// No real need for this function.
// GGA does the same and more.
static void updateRMC(void* arg) {
	argsCarrier *Args = (argsCarrier*)arg;
	nmeaINFO *info = Args->info;
	char* NMEA2 = Args->NMEA;
	GpsLocation newLoc;

	if (info->sig == 0) {
		LOGV("No valid fix data.");
		goto endRMC;
	}

	newLoc.size = sizeof(GpsLocation);
	newLoc.flags = GPS_LOCATION_HAS_LAT_LONG;
	newLoc.latitude = convertCoord(info->lat);
	newLoc.longitude = convertCoord(info->lon);
	newLoc.timestamp = getUTCTime(&(info->utc));
	LOGV("Lat: %lf Long: %lf", newLoc.latitude, newLoc.longitude);
	if (adamGpsCallbacks != NULL) {
		adamGpsCallbacks->location_cb(&newLoc);
	}
	endRMC:
	free(info);
	free(NMEA2);
	free(arg);
}

static void updateGSA(void* arg) {
	argsCarrier *Args = (argsCarrier*)arg;
	nmeaINFO *info = Args->info;
	char* NMEA2 = Args->NMEA;
	nmeaGPGSA *pack = malloc(sizeof(nmeaGPGSA));
	nmea_parse_GPGSA(NMEA2, strlen(NMEA2), pack);
	int count = 0;
	pthread_mutex_lock(&mutUseMask);
	useMask = 0;
	for (count = 0; count < NMEA_MAXSATS; count++) {
		if (pack->sat_prn[count] != 0)
		{
			LOGV("%i is in use", pack->sat_prn[count]);
			useMask |= (1 << (pack->sat_prn[count]-1));
		}
	}
	pthread_mutex_unlock(&mutUseMask);

	endRMC:
	free(pack);
	free(info);
	free(NMEA2);
	free(arg);
}

static void updateGGA(void* arg) {
	argsCarrier *Args = (argsCarrier*)arg;
	nmeaINFO *info = Args->info;
	char* NMEA2 = Args->NMEA;
	GpsLocation newLoc;
	GpsStatus gpsStat;

	gpsStat.size = sizeof(gpsStat);
	if (info->sig == 0) {
		LOGV("No valid fix data.");
		goto endGGA;
	}

	newLoc.size = sizeof(GpsLocation);
	newLoc.flags = GPS_LOCATION_HAS_LAT_LONG | GPS_LOCATION_HAS_ALTITUDE | GPS_LOCATION_HAS_ACCURACY;
	newLoc.accuracy = info->HDOP;
	newLoc.altitude = info->elv;
	newLoc.latitude = convertCoord(info->lat);
	newLoc.longitude = convertCoord(info->lon);
	newLoc.timestamp = getUTCTime(&(info->utc));
	LOGV("Lat: %lf Long: %lf", newLoc.latitude, newLoc.longitude);
	if (adamGpsCallbacks != NULL) {
		adamGpsCallbacks->location_cb(&newLoc);
	}
	endGGA:	
	free(info);
	free(NMEA2);
	free(arg);
}
static void updateGSV(void* arg) {
	// This is NOT thread safe, block until it's our turn.
	pthread_mutex_lock(&mutGSV);
	argsCarrier *Args = (argsCarrier*)arg;
	nmeaINFO *info = Args->info;
	char* NMEA2 = Args->NMEA;
	nmeaSATINFO sats = info->satinfo;
	int num = sats.inview;
	int count = 0;
	GpsSvStatus *svStatus = NULL;
	int numMessages = NMEA2[7] - 48;
	int msgNumber = NMEA2[9] - 48;

	//LOGV("Updating %i sats: msg %i/%i", num, numMessages, msgNumber);
	
	switch (msgNumber) {
	
	case (1):
		if (svMask & MASK_GSV_MSG1) {
		// We already have a message one.. dump the old, run with the new.
		free(storeSV);
		storeSV = NULL;
		svMask = 0;
		} 
		svMask |= MASK_GSV_MSG1;
		break;
	case (2):
		if (svMask & MASK_GSV_MSG2) {
		// We already have a message two.. dump the old, run with the new.
		free(storeSV);
		storeSV = NULL;
		svMask = 0;
		} 
		svMask |= MASK_GSV_MSG2;
		break;
	case (3):
		if (svMask & MASK_GSV_MSG3) {
		// We already have a message three.. dump the old, run with the new.
		free(storeSV);
		storeSV = NULL;
		svMask = 0;
		} 
		svMask |= MASK_GSV_MSG3;
		break;
	default:
		// We should never be here..
		break;
	}
	

	// Retrieve stored messages
	if (storeSV == NULL) {
		//  No Msgs stored, allocate new memory
		svStatus = malloc(sizeof(GpsSvStatus));
	} else {
		svStatus = storeSV;
	}

	for (count = (msgNumber-1)*4; ((count < num) && (count < 4*msgNumber)); count++) {
		svStatus->sv_list[count].size = sizeof(GpsSvInfo);
		svStatus->sv_list[count].prn = sats.sat[count].id;
		svStatus->sv_list[count].snr = sats.sat[count].sig;
		svStatus->sv_list[count].elevation = sats.sat[count].elv;
		svStatus->sv_list[count].azimuth = sats.sat[count].azimuth;
		//LOGV("ID: %i; SIG: %i; ELE: %i; AZI: %i", sats.sat[count].id, sats.sat[count].sig, sats.sat[count].elv, sats.sat[count].azimuth);
	}

	
	switch (numMessages) {
	case 1:
		// Only one msg, and we're it
		goto deliverMsg;
		break;
	case 2:
		if (svMask == (MASK_GSV_MSG1 | MASK_GSV_MSG2)) {
			// We've got both messages
			//LOGV("Delivering 2: %i, %i", svMask, (MASK_GSV_MSG1 | MASK_GSV_MSG2));
			goto deliverMsg;
		} else {
			//LOGV("Debug 2: %i, %i", svMask, (MASK_GSV_MSG1 | MASK_GSV_MSG2));
			// Store for next run
			storeSV = svStatus;
			goto gsvEnd;
		}
		break;
	case 3:
		if (svMask == ((MASK_GSV_MSG1 | MASK_GSV_MSG2) | MASK_GSV_MSG3)) {
			//LOGV("Delivering 3: %i, %i", svMask, ((MASK_GSV_MSG1 | MASK_GSV_MSG2) | MASK_GSV_MSG3));
			goto deliverMsg;
		} else {
			//LOGV("Debug 2: %i, %i",svMask, ((MASK_GSV_MSG1 | MASK_GSV_MSG2) | MASK_GSV_MSG3));
			storeSV = svStatus;
			goto gsvEnd;
		}
		break;
	default:
		// Huh?
		LOGE("Logic error in GSV! numMessages: %i", numMessages);
		goto gsvEnd;
	}
		

	///////////////////////////////////////////////////////
	deliverMsg:
	svStatus->size = sizeof(GpsSvStatus);
	svStatus->num_svs = num; 
	// TODO: Make these accurate
	svStatus->ephemeris_mask = 0;
	svStatus->almanac_mask = 0;
	pthread_mutex_lock(&mutUseMask);
	svStatus->used_in_fix_mask = useMask;
	pthread_mutex_unlock(&mutUseMask);
	if (adamGpsCallbacks != NULL) {
		adamGpsCallbacks->sv_status_cb(svStatus);
	}

	//LOGV("Pushing data");
	free(svStatus);		
	storeSV = NULL;
	svMask = 0;
	//////////////////////////////////////////////////////	
	gsvEnd:
	free(info);
	free(NMEA2);
	free(arg);
	pthread_mutex_unlock(&mutGSV);
}

void processNMEA() {
	argsCarrier *Args = malloc(sizeof(argsCarrier));
	nmeaArgs *nArgs = malloc(sizeof(nmeaArgs));      
	nmeaINFO *info = malloc(sizeof(nmeaINFO));
        nmeaPARSER parser;
        nmea_zero_INFO(info);
        nmea_parser_init(&parser);
	char *NMEA2;
	nmeaINFO info2;
	// Fix up the end so the NMEA parser accepts it
	int count = (int)strlen(NMEA);
	NMEA[count-1] = '\r';
	NMEA[count] = '\n';
	NMEA[count+1] = '\0';
	// Parse the data
	nmea_parse(&parser, NMEA, (int)strlen(NMEA), info);

	if (info->smask == 0) {
		//Bad data
		free(nArgs);
		free(info);
		free(Args);
		return;
	}
	// NOTE: Make copy of NMEA and Data for these threads - Don't want them to be overwritten by other threads
	// Have the individual case function take care of freeing them
	Args->NMEA = (char *)malloc((strlen(&NMEA[0])+1)*sizeof(char));
	nArgs->NMEA = (char *)malloc((strlen(&NMEA[0])+1)*sizeof(char));
	strcpy(Args->NMEA, NMEA);
	strcpy(nArgs->NMEA, NMEA);
	Args->info = info;
	nArgs->time = getUTCTime(&(info->utc));
	
	adamGpsCallbacks->create_thread_cb("adamgps-nmea", updateNMEA, nArgs);
	
	switch (info->smask) {
	case 1:
		//< GGA - Essential fix data which provide 3D location and accuracy data.
		adamGpsCallbacks->create_thread_cb("adamgps-gga", updateGGA, Args);
		break;
	case 2: 
		//< GSA - GPS receiver operating mode, SVs used for navigation, and DOP values.
		adamGpsCallbacks->create_thread_cb("adamgps-gsa", updateGSA, Args);
		break;
	case 4: 
		//< GSV - Number of SVs in view, PRN numbers, elevation, azimuth & SNR values.
		adamGpsCallbacks->create_thread_cb("adamgps-gsv", updateGSV, Args);
		break;
	case 8: 
		//< RMC - Recommended Minimum Specific GPS/TRANSIT Data.
		//adamGpsCallbacks->create_thread_cb("adamgps-loc", updateRMC, Args);
		free(Args->info);
		free(Args->NMEA);
		free(Args);			
		break;
	case 16:
		//< VTG - Actual track made good and speed over ground.
		free(Args->info);
		free(Args->NMEA);
		free(Args);
		break;
	default:
		free(Args->info);
		free(Args->NMEA);
		free(Args);
		break;
	}

	//LOGV("Successful read: %i", info->smask);	
}


static void* doGPS (void* arg) {
	FILE *gpsTTY = NULL;
	char* buffer = NULL;

        // Maximum NMEA sentence SHOULD be 80 characters  + terminators - include a small safety buffer 
	struct timespec slp;
	char go = 1;	
	slp.tv_sec = 1;
	slp.tv_nsec = 0;
	// Open the GPS port
	gpsTTY = fopen(GPS_TTYPORT, "r");
	if (gpsTTY == NULL) {
		LOGE("Failed opening TTY port: %s", GPS_TTYPORT);
		return NULL;
	}
	// Obtain mutex lock and check if we're good to go
	pthread_mutex_lock(&mutGPS);
	go = gpsOn;
	pthread_mutex_unlock(&mutGPS);

	while (go) {
		buffer = fgets(NMEA, MAX_NMEA_CHARS, gpsTTY);
		if (buffer == NULL) {
			LOGV("NMEA data read fail, sleeping for 1 sec.");
			nanosleep(&slp, NULL);
		}
		if (NMEA[0] == '$') {
			//We have a good sentance
			processNMEA();
			// Get rid of the extra LF
			fgetc(gpsTTY);
		} 
		pthread_mutex_lock(&mutGPS);
		go = gpsOn;
		pthread_mutex_unlock(&mutGPS);
	}
fclose(gpsTTY);
return NULL;
}


/////////////////////////////////////////////////////////
//		     GPS INTERFACE       	       //
/////////////////////////////////////////////////////////



static int gpslib_init(GpsCallbacks* callbacks) {
int ret = 0;
LOGV("Callbacks set");
adamGpsCallbacks = callbacks;
adamGpsCallbacks->set_capabilities_cb(0);
GpsStatus *status = malloc(sizeof(GpsStatus));

struct stat st;
if(stat(GPS_TTYPORT, &st) != 0) {
	ret = -1;
	LOGE("Specified tty port: %s does not exist", GPS_TTYPORT);
	goto end;
}


status->size = sizeof(GpsStatus);
status->status = GPS_STATUS_ENGINE_ON;
adamGpsCallbacks->create_thread_cb("adamgps-status", updateStatus, status);

end:
return ret;
}

static int gpslib_start() {
LOGV("Gps start");
GpsStatus *stat = malloc(sizeof(GpsStatus));
stat->size = sizeof(GpsStatus);
stat->status = GPS_STATUS_SESSION_BEGIN;
adamGpsCallbacks->create_thread_cb("adamgps-status", updateStatus, stat);
pthread_mutex_lock(&mutGPS);
gpsOn = 1;
pthread_mutex_unlock(&mutGPS);	
pthread_create(&NMEAThread, NULL, doGPS, NULL);
return 0;
}

static int gpslib_stop() {
LOGV("GPS stop");
GpsStatus *stat = malloc(sizeof(GpsStatus));
stat->size = sizeof(GpsStatus);
stat->status = GPS_STATUS_SESSION_END;
adamGpsCallbacks->create_thread_cb("adamgps-status", updateStatus, stat);
pthread_mutex_lock(&mutGPS);
gpsOn = 0;
pthread_mutex_unlock(&mutGPS);
return 0;
}

static void gpslib_cleanup() {
GpsStatus *stat = malloc(sizeof(GpsStatus));
stat->size = sizeof(GpsStatus);
stat->status = GPS_STATUS_ENGINE_OFF;
adamGpsCallbacks->create_thread_cb("adamgps-status", updateStatus, stat);
LOGV("GPS clean");
return;
}

static int gpslib_inject_time(GpsUtcTime time, int64_t timeReference,
                         int uncertainty) {
LOGV("GPS inject time");
return 0;
}

static int gpslib_inject_location(double latitude, double longitude, float accuracy) {
LOGV("GPS inject location");
return 0;
}


static void gpslib_delete_aiding_data(GpsAidingData flags) {

}

static int gpslib_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time) {

return 0;
}


static const void* gpslib_get_extension(const char* name) {
return NULL;
}

const GpsInterface* gps__get_gps_interface(struct gps_device_t* dev) 
{
	LOGV("Gps get_interface");
	adamGpsInterface.size = sizeof(GpsInterface);
	adamGpsInterface.init = gpslib_init;
	adamGpsInterface.start = gpslib_start;
	adamGpsInterface.stop = gpslib_stop;
	adamGpsInterface.cleanup = gpslib_cleanup;
	adamGpsInterface.inject_time = gpslib_inject_time;
	adamGpsInterface.inject_location = gpslib_inject_location;
	adamGpsInterface.delete_aiding_data = gpslib_delete_aiding_data;
	adamGpsInterface.set_position_mode = gpslib_set_position_mode;
	adamGpsInterface.get_extension = gpslib_get_extension;

	return &adamGpsInterface;
}
	
	
static int open_gps(const struct hw_module_t* module, char const* name,
	struct hw_device_t** device) 
{
	struct gps_device_t *dev = malloc (sizeof(struct gps_device_t));
	memset(dev, 0 , sizeof(*dev));
	
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t*)module;
	dev->get_gps_interface = gps__get_gps_interface;
	
	*device = (struct hw_device_t*)dev;
	return 0;
}


static struct hw_module_methods_t gps_module_methods = {
	.open = open_gps
};

const struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = GPS_HARDWARE_MODULE_ID,
	.name = "Adam GPS Module",
	.author = "MrGuy",
	.methods = &gps_module_methods,
};
