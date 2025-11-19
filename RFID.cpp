/*
 * RFID.cpp - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * Based on code Dr.Leong   ( WWW.B2CQSHOP.COM )
 * Created by Miguel Balboa, Jan, 2012.
 * Modified to Software SPI (no external libraries)
 * Released into the public domain.
 */

/******************************************************************************
 * 包含文件
 ******************************************************************************/
#include <Arduino.h>
#include <RFID.h>

/******************************************************************************
 * 构造 RFID
 * 输入参数：
 *   chipSelectPin: RFID模块CS引脚
 *   NRSTPD: RFID模块复位引脚
 *   sckPin: 软件SPI时钟引脚
 *   mosiPin: 软件SPI数据输出引脚
 *   misoPin: 软件SPI数据输入引脚
 ******************************************************************************/
RFID::RFID(int chipSelectPin, int NRSTPD, int sckPin, int mosiPin, int misoPin)
{
  _chipSelectPin = chipSelectPin;
  _NRSTPD = NRSTPD;
  _sckPin = sckPin;
  _mosiPin = mosiPin;
  _misoPin = misoPin;

  // 初始化引脚模式
  pinMode(_chipSelectPin, OUTPUT);
  pinMode(_NRSTPD, OUTPUT);
  pinMode(_sckPin, OUTPUT);
  pinMode(_mosiPin, OUTPUT);
  pinMode(_misoPin, INPUT);

  // 初始状态设置
  digitalWrite(_chipSelectPin, HIGH);  // 初始禁用RFID模块
  digitalWrite(_NRSTPD, HIGH);         // 禁用复位
  digitalWrite(_sckPin, LOW);          // SCK初始低电平
  digitalWrite(_mosiPin, LOW);         // MOSI初始低电平
}

/******************************************************************************
 * 软件SPI核心函数：发送一个字节并接收返回字节（MSB先传）
 ******************************************************************************/
unsigned char RFID::spiTransfer(unsigned char data)
{
  unsigned char received = 0;

  for (int i = 7; i >= 0; i--)  // 从最高位(bit7)到最低位(bit0)
  {
    // 1. 输出当前bit到MOSI
    digitalWrite(_mosiPin, (data >> i) & 0x01);
    
    // 2. 拉高SCK，让从机读取MOSI数据
    digitalWrite(_sckPin, HIGH);
    delayMicroseconds(1);  // 时序匹配，可根据实际情况调整
    
    // 3. 读取MISO数据
    received |= (digitalRead(_misoPin) << i);
    
    // 4. 拉低SCK，准备下一位数据
    digitalWrite(_sckPin, LOW);
    delayMicroseconds(1);
  }

  return received;
}

/******************************************************************************
 * 用户 API 函数
 ******************************************************************************/

/******************************************************************************
 * 函 数 名：isCard
 * 功能描述：寻卡
 * 返 回 值：成功返回true 失败返回false
 ******************************************************************************/
bool RFID::isCard()
{
  unsigned char status;
  unsigned char str[MAX_LEN];

  status = MFRC522Request(PICC_REQIDL, str);
  return (status == MI_OK);
}

/******************************************************************************
 * 函 数 名：readCardSerial
 * 功能描述：读取卡序列号（4字节），存入serNum数组
 * 返 回 值：成功返回true 失败返回false
 ******************************************************************************/
bool RFID::readCardSerial()
{
  unsigned char status;
  unsigned char str[MAX_LEN];
  
  // 防冲撞，返回卡序列号（5字节：4字节序列号+1字节校验）
  status = anticoll(str);
  memcpy(serNum, str, 5);
  
  return (status == MI_OK);
}

/******************************************************************************
 * 函 数 名：init
 * 功能描述：初始化RC522模块
 ******************************************************************************/
void RFID::init()
{
  digitalWrite(_NRSTPD, HIGH);
  reset();

  // 定时器配置：TPrescaler*TreloadVal/6.78MHz = 24ms
  writeMFRC522(TModeReg, 0x8D);        // Tauto=1; f(Timer) = 6.78MHz/TPreScaler
  writeMFRC522(TPrescalerReg, 0x3E);   // TModeReg[3..0] + TPrescalerReg
  writeMFRC522(TReloadRegL, 30);
  writeMFRC522(TReloadRegH, 0);
  writeMFRC522(TxAutoReg, 0x40);       // 100% ASK调制
  writeMFRC522(ModeReg, 0x3D);         // CRC初始值 0x6363

  antennaOn();  // 打开天线
}

