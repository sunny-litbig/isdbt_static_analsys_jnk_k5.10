/*******************************************************************************

*   FileName : mpegpars.h

*   Copyright (c) Telechips Inc.

*   Description : mpegpars.h

********************************************************************************
*
*   TCC Version 1.0

This source code contains confidential information of Telechips.

Any unauthorized use without a written permission of Telechips including not
limited to re-distribution in source or binary form is strictly prohibited.

This source code is provided "AS IS" and nothing contained in this source code
shall constitute any express or implied warranty of any kind, including without
limitation, any warranty of merchantability, fitness for a particular purpose
or non-infringement of any patent, copyright or other third party intellectual
property right.
No warranty is made, express or implied, regarding the information's accuracy,
completeness, or performance.

In no event shall Telechips be liable for any claim, damages or other
liability arising from, out of or in connection with this source code or
the use in the source code.

This source code is provided subject to the terms of a Mutual Non-Disclosure
Agreement between Telechips and Company.
*
*******************************************************************************/
 
/*****************************************************************************
*                                                                            *
*     ROUTINE : MPEGPARS_ParseTable                                          *
*                                                                            *
* DESCRIPTION : Processes the MPEG PSI data in the buffer chaining           *
*               the appropriate "struct"s as required. Each MPEG table       *
*               is proceesed seperately.                                     *
*                                                                            *
*                                                                            *
*        NOTE : Routine is NOT re-enterant.                                  *
*               (Use is made of module wide variable to avoid passing too    *
*                many pointers recursively.)                                 *
*                                                                            *
*                                                                            *
*        NOTE : This routines does NOT release the memory of the unparsed    *
*               MPEG table data, this must be done by the caller.            *
*                                                                            *
*                                                                            *
*       INPUT : pRawData - pointer to an unparsed MPEG table                 *
*               uiRawDataLen - size of pRawData                             *
*               pstTable - pointer to take parsed structure(s)               *
*                                                                            *
*      OUTPUT : penCurrentTable - MPEG spec value of table found.            *
*               pstTable        - pointer to "struct"(s) of table data.      *
*                                                                            *
*     RETURNS :  0 : succesful conversion, else                              *
*               -1 : invalid CRC32                                           *
*               -2 : invalid table ID                                        *
*               -3 : failure allocating primary table struct's memory        *
*               -4 : processing beyond section length                        *
*               -5 : failure allocating descriptor's struct's memory         *
*               -6 : failure allocating elementary stream descriptor memory  *
*               -7 : failure allocating SDT's service data memory            *
*               -8 : failure allocating transport stream descriptor's memory *
*               -9 : failure, section_syntax_indicator NOT set               *
*              -10 : processing beyond descriptor's length                   *
*              -11 : invalid descriptor                                      *
*              -12 : NOT U8 aligned, bit parsing error                       *
*              -13 : malloc failure GetRSTData                               *
*              -14 : processing descriptor beyond section length             *
*              -15 : malloc failure GetExtEvtData                            *
*              -16 : malloc failure GetMosaicData                            *
*              -17 : malloc failure GetEventData                             *
*                    Memory allocation failures:                             *
*              -18 : Main Control Table - service list                       *
*              -19 : Main Control Table - playout rate list                  *
*              -20 : Main Control Table - lang dep info                      *
*              -21 : Main Control Table - lang dep block list                *
*              -22 : Main Control Table - PIDs                               *
*              -23 : Main Control Table - country info                       *
*              -24 : Main Control Table - lang indep block list              *
*              -25 : Main Control Table - category PIDs                      *
*              -26 : Category Name Table - category list                     *
*              -27 : Category Name Table - category name list                *
*              -28 : Category Name Table - category item list                *
*              -29 : Category Name Table - category item language list       *
*              -30 : Category Name Table - category sub-item list            *
*              -31 : Category Name Table - category sub-item language list   *
*              -32 : Category Item Table - event list                        *
*              -33 : Category Item Table - subitems in event list            *
*              -34 : Category Item Table - event ID offset list              *
*              -35 : Two Dimensional Table - service list                    *
*              -36 : Two Dimensional Table - event   list                    *
*              -37 : Aux Control Table - item list                           *
*              -38 : Aux Table - Detail item list                            *
*              -39 : Aux Table - Extra data                                  *
*              -40 : Active Events Table - provider list                     *
*              -41 : Active Events Table - event ID list                     *
*                                                                            *
*                                                                            *
*****************************************************************************/
int MPEGPARS_ParseTable (void *handle, void *pRawData, unsigned int uiRawDataLen, void **pstTable);

