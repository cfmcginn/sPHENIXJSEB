//Author: Chris McGinn, 2021.07.23
//If bug is discovered, please contact at chmc7718@colorado.edu, cffionn@gmail.com, or cffionn on skype
//The following borrows heavily from Cheng-Yi Chi's sPHENIX JSEB2 testing code
//Changes are largely rewrites for simplicity and clarity for future users

//c headers
#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//Windrvr specific header files
#include "wdc_defs.h"
#include "wdc_lib.h"
#include "samples/shared/diag_lib.h"
#include "samples/shared/wdc_diag_lib.h"
#include "include/pci_regs.h"

//JSEB specific headers:
#include "include/jseb2_lib.h"

/* Error messages display */
#define JSEB2_ERR printf

//we are declaring a large multidimensional array for all string handling
const int nStrings = 100;
const int nMaxStrChar = 256;
//Permaglobal location of all strings
char allStrings[100][256];

char allParamNames[100][256];
char allParamVals[100][256];

//Defined getSize temporarily here - just took direct from stackoverflow
//https://stackoverflow.com/questions/48367022/c-iterate-through-char-array-with-a-pointer
int getSize(char* s)
{
  char* t; // first copy the pointer to not change the original
  int size = 0;

  for(t = s; *t != '\0'; t++){
    size++;
  }

  return size;
}

//Tied to array allStrings - hardcoding sizes for now at 100 strings, 100 char w/ check as defined by nStrings and nMaxStrChar
void setPermaString(int pos, char* inStr)
{
  if(pos >= nStrings){
    printf("WARNING IN SET PERMA STRING: Position requested \'%d\', exceeds max allowed \'%d\'. return\n", pos, nStrings);
    return;
  }

  int inStrSize = getSize(inStr);
  if(inStrSize > nMaxStrChar){
    printf("WARNING IN SET PERMA STRING: \'%s\' has size \'%d\', exceeding max allowed \'%d\'. return\n", inStr, inStrSize, nMaxStrChar);
    return;
  }

  //We wanna strip out the last character if its new line
  if(inStr[inStrSize - 1] == '\n') --inStrSize;
  
  for(int cI = 0; cI < inStrSize; ++cI){
    allStrings[pos][cI] = (inStr)[cI];
  }
  
  return;
}

void combinePermaString(int pos, char* inStr1, char* inStr2)
{
  if(pos >= nStrings){
    printf("WARNING IN SET PERMA STRING: Position requested \'%d\', exceeds max allowed \'%d\'. return\n", pos, nStrings);
    return;
  }

  int inStr1Size = getSize(inStr1);
  int inStr2Size = getSize(inStr1);
  if(inStr1Size + inStr2Size > nMaxStrChar){
    printf("WARNING IN COMBINE PERMA STRING: \'%s\' has size \'%d\' and \'%s\' has size \'%d\', exceeding max allowed \'%d\'. return\n", inStr1, inStr1Size, inStr2, inStr2Size, nMaxStrChar);
    return;
  }

  //We wanna strip out the last character if its new line
  if(inStr1[inStr1Size - 1] == '\n') --inStr1Size;
  if(inStr2[inStr2Size - 2] == '\n') --inStr2Size;
  
  for(int cI = 0; cI < inStr1Size; ++cI){
    allStrings[pos][cI] = (inStr1)[cI];
  }

  for(int cI = 0; cI < inStr2Size; ++cI){
    allStrings[pos][cI + inStr1Size] = (inStr2)[cI];
  }
  
  return;
}

//Dedicated function for setting paramnames; Probably can be down w/ a template or array as an argument
void setParamArrName(int pos, char* inParamName)
{
  if(pos >= nStrings){
    printf("WARNING IN SET PERMA STRING: Position requested \'%d\', exceeds max allowed \'%d\'. return\n", pos, nStrings);
    return;
  }

  int inStrSize = getSize(inParamName);
  if(inStrSize > nMaxStrChar){
    printf("WARNING IN SET PERMA STRING: \'%s\' has size \'%d\', exceeding max allowed \'%d\'. return\n", inParamName, inStrSize, nMaxStrChar);
    return;
  }

  for(int i = 0; i < inStrSize; ++i){
    allParamNames[pos][i] = inParamName[i];
  }

  return;
}

//Dedicated function for setting paramvals; Probably can be down w/ a template or array as an argument
void setParamArrVal(int pos, char* inParamVal)
{
  if(pos >= nStrings){
    printf("WARNING IN SET PERMA STRING: Position requested \'%d\', exceeds max allowed \'%d\'. return\n", pos, nStrings);
    return;
  }

  int inStrSize = getSize(inParamVal);
  if(inStrSize > nMaxStrChar){
    printf("WARNING IN SET PERMA STRING: \'%s\' has size \'%d\', exceeding max allowed \'%d\'. return\n", inParamVal, inStrSize, nMaxStrChar);
    return;
  }

  for(int i = 0; i < inStrSize; ++i){
    allParamVals[pos][i] = inParamVal[i];
  }

  return;
}

//Short little function for parameter parsing via input config lines
//Typically something like
//'NAME: VALUE'
void splitPermaStringToParams(int pos)
{
  char* tempStr = allStrings[pos];
  int colonPoint = -1;
  int strSize = getSize(tempStr);
  
  for(int cI = 0; cI < strSize; ++cI){
    if(tempStr[cI] == ':'){
      colonPoint = cI;
      break;
    }
  }

  if(colonPoint < 0){
    printf("splitPermaStringToParams ERROR: Colon not found for pos \'%d\', word \'%s\'. return\n", pos, allStrings[pos]);
    return;
  }

  
  for(int cI = 0; cI < strSize; ++cI){
    if(cI < colonPoint){
      allParamNames[pos][cI] = allStrings[pos][cI];
    }
    else if(cI > colonPoint){
      if(allStrings[pos][cI] == ' ') ++colonPoint;
      else allParamVals[pos][cI - colonPoint - 1] = allStrings[pos][cI];      
    }
  }
  
  return;
}

//Just find JSEB2 device - copied direct from Chi's code
static BOOL DeviceFind(DWORD dwVendorId, DWORD dwDeviceId, WD_PCI_SLOT *pSlot)
{
  DWORD dwStatus;
  DWORD i, dwNumDevices;
  WDC_PCI_SCAN_RESULT scanResult;

  if(dwVendorId == 0){
    if(DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwVendorId, "Enter vendor ID", TRUE, 0, 0)){
      return FALSE;
    }

    if(DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&dwDeviceId, "Enter device ID", TRUE, 0, 0)){
      return FALSE;
    }
  }

  BZERO(scanResult);
  dwStatus = WDC_PciScanDevices(dwVendorId, dwDeviceId, &scanResult);
  if(WD_STATUS_SUCCESS != dwStatus){
    JSEB2_ERR("DeviceFind: Failed scanning the PCI bus.\n"
	      "Error: 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    return FALSE;
  }

  dwNumDevices = scanResult.dwNumDevices;
  if(!dwNumDevices){
    JSEB2_ERR("No matching device was found for search criteria "
	      "(Vendor ID 0x%lX, Device ID 0x%lX)\n",
	      dwVendorId, dwDeviceId);

    return FALSE;
  }

  printf("\n");
  printf("Found %ld matching device%s [ Vendor ID 0x%lX%s, Device ID 0x%lX%s ]:\n",
	 dwNumDevices, dwNumDevices > 1 ? "s" : "",
	 dwVendorId, dwVendorId ? "" : " (ALL)",
	 dwDeviceId, dwDeviceId ? "" : " (ALL)");

  for(int i = 0; i < dwNumDevices; i++){
    printf("\n");
    printf("%2ld. Vendor ID: 0x%lX, Device ID: 0x%lX\n",
	   i + 1,
	   scanResult.deviceId[i].dwVendorId,
	   scanResult.deviceId[i].dwDeviceId);

    WDC_DIAG_PciDeviceInfoPrint(&scanResult.deviceSlot[i], FALSE);
  }
  printf("\n");

  if(dwNumDevices > 1){
    sprintf(allStrings[nStrings-1], "Select a device (1 - %ld): ", dwNumDevices);
    i = 0;
    if(DIAG_INPUT_SUCCESS != DIAG_InputDWORD((PVOID)&i,
					     allStrings[nStrings-1], FALSE, 1, dwNumDevices)){
      return FALSE;
    }
  }

  *pSlot = scanResult.deviceSlot[i - 1];

  return TRUE;
}

