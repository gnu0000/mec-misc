// BarChop.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include "io.h"
#include "stdio.h"
#include "stdlib.h"
#include "BarChop.h"
#include "GnuCfg.h"
#include "GnuStr.h"
#include "GnuMisc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CWinApp theApp;
using namespace std;

#define MAX_PART_TYPES     16
#define MAX_PARTS_PER_BAR  128


typedef struct
   {
   char   cParts [MAX_PARTS_PER_BAR];  // List of parts used in this bar
   int    iPartCount;                  // The count of parts used in this bar
   int    iPartCounts[MAX_PART_TYPES]; // The count of each part type used
   double fLength;                     // The used length of the bar
   } CBarDef;                          // Bar Definition

typedef CList<CBarDef,CBarDef&> CJobList; // defines composition of the entire job

double fBAR_LENGTH;                    // Legth of stock
double fSAW_WIDTH;                     // Width of Cut
int    iPART_TYPES;                    // # of different part types  max=MAX_PART_TYPES
int    iPART_COUNTS[MAX_PART_TYPES];   // Array of Part Counts, 1 per type
double fPART_LENGTHS[MAX_PART_TYPES];  // Array of Part Lengths, 1 per type

int    iREMAINING_PART_COUNTS[MAX_PART_TYPES]; // used in calc parts not used yet
int    iBAR_COUNT;                     // used in calc bars used so far

CJobList cJOB_LIST;

void InitBarInfo (CBarDef& cBarDef)
   {
   cBarDef.fLength = 0.0;
   cBarDef.iPartCount = 0;
   for (int i=0; i < MAX_PART_TYPES; i++)
      cBarDef.iPartCounts[i] = 0;
   
   for (i=0; i < MAX_PARTS_PER_BAR; i++) // not necessary ...
      cBarDef.cParts[i] = 0;
   }


BOOL AddPart (CBarDef& cBarDef, int iType)
   {
   if (iREMAINING_PART_COUNTS[iType] - cBarDef.iPartCounts[iType] <= 0)
      return FALSE; // no more stock for that part type

   double fLenNeeded = fPART_LENGTHS[iType] + (cBarDef.iPartCount ? fSAW_WIDTH : 0.0);
   if (fLenNeeded  > fBAR_LENGTH - cBarDef.fLength + 0.0001)
      return FALSE; // part won't fit

   cBarDef.iPartCount++;
   cBarDef.iPartCounts[iType]++;
   cBarDef.cParts[cBarDef.iPartCount-1] = iType;
   cBarDef.fLength += fLenNeeded;

   return TRUE;
   }


CBarDef _FillBar (CBarDef cBarDef)
   {
   CBarDef cCandidate;
   CBarDef cBest = cBarDef;

   // Optimization #1
   int iStart = (cBarDef.iPartCount ? cBarDef.cParts [cBarDef.iPartCount-1] : 0);

   for (int i=iStart; i < iPART_TYPES; i++)
      {
      cCandidate = cBarDef;
      if (!AddPart (cCandidate, i))
         continue;

      cCandidate = _FillBar (cCandidate);
      if (cCandidate.fLength > cBest.fLength + 0.0001)
         cBest = cCandidate;
      }
   return cBest;
   }

void InitStock ()
   {
   for (int i=0; i < MAX_PART_TYPES; i++)
      iREMAINING_PART_COUNTS[i] = iPART_COUNTS[i];
   }

void DecrementStock (CBarDef cBarDef)
   {
   for (int i=0; i < MAX_PART_TYPES; i++)
      {
      iREMAINING_PART_COUNTS[i] -= cBarDef.iPartCounts[i];
      ASSERT (iREMAINING_PART_COUNTS[i] > -1);
      }
   printf ("."); // cmd line status info
   }

CBarDef FillBar (void)
   {
   CBarDef cBarDef;
   InitBarInfo (cBarDef);
    cBarDef = _FillBar (cBarDef);
   DecrementStock (cBarDef);
   return cBarDef; 
   }

BOOL CopyBar (CBarDef cBarDef)
   {
   for (int i=0; i < iPART_TYPES; i++)
      if (iREMAINING_PART_COUNTS[i] < cBarDef.iPartCounts[i])
         return FALSE;
   DecrementStock (cBarDef);
   return TRUE;
   }

int FillBars (void)
   {
   CBarDef cBarDef;

   InitStock ();
   for (iBAR_COUNT = 0; ; iBAR_COUNT++)
      {
      cBarDef = FillBar ();
      if (!cBarDef.iPartCount)
         break;
      cJOB_LIST.AddTail (cBarDef);
      
      // Optimization #2
      for (; CopyBar (cBarDef); iBAR_COUNT++)
         cJOB_LIST.AddTail (cBarDef);
      }
   return iBAR_COUNT;
   }