/*****************************************************************************
*                                                                            *
*     ROUTINE : MPEGPARS_FreeTable                                           *
*                                                                            *
* DESCRIPTION : This routine frees all memory allocated by                   *
*               MPEGPARS_ParseTable. This routine and it's "nested" routines *
*               deallocate the memory in the reverse order of allocation.    *
*               The passed pointer is set to NULL on exit.                   *
*                                                                            *
*                                                                            *
*        NOTE : The deallocation fails if the table_id is NOT valid.         *
*                                                                            *
*                                                                            *
*       INPUT : pstTable - pointer to a table created (parsed) by            *
*                          MPEGPARS_ParseTable                               *
*                                                                            *
*      OUTPUT : none                                                         *
*                                                                            *
*     RETURNS : none                                                         *
*                                                                            *
*      Author :                                                              *
*                                                                            *
* Last Changed:                                                              *
*                                                                            *
*****************************************************************************/
void MPEGPARS_FreeTable (void **ppstTable);

/*****************************************************************************
*                                                                            *
*     ROUTINE : MPEGPARS_ParseTSHeader                                       *
*                                                                            *
* DESCRIPTION : Processes the MPEG TS data in the buffe. 					 *
*				All TS packets are proceesed in every time.                  *
*               Check sync_byte and adaptation_field.                        *
*                                                                            *
*        NOTE :                                                              *
*                                                                            *
*       INPUT : pRawData - pointer to an TS packet (188bytes)                *
*                                                                            *
*      OUTPUT : pstRealData - pointer of data_byte                           *
*                                                                            *
*     RETURNS : PID                                                          *
*                                                                            *
*      Author :                                                              *
*                                                                            *
* Last Changed:                                                              *
*                                                                            *
*****************************************************************************/
extern unsigned short MPEGPARS_ParseTSHeader (void *handle, void *pRawData, void **pstRealData, void *pstTSInfo);

/*****************************************************************************
*                                                                            *
*     ROUTINE : MPEGPARS_ParseSLPacketHeader                                 *
*                                                                            *
* DESCRIPTION : Processes the MPEG4 Sync Layer data in the buffer.           *
*                                                                            *
*        NOTE :                                                              *
*                                                                            *
*       INPUT : pRawData - pointer to an SL packet Header                    *
*                                                                            *
*      OUTPUT : pstRealData - pointer to SL packet Payload                   *
*                                                                            *
*     RETURNS : SL Pkt Header Length                                         *
*                                                                            *
*      Author :                                                              *
*                                                                            *
* Last Changed:                                                              *
*                                                                            *
*****************************************************************************/
extern int MPEGPARS_ParseSLPacketHeader (void *handle, void *pRawData, void **pstRealData,
                    void *pstSLConfigDescr, void *pstSLHeaderInfo );

/*****************************************************************************
*                                                                            
*     ROUTINE : ISDBT_MPEGPARS_ParsePES                                
*                                                                            
* DESCRIPTION : 
*                                                                            
*        NOTE :                                                             
*                                                                            
*       INPUT :
*                                                                            
*      OUTPUT : 
*                                                                            
*     RETURNS : 
*                                                                            
*      Author :                                                              
*                                                                            
* Last Changed:                                                              
*                                                                            
*****************************************************************************/
extern int ISDBT_MPEGPARS_ParsePES (void *handle, void *pRawData, void **pstRealData, void *pstPESInfo, unsigned int uiMode);

