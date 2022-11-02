#include "wifi_scan.h"
#include <stdio.h>  //printf
#include <unistd.h> //sleep
#include <string.h>
#include <math.h>
#include "pbPlots.h"
#include "supportLib.h"

const int BSS_INFOS=25; //the maximum amounts of APs (Access Points) we want to store
const int iterations = 5;


//convert bssid to printable hardware mac address
const char *bssid_to_string(const uint8_t bssid[BSSID_LENGTH], char bssid_string[BSSID_STRING_LENGTH])
{
	snprintf(bssid_string, BSSID_STRING_LENGTH, "%02x:%02x:%02x:%02x:%02x:%02x",
         bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
	return bssid_string;
}


int generateGraph(double x, double y){
	_Bool success;
	StringReference *errorMessage;

	RGBABitmapImageReference *imageReference = CreateRGBABitmapImageReference();

	double xs [] = {x};
	double ys [] = {y};

	double xs2 [] = {0, 5, 10, 0};
	double ys2 [] = {0, 10, 0, 0};

	ScatterPlotSeries *series = GetDefaultScatterPlotSeriesSettings();
	series->xs = xs;
	series->xsLength = sizeof(xs)/sizeof(double);
	series->ys = ys;
	series->ysLength = sizeof(ys)/sizeof(double);
	series->linearInterpolation = false;
	series->pointType = L"dots";
	series->pointTypeLength = wcslen(series->pointType);
	series->color = CreateRGBColor(1, 0, 0);

	ScatterPlotSeries *series2 = GetDefaultScatterPlotSeriesSettings();
	series2->xs = xs2;
	series2->xsLength = sizeof(xs2)/sizeof(double);
	series2->ys = ys2;
	series2->ysLength = sizeof(ys2)/sizeof(double);
	series2->linearInterpolation = true;
	series2->lineType = L"solid";
	series2->lineTypeLength = wcslen(series->lineType);
	series2->lineThickness = 2;
	series2->color = CreateRGBColor(0, 0, 1);

	ScatterPlotSettings *settings = GetDefaultScatterPlotSettings();
	settings->width = 600;
	settings->height = 400;
	settings->autoBoundaries = true;
	settings->autoPadding = true;
	settings->title = L"";
	settings->titleLength = wcslen(settings->title);
	settings->xLabel = L"";
	settings->xLabelLength = wcslen(settings->xLabel);
	settings->yLabel = L"";
	settings->yLabelLength = wcslen(settings->yLabel);
	ScatterPlotSeries *s [] = {series, series2};
	settings->scatterPlotSeries = s;
	settings->scatterPlotSeriesLength = 2;

	errorMessage = (StringReference *)malloc(sizeof(StringReference));
	success = DrawScatterPlotFromSettings(imageReference, settings, errorMessage);

	if(success){
		size_t length;
		double *pngdata = ConvertToPNG(&length, imageReference->image);
		WriteToFile(pngdata, length, "Location.png");
		DeleteImage(imageReference->image);
	}else{
		fprintf(stderr, "Error: ");
		for(int i = 0; i < errorMessage->stringLength; i++){
			fprintf(stderr, "%c", errorMessage->string[i]);
		}
		fprintf(stderr, "\n");
	}

	FreeAllocations();

	return success ? 0 : 1;
}


double triangulate(int strength1[], int strength2[], int strength3[]) {

    int avg1 = 0, avg2 = 0, avg3 = 0;

    for(int i = 0; i < iterations; i++){
        avg1 -= strength1[i];
    }
    for(int i = 0; i < iterations; i++){
        avg2 -= strength2[i];
    }
    for(int i = 0; i < iterations; i++){
        avg3 -= strength3[i];
    }


    printf("%d, %d, %d\n", avg1, avg2, avg3);

    int multiplier = 3;         //For my set up, it's showing that about -3 dBm = 1ft


    // wikipedia ==> True-Range Multilateration
    int r1 = avg1/(iterations * multiplier), r2 = avg2/(iterations * multiplier), r3 = avg3/(iterations * multiplier);

    int U = 10;
    int Vx = U/2;
    int Vy = U;

    double x = (pow(r1, 2) - pow(r2, 2) + pow(U, 2)) / (2 * U);      // For my setup, every side of the triangle is going to be 10 feet apart
    double y = (pow(r1, 2) - pow(r3, 2) + pow(Vx, 2) + pow(Vy, 2) - 2*Vx*x) / (2 * Vy);

    printf("x = %f \ny = %f\n", x, y);

    generateGraph(x, y);
}


int main(int argc, char **argv)
{
    struct wifi_scan *wifi=NULL;    //this stores all the library information
    struct bss_info bss[BSS_INFOS]; //this is where we are going to keep informatoin about APs (Access Points)

	int status, i, timeExpired = iterations;
    wifi=wifi_scan_init(argv[1]);

    char signal1[timeExpired][15];
    int strength1[timeExpired];
    char signal2[timeExpired][15];
    int strength2[timeExpired];
    char signal3[timeExpired][15];
    int strength3[timeExpired];
    
    

    while(timeExpired)
    {
        timeExpired--;
        status=wifi_scan_all(wifi, bss, BSS_INFOS);
        
        //it may happen that device is unreachable (e.g. the device works in such way that it doesn't respond while scanning)
        //you may test for errno==EBUSY here and make a retry after a while, this is how my hardware works for example
        if(status<0) {
            perror("Unable to get scan data");
            timeExpired++;
        }
        else //wifi_scan_all returns the number of found stations, it may be greater than BSS_INFOS that's why we test for both in the loop
            for(i=0;i<status && i<BSS_INFOS;i++) {
                char wifiSSID[50];
                sprintf(wifiSSID, "%s", bss[i].ssid);

                if(strcmp(wifiSSID, "Amped_Wireless_1") == 0)
                {
                    strength1[timeExpired] = bss[i].signal_mbm/100;
                    strcpy(signal1[timeExpired], wifiSSID);
                }

                if(strcmp(wifiSSID, "Amped_Wireless_2") == 0)
                {
                    strcpy(signal2[timeExpired], wifiSSID);
                    strength2[timeExpired] = bss[i].signal_mbm/100;
                }

                if(strcmp(wifiSSID, "Amped_Wireless_3") == 0)
                {
                    strcpy(signal3[timeExpired], wifiSSID);
                    strength3[timeExpired] = bss[i].signal_mbm/100;
                }
            }

            printf("%s\n", signal1[timeExpired]);
            printf("%d\n", strength1[timeExpired]);
            printf("\n%s\n", signal2[timeExpired]);
            printf("%d\n", strength2[timeExpired]);
            printf("\n%s\n", signal3[timeExpired]);
            printf("%d\n", strength3[timeExpired]);
        sleep(2);
    }

    //free the library resources
    wifi_scan_close(wifi);

    triangulate(strength1, strength2, strength3);

    return 0;
}