/******************************************************************************
 * 函 数 名：reset
 * 功能描述：复位RC522模块
 ******************************************************************************/
void RFID::reset()
{
  writeMFRC522(CommandReg, PCD_RESETPHASE);
}

/******************************************************************************
 * 函 数 名：writeMFRC522
 * 功能描述：向RC522寄存器写入一个字节
 * 输入参数：addr-寄存器地址；val-要写入的值
 ******************************************************************************/
void RFID::writeMFRC522(unsigned char addr, unsigned char val)
{
  digitalWrite(_chipSelectPin, LOW);  // 选中RC522模块

  // 地址格式：0XXXXXX0（最低位为0表示写操作）
  spiTransfer((addr << 1) & 0x7E);
  spiTransfer(val);

  digitalWrite(_chipSelectPin, HIGH);  // 取消选中
}

/******************************************************************************
 * 函 数 名：readMFRC522
 * 功能描述：从RC522寄存器读取一个字节
 * 输入参数：addr-寄存器地址
 * 返 回 值：读取到的字节数据
 ******************************************************************************/
unsigned char RFID::readMFRC522(unsigned char addr)
{
  unsigned char val;
  
  digitalWrite(_chipSelectPin, LOW);  // 选中RC522模块

  // 地址格式：1XXXXXX0（最高位为1表示读操作，最低位固定为0）
  spiTransfer(((addr << 1) & 0x7E) | 0x80);
  val = spiTransfer(0x00);  // 发送空字节，读取返回数据

  digitalWrite(_chipSelectPin, HIGH);  // 取消选中
  return val;
}

/******************************************************************************
 * 函 数 名：setBitMask
 * 功能描述：设置RC522寄存器的指定位
 * 输入参数：reg-寄存器地址；mask-要置1的位掩码
 ******************************************************************************/
void RFID::setBitMask(unsigned char reg, unsigned char mask)
{
  unsigned char tmp = readMFRC522(reg);
  writeMFRC522(reg, tmp | mask);
}

/******************************************************************************
 * 函 数 名：clearBitMask
 * 功能描述：清除RC522寄存器的指定位
 * 输入参数：reg-寄存器地址；mask-要清0的位掩码
 ******************************************************************************/
void RFID::clearBitMask(unsigned char reg, unsigned char mask)
{
  unsigned char tmp = readMFRC522(reg);
  writeMFRC522(reg, tmp & (~mask));
}

/******************************************************************************
 * 函 数 名：antennaOn
 * 功能描述：开启天线（每次开关天线间隔至少1ms）
 ******************************************************************************/
void RFID::antennaOn(void)
{
  unsigned char temp = readMFRC522(TxControlReg);
  if (!(temp & 0x03))
  {
    setBitMask(TxControlReg, 0x03);
  }
}

/******************************************************************************
 * 函 数 名：antennaOff
 * 功能描述：关闭天线（每次开关天线间隔至少1ms）
 ******************************************************************************/
void RFID::antennaOff(void)
{
  clearBitMask(TxControlReg, 0x03);
}

/******************************************************************************
 * 函 数 名：calculateCRC
 * 功能描述：用RC522硬件CRC模块计算CRC16
 * 输入参数：pIndata-输入数据；len-数据长度；pOutData-输出CRC结果（2字节）
 ******************************************************************************/
void RFID::calculateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
  unsigned char i, n;

  clearBitMask(DivIrqReg, 0x04);  // CRCIrq = 0
  setBitMask(FIFOLevelReg, 0x80);  // 清空FIFO缓冲区

  // 向FIFO写入数据
  for (i = 0; i < len; i++)
    writeMFRC522(FIFODataReg, *(pIndata + i));
  
  writeMFRC522(CommandReg, PCD_CALCCRC);  // 启动CRC计算

  // 等待CRC计算完成
  i = 0xFF;
  do
  {
    n = readMFRC522(DivIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x04));  // CRCIrq = 1表示计算完成

  // 读取CRC结果（低字节在前，高字节在后）
  pOutData[0] = readMFRC522(CRCResultRegL);
  pOutData[1] = readMFRC522(CRCResultRegM);
}