void PrintDetails (FILE* fp)
   {
   fprintf (fp, "============================================================\n");
   fprintf (fp, "Bar Stock Length: %.3f\n", fBAR_LENGTH);
   fprintf (fp, "Saw Blade Width : %.3f\n", fSAW_WIDTH);
   fprintf (fp, "Parts (%d):\n", iPART_TYPES);
   for (int i=0; i < iPART_TYPES; i++)
      fprintf (fp, "    Part%c: Count: %4.4d, Length: %.3f\n", (char)(i + 'A'), iPART_COUNTS[i], fPART_LENGTHS[i]);
   fprintf (fp, "\n");

   double fBarStockLen = 0.0;
   double fPartsLen = 0.0;
   double fSawCutLen = 0.0;
   POSITION pos = cJOB_LIST.GetHeadPosition ();
   CBarDef cBarDef;
   for (i=0; pos; i++)
      {
      cBarDef = cJOB_LIST.GetNext (pos);
      fBarStockLen += fBAR_LENGTH;
      fSawCutLen   += fSAW_WIDTH * (cBarDef.iPartCount - 1);
      for (int j=0; j < cBarDef.iPartCount; j++)
         fPartsLen += fPART_LENGTHS[cBarDef.cParts [j]];
      }
   fprintf (fp, "Total Bars Used          : %d\n", iBAR_COUNT);
   fprintf (fp, "Total Length of Bar Stock: %.3f\n", fBarStockLen);
   fprintf (fp, "Total Length of Parts    : %.3f\n", fPartsLen   );
   fprintf (fp, "Total Length of Saw cuts : %.3f\n", fSawCutLen  );
   fprintf (fp, "Efficiency               : %.3f%%\n", (fPartsLen + fSawCutLen) / fBarStockLen * 100.0);
   fprintf (fp, "============================================================\n");
   fprintf (fp, "\n");

   pos = cJOB_LIST.GetHeadPosition ();
   for (i=0; pos; i++)
      {
      cBarDef = cJOB_LIST.GetNext (pos);
      fprintf (fp, "Bar %4.4d: ", i+1);
      for (int j=0; j< cBarDef.iPartCount; j++)
         fprintf (fp, "%c ", cBarDef.cParts [j] + 'A');
      fprintf (fp, "  (used: %4.3f  unused: %4.3f)", cBarDef.fLength, fBAR_LENGTH - cBarDef.fLength);
      fprintf (fp, "\n");
      }
   fprintf (fp, "============================================================\n");
   }

void CreateSampleParmFile (PSZ pszCfgFile)
   {
   FILE *fp;
   fp = fopen (pszCfgFile, "wt");

   fprintf (fp, "; BarChop Configuration File\n");
   fprintf (fp, "; Lines beginning with a semicolon are comment lines\n");
   fprintf (fp, ";\n");
   fprintf (fp, "; This file defines the input data for the BarChop program\n");
   fprintf (fp, ";\n");
   fprintf (fp, "; Valid parameters are:\n");
   fprintf (fp, ";    BarLength = #length            Length of the bar stock\n");
   fprintf (fp, ";    SawWidth  = #width             Width of the saw blade\n");
   fprintf (fp, ";    PartA     = #count , #length   Count of PartA, length of partA\n");
   fprintf (fp, ";    PartB     = #count , #length   Count of PartB, length of partA\n");
   fprintf (fp, ";    ....                           Up to %d parts total (Part%c)\n", MAX_PART_TYPES, MAX_PART_TYPES+'A'-1);
   fprintf (fp, ";\n");
   fprintf (fp, "[BarChop]\n");
   fprintf (fp, "BarLength = 96.0\n");
   fprintf (fp, "SawWidth  = 0.125\n");
   fprintf (fp, "PartA     = 200, 28.375\n");
   fprintf (fp, "PartB     = 200, 22.375\n");
   fprintf (fp, "PartC     = 400, 12.375\n");
   fprintf (fp, "PartD     = 400, 10.375\n");
   fclose (fp);
   }

void ReadParamData (PSZ pszCfgFile)
   {
   char sz[256];
   PSZ psz;

   if (_access (pszCfgFile, 0))
      {
      printf ("Error: Cannot find config file: %s. Producing sample config file ...\n", pszCfgFile);
      CreateSampleParmFile (pszCfgFile);
		exit (1);
      }

   CfgGetLine (pszCfgFile, "BarChop", "BarLength", sz);
   fBAR_LENGTH = atof (sz);

   CfgGetLine (pszCfgFile, "BarChop", "SawWidth", sz);
   fSAW_WIDTH  = atof (sz);

   char szName[32];
   for (iPART_TYPES=0; iPART_TYPES<MAX_PART_TYPES; iPART_TYPES++)
      {
      sprintf (szName, "Part%c", (char)(iPART_TYPES + 'A'));
      if (!CfgGetLine (pszCfgFile, "BarChop", szName, sz))
         break;
      if (!(psz = strchr (sz, ',')))
         Error ("Comma expected to separate part count from part length: %s\n", sz);
      *psz = '\0';
      iPART_COUNTS[iPART_TYPES] = atoi (sz);
      fPART_LENGTHS[iPART_TYPES] = atof (psz+1);
      }
   if (!iPART_TYPES)
      Error ("No part types defined in cfg file\n");
   }


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
   {
   AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0);
   printf ("BarChop  Part layout optimizer   v2.0 05/20/02                 Craig Fitzgerald\n\n");

   printf ("Reading config file: BarChop.cfg ...\n");
   ReadParamData ("BarChop.cfg");

   printf ("Creating part layout ...\n");
   int iBarCount = FillBars ();

   printf ("\nWriting output file: BarChop.txt ...\n");
   FILE *fp;
   fp = fopen ("BarChop.txt", "wt");
   PrintDetails (fp);
   fclose (fp);

   printf ("Done.\n");
   return 0;
   }
