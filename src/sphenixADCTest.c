//Author: Chris McGinn, 2021.07.23
//If bug is discovered, please contact at chmc7718@colorado.edu, cffionn@gmail.com, or cffionn on skype
//The following borrows heavily from Cheng-Yi Chi's sPHENIX JSEB2 testing code
//Changes are largely rewrites for simplicity and clarity for future users

//c headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

//Main function - Parse config file + run test + write output
int sphenixADCTest(char* inConfigFileName)
{
  //Start out defining some numbers
#define  dma_buffer_size        10000000  

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
  static ssize_t configRead;

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

  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, "_");
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, allParamVals[additionalTagPos]);
  tempStr = allStrings[nStrings-1];
  combinePermaString(nStrings-1, tempStr, ".dat");

  char* outFileName = allStrings[nStrings-1];
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

  //Finished writing most of the configuration params to output for metadata purposes
  //Now start doing actually processing
  //Start declarations of variables we will use to talk to the crate
  //Note that here on is really taking directly from Chi's code
  //sphenix_adc_test_jseb2.c, case 4 test sequence

  //First we need to find and open some PCI devices (JSEB2s)
  
  
  static DWORD dwAddrSpace;
  static UINT32 u32Data;
  static DWORD dwOffset;
  
  static UINT32 read_array[dma_buffer_size];
  UINT32 buf_send[40000];
  UINT32 *px, *py;
  static int imod_xmit;
    
  px = &buf_send;
  py = &read_array;
  imod_xmit = positionOfModule + numberFEM;
  ichip = 6;

  //Skipping some dcm2 necessary lines here - lets just get injector testing up

  dwAddrSpace = 2;
  u32Data = 0xf0000008;
  dwOffset = 0x28;

  
  
  

  fclose(outFile);
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