/******************************************************************************
 * 函 数 名：MFRC522ToCard
 * 功能描述：RC522与ISO14443卡通讯
 * 输入参数：
 *   command-RC522命令字
 *   sendData-发送数据缓冲区
 *   sendLen-发送数据长度
 *   backData-接收数据缓冲区
 *   backLen-接收数据位长度（输出）
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char RFID::MFRC522ToCard(unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen)
{
  unsigned char status = MI_ERR;
  unsigned char irqEn = 0x00;
  unsigned char waitIRq = 0x00;
  unsigned char lastBits;
  unsigned char n;
  unsigned int i;

  // 根据命令类型配置中断
  switch (command)
  {
    case PCD_AUTHENT:    // 认证命令
    {
      irqEn = 0x12;
      waitIRq = 0x10;
      break;
    }
    case PCD_TRANSCEIVE: // 收发数据命令
    {
      irqEn = 0x77;
      waitIRq = 0x30;
      break;
    }
    default:
      break;
  }

  writeMFRC522(CommIEnReg, irqEn | 0x80);  // 允许中断请求
  clearBitMask(CommIrqReg, 0x80);          // 清除所有中断标志
  setBitMask(FIFOLevelReg, 0x80);          // 清空FIFO缓冲区
  writeMFRC522(CommandReg, PCD_IDLE);      // 取消当前命令

  // 向FIFO写入发送数据
  for (i = 0; i < sendLen; i++)
    writeMFRC522(FIFODataReg, sendData[i]);

  // 执行命令
  writeMFRC522(CommandReg, command);
  if (command == PCD_TRANSCEIVE)
    setBitMask(BitFramingReg, 0x80);  // StartSend=1，开始发送数据

  // 等待通讯完成
  i = 2000;  // 超时时间（根据时钟频率调整，最大25ms）
  do
  {
    n = readMFRC522(CommIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

  clearBitMask(BitFramingReg, 0x80);  // StartSend=0

  if (i != 0)
  {
    // 检查是否有错误
    if (!(readMFRC522(ErrorReg) & 0x1B))  // 无缓冲区溢出、碰撞、CRC、协议错误
    {
      status = MI_OK;
      if (n & irqEn & 0x01)
        status = MI_NOTAGERR;  // 无卡错误

      if (command == PCD_TRANSCEIVE)
      {
        n = readMFRC522(FIFOLevelReg);    // 读取FIFO中接收的数据长度（字节数）
        lastBits = readMFRC522(ControlReg) & 0x07;  // 最后一个字节的有效位数

        if (lastBits)
          *backLen = (n - 1) * 8 + lastBits;
        else
          *backLen = n * 8;

        if (n == 0)
          n = 1;
        if (n > MAX_LEN)
          n = MAX_LEN;

        // 读取接收数据
        for (i = 0; i < n; i++)
          backData[i] = readMFRC522(FIFODataReg);
      }
    }
    else
      status = MI_ERR;
  }

  return status;
}

/******************************************************************************
 * 函 数 名：MFRC522Request
 * 功能描述：寻卡，读取卡类型
 * 输入参数：reqMode-寻卡模式；TagType-返回卡类型（输出）
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char RFID::MFRC522Request(unsigned char reqMode, unsigned char *TagType)
{
  unsigned char status;
  unsigned int backBits;  // 接收数据位数

  writeMFRC522(BitFramingReg, 0x07);  // TxLastBits = 7

  TagType[0] = reqMode;
  status = MFRC522ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

  if ((status != MI_OK) || (backBits != 0x10))
    status = MI_ERR;

  return status;
}

/******************************************************************************
 * 函 数 名：anticoll
 * 功能描述：防冲突检测，读取卡序列号（5字节）
 * 输入参数：serNum-返回卡序列号（输出）
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char RFID::anticoll(unsigned char *serNum)
{
  unsigned char status;
  unsigned char i;
  unsigned char serNumCheck = 0;
  unsigned int unLen;

  writeMFRC522(BitFramingReg, 0x00);  // TxLastBits = 0

  serNum[0] = PICC_ANTICOLL;
  serNum[1] = 0x20;
  status = MFRC522ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

  if (status == MI_OK)
  {
    // 校验卡序列号（前4字节异或等于第5字节）
    for (i = 0; i < 4; i++)
      serNumCheck ^= serNum[i];
    if (serNumCheck != serNum[4])
      status = MI_ERR;
  }

  return status;
}

/******************************************************************************
 * 函 数 名：auth
 * 功能描述：验证卡片密钥
 * 输入参数：
 *   authMode-认证模式（PICC_AUTHENT1A/PICC_AUTHENT1B）
 *   BlockAddr-块地址
 *   Sectorkey-扇区密钥（6字节）
 *   serNum-卡序列号（4字节）
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char RFID::auth(unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum)
{
  unsigned char status;
  unsigned int recvBits;
  unsigned char i;
  unsigned char buff[12];

  // 构建认证数据：认证模式+块地址+密钥+卡序列号
  buff[0] = authMode;
  buff[1] = BlockAddr;
  for (i = 0; i < 6; i++)
    buff[i + 2] = Sectorkey[i];
  for (i = 0; i < 4; i++)
    buff[i + 8] = serNum[i];
    
  status = MFRC522ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);
  if ((status != MI_OK) || (!(readMFRC522(Status2Reg) & 0x08)))
    status = MI_ERR;

  return status;
}

/******************************************************************************
 * 函 数 名：read
 * 功能描述：读取Mifare卡指定块的16字节数据
 * 输入参数：blockAddr-块地址；recvData-接收数据缓冲区（16字节）
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char RFID::read(unsigned char blockAddr, unsigned char *recvData)
{
  unsigned char status;
  unsigned int unLen;

  recvData[0] = PICC_READ;
  recvData[1] = blockAddr;
  calculateCRC(recvData, 2, &recvData[2]);  // 计算CRC
  status = MFRC522ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

  if ((status != MI_OK) || (unLen != 0x90))  // 正常应返回144位（16字节+2字节CRC）
    status = MI_ERR;

  return status;
}

/******************************************************************************
 * 函 数 名：write
 * 功能描述：向Mifare卡指定块写入16字节数据
 * 输入参数：blockAddr-块地址；writeData-写入数据缓冲区（16字节）
 * 返 回 值：成功返回MI_OK
 ******************************************************************************/