static WDC_DEVICE_HANDLE DeviceOpen(const WD_PCI_SLOT *pSlot)
{
  WDC_DEVICE_HANDLE hDev;
  DWORD dwStatus;
  WD_PCI_CARD_INFO deviceInfo;

  /* Retrieve the device's resources information */
  BZERO(deviceInfo);
  deviceInfo.pciSlot = *pSlot;
  dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
  if(WD_STATUS_SUCCESS != dwStatus){
    JSEB2_ERR("DeviceOpen: Failed retrieving the device's resources information.\n"
	      "Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    return NULL;
  }

  /* NOTE: You can modify the device's resources information here, if                                 
       necessary (mainly the deviceInfo.Card.Items array or the items number -
       deviceInfo.Card.dwItems) in order to register only some of the resources
       or register only a portion of a specific address space, for example. */
  /* Open a handle to the device */
  hDev = JSEB2_DeviceOpen(&deviceInfo);
  if(!hDev){
    JSEB2_ERR("DeviceOpen: Failed opening a handle to the device: %s",
	      JSEB2_GetLastErr());
    return NULL;
  }

  return hDev;
}


//Find and open a JSEB2 device
static WDC_DEVICE_HANDLE DeviceFindAndOpen(DWORD dwVendorId, DWORD dwDeviceId)
{
  WD_PCI_SLOT slot;
  if(!DeviceFind(dwVendorId, dwDeviceId, &slot)) return NULL;

  return DeviceOpen(&slot);
}

//Temp declaration of pcie_send
static int pcie_send(WDC_DEVICE_HANDLE hDev, int mode, int nword, UINT32 *buff_send)
{
  /* imode =0 single word transfer, imode =1 DMA */
#include "wdc_defs.h"
  static DWORD dwAddrSpace;
  static DWORD dwDMABufSize;

  static UINT32 *buf_send;
  static WD_DMA *pDma_send;
  static DWORD dwStatus;
  static DWORD dwOptions_send = DMA_TO_DEVICE;
  static DWORD dwOffset;
  static UINT32 u32Data;
  static PVOID pbuf_send;
  int nwrite,i,j, iprint;
  static int ifr=0;

  if (ifr == 0) {
    ifr=1;
    dwDMABufSize = 140000;
    dwStatus = WDC_DMAContigBufLock(hDev, &pbuf_send, dwOptions_send, dwDMABufSize, &pDma_send);
    if (WD_STATUS_SUCCESS != dwStatus) {
      printf("Failed locking a send Contiguous DMA buffer. Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    }
    buf_send = pbuf_send;
  }
  iprint =0;
  if(mode ==1 ) {
    for (i=0; i< nword; i++) {
      *(buf_send+i) = *buff_send++;
      /*      printf("%d \n",*(buf_send+i));   */
    }
  }
  if(mode == 0) {
    nwrite = nword*4;
    /*setup transmiiter */
    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x18;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    dwAddrSpace =2;
    u32Data = 0x40000000+nwrite;
    dwOffset = 0x18;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    for (j=0; j< nword; j++) {
      dwAddrSpace =0;
      dwOffset = 0x0;
      u32Data = *buff_send++;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    }
    for (i=0; i<20000; i++) {
      dwAddrSpace =2;
      dwOffset = 0xC;
      WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
      if(iprint ==1) printf(" status reed %d %X \n", i, u32Data);
      if(((u32Data & 0x80000000) == 0) && iprint == 1) printf(" Data Transfer complete %d \n", i);
      if((u32Data & 0x80000000) == 0) break;
    }
  }
  if( mode ==1 ){
    nwrite = nword*4;
    WDC_DMASyncCpu(pDma_send);
    /*
      printf(" nwrite = %d \n", nwrite);
      printf(" pcie_send hDev = %d\n", hDev);
      printf(" buf_send = %X\n",*buf_send);
    */
    /*setup transmiiter */
    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x18;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    dwAddrSpace =2;
    u32Data = 0x40000000+nwrite;
    dwOffset = 0x18;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    /* set up sending DMA starting address */

    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x0;
    u32Data = pDma_send->Page->pPhysicalAddr & 0xffffffff;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x4;
    u32Data = (pDma_send->Page->pPhysicalAddr >> 32) & 0xffffffff;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    /* byte count */
    dwAddrSpace =2;
    dwOffset = 0x8;
    u32Data = nwrite;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    /* write this will start DMA */
    dwAddrSpace =2;
    dwOffset = 0xc;
    u32Data = 0x00100000;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    for (i=0; i<20000; i++) {
      dwAddrSpace =2;
      dwOffset = 0xC;
      WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
      if(iprint ==1) printf(" DMA status reed %d %X \n", i, u32Data);
      if(((u32Data & 0x80000000) == 0) && iprint == 1) printf(" DMA complete %d \n", i);
      if((u32Data & 0x80000000) == 0) break;
    }
    WDC_DMASyncIo(pDma_send);
  }
  return i;
}


//Temp declaration of pcie_send_1
static int pcie_send_1(WDC_DEVICE_HANDLE hDev, int mode, int nword, UINT32 *buff_send)
{
  /* imode =0 single word transfer, imode =1 DMA */
#include "wdc_defs.h"
  static DWORD dwAddrSpace;
  static DWORD dwDMABufSize;

  static UINT32 *buf_send;
  static WD_DMA *pDma_send;
  static DWORD dwStatus;
  static DWORD dwOptions_send = DMA_TO_DEVICE;
  static DWORD dwOffset;
  static UINT32 u32Data;
  static PVOID pbuf_send;
  int nwrite,i,j, iprint,is;
  static int ifr=0;

  if(ifr == 0){
    ifr=1;
    dwDMABufSize = 140000;
    dwStatus = WDC_DMAContigBufLock(hDev, &pbuf_send, dwOptions_send, dwDMABufSize, &pDma_send);
    if (WD_STATUS_SUCCESS != dwStatus) {
      printf("Failed locking a send Contiguous DMA buffer. Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    }
    buf_send = pbuf_send;
  }
  iprint =0;
  if(mode == 1){
    for(i=0; i< nword; i++){
      *(buf_send+i) = *buff_send++;
    }
  }
  if(mode == 0){
    nwrite = nword*4;
    /*setup transmiiter */
    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x20;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    dwAddrSpace =2;
    u32Data = 0x40000000+nwrite;
    dwOffset = 0x20;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    for (j=0; j< nword; j++) {
      dwAddrSpace =4;
      dwOffset = 0x0;
      u32Data = *buff_send++;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    }
    for (i=0; i<20000; i++) {
      dwAddrSpace =2;
      dwOffset = 0xC;
      WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
      if(iprint ==1) printf(" status reed %d %X \n", i, u32Data);
      if(((u32Data & 0x80000000) == 0) && iprint == 1) printf(" Data Transfer complete %d \n", i);
      if((u32Data & 0x80000000) == 0) break;
    }
  }

  if(mode == 1){
    nwrite = nword*4;
    WDC_DMASyncCpu(pDma_send);

    /*setup transmiiter */
    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x20;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    dwAddrSpace =2;
    u32Data = 0x40000000+nwrite;
    dwOffset = 0x20;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    /* set up sending DMA starting address */

    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x0;
    u32Data = pDma_send->Page->pPhysicalAddr & 0xffffffff;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x4;
    u32Data = (pDma_send->Page->pPhysicalAddr >> 32) & 0xffffffff;
     
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    /* byte count */
    dwAddrSpace =2;
    dwOffset = 0x8;
    u32Data = nwrite;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    /* write this will start DMA */
    dwAddrSpace =2;
    dwOffset = 0xc;
    u32Data = 0x00200000;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    for (i=0; i<2000000; i++) {
      dwAddrSpace =2;
      dwOffset = 0xC;
      WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
      if(iprint ==1) printf(" DMA status reed %d %X \n", i, u32Data);
      if(((u32Data & 0x80000000) == 0) && iprint == 1) printf(" DMA complete %d \n", i);
      if((u32Data & 0x80000000) == 0) break;
    }
    WDC_DMASyncIo(pDma_send);
  }
  return i;
}

//Temp declaration of pcie_rec
static int pcie_rec(WDC_DEVICE_HANDLE hDev, int mode, int istart, int nword, int ipr_status, UINT32 *buff_rec)
{
  /* imode =0 single word transfer, imode =1 DMA */
#include "wdc_defs.h"
  static DWORD dwAddrSpace;
  static DWORD dwDMABufSize;

  static UINT32 *buf_rec;
  static WD_DMA *pDma_rec;
  static DWORD dwStatus;
  static DWORD dwOptions_rec = DMA_FROM_DEVICE;
  static DWORD dwOffset;
  static UINT32 u32Data;
  static UINT64 u64Data;
  static PVOID pbuf_rec;
  int nread,i,j, iprint,icomp;
  static int ifr=0;
  
  if (ifr == 0) {
    ifr=1;
    dwDMABufSize = 140000;
    dwStatus = WDC_DMAContigBufLock(hDev, &pbuf_rec, dwOptions_rec, dwDMABufSize, &pDma_rec);
    if (WD_STATUS_SUCCESS != dwStatus) {
      printf("Failed locking a send Contiguous DMA buffer. Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    }
    buf_rec = pbuf_rec;
  }
  iprint =0;
  if((istart == 1) | (istart == 3)) {
    dwAddrSpace =2;
    u32Data = 0xf0000008;
    dwOffset = 0x28;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

    /*initialize the receiver */
    dwAddrSpace =2;
    u32Data = 0x20000000;
    dwOffset = 0x1c;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    /* write byte count **/
    dwAddrSpace =2;
    u32Data = 0x40000000+nword*4;
    dwOffset = 0x1c;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    if(ipr_status ==1) {
      dwAddrSpace =2;
      u64Data =0;
      dwOffset = 0x18;
      WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
      printf (" status word before read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
    }

    return 0;
  }
  if ((istart == 2) | (istart == 3)) {
    if(mode == 0) {
      nread = nword/2+1;
      if(nword%2 == 0) nread = nword/2;
      for (j=0; j< nread; j++) {
	dwAddrSpace =0;
	dwOffset = 0x0;
	u64Data =0xbad;
	WDC_ReadAddr64(hDev,dwAddrSpace, dwOffset, &u64Data);
	//       printf("u64Data = %16X\n",u64Data);
	*buff_rec++ = (u64Data &0xffffffff);
	*buff_rec++ = u64Data >>32;
	//       printf("%x \n",(u64Data &0xffffffff));
	//       printf("%x \n",(u64Data >>32 ));
	//       if(j*2+1 > nword) *buff_rec++ = (u64Data)>>32;
	//       *buff_rec++ = 0x0;
      }
      if(ipr_status ==1) {
	dwAddrSpace =2;
	u64Data =0;
	dwOffset = 0x18;
	WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
	printf (" status word after read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
      }
      return 0;
    }
    if( mode ==1 ){
      nread = nword*4;
      WDC_DMASyncCpu(pDma_rec);

      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x0;
      u32Data = pDma_rec->Page->pPhysicalAddr & 0xffffffff;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

      dwAddrSpace =2;
      u32Data = 0x20000000;
      dwOffset = 0x4;
      u32Data = (pDma_rec->Page->pPhysicalAddr >> 32) & 0xffffffff;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

      /* byte count */
      dwAddrSpace =2;
      dwOffset = 0x8;
      u32Data = nread;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

      /* write this will start DMA */
      dwAddrSpace =2;
      dwOffset = 0xc;
      u32Data = 0x00100040;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
      icomp=0;
      for (i=0; i<20000; i++) {
        dwAddrSpace =2;
        dwOffset = 0xC;
        WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
        if(iprint ==1) printf(" DMA status read %d %X \n", i, u32Data);
        if(((u32Data & 0x80000000) == 0)) {
          icomp=1;
          if(iprint == 1) printf(" DMA complete %d \n", i);
        }
        if((u32Data & 0x80000000) == 0) break;
      }
      if(icomp == 0) {
        printf("DMA timeout\n");
        return 1;
      }
      WDC_DMASyncIo(pDma_rec);
      for (i=0; i< nword; i++) {
        *buff_rec++ = *(buf_rec+i);
	/*      printf("%d \n",*(buf_send+i));   */
      }
    }
  }
  return 0;
}

//Temp declaration of pcie_rec_2
static int pcie_rec_2(WDC_DEVICE_HANDLE hDev, int mode, int istart, int nword, int ipr_status, UINT32 *buff_rec)
{
  /* imode =0 single word transfer, imode =1 DMA */
  /* nword assume to be number of 16 bits word */
  
#include "wdc_defs.h"
#define  t1_tr_bar 0
#define  t2_tr_bar 4
#define  cs_bar 2

  /**  command register location **/

#define  tx_mode_reg 0x28
#define  t1_cs_reg 0x18
#define  r1_cs_reg 0x1c
#define  t2_cs_reg 0x20
#define  r2_cs_reg 0x24

#define  tx_md_reg 0x28

#define  cs_dma_add_low_reg 0x0
#define  cs_dma_add_high_reg  0x4
#define  cs_dma_by_cnt 0x8
#define  cs_dma_cntrl 0xc
#define  cs_dma_msi_abort 0x10

  /** define status bits **/

#define  cs_init  0x20000000
#define  cs_mode_p 0x8000000
#define  cs_mode_n 0x0
#define  cs_start 0x40000000
#define  cs_done  0x80000000

#define  dma_tr1  0x100000
#define  dma_tr2  0x200000
#define  dma_tr12 0x300000
#define  dma_3dw_trans 0x0
#define  dma_3dw_rec   0x40
#define  dma_4dw_rec   0x60
#define  dma_in_progress 0x80000000

#define  dma_abort 0x2

  static DWORD dwAddrSpace;
  static DWORD dwDMABufSize;

  static UINT32 *buf_rec;
  static WD_DMA *pDma_rec;
  static DWORD dwStatus;
  static DWORD dwOptions_rec = DMA_FROM_DEVICE;
  static DWORD dwOffset;
  static UINT32 u32Data;
  static UINT64 u64Data;
  static PVOID pbuf_rec;
  int nread,i,j, iprint,icomp,is;
  static int ifr=0;


  if (ifr == 0) {
    ifr=1;
    dwDMABufSize = 140000;
    dwStatus = WDC_DMAContigBufLock(hDev, &pbuf_rec, dwOptions_rec, dwDMABufSize, &pDma_rec);
    if(WD_STATUS_SUCCESS != dwStatus){
      printf("Failed locking a send Contiguous DMA buffer. Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
    }
    buf_rec = pbuf_rec;
  }
  iprint =0;
  //   printf(" istart = %d\n", istart);
  //   printf(" mode   = %d\n", mode);
  /** set up the receiver **/
  if((istart == 1) | (istart == 3)) {
    // initalize transmitter mode register...
    dwAddrSpace =2;
    u32Data = 0xf0000008;
    dwOffset = tx_mode_reg;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    if(mode == 1) {
      /* write this will abort previous DMA */
      dwAddrSpace =2;
      dwOffset = cs_dma_msi_abort;
      u32Data = dma_abort;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
      /* clear DMA register after the abort */
      dwAddrSpace =2;
      dwOffset = cs_dma_msi_abort;
      u32Data = 0;
      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    }
    /*initialize the receiver */
    dwAddrSpace =cs_bar;
    u32Data = cs_init;
    dwOffset = r2_cs_reg;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    /* write byte count **/
    dwAddrSpace =cs_bar;
    u32Data = cs_start+nword*4;
    dwOffset = r2_cs_reg;
    WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
    if(ipr_status ==1) {
      dwAddrSpace =cs_bar;
      u64Data =0;
      dwOffset = t2_cs_reg;
      WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
      printf (" status word before read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
    }
    
    return 0;
  }
  if ((istart == 2) | (istart == 3)) {
    //     if(ipr_status ==1) {
    //      dwAddrSpace =2;
    //      u64Data =0;
    //      dwOffset = 0x18;
    //      WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
    //      printf (" status word before read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
    //     }

    if(mode == 0) {
      nread = nword/2+1;
      if(nword%2 == 0) nread = nword/2;
      for (j=0; j< nread; j++) {
	dwAddrSpace =t2_tr_bar;
	dwOffset = 0x0;
	u64Data =0xbad;
	WDC_ReadAddr64(hDev,dwAddrSpace, dwOffset, &u64Data);
	//       printf("u64Data = %16X\n",u64Data);
	*buff_rec++ = (u64Data &0xffffffff);
	*buff_rec++ = u64Data >>32;
	//       printf("%x \n",(u64Data &0xffffffff));
	//       printf("%x \n",(u64Data >>32 ));
	//       if(j*2+1 > nword) *buff_rec++ = (u64Data)>>32;
	//       *buff_rec++ = 0x0;
      }
      if(ipr_status ==1) {
	dwAddrSpace =cs_bar;
	u64Data =0;
	dwOffset = t2_cs_reg;
	WDC_ReadAddr64(hDev, dwAddrSpace, dwOffset, &u64Data);
	printf (" status word after read = %x, %x \n",(u64Data>>32), (u64Data &0xffff));
      }
      return 0;
    }
    if( mode ==1 ){            ///**** not up to date ****///           
      nread = nword*2;
      WDC_DMASyncCpu(pDma_rec);
      /*
	printf(" nwrite = %d \n", nwrite);
	printf(" pcie_send hDev = %d\n", hDev);
	printf(" buf_send = %X\n",*buf_send);
      */
      /*setup receiver
	dwAddrSpace =2;
	u32Data = 0x20000000;
	dwOffset = 0x1c;
	WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
	dwAddrSpace =2;
	u32Data = 0x40000000+nread;
	dwOffset = 0x1c;
	WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);
      */
      /* set up sending DMA starting address */

	dwAddrSpace =2;
	u32Data = 0x20000000;
	dwOffset = 0x0;
	u32Data = pDma_rec->Page->pPhysicalAddr & 0xffffffff;
	WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

	dwAddrSpace =2;
	u32Data = 0x20000000;
	dwOffset = 0x4;
	u32Data = (pDma_rec->Page->pPhysicalAddr >> 32) & 0xffffffff;
	WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

	/* byte count */
	dwAddrSpace =2;
	dwOffset = 0x8;
	u32Data = nread;
	WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

	/* write this will start DMA */
	//      dwAddrSpace =2;
	//      dwOffset = 0xc;
	//      u32Data = 0x00100040;
	//      WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

	/* write this will start DMA */
	dwAddrSpace =2;
	dwOffset = cs_dma_cntrl;
	is = (pDma_rec->Page->pPhysicalAddr >> 32) & 0xffffffff;
	if(is == 0) {
	  //**         if(iwrite !=1 ) printf(" use 3dw \n");
	  u32Data = dma_tr2+dma_3dw_rec;
	}
	else {
	  u32Data = dma_tr2+dma_4dw_rec;
	  //**        if(iwrite !=1 ) printf(" use 4dw \n");
	}
	WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

	icomp=0;
	for (i=0; i<20000; i++) {
	  dwAddrSpace =2;
	  dwOffset = 0xc;
	  WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, &u32Data);
	  if(iprint ==1 && i%1000 == 0) printf(" DMA status read %d %X \n", i, u32Data); // CFM Edit 2021.05.11 - set to only print on every 1000th event
	  if(((u32Data & 0x80000000) == 0)) {
	    icomp=1;
	    //          if(iprint == 1) printf(" DMA complete %d \n", i);
	    printf(" DMA complete %d \n", i);
	  }
	  if((u32Data & 0x80000000) == 0) break;
	}
	if(icomp == 0) {
	  printf("DMA timeout, %d\n", __LINE__);
	  return 1;
	}
	WDC_DMASyncIo(pDma_rec);
	for (i=0; i< nword; i++) {
	  *buff_rec++ = *(buf_rec+i);
	  /*      printf("%d \n",*(buf_send+i));   */
	}
    }
  }
  return 0;
}




//Defining adc_setup
static int adc_setup(WDC_DEVICE_HANDLE hDev, int imod)
{
#define  sp_cntrl_timing            3
#define  sp_cntrl_init           0x30
#define  sp_cntrl_reset          0x28
#define  sp_cntrl_l1             0x24
#define  sp_cntrl_pulse          0x22



#define  sp_adc_readback_sub        4
#define  sp_adc_readback_transfer   1
#define  sp_adc_readback_read       2
#define  sp_adc_readback_status     3

#define  sp_adc_input_sub           2
#define  sp_adc_slowcntl_sub        1

#define  sp_adc_l1_delay            1
#define  sp_adc_evt_sample          2

#define  sp_adc_rd_link             3
#define  sp_adc_rd_cntrl            4

#define  sp_adc_sel_l1              5
#define  sp_adc_sel_pulse           6
#define  sp_adc_sel_test_trig       7

#define  sp_adc_sel_linux_rxoff     8

#define  sp_adc_u_adc_align        10
#define  sp_adc_l_adc_align        11
#define  sp_adc_pll_reset          12

#define  sp_adc_rstblk             13
#define  sp_adc_test_pulse         14

#define  sp_adc_dpa_reset          15

#define  sp_adc_spi_add            20
#define  sp_adc_spi_data           30

#define  sp_adc_lnk_tx_dreset      20
#define  sp_adc_lnk_tx_areset      21
#define  sp_adc_trg_tx_dreset      22
#define  sp_adc_trg_tx_areset      23
#define  sp_adc_lnk_rx_dreset      24
#define  sp_adc_lnk_rx_areset      25

#define  sp_adc_link_mgmt_reset    26
#define  sp_adc_link_conf_w        27
#define  sp_adc_link_conf_add      30
#define  sp_adc_link_conf_data_l   31
#define  sp_adc_link_conf_data_u   32

#define  sp_adc_calib_gate        104

#define  sp_xmit_lastmod            1
#define  sp_xmit_rxanalogreset      2
#define  sp_xmit_rxdigitalreset     3
#define  sp_xmit_init               4

#define  sp_xmit_sub                1


  int ichip,ich,i,k;
  UINT32 buf_send[40000];
  UINT32 *px;

  px = &buf_send;
  ichip = sp_adc_input_sub ;   // controller data go to ADC input section                             
  for (ich=0; ich<8; ich++) {
    //    set spi address
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_spi_add + (ich<<16);
    i = 1;
    k = 1;
    i = pcie_send_1(hDev, i, k, px);

    //      printf(" set spi address to channel %d, module %d\n", ich, imod);
    //      printf(" power reset \
    //      buf_send[0]=(imod<<11)+(ichip<<8)+(mb_pmt_spi_)+((is & 0xf)<<16); //set spi address
    //      i=1;
    //        k=1;
    //        i = pcie_send(hDev, i, k, px);
    //       printf(" spi port %d \n",is);
    //       scanf("%d",&ik);
    //        imod=11;
    //        ichip=5;
    //
    //
    //     power reset the ADC
    //
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0x8))+((0x03)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x8, data =0x03;
//
    i=1;
    k=2;
 //       i = pcie_send(hDev, i, k, px);
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us                                                                  
    //printf(" set spi address to channel %d, module %d\n", ich, imod);
    //   printf(" remove power reset \n");
   //       scanf("%d",&is);

 //
 //     reove power reset
 //
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word                                                                                 
    buf_send[1]=(((0x0)<<13)+(0x8))+((0x00)<<24)+((0x0)<<16);
    //
    //  set /w =0, w1,w2 =0, a12-a0 = 0x8, data =0x00;
    //
    i=1;
    k=2;
    //       i = pcie_send(hDev, i, k, px);
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us   
    //
    //     reset ADC
    //
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word                                                                                 
    buf_send[1]=(((0x0)<<13)+(0x0))+((0x3c)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0x0, data =0x3c;
//
    i=1;
    k=2;
    //       i = pcie_send(hDev, i, k, px);
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us                                                                  
    //

    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<12)+(0x15))+((0x20)<<24)+((0x0)<<16);   // 100 ohms termation
    //     buf_send[1]=(((0x0)<<12)+(0x15))+((0x0)<<24)+((0x0)<<16);   // no ohms termation, 1x drive
    //     buf_send[1]=(((0x0)<<12)+(0x15))+((0x1)<<24)+((0x0)<<16);   // no ohms termation, 2x drive
    //
    //  set /w =0, w1,w2 =0, a12-a0 = 0x15, data =0x1;
    //
    i=1;
    k=2;
    //       i = pcie_send(hDev, i, k, px);
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us                                                                  
    //       printf(" termination set, type 1 to continue \n");
    //       scanf("%d",&i);

 //
 //    set fix pattern  0xa = sync pattern
 //
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0xd))+((0xa)<<24)+((0x0)<<16);
    //
    //  set /w =0, w1,w2 =0, a12-a0 = 0xd, data =0xa;
    //
    i=1;
    k=2;
    //      i = pcie_send(hDev, i, k, px);
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us                                                                  
    //
    //       printf(" fix pattern set type 1 to continue \n");
    //       scanf("%d",&i);
  }

  usleep(100);
  ichip = sp_adc_slowcntl_sub ;   // controller data go to ADC input section
  //       pll reset
  //   
  buf_send[0] = (imod <<11) + (ichip << 8) + sp_adc_pll_reset + (0<<16);
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(1000);

  //
  //       DPA reset
  //   
  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_dpa_reset + (0<<16) ;
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(1000);

  //
  //       upper ADC alignment
  //
  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_u_adc_align + (0<<16) ;
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(1000);
  //
  //       lower ADC alignment
  //
  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_l_adc_align + (0<<16) ;
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(1000);

  ichip = sp_adc_input_sub ;   // controller data go to ADC input section
  for (ich=0; ich<8; ich++) {
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_spi_add + (ich<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);
    //
    //    output offset binary code
    //
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word                                                                                 
    buf_send[1]=(((0x0)<<12)+(0x14))+((0x0)<<24)+((0x0)<<16);
 //
 //  set /w =0, w1,w2 =0, a12-a0 = 0x14, data =0x0;
 //

    i=1;
    k=2;

    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us

    //
    //    unset fix pattern  0x0 for normal data taking   --- set to test condition
    //
    buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
    buf_send[1]=(((0x0)<<13)+(0xd))+((0x0)<<24)+((0x0)<<16);
 //
 //  set /w =0, w1,w2 =0, a12-a0 = 0xd, data =0x0;
 //

    i=1;
    k=2;

    i = pcie_send_1(hDev, i, k, px);
    usleep(100);   // sleep for 100us
  }
  //
  //  test routine
  //
  //    set spi address
  ich =0;
  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_spi_add + (ich<<16) ;
  i= 1;
  k= 1;
  //    i = pcie_send_1(hDev, i, k, px);
  usleep(100);
  //
  //    remove channel H,G,F,E from the selection list
  //
  buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
  buf_send[1]=(((0x0)<<13)+(0x4))+((0x0)<<24)+((0x0)<<16);
 //
 //  set /w =0, w1,w2 =0, a12-a0 = 0x4, data =0x0;
 //

  i=1;
  k=2;

  //    i = pcie_send_1(hDev, i, k, px);
  usleep(100);   // sleep for 100us
  //
  //    only keep channel a from the selection list
  //
  buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word                                                                                  
  buf_send[1]=(((0x0)<<13)+(0x5))+((0x1)<<24)+((0x0)<<16);
  //
  //  set /w =0, w1,w2 =0, a12-a0 = 0x5, data =0x1;
  //

  i=1;
  k=2;

  ///    i = pcie_send_1(hDev, i, k, px);
  usleep(100);   // sleep for 100us
  //
  //    set fix pattern  0xa = sync pattern
  //
  buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word                                                                                 
  buf_send[1]=(((0x0)<<13)+(0xd))+((0xc)<<24)+((0x0)<<16);
//
//  set /w =0, w1,w2 =0, a12-a0 = 0xd, data =0xc;
//

  i=1;
  k=2;
  //      i = pcie_send(hDev, i, k, px);
  //     i = pcie_send_1(hDev, i, k, px);
  usleep(100);   // sleep for 100us                                                                  
 //
 //    restore selection list 1
 //
  buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word                                                                                  
  buf_send[1]=(((0x0)<<13)+(0x4))+((0xf)<<24)+((0x0)<<16);
 //
 //  set /w =0, w1,w2 =0, a12-a0 = 0x4, data =0x0;
 //
  i=1;
  k=2;
  //    i = pcie_send_1(hDev, i, k, px);
  usleep(100);   // sleep for 100us                                
  //
  //    restore selection list 2
  //
  buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_spi_data)+(0x0300<<16); //1st next word will be overwrite by the next next word
   buf_send[1]=(((0x0)<<13)+(0x5))+((0x3f)<<24)+((0x0)<<16);
 //
 //  set /w =0, w1,w2 =0, a12-a0 = 0x5, data =0x1;
 //

 i=1;
 k=2;

 //    i = pcie_send_1(hDev, i, k, px);
 usleep(100);   // sleep for 100us

 return i;
}


//Also just copied in adc_testram_load
static int adc_testram_load(WDC_DEVICE_HANDLE hDev, int imod, int idelay)
{
#define  sp_cntrl_timing            3
#define  sp_cntrl_init           0x30
#define  sp_cntrl_reset          0x28
#define  sp_cntrl_l1             0x24
#define  sp_cntrl_pulse          0x22



#define  sp_adc_readback_sub        4
#define  sp_adc_readback_transfer   1
#define  sp_adc_readback_read       2
#define  sp_adc_readback_status     3
#
#define  sp_adc_input_sub           2
#define  sp_adc_slowcntl_sub        1

#define  sp_adc_l1_delay            1
#define  sp_adc_evt_sample          2

#define  sp_adc_rd_link             3
#define  sp_adc_rd_cntrl            4

#define  sp_adc_sel_l1              5
#define  sp_adc_sel_pulse           6
#define  sp_adc_sel_test_trig       7

#define  sp_adc_sel_linux_rxoff     8


#define  sp_adc_u_adc_align        10
#define  sp_adc_l_adc_align        11
#define  sp_adc_pll_reset          12

#define  sp_adc_rstblk             13
#define  sp_adc_test_pulse         14

#define  sp_adc_dpa_reset          15

#define  sp_adc_spi_add            20
#define  sp_adc_spi_data           30

#define  sp_adc_testram_load_ch     4
#define  sp_adc_testram_load_data   5
#define  sp_adc_testram_trig_delay  7

#define  sp_adc_lnk_tx_dreset      20
#define  sp_adc_lnk_tx_areset      21
#define  sp_adc_trg_tx_dreset      22
#define  sp_adc_trg_tx_areset      23
#define  sp_adc_lnk_rx_dreset      24
#define  sp_adc_lnk_rx_areset      25

#define  sp_adc_link_mgmt_reset    26
#define  sp_adc_link_conf_w        27
#define  sp_adc_link_conf_add      30
#define  sp_adc_link_conf_data_l   31
#define  sp_adc_link_conf_data_u   32

#define  sp_xmit_lastmod            1
#define  sp_xmit_rxanalogreset      2
#define  sp_xmit_rxdigitalreset     3
#define  sp_xmit_init               4

#define  sp_xmit_sub                1
  
  int ichip,ich,i,k,idata, iad;
 UINT32 buf_send[40000];
 UINT32 *px;
 
 
 
 px = &buf_send;
 ichip = sp_adc_input_sub ;   // controller data go to ADC input section
 
 for (ich =0; ich< 64; ich++) {
  if(ich== 0)  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_testram_load_ch + (64<<16);  // set channel 0 to 64                                                                                           
  else buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_testram_load_ch + (ich<<16) ;
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(1);
  printf(" load testram ch = %d, module %d\n", ich, imod);
   
  for (iad =0; iad<512 ; iad++) {
  idata =  ich*128 + iad;
  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_testram_load_data + (idata<<16) ;
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(1);   
}
}
 
 buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_testram_trig_delay + (idelay<<16) ;
 i= 1;
 k= 1;
 i = pcie_send_1(hDev, i, k, px);
 usleep(1);
 
 return i;
}




//Main function - Parse config file + run test + write output
int sphenixADCTest(char* inConfigFileName)
{
  //Start out defining some numbers
#define dcm2_run_off  254
#define dcm2_run_on   255

#define  dma_buffer_size        10000000  

#define  sp_cntrl_offline           3
#define  sp_cntrl_pulse          0x22
#define  sp_cntrl_timing            3
#define  sp_cntrl_init           0x30
#define  sp_cntrl_reset          0x28
#define  sp_adc_slowcntl_sub        1
#define  sp_adc_trg_tx_areset      23
#define  sp_adc_trg_tx_dreset      22
#define  sp_adc_calib_ch          101
#define  sp_adc_calib_dac         100
#define  sp_adc_calib_send        103


#define  sp_adc_l1_delay            1
#define  sp_adc_evt_sample          2
#define  sp_adc_rd_link             3
#define  sp_adc_rd_cntrl            4
#define  sp_adc_sel_l1              5
#define  sp_adc_sel_pulse           6

#define  sp_adc_test_pulse         14

#define  sp_adc_sel_caltrig        33

#define  sp_adc_readback_sub        4
#define  sp_adc_readback_status     3

#define  sp_adc_sel_link_rxoff     8

#define  sp_xmit_sub                1
#define  sp_xmit_rxanalogreset      2
#define  sp_xmit_rxdigitalreset     3

#define  sp_xmit_rxbytord          15
#define  sp_xmit_init               4

#define dcm2_online   2
#define dcm2_setmask  3
#define dcm2_run_off  254
#define dcm2_run_on   255
#define dcm2_5_firstdcm 8
#define dcm2_5_lastdcm  9

#define dcm2_5_readdata 4


  //Start working w/ the config file
  printf("'%s'\n", inConfigFileName);
  const int configFileNamePos = 0;
  setPermaString(configFileNamePos, inConfigFileName);

  //Read out our config file and save the options
  FILE* configFile = fopen(inConfigFileName, "r");
  if(configFile == NULL){
    printf("SPHENIXADCTEST ERROR: Given inConfigFileName \'%s\' is not valid. return 1\n", inConfigFileName);
    return 1;
  }
  
  static char* configLine;
  static size_t configLen;
  static size_t configRead;

  int paramPos = configFileNamePos+1;
  while((configRead = getline(&configLine, &configLen, configFile)) != -1){
    //Skipping empty and/or commented lines in the input config                                   
    if(configLen == 0) continue;
    if(configLine[0] == '#') continue;
    if(configLine[0] == '\0') continue;
    if(configLine[0] == '\n') continue;    
    
    setPermaString(paramPos, configLine);    
    ++paramPos;
  }

  //Setting the configfilename param manually
  setParamArrName(0, "INCONFIGFILENAME");
  setParamArrVal(0, inConfigFileName);

  const int nParams = paramPos;//All params w/ a +1 for the input config file name
  for(int pI = 1; pI < nParams; ++pI){
    splitPermaStringToParams(pI);
  }

  //Positions of all configurable params in the input
  int boardIDPos = -1;
  int channelMinPos = -1;
  int channelMaxPos = -1;
  int additionalTagPos = -1;
  int doDebugPos = -1;
  int numberOfLoopPos = -1;
  int numberOfEventPos = -1;
  int positionOfModulePos = -1;
  int numberFEMPos = -1;
  int useXMITPos = -1;
  int writeToFilePos = -1;
  int usePulsePos = -1;
  int useExternalTriggerPos = -1;
  int usePulseGenPos = -1;
  int useDCMPos = -1;
  int numberOfStepsPos = -1;
  int eventsPerStepPos = -1;
  int dacPerStepPos = -1;
  int signalPos = -1;
  int useFixedDACPos = -1;
  int l1DelayPos = -1;  
  int loadADCMemPos = -1;
  int trigXMITResetPos = -1;
  
  //We need to search for params now and fill out corresponding booleans etc.
  for(int pI = 0; pI < nParams; ++pI){
    //    printf("'%s', '%s'\n", allParamNames[pI], allParamVals[pI]);

    if(strcmp(allParamNames[pI], "BOARDID") == 0) boardIDPos = pI;
    else if(strcmp(allParamNames[pI], "CHANNELMIN") == 0) channelMinPos = pI;
    else if(strcmp(allParamNames[pI], "CHANNELMAX") == 0) channelMaxPos = pI;
    else if(strcmp(allParamNames[pI], "ADDITIONALTAG") == 0) additionalTagPos = pI;
    else if(strcmp(allParamNames[pI], "DODEBUG") == 0) doDebugPos = pI;
    else if(strcmp(allParamNames[pI], "NUMBEROFLOOP") == 0) numberOfLoopPos = pI;
    else if(strcmp(allParamNames[pI], "NUMBEROFEVENT") == 0) numberOfEventPos = pI;
    else if(strcmp(allParamNames[pI], "POSITIONOFMODULE") == 0) positionOfModulePos = pI;
    else if(strcmp(allParamNames[pI], "NUMBERFEM") == 0) numberFEMPos = pI;
    else if(strcmp(allParamNames[pI], "USEXMIT") == 0) useXMITPos = pI;
    else if(strcmp(allParamNames[pI], "WRITETOFILE") == 0) writeToFilePos = pI;
    else if(strcmp(allParamNames[pI], "WRITETOFILE") == 0) writeToFilePos = pI;
    else if(strcmp(allParamNames[pI], "USEPULSE") == 0) usePulsePos = pI;
    else if(strcmp(allParamNames[pI], "USEEXTERNALTRIGGER") == 0) useExternalTriggerPos = pI;
    else if(strcmp(allParamNames[pI], "USEPULSEGEN") == 0) usePulseGenPos = pI;
    else if(strcmp(allParamNames[pI], "USEDCM") == 0) useDCMPos = pI;
    else if(strcmp(allParamNames[pI], "NUMBEROFSTEPS") == 0) numberOfStepsPos = pI;
    else if(strcmp(allParamNames[pI], "EVENTSPERSTEP") == 0) eventsPerStepPos = pI;
    else if(strcmp(allParamNames[pI], "DACPERSTEP") == 0) dacPerStepPos = pI;
    else if(strcmp(allParamNames[pI], "SIGNAL") == 0) signalPos = pI;
    else if(strcmp(allParamNames[pI], "USEFIXEDDAC") == 0) useFixedDACPos = pI;
    else if(strcmp(allParamNames[pI], "L1DELAY") == 0) l1DelayPos = pI;
    else if(strcmp(allParamNames[pI], "LOADADCMEM") == 0) loadADCMemPos = pI;
    else if(strcmp(allParamNames[pI], "TRIGXMITRESET") == 0) trigXMITResetPos = pI;
  }

  //Assuming all positions needed were found (add some basic gating for needed function args later
  //ints/bools first
  const int channelMin = atoi(allParamVals[channelMinPos]);
  const int channelMax = atoi(allParamVals[channelMaxPos]);
  const int doDebug = atoi(allParamVals[doDebugPos]);

  if(doDebug) printf("DEBUG FILE, LINE: '%s', L%d\n", __FILE__, __LINE__);

  const int numberOfLoop = atoi(allParamVals[numberOfLoopPos]);
  const int numberOfEvent = atoi(allParamVals[numberOfEventPos]);
  const int positionOfModule = atoi(allParamVals[positionOfModulePos]);
  const int numberFEM = atoi(allParamVals[numberFEMPos]);

  const int useXMIT = atoi(allParamVals[useXMITPos]);
  const int writeToFile = atoi(allParamVals[writeToFilePos]);
  const int usePulse = atoi(allParamVals[usePulsePos]);
  const int useExternalTrigger = atoi(allParamVals[useExternalTriggerPos]);
  const int usePulseGen = atoi(allParamVals[usePulseGenPos]);
  const int useDCM = atoi(allParamVals[useDCMPos]);

  const int numberOfSteps = atoi(allParamVals[numberOfStepsPos]);
  const int eventsPerStep = atoi(allParamVals[eventsPerStepPos]);
  const int dacPerStep = atoi(allParamVals[dacPerStepPos]);

  const int useFixedDAC = atoi(allParamVals[useFixedDACPos]);
  const int l1Delay = atoi(allParamVals[l1DelayPos]);

  const int loadADCMem = atoi(allParamVals[loadADCMemPos]);
  const int trigXMITReset = atoi(allParamVals[trigXMITResetPos]);


  //Go ahead and make it so that this won't run if useXMIT and useDCM dont match
  //Both should be true if doing ADC
  if(useXMIT != useDCM){
    printf("useXMIT value \'%d\' does not match useDCM value \'%d\'. return 1\n", useXMIT, useDCM);
    return 1;
  }

  //Now do the strings
  char* boardID = allParamVals[boardIDPos];
  char* additionalTag = allParamVals[additionalTagPos];
  char* signal = allParamVals[signalPos];

  if(doDebug){
    printf("DEBUG FILE, LINE: '%s', L%d\n", __FILE__, __LINE__);    
    printf("BOARDID: \'%s\'\n", boardID);
    printf("ADDITIONALTAG: \'%s\'\n", additionalTag);
    printf("SIGNAL: \'%s\'\n", signal);
  }

  //Read full config file, lets create the output file name before continuing
  combinePermaString(nStrings-1, "output/board", boardID);

  if(doDebug) printf("DEBUG FILE, LINE: '%s', L%d\n", __FILE__, __LINE__);

  char* tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, "_Channel");
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, allParamVals[channelMinPos]);  
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, "to");
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, allParamVals[channelMaxPos]);  
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, "_");
  
  //Get today's date to append to the file name
  static time_t timer;
  struct tm localt;
  timer = time(NULL);
  localt = *localtime(&timer);
  snprintf(allStrings[nStrings-2], sizeof(allStrings[nStrings-2]), "%04d%02d%02d", localt.tm_year + 1900, localt.tm_mon + 1, localt.tm_mday);
  tempStr = allStrings[nStrings-1];
  char* tempStr2 = allStrings[nStrings-2];
  combinePermaString(nStrings-1, tempStr, tempStr2);  

  if(doDebug) printf("DEBUG FILE, LINE: '%s', L%d\n", __FILE__, __LINE__);

  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, "_");
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, allParamVals[additionalTagPos]);
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, ".dat");

  //Since i use the last position in the array for temporary storage, move to the latest position
  
  tempStr = allStrings[nStrings-1];
  setPermaString(nParams, tempStr);
  char* outFileName = allStrings[nParams];
  FILE* outFile = fopen(outFileName, "w");
  fprintf(outFile, "%s: %s\n", allParamNames[boardIDPos], allParamVals[boardIDPos]);
  fprintf(outFile, "%s: %s\n", allParamNames[channelMinPos], allParamVals[channelMinPos]);
  fprintf(outFile, "%s: %s\n", allParamNames[channelMaxPos], allParamVals[channelMaxPos]);
  fprintf(outFile, "%s: %s\n", allParamNames[additionalTagPos], allParamVals[additionalTagPos]);
  //This one is special for the configFileName
  fprintf(outFile, "%s: %s\n", allParamNames[0], allParamVals[0]);
  //DATE is also special
  fprintf(outFile, "DATE: %s\n", allStrings[nStrings-2]);
  fprintf(outFile, "%s: %s\n", allParamNames[numberOfStepsPos], allParamVals[numberOfStepsPos]);
  fprintf(outFile, "%s: %s\n", allParamNames[eventsPerStepPos], allParamVals[eventsPerStepPos]);
  fprintf(outFile, "%s: %s\n", allParamNames[dacPerStepPos], allParamVals[dacPerStepPos]);  

  if(doDebug) printf("DEBUG FILE, LINE: '%s', L%d\n", __FILE__, __LINE__);

  //Finished writing most of the configuration params to output for metadata purposes
  //Now start doing actually processing
  //Start declarations of variables we will use to talk to the crate
  //Note that here on is really taking directly from Chi's code
  //sphenix_adc_test_jseb2.c, case 4 test sequence

  //First we need to find and open some PCI devices (JSEB2s)
  WDC_DEVICE_HANDLE hDev = NULL;
  WDC_DEVICE_HANDLE hDev2 = NULL;
  DWORD dwStatus;
  dwStatus = JSEB2_LibInit();
  if(WD_STATUS_SUCCESS != dwStatus){
    JSEB2_ERR("pcie_diag: Failed to initialize the JSEB2 library: %s", JSEB2_GetLastErr());
    return dwStatus;
  }

  printf("Select first device (ADC crate controller JSEB): \n");
  if(JSEB2_DEFAULT_VENDOR_ID) hDev = DeviceFindAndOpen(JSEB2_DEFAULT_VENDOR_ID, JSEB2_DEFAULT_DEVICE_ID);
  printf("Will the same JSEB2 control DCM2?");
  int answer1 = -1;
  while(answer1 < 0){
    scanf("%d", answer1);
    
    if(answer1 != 1 && answer1 != 0){
      printf(" Please answer1 1/0\n");
      answer1 = -1;
    }
  }

  if(answer1 == 1){
    hDev2 = hDev;
  }
  else{
    printf("Select second device (DCM2 crate controller JSEB): \n");
    if(JSEB2_DEFAULT_VENDOR_ID) hDev2 = DeviceFindAndOpen(JSEB2_DEFAULT_VENDOR_ID, JSEB2_DEFAULT_DEVICE_ID);
  }


  //Now lets declare variables we will use to talk to the jseb2  
  static DWORD dwAddrSpace;
  static UINT32 u32Data, ioffset, nmask, kword;
  static DWORD dwOffset;
  static long imod,ichip; 

  static UINT32 read_array[dma_buffer_size], read_array1[40000];
  UINT32 buf_send[40000];
  UINT32 *px, *py;
  static int imod_xmit;

  UINT32 iread;
  
  static UINT32 i,j,k,ifr,nread;

  //Temp hard coded
  const int nsample = 28;
  static int idac_shaper, idac_shaper_load, ic, ipattern, nword, ik, iparity, imod_dcm, iadd, ioffset_t;  
  static int adc_data[64][40];

  px = &buf_send;
  py = &read_array;
  imod_xmit = positionOfModule + numberFEM;
  ichip = 6;

  //L3215 in chiupdated code
  if(useDCM == 1){
    imod_dcm=11;
    printf(" boot 5th FPGA \n");
    i=dcm2_fpga_boot(hDev2,imod_dcm,1);
    printf(" boot FPGA 1-4 \n");
    i=dcm2_fpga_boot(hDev2,imod_dcm,2);
    printf(" DCM II booting done \n");
    scanf("%d",&i);

    ioffset=4;
    ichip =5;
    iadd=(imod_dcm<<11)+(ichip<<8);  /* don't care about chip number **/
    /* set run =0 -- clear everything */
    buf_send[0]=iadd+dcm2_run_off;
    ik=pcie_send(hDev2,1,1,px);
  }

  dwAddrSpace = 2;
  u32Data = 0xf0000008;
  dwOffset = 0x28;
  WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, u32Data);

  ifr=0;
  buf_send[0]=0x0;
  buf_send[1]=0x0;
  i=1;
  k=1;
  i = pcie_send_1(hDev, i, k, px);

  printf(" set the controller to offline state before reset can be applied \n");
  imod=0;
  ichip=0;
  buf_send[0]=(imod<<11) + (ichip<<8) + (sp_cntrl_offline) + (0x1<<16); //enable offline run on
  i=1;
  k=1;
  i = pcie_send_1(hDev, i, k, px);

  //xmit selection specific lines L3256
  if(useXMIT){
    ichip = sp_xmit_sub;

    buf_send[0]=(imod_xmit << 11)+ (ichip << 8) + sp_xmit_rxanalogreset + (0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);
    buf_send[0]=(imod_xmit <<11)+ (ichip << 8) + sp_xmit_rxdigitalreset + (0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);  

    buf_send[0]=(imod_xmit <<11)+ (ichip << 8) + sp_xmit_rxbytord + (0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);
    //      }
    buf_send[0]=(imod_xmit <<11)+ (ichip << 8) + sp_xmit_init + (0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(100);
  }

  //Some more lines for sending init to controller
  buf_send[0]= (0x2<<8) + sp_cntrl_timing + (sp_cntrl_init<<16);
  i = 1;
  k = 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(10);
  buf_send[0]= (0x2<<8) + sp_cntrl_timing + (sp_cntrl_init<<16);
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);

  //L3311 in Chi code
  buf_send[0]= (0x2<<8) + sp_cntrl_timing + (sp_cntrl_reset<<16);
  i= 1;
  k= 1;
  i = pcie_send_1(hDev, i, k, px);
  usleep(1000);

  //Skipping an isel_dcm section again
  if(useDCM == 1){
    nmask = 0x1; /*turn non all channel */
    for (i=1; i<5; i++) {
      ichip=i;
      iadd=(imod_dcm<<11)+(ichip<<8);
      /* set module to online mode */
      buf_send[0]=iadd+dcm2_online+(0x1<<16);
      ik=pcie_send(hDev2,1,1,px);
      /* set mask on for all channel*/
      buf_send[0]=iadd+dcm2_setmask+((nmask &0xff) <<16);
      ik=pcie_send(hDev2,1,1,px);
    }
    /** work on 5th FPGA **/
    ichip=5;
    iadd=(imod_dcm<<11)+(ichip<<8);
    /* set module to offline mode */
    buf_send[0]=iadd+dcm2_online+(0x0<<16);
    ik=pcie_send(hDev2,1,1,px);
    /* set mask on for all channel*/
    buf_send[0]=iadd+dcm2_setmask+((nmask &0xff) <<16);
    ik=pcie_send(hDev2,1,1,px);

    /* set dcm first module */
    buf_send[0]=iadd+dcm2_5_firstdcm+(0x1<<16);   // bit 0 =1 on                                      
    ik=pcie_send(hDev2,1,1,px);
    /* set last module*/
    buf_send[0]=iadd+dcm2_5_lastdcm+(0x1<<16);    // bit 0 =1 on                                      
    ik=pcie_send(hDev2,1,1,px);
    //                                                                                                      
    //                                                                                                      
    ichip =5;
    iadd=(imod_dcm<<11)+(ichip<<8);
    /* set run =1 */
    buf_send[0]=iadd+dcm2_run_on;
    ik=pcie_send(hDev,1,1,px);
  }

  //For loop starts at L3380
  for(ik = 0; ik < numberFEM; ik++){//numberFEM is the equiv of nmod
    if(usePulseGen){
      imod = ik + positionOfModule; //positionOfModule is equiv. of positionOfModule
      ichip = sp_adc_slowcntl_sub;
      buf_send[0] = (imod<<11) + (ichip << 8) + sp_adc_trg_tx_areset + (0<<16);
      i = 1;
      k = 1;
      i = pcie_send_1(hDev, i, k, px);
      usleep(10);    

      imod = ik+positionOfModule;
      ichip = sp_adc_slowcntl_sub;
      buf_send[0]=(imod <<11) + (ichip << 8) + sp_adc_trg_tx_dreset + (0<<16) ;
      i= 1;
      k= 1;
      i = pcie_send_1(hDev, i, k, px);
      usleep(10);

      i = trigXMITReset; // This is an if/else statement in your modded version of chi's code - replicate w/ a hardcoded setting + usleep
      usleep(10);

      //Long usleep here
      usleep(1000000);

      usleep(1000000);      

      for(int is=0; is < 16; is++){
	imod = ik + positionOfModule;
	ichip = sp_adc_slowcntl_sub;
	buf_send[0]=(imod <<11) + (ichip << 8) + sp_adc_calib_ch + (is<<16) ;
	i= 1;
	k= 1;
	i = pcie_send_1(hDev, i, k, px);
	usleep(10);

	imod = ik+positionOfModule;
	ichip = sp_adc_slowcntl_sub;
	buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_calib_dac + ((0xe00)<<16) ;
	i= 1;
	k= 1;
	i = pcie_send_1(hDev, i, k, px);
	usleep(10);

	imod = ik+positionOfModule;
	ichip = sp_adc_slowcntl_sub;
	buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_calib_send + (is<<16) ;
	i= 1;
	k= 1;
	i = pcie_send_1(hDev, i, k, px);
	usleep(10);
      }    
    }

    //Picking up in chi code L3982 (Note this is your version of Chi's code; not the one that says ChiVersion
    imod = ik + positionOfModule;
    ichip = sp_adc_slowcntl_sub;
    buf_send[0] = (imod <<11) + (ichip << 8) + sp_adc_evt_sample + ((nsample-1)<<16);
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    if((usePulse != 0) | (usePulseGen !=0)) buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_l1_delay + (l1Delay<<16);
    else buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_l1_delay + (0xff<<16);
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    if(useXMIT == 1) buf_send[0]= (imod <<11) + (ichip << 8) + sp_adc_rd_link + (1<<16);
    else buf_send[0] = (imod <<11) + (ichip << 8) + sp_adc_rd_link + (0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    if(useXMIT == 1) buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_rd_cntrl + (0<<16) ;
    else buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_rd_cntrl + (1<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    if(usePulse != 0) buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_pulse + (0x1<<16) ;
    else  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_pulse + (0x0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    if(usePulseGen != 0) buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_caltrig + (0x1<<16) ;
    else  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_caltrig + (0x0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    //L4069 now in sphenix_adc_test_jseb2
    if(loadADCMem == 1) buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_test_pulse + (0x1<<16) ;
    else buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_test_pulse + (0x0<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    if((usePulse != 0) | (usePulseGen  !=0)) buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_l1 + (0x0<<16);
    else buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_l1 + (0x1<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);

    //Another section for XMIT  L4108-L4119
    if(useXMIT == 1) {
      if(imod != positionOfModule) {
        ichip = sp_adc_slowcntl_sub;
        buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_sel_link_rxoff + (0<<16) ;
        i= 1;
        k= 1;
	//Note the call of pcie_send_1 here instead of pcie_send, hence passing hdev 
      i = pcie_send_1(hDev, i, k, px);
        usleep(10);
        printf(" rx_off called , module %d\n", imod);
      }
    }
    
    //now start calling adc setup
    i = adc_setup(hDev,imod);

    //L4134
    usleep(1000000);
    if(loadADCMem == 1) i = adc_testram_load(hDev,imod, 400);
  }

  //Out of initial setup loop
  //Skipping the post setup xmit stuff again,  here L4153
  if(useXMIT == 1) {
    imod = imod_xmit;
    ichip = sp_xmit_sub;
    buf_send[0]=(imod <<11)+ (ichip << 8) + sp_xmit_lastmod + (positionOfModule<<16) ;
    i= 1;
    k= 1;
    i = pcie_send_1(hDev, i, k, px);
    usleep(10);
  }
  
  for(j = 0; j < numberOfLoop; j++){//Number of loops
    for(int ia = 0; ia < numberOfEvent; ++ia){//numberOfEvent loop
   
      for(ik = 0; ik < numberFEM; ik++){
	imod = ik+positionOfModule;
	int nword = 1;
	py = &read_array;

	ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
	buf_send[0]= (imod<<11) + (ichip<<8) + (8) + (0x0<<16);  // read out status
	i=1;
	k=1;
	i = pcie_send_1(hDev, i, k, px);

	i = pcie_rec_2(hDev,0,1,nword,0,py);       // init receiver
	ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
	buf_send[0] = (imod<<11) + (ichip<<8) + (sp_adc_readback_status) + (0x0<<16);  // read out status
	i=1;
	k=1;
	i = pcie_send_1(hDev, i, k, px);
	usleep(10);
	py = &read_array;
	i = pcie_rec_2(hDev,0,2,nword,0,py);     // read out 2 32 bits words
      }

      if(usePulse != 0)	buf_send[0]= (0x2<<8) + sp_cntrl_timing + ((sp_cntrl_pulse)<<16);
      else if(usePulseGen){//Now on 4244
	idac_shaper = (ia/eventsPerStep);
	if(useFixedDAC != 1) idac_shaper = idac_shaper * dacPerStep;    //set up the dac value
	else idac_shaper = 4000;
	
	imod = positionOfModule;

	for(int is = 0; is < 16; is++){
	  ic = ipattern >> is;
	  if((ic & 0x1) != 1) idac_shaper_load = 0;
	  else idac_shaper_load = idac_shaper;
	  ichip = sp_adc_slowcntl_sub;
	  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_calib_ch + (is<<16) ;
	  i= 1;
	  k= 1;
	  i = pcie_send_1(hDev, i, k, px);
	  usleep(10);

	  imod = positionOfModule;
	  ichip = sp_adc_slowcntl_sub;
	  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_calib_dac + ((idac_shaper_load)<<16) ;
	  i= 1;
	  k= 1;
	  i = pcie_send_1(hDev, i, k, px);
	  usleep(10);

	  imod = positionOfModule;
	  ichip = sp_adc_slowcntl_sub;
	  buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_calib_send + (is<<16) ;
	  i= 1;
	  k= 1;
	  i = pcie_send_1(hDev, i, k, px);
	  usleep(10);
	}
	//End of this for is L4303
	imod = positionOfModule;
	ichip = sp_adc_slowcntl_sub;
	buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_calib_gate + (0x1<<16) ;
	i= 1;
	k= 1;
	i = pcie_send_1(hDev, i, k, px);
	usleep(1);

	ichip = sp_adc_slowcntl_sub;
	buf_send[0]=(imod <<11)+ (ichip << 8) + sp_adc_calib_gate + (0x0<<16) ;
	i= 1;
	k= 1;
	i = pcie_send_1(hDev, i, k, px);
	usleep(10);
      }      
      else if((useExternalTrigger != 1) & (usePulseGen != 1)) buf_send[0]= (0x2<<8) + sp_cntrl_timing + ((sp_cntrl_l1)<<16);
      i= 1;
      k= 1;
      if((useExternalTrigger != 1) & (usePulseGen != 1)) i = pcie_send_1(hDev, i, k, px);
      usleep(100);
 
      if(useExternalTrigger == 1){
	ik = 0;
	while (ik != 1) {
	  imod = positionOfModule;
	  nword =1;
	  py = &read_array;
	  //
	  //     command no-op to enable module output enable
	  // otherwise the LVDS input at controller will be floating high
	  //                                                                                                      
	  ichip = sp_adc_readback_sub ;   // controller data go to ADC input section                      
	  buf_send[0]=(imod<<11)+(ichip<<8)+(8) + (0x0<<16);  // read out status                          
	  i=1;
	  k=1;
	  i = pcie_send_1(hDev, i, k, px);
	  //                                                                                                      
	  i = pcie_rec_2(hDev,0,1,nword,0,py);       // init receiver                                
	  ichip = sp_adc_readback_sub ;   // controller data go to ADC input section                      
	  buf_send[0]=(imod<<11)+(ichip<<8)+(sp_adc_readback_status) + (0x0<<16);  // read out status     
	  i=1;
	  k=1;
	  i = pcie_send_1(hDev, i, k, px);
	  usleep(10);
	  py = &read_array;
	  i = pcie_rec_2(hDev,0,2,nword,0,py);     // read out 2 32 bits words                       
	  if (((read_array[0] >>21) & 0x1) == 0) ik=1;
	}
      }//L4374

      //Skipping a big iselDCM section here just to get the code up from L4380-L4573
      //when you add the isel_dcm section, uncomment the else below
      
      //CFM DCM EDIT 2021.08.02
      if(useDCM){
	nread = 3;
	i = pcie_rec(hDev,0,1,nread,0,py);     // read out 2 32 bits words
	ichip =5;
	iadd = (imod_dcm<<11)+ (ichip<<8);
	buf_send[0]=(imod_dcm <<11)+ (ichip << 8) + dcm2_5_readdata + ((nread-2)<< 16);  /* number word to read- (header+trailer)*/
	buf_send[1]=0x5555aaaa;// -1 for the counter                                     
	
	ik = pcie_send(hDev, 1, 1, px);  //** for dcm2 status read send 2 words **//
	usleep(100);
	i = pcie_rec(hDev,0,2,nread,0,py);
	//
	iread = read_array[2] & 0xffff;
	nread = iread+1;
	kword = (nread/2);
	if(nread%2 !=0) kword = kword+1;
	
	i = pcie_rec(hDev,0,1,nread,0,py);     // read out 2 32 bits words
	buf_send[0]=(imod_dcm <<11)+ (ichip << 8) + dcm2_5_readdata + ((nread-2)<< 16);  /* number word to read- (header+trailer)*/
	buf_send[1]=0x5555aaaa;  // -1 for the counter
	ik = pcie_send(hDev, 1, 1, px);  //** for dcm2 status read send 2 words **//
	usleep(100);
	i = pcie_rec(hDev,0,2,nread,0,py);

	for (i=0; i<(nread-1); i++) {
	  u32Data = (read_array[i] & 0xffff0000) + (read_array[i+1] &0xffff);
	  read_array1[i] = u32Data;
	  if(writeToFile != 1) {
	    if(i%8 == 0) printf("%3d",i);
	    printf(" %9x",u32Data);
	    if(i%8 == 7) printf("\n");
	  }
	}
	if(writeToFile == 1) {
	  fprintf(outFile,"%d\n", nread);
	  for (i=0; i<(nread-1); i++) {
	    //        u32Data = (idcm_read_array[2*i+1]<<16)+idcm_read_array[2*i+2];
	    fprintf(outFile," %9x",read_array1[i]);
	    if((i%8) == 7) fprintf(outFile,"\n");
	  }
	  fprintf(outFile,"\n");
	  //        if((i%8) != 7) fprintf(outFile,"\n");
	}
	//       fprintf(outFile,"\n");

	for (ic=0; ic< numberFEM; ic++ ){
	  ioffset = (64*nsample+4+2)*ic;              // 4 words header + 2 word parity
	  iparity =0;

	  for(int is=0; is<4+(nsample*64); is++) {
	    if(is%2 == 0) u32Data = (read_array1[is+6+ioffset] & 0xffff) <<16;
	    else {
	      u32Data = u32Data + (read_array1[is+6+ioffset] & 0xffff);
	      //         u32Data = u32Data+ ((read_array1[is+6] & 0xffff) <<16);
	      iparity = iparity ^ u32Data;
	    }
	  }
	  ioffset_t = ioffset + (64*nsample+4)+6;   // 4 words header + 5 event header -1 for array started at 0
	  i= ((read_array1[ioffset_t] & 0xffff) <<16) +(read_array1[ioffset_t+1] & 0xffff);
	  if(i != iparity) {
	    printf(" event = %d, module %d Partity error....... = %x %x\n", ia, (positionOfModule+ic), i, iparity);
	  }
	}      
      }
      else{      
	ichip = sp_adc_readback_sub ;   // controller data go to ADC input section
	for (ik =0; ik<numberFEM; ik++) {
	  imod = ik + positionOfModule;
	  iparity =0;
           
	  buf_send[0]=(imod <<11) + (ichip << 8) + sp_adc_readback_transfer + (0<<16);
 	  i= 1;
	  k= 1;
	  i = pcie_send_1(hDev, i, k, px);
	  usleep(100);
	  //
	  //
	  //
	  nread =2+(64*nsample/2)+1;
	  i = pcie_rec_2(hDev,0,1,nread,0,py);     // read out 2 32 bits words

	  buf_send[0]=(imod <<11) + (ichip << 8) + sp_adc_readback_read + (1<<16) ;
	  i= 1;
	  k= 1;
	  i = pcie_send_1(hDev, i, k, px);
	  usleep(100);

	  
	  i = pcie_rec_2(hDev,0,2,nread,0,py);     // read out 2 32 bits words
	  for(int is = 0; is < nread-1; is++){
	    iparity = iparity ^ read_array[is];
	  }
	  if(writeToFile){
	    fprintf(outFile," %x\n", read_array[0]);
	    fprintf(outFile," %x\n", read_array[1]);
	  }
	  k=0;//L4629
	  {
	    int is = 0;
	    for(is=0; is< (nread-2); is++) {
	      if(writeToFile == 1) fprintf(outFile," %8X", read_array[is+2]);
	      else{       
		if(is%8 ==0) printf(" %d ", is);
		printf(" %x", read_array[is+2]);
	      }
	      k=k+1;
	      
	      if(writeToFile == 1) {
		if((k%8) ==0) fprintf(outFile,"\n");
	      }
	      else{
		if((k%8) ==0) printf("\n");
	      }
	    }
	    
	    if(writeToFile == 1) {
	      if((is%8) !=0) fprintf(outFile,"\n");
	    }
	    else{
	      if((is%8) !=0) printf("\n");
	    }
	  
	    
	    if(writeToFile != 1) printf(" data parity = %x generated parity = %x  \n", read_array[is+1], iparity);
	  
	    if(read_array[is+1] !=  iparity){
	      printf(" event = %d, Partity error....... = %x %x\n", ia,read_array[is+1], iparity);
	    }
	  }

	  if(writeToFile == 1) fprintf(outFile,"\n");
	  else{
	    printf("\n");
	    printf(" header = %x \n", (read_array[0] & 0xffff));
	    printf(" module number = %d \n", ((read_array[0]>> 16) & 0x1f));
	    printf(" triggernumber = %x \n", (read_array[1] & 0xffff));
	    printf(" beam crossing number = %x \n", ((read_array[1]>> 16) & 0xffff));
	    for (int is=0; is< nsample; is++ ) {
	      for (k=0; k< 32; k++) {
		//        adc_data[(k*2)][is] = read_array[(is*32)+k+2] & 0xffff;
		//        adc_data[((k*2)+1)][is] = (read_array[(is*32)+k+2] >>16) & 0xffff;
		adc_data[(k*2)][is] = read_array[(k*nsample)+is+2] & 0xffff;
		adc_data[((k*2)+1)][is] = (read_array[(k*nsample)+is+2] >>16) & 0xffff;
	      }
	    }

	    for (int is=0; is<64; is++) {
	      printf(" channel %d ", is);
	      for (k=0; k<nsample; k++) {
		printf(" %4x", adc_data[is][k]);
	      }
	      printf("\n");
	    }

	  }//L4695
	}	  
      }      
    }

    if(writeToFile) fclose(outFile);

  }
  
  
  if(doDebug) printf("DEBUG FILE, LINE: '%s', L%d\n", __FILE__, __LINE__);

  //  fclose(outFile);
  fclose(configFile);
  
  return 0;
}

//Main function, kept simple for single call of main function / argument handling
int main(int argc, char* argv[])
{
  if(argc < 1 || argc > 2){
    printf("Usage: ./bin/sphenixADCTest <inConfigFileName-optional>. return 1");   
    return 1;
  }
  
  char* emptyStr = "";
  int retVal = 0;
  if(argc == 1) retVal += sphenixADCTest(emptyStr);
  else if(argc == 2) retVal += sphenixADCTest(argv[1]);
  return retVal;
}
