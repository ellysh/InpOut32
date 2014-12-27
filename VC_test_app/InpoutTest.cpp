// InpoutTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "stdio.h"

typedef void	(__stdcall *lpOut32)(short, short);
typedef short	(__stdcall *lpInp32)(short);
typedef BOOL	(__stdcall *lpIsInpOutDriverOpen)(void);
typedef BOOL	(__stdcall *lpIsXP64Bit)(void);

//Some global function pointers (messy but fine for an example)
lpOut32 gfpOut32;
lpInp32 gfpInp32;
lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
lpIsXP64Bit gfpIsXP64Bit;

void Beep(unsigned int freq)
{
	gfpOut32(0x43, 0xB6);
	gfpOut32(0x42, (freq & 0xFF));
	gfpOut32(0x42, (freq >> 9));
	Sleep(10);
	gfpOut32(0x61, gfpInp32(0x61) | 0x03);
}

void StopBeep()
{
	gfpOut32(0x61, (gfpInp32(0x61) & 0xFC));
}

int main(int argc, char* argv[])
{
	if(argc<3)
	{
		//too few command line arguments, show usage
		printf("Error : too few arguments\n\n***** Usage *****\n\nInpoutTest read <ADDRESS> \nor \nInpoutTest write <ADDRESS> <DATA>\n\n\n\n\n");
	} 
	else
	{
		//Dynamically load the DLL at runtime (not linked at compile time)
		HINSTANCE hInpOutDll ;
		hInpOutDll = LoadLibrary ( "InpOut32.DLL" ) ;	//The 32bit DLL. If we are building x64 C++ 
														//applicaiton then use InpOutx64.dll
		if ( hInpOutDll != NULL )
		{
			gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
			gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
			gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
			gfpIsXP64Bit = (lpIsXP64Bit)GetProcAddress(hInpOutDll, "IsXP64Bit");

			if (gfpIsInpOutDriverOpen())
			{
				//Make some noise through the PC Speaker - hey it can do more that a single beep using InpOut32
				Beep(2000);
				Sleep(200);
				Beep(1000);
				Sleep(300);
				Beep(2000);
				Sleep(250);
				StopBeep();

				if(!strcmp(argv[1],"read"))
				{
					short iPort = atoi(argv[2]);
					WORD wData = gfpInp32(iPort);	//Read the port
					printf("Data read from address %s is %d \n\n\n\n", argv[2], wData);
				}
				else if(!strcmp(argv[1],"write"))
				{
					if(argc<4)
					{
						printf("Error in arguments supplied");
						printf("\n***** Usage *****\n\nInpoutTest read <ADDRESS> \nor \nInpoutTest write <ADDRESS> <DATA>\n\n\n\n\n");
					}
					else
					{
						short iPort = atoi(argv[2]);
						WORD wData = atoi(argv[3]);
						gfpOut32(iPort, wData);
						printf("data written to %s\n\n\n", argv[2]);
					}
				}
			}
			else
			{
				printf("Unable to open InpOut32 Driver!\n");
			}

			//All done
			FreeLibrary ( hInpOutDll ) ;
			return 0;
		}
		else
		{
			printf("Unable to load InpOut32 DLL!\n");
			return -1;
		}
	}
	return -2;
}
