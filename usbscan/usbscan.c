/**
	@file
	@author SM
*/

#include "stdio.h"
#include "libusb-1.0/libusb.h"

#define BULK_EP_OUT     0x01
#define BULK_EP_IN      0x81

char gBuffer[300];




char* GetDescriptorTypeDesc(int type)
{
	switch (type) {
	case LIBUSB_DT_DEVICE: return "DEVICE";
	default: return "?";
	}
}


char* GetClassDesc(int nClass, int nSubClass)
{
	static char desc[100];
	switch(nClass)
	{
	case LIBUSB_CLASS_PER_INTERFACE: return "Interface"; break;
	case LIBUSB_CLASS_HID: 			return "HID"; break;
	case LIBUSB_CLASS_APPLICATION: 	return "Application"; break;
	case LIBUSB_CLASS_HUB: 			return "Hub"; break;
	case LIBUSB_CLASS_VENDOR_SPEC: 	return "VendSpec"; break;
	default:
		sprintf( desc, "?%d+%d", nClass, nSubClass);
		return desc;
	}
}


char* GetDeviceSpeed(int nSpeed)
{
	switch(nSpeed){
	case LIBUSB_SPEED_LOW: return "Low speed"; break;
	case LIBUSB_SPEED_FULL: return "Full speed"; break;
	case LIBUSB_SPEED_HIGH: return "High speed"; break;
	case LIBUSB_SPEED_SUPER: return "Super speed"; break;
	default: return "? Not handled";
	}
}

char* GetAttributesDesc (int nAttrib)
{
	gBuffer[0]=0;
	if (nAttrib & 0x40) strcat( gBuffer, "SelfPowered ");
	if (nAttrib & 0x20) strcat( gBuffer, "RemoteWakeup ");
	return gBuffer;
}


/*
int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                     libusb_hotplug_event event, void *user_data) {
  static libusb_device_handle *dev_handle = NULL;
  struct libusb_device_descriptor desc;
  int rc;
  (void)libusb_get_device_descriptor(dev, &desc);
  if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
    rc = libusb_open(dev, &dev_handle);
    if (LIBUSB_SUCCESS != rc) {
      printf("Could not open USB device\n");
    }
  } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
    if (dev_handle) {
      libusb_close(dev_handle);
      dev_handle = NULL;
    }
  } else {
    printf("Unhandled event %d\n", event);
  }
  count++;
  return 0;
}
*/


/**
 * Punto d'ingresso
 */
int main (int argc, char** argv)
{
	libusb_context *context;
	libusb_device **deviceList;

	libusb_init(&context);

	/*
	  rc = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
	                                        LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, 0x045a, 0x5005,
	                                        LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL,
	                                        &callback_handle);
	  if (LIBUSB_SUCCESS != rc) {
	    printf("Error creating a hotplug callback\n");
	    libusb_exit(NULL);
	    return EXIT_FAILURE;
	  }
	 */

	const struct libusb_version* ver = libusb_get_version();
	printf("Lib ver: %d.%d.%d - %s\n", ver->major, ver->minor, ver->micro, ver->describe);

	int nDevices = libusb_get_device_list( context, &deviceList);
	printf("Found %d USB devices\n", nDevices);

	int i = 0;
	while( i < nDevices){
		libusb_device *pDevice = deviceList[i];
		struct libusb_device_descriptor devDescriptor;

		libusb_get_device_descriptor( pDevice, &devDescriptor);

		uint8_t nBus = libusb_get_bus_number(pDevice);
		uint8_t nAddr = libusb_get_device_address(pDevice);

		int speed = libusb_get_device_speed(pDevice);

		printf("\n%d - VID:%04X PID:%04X - class:%s - Bus:%d Device:%d - speed:%s \n",
				i+1,
				devDescriptor.idVendor, devDescriptor.idProduct,
				GetClassDesc(devDescriptor.bDeviceClass, devDescriptor.bDeviceSubClass),
				nBus, nAddr,
				GetDeviceSpeed(speed));
		printf("-- NumConfig:%d - MaxPacketSise:%d\n", devDescriptor.bNumConfigurations, devDescriptor.bMaxPacketSize0);

		printf("-- Descriptor type: %s\n", GetDescriptorTypeDesc(devDescriptor.bDescriptorType));


#define BUFFER_SIZE		260

		int nConfig;
		libusb_device_handle* devHandle;
		unsigned char buffer[BUFFER_SIZE];

		int nRet = libusb_open( pDevice, &devHandle);
		if (nRet==0){
			if (devDescriptor.iManufacturer){
				libusb_get_string_descriptor_ascii( devHandle, devDescriptor.iManufacturer, buffer, BUFFER_SIZE);
				printf("-- Manuf: %s\n", buffer);
			}
			if (devDescriptor.iProduct){
				libusb_get_string_descriptor_ascii( devHandle, devDescriptor.iProduct, buffer, BUFFER_SIZE);
				printf("-- Product: %s\n", buffer);
			}
			if (devDescriptor.iSerialNumber){
				libusb_get_string_descriptor_ascii( devHandle, devDescriptor.iSerialNumber, buffer, BUFFER_SIZE);
				printf("-- Serial: %s\n", buffer);
			}

			// libusb_get_configuration( devHandle, &nConfig);

			int n = 0;
			struct libusb_config_descriptor* configDesc;
			while( n < devDescriptor.bNumConfigurations ){
				nRet = libusb_get_config_descriptor( pDevice, n, &configDesc);
				if (nRet == LIBUSB_SUCCESS){
					printf("--- MaxPower: %d\n", configDesc->MaxPower);
					printf("--- bmAttributes: %02X - %s\n", configDesc->bmAttributes, GetAttributesDesc(configDesc->bmAttributes));
					printf("--- NumInterfaces: %d\n", configDesc->bNumInterfaces);

					libusb_free_config_descriptor(configDesc);
				}
				else {
					printf("-- Error getting config descr\n");
				};
				++n;
			}

			libusb_close(devHandle);
		}
		else printf("-- Open error: %d\n", nRet);

		++i;
	}
	libusb_free_device_list( deviceList, 1);

	// Scanner Canon LIDE VID:04A9 PID:2202 FB620U
	printf("\nOpening scanner ..\n");

	libusb_device_handle *hScanner;

	unsigned char bufCmd[] = { 0x01, 0x00, 0x0, 0x03, 0x04 };
	int nActualLen;

	unsigned char bufIn[100];

	hScanner = libusb_open_device_with_vid_pid( context, 0x04A9, 0x2202);

	if (hScanner) {
		int nRet;

		// 0000  1b 00 08 f0 2d 84 ff ff ff ff 00 00 00 00 09 00   ....-...........
		// 0010  00 08 00 01 00 03 03 04 00 00 00 01 07 00 01      ...............
		//
		// 0000  1b 00 08 f0 2d 84 ff ff ff ff 00 00 00 00 09 00   ....-...........
		// 0010  01 08 00 01 00 82 03 01 00 00 00 00
		nRet = libusb_bulk_transfer( hScanner, BULK_EP_OUT, bufCmd, sizeof(bufCmd), &nActualLen, 2000);
		printf("Write ret=%d tx=%d\n", nRet, nActualLen);

		nRet = libusb_bulk_transfer( hScanner, BULK_EP_IN, bufIn, sizeof(bufIn), &nActualLen, 2000);
		printf("Read ret=%d tx=%d\n", nRet, nActualLen);

		libusb_close(hScanner);
	}
	else
		printf("Device not found !!\n\n");

	// libusb_hotplug_deregister_callback(NULL, callback_handle);
	libusb_exit(context);
}
