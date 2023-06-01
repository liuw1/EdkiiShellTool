/** @file

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/



#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <PiDxe.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <IndustryStandard/Tdx.h>
#include <Library/TdxLib.h>

#define RTMR_INDEX_RTMR0  0
#define RTMR_INDEX_RTMR1  1
#define RTMR_INDEX_RTMR2  2
#define RTMR_INDEX_RTMR3  3

#define SHA384_DIGEST_SIZE  48

/**

  This function dump raw data.

  @param  Data  raw data
  @param  Size  raw data size

**/
VOID
InternalDumpData (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;
  for (Index = 0; Index < Size; Index++) {
    Print (L"%02x", (UINTN)Data[Index]);
  }
}

/**
 * Call the TDCALL to get TD_REPORT and then check the RTMR[3]
 *
 * @return EFI_SUCCESS    The RTMR[3] is zero.
 * @return Others         The RTMR[3] is non-zero.
*/
EFI_STATUS
DumpRtmr3WithTdReport (
  VOID
  )
{
  EFI_STATUS       Status;
  TDREPORT_STRUCT  *TdReport;
  UINT8            *Report;
  UINT8            *AdditionalData;
  UINT8            index;
  UINTN            Pages;

  TdReport       = NULL;
  Report         = NULL;
  AdditionalData = NULL;

  Pages = EFI_SIZE_TO_PAGES (sizeof (TDREPORT_STRUCT));

  // The Report buffer must be 1024-B aligned
  Report = (UINT8 *)AllocatePages (Pages);
  if ((Report == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto QuitCheckRTMRForVTPM;
  }

  ZeroMem (Report, EFI_PAGES_TO_SIZE(Pages));

  AdditionalData = Report + sizeof (TDREPORT_STRUCT);

  Status = TdGetReport (
                        Report,
                        sizeof (TDREPORT_STRUCT),
                        AdditionalData,
                        TDREPORT_ADDITIONAL_DATA_SIZE
                        );
  if (EFI_ERROR (Status)) {
    Print (L"TdGetReport failed");
    goto QuitCheckRTMRForVTPM;
  }

  TdReport = (TDREPORT_STRUCT *)Report;
  for (index=0; index<4; index++) {
    Print (L"RTMR%01x: ", index);
    InternalDumpData(TdReport->Tdinfo.Rtmrs[index], SHA384_DIGEST_SIZE);
    Print (L"\n");
  }

  Status = EFI_SUCCESS;

QuitCheckRTMRForVTPM:
  if (Report) {
    FreePages (Report, Pages);
  }

  return Status;
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  ) 
{
  return DumpRtmr3WithTdReport();
}