unsigned char RFID::write(unsigned char blockAddr, unsigned char *writeData)
{
  unsigned char status;
  unsigned int recvBits;
  unsigned char i;
  unsigned char buff[18];

  // 第一步：发送写命令和块地址
  buff[0] = PICC_WRITE;
  buff[1] = blockAddr;
  calculateCRC(buff, 2, &buff[2]);  // 计算CRC
  status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

  // 检查响应是否正确（应返回0x0A）
  if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    status = MI_ERR;

  // 第二步：发送16字节数据
  if (status == MI_OK)
  {
    // 填充要写入的数据
    for (i = 0; i < 16; i++)
      buff[i] = writeData[i];
      
    calculateCRC(buff, 16, &buff[16]);  // 计算数据CRC
    status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

    // 检查响应是否正确
    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
      status = MI_ERR;
  }

  return status;
}

/******************************************************************************
 * 函 数 名：selectTag
 * 功能描述：选卡，返回卡容量
 * 输入参数：serNum-卡序列号（5字节）
 * 返 回 值：成功返回卡容量（字节），失败返回0
 ******************************************************************************/
unsigned char RFID::selectTag(unsigned char *serNum)
{
  unsigned char i;
  unsigned char status;
  unsigned char size;
  unsigned int recvBits;
  unsigned char buffer[9];

  buffer[0] = PICC_SElECTTAG;
  buffer[1] = 0x70;  // 选卡命令参数

  // 填充卡序列号
  for (i = 0; i < 5; i++)
    buffer[i + 2] = serNum[i];

  calculateCRC(buffer, 7, &buffer[7]);  // 计算CRC
  status = MFRC522ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);

  if ((status == MI_OK) && (recvBits == 0x18))  // 正常返回24位（3字节）
    size = buffer[0];  // 卡容量（如4字节表示1KB卡）
  else
    size = 0;

  return size;
}

/******************************************************************************
 * 函 数 名：halt
 * 功能描述：命令卡片进入休眠状态
 ******************************************************************************/
void RFID::halt()
{
  unsigned int unLen;
  unsigned char buff[4];

  buff[0] = PICC_HALT;
  buff[1] = 0;
  calculateCRC(buff, 2, &buff[2]);  // 计算CRC

  MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen);
}