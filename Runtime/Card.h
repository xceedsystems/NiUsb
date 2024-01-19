/***************************************************************

                Card.h             

   This file contains the interface to the manufacturer code

****************************************************************/


#ifndef  __CARD_H__
#define  __CARD_H__

int     Init( void* const dp, UINT16 const DprHWTests, P_ERR_PARAM const lpErrors);
int     TestConfig( LPDRIVER_INST const pNet, P_ERR_PARAM const lpErrors);
void    PortInput(  SPECIAL_INST_PORT* const pData);
void    PortOutput( SPECIAL_INST_PORT* const pData);
void    GetDriverStatus(const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData );
void    GetDeviceStatus(const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData );
void    DoPeekCommand(  const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData );
void    DoPokeCommand(  const LPDRIVER_INST pNet, SPECIAL_INST_COMMAND* const pData );
void    DoCommand(      const LPDRIVER_INST pNet, SPECIAL_INST* const pData );

void    DoGpibWrite(const LPDRIVER_INST pNet, GPIB_WRITE_COMMAND* const pData);
void    DoGpibRead(const LPDRIVER_INST pNet, SPECIAL_INST* const _pData);

void    DoPadWrite(const LPDRIVER_INST pNet, SET_PAD_COMMAND* const pData);
void    DoSadWrite(const LPDRIVER_INST pNet, SET_SAD_COMMAND* const pData);
void	DoIbRSVWrite(const LPDRIVER_INST pNet, SPECIAL_INST* const _pData);
void DoClearTxRxBuffer(const LPDRIVER_INST pNet, SPECIAL_INST * const _pData);


void	GpibsStatusUpdate(const LPDRIVER_INST pNet,  const LPDEVICE_INST pDevice, VOID *Dest);

#endif      /* __CARD_H__ */

