/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// This code was adapted from:
//
//  https://github.com/bztsrc/raspi3-tutorial/tree/master/0B_readsector
//
//  Copyright (C) 2018 bzt (bztsrc@github)


#include "bare.h"

#include "bare/SDCard.h"

#include "bare/GPIO.h"
#include "bare/Serial.h"
#include "bare/Timer.h"

//#define ENABLE_DEBUG_LOG
#include "bare/Log.h"

using namespace bare;

static void finishFail()
{
    DEBUG_LOG("SDCard: EMMC init FAILED!\n");
}

static constexpr uint32_t EMMCBase = 0x20300000;

struct EMMC
{
    uint32_t arg2;
    uint32_t blockSizeCount;
    uint32_t arg1;
    uint32_t commandAndTransferMode;
    uint32_t response0;
    uint32_t response1;
    uint32_t response2;
    uint32_t response3;
    uint32_t data;
    uint32_t status;
    uint32_t control0;
    uint32_t control1;
    uint32_t interrupt;
    uint32_t interruptMask;
    uint32_t interruptEnable;
    uint32_t control2;
    uint32_t unused[47];
    uint32_t slotISRVersion;
};

static_assert(sizeof(EMMC) == 0x100, "Wrong size for EMMC");

inline volatile EMMC& emmc()
{
    return *(reinterpret_cast<volatile EMMC*>(EMMCBase));
}

// command flags
#define CMDFLAG_NEED_APP        0x80000000
#define CMDFLAG_RSPNS_48        0x00020000
#define CMDFLAG_ERRORS_MASK     0xfff9c004
#define CMDFLAG_RCA_MASK        0xffff0000

// COMMANDs
#ifdef DEBUG_SD
#define CMD(name, code) static inline Command name() { return { code, #name }; }
#else
#define CMD(name, code) static inline Command name() { return { code }; }
#endif
CMD(CMD_GO_IDLE,        0x00000000)
CMD(CMD_ALL_SEND_CID,   0x02010000)
CMD(CMD_SEND_REL_ADDR,  0x03020000)
CMD(CMD_CARD_SELECT,    0x07030000)
CMD(CMD_SEND_IF_COND,   0x08020000)
CMD(CMD_STOP_TRANS,     0x0C030000)
CMD(CMD_READ_SINGLE,    0x11220010)
CMD(CMD_READ_MULTI,     0x12220032)
CMD(CMD_WRITE_SINGLE,   0x18220000)
CMD(CMD_WRITE_MULTI,    0x19220022)
CMD(CMD_SET_BLOCKCNT,   0x17020000)
CMD(CMD_APP_CMD,        0x37000000)
CMD(CMD_APP_CMD_48,     (0x37000000 | CMDFLAG_RSPNS_48))
CMD(CMD_SET_BUS_WIDTH,  (0x06020000 | CMDFLAG_NEED_APP))
CMD(CMD_SEND_OP_COND,   (0x29020000 | CMDFLAG_NEED_APP))
CMD(CMD_SEND_SCR,       (0x33220010 | CMDFLAG_NEED_APP))

// STATUS register settings
#define SR_READ_AVAILABLE   0x00000800
#define SR_DAT_INHIBIT      0x00000002
#define SR_CMD_INHIBIT      0x00000001
#define SR_APP_CMD          0x00000020

// INTERRUPT register settings
#define INT_DATA_TIMEOUT    0x00100000
#define INT_CMD_TIMEOUT     0x00010000
#define INT_READ_RDY        0x00000020
#define INT_WRITE_RDY       0x00000010
#define INT_CMD_DONE        0x00000001

#define INT_ERROR_MASK      0x017E8000

// CONTROL register settings
#define C0_SPI_MODE_EN      0x00100000
#define C0_HCTL_HS_EN       0x00000004
#define C0_HCTL_DWITDH      0x00000002

#define C1_SRST_DATA        0x04000000
#define C1_SRST_CMD         0x02000000
#define C1_SRST_HC          0x01000000
#define C1_TOUNIT_DIS       0x000f0000
#define C1_TOUNIT_MAX       0x000e0000
#define C1_CLK_GENSEL       0x00000020
#define C1_CLK_EN           0x00000004
#define C1_CLK_STABLE       0x00000002
#define C1_CLK_INTLEN       0x00000001

// SLOTISR_VER values
#define HOST_SPEC_NUM       0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16
#define HOST_SPEC_V3        2
#define HOST_SPEC_V2        1
#define HOST_SPEC_V1        0

// SCR flags
#define SCR_SD_BUS_WIDTH_4  0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000
#define SCR_SUPP_CCS        0x00000001

#define ACMD41_VOLTAGE      0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS      0x40000000
#define ACMD41_ARG_HC       0x51ff8000
    
constexpr uint32_t GPIO_CD   = 47;
constexpr uint32_t GPIO_CLK  = 48;
constexpr uint32_t GPIO_CMD  = 49;
constexpr uint32_t GPIO_DAT0 = 50;
constexpr uint32_t GPIO_DAT1 = 51;
constexpr uint32_t GPIO_DAT2 = 52;
constexpr uint32_t GPIO_DAT3 = 53;

static uint32_t _rca = 0;
static uint32_t _scr[2] { 0, 0 };


static bool checkStatusWithTimeout(std::function<bool()> func, const char* error, uint32_t count = 1000)
{
    for (uint32_t i = 0; ; ++i) {
        if (func()) {
            return true;
        }
        if (i >= count) {
            if (error) {
                ERROR_LOG("ERROR: %s\n", error);
            }
            return false;
        }
        Timer::usleep(1000);
    }
}

static SDCard::Error setClock(uint32_t freq, uint32_t hostVersion)
{
    uint32_t d;
    uint32_t c= 41666666 / freq;
    uint32_t x;
    uint32_t s = 32;
    uint32_t h = 0;
    
    if (!checkStatusWithTimeout(
        []{ return (emmc().status & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) == 0; },
        "setClock timeout waiting for inhibit flag")) {
        return SDCard::Error::Error;
    }
        
    emmc().control1 &= ~C1_CLK_EN;
    Timer::usleep(10000);
    
    x = c - 1;
    if (!x) {
        s = 0;
    } else {
        if (!(x & 0xffff0000u)) {
            x <<= 16;
            s -= 16;
        }
        if (!(x & 0xff000000u)) {
            x <<= 8;
            s -= 8;
        }
        if (!(x & 0xf0000000u)) {
            x <<= 4;
            s -= 4;
        }
        if (!(x & 0xc0000000u)) {
            x <<= 2;
            s -= 2;
        }
        if (!(x & 0x80000000u)) {
            x <<= 1;
            s -= 1;
        }
        if (s > 0) {
            s--;
        }
        if (s > 7) {
            s = 7;
        }
    }
    
    if (hostVersion > HOST_SPEC_V2) {
        d = c;
    } else {
        d = (1 << s);
    }
    if (d <= 2) {
        d=2;
        s=0;
    }
    
    DEBUG_LOG("SDCard: clock divisor=0x%08x, shift=0x%08x\n", d, s);

    if (hostVersion > HOST_SPEC_V2) {
        h = (d & 0x300) >> 2;
    }
    d = ((d & 0x0ff) << 8) | h;
    
    emmc().control1 = (emmc().control1 & 0xffff003f) | d;
    Timer::usleep(10000);
    
    emmc().control1 |= C1_CLK_EN;
    Timer::usleep(10000);
    
    if (!checkStatusWithTimeout(
        []{ return (emmc().control1 & C1_CLK_STABLE) != 0; },
        "setClock failed to get stable clock")) {
        return SDCard::Error::Error;
    }
    DEBUG_LOG("SDCard: setClock succeeded\n");
    return SDCard::Error::OK;
}

static SDCard::Error readStatus(uint32_t mask)
{
    bool result = checkStatusWithTimeout(
        [mask]{ return (emmc().status & mask && emmc().interrupt & INT_ERROR_MASK) == 0; },
        nullptr, 500000);
        
    if (!result || emmc().interrupt & INT_ERROR_MASK) {
        return SDCard::Error::Error;
    }
    return SDCard::Error::OK;
}

static SDCard::Error waitForInterrupt(uint32_t mask)
{
    uint32_t m = mask | INT_ERROR_MASK;
    bool result = checkStatusWithTimeout(
        [m]{ return (emmc().interrupt & m) != 0; },
        nullptr);
        
    uint32_t r = emmc().interrupt;
    if (!result || (r & INT_CMD_TIMEOUT) || r & INT_DATA_TIMEOUT) {
        emmc().interrupt = r;
        ERROR_LOG("SDCard: ERROR waitForInterrupt timed out\n");
        return SDCard::Error::Timeout;
    }
    if (r & INT_ERROR_MASK) {
        emmc().interrupt = r;
        ERROR_LOG("SDCard: ERROR waitForInterrupt error\n");
        return SDCard::Error::Error;
    }
    emmc().interrupt = mask;
    return SDCard::Error::OK;
}

static SDCard::Error sendCommand(const Command& cmd, uint32_t arg, uint32_t& response)
{
    SDCard::Error error = SDCard::Error::OK;
    uint32_t code = cmd.code;

    if(cmd.code & CMDFLAG_NEED_APP) {
        error = sendCommand(_rca ? CMD_APP_CMD_48() : CMD_APP_CMD(), _rca, response);
        
        if(_rca && error != SDCard::Error::OK) {
            ERROR_LOG("ERROR: failed to send SD APP command\n");
            return error;
        }
        
        code &= ~CMDFLAG_NEED_APP;
    }
    
    if (readStatus(SR_CMD_INHIBIT) != SDCard::Error::OK) {
        ERROR_LOG("ERROR: EMMC busy\n");
        return SDCard::Error::Timeout;
    }
    
#ifdef DEBUG_SD
    DEBUG_LOG("EMMC: Sending command %s arg 0x%08x\n", cmd.name, arg);
#endif

    emmc().interrupt = emmc().interrupt;
    emmc().arg1 = arg;
    emmc().commandAndTransferMode = code;

    if (code == CMD_SEND_OP_COND()) {
        Timer::usleep(1000000);
    } else if (code == CMD_SEND_IF_COND() || code == CMD_APP_CMD()) {
        Timer::usleep(100000);
    }
    
    error = waitForInterrupt(INT_CMD_DONE);
    if (error != SDCard::Error::OK) {
        ERROR_LOG("ERROR: failed to send EMMC command\n");
        return error;
    }
    
    uint32_t resp0 = emmc().response0;
    if (code == CMD_GO_IDLE() || code == CMD_APP_CMD()) {
        return SDCard::Error::OK; 
    } else if (code == (CMD_APP_CMD() | CMDFLAG_RSPNS_48)) {
        response = resp0 & SR_APP_CMD;
        return SDCard::Error::OK;
    } else if (code == CMD_SEND_OP_COND()) {
        response = resp0;
        return SDCard::Error::OK;
    } else if (code == CMD_SEND_IF_COND()) {
        return (resp0 == arg) ? SDCard::Error::OK : SDCard::Error::Error;
    } else if (code == CMD_ALL_SEND_CID()) {
        resp0 |= emmc().response3;
        resp0 |= emmc().response2;
        resp0 |= emmc().response1;
        response = resp0;
        return SDCard::Error::OK;
    } else if (code == CMD_SEND_REL_ADDR()) {
        // FIXME:should do better error handling
        //sd_err = (((resp0 & 0x1fff)) | ((resp0 & 0x2000) << 6) | ((resp0 & 0x4000) << 8) | ((resp0 & 0x8000) << 8)) & CMDFLAG_ERRORS_MASK;
        response = resp0 & CMDFLAG_RCA_MASK;
        return SDCard::Error::Error;
    }
    
    response = resp0 & CMDFLAG_ERRORS_MASK;
    return SDCard::Error::OK;
}

static SDCard::Error sendCommand(const Command& cmd, uint32_t arg)
{
    uint32_t response;
    return sendCommand(cmd, arg, response);
}

static SDCard::Error setSCRValues()
{    
    SDCard::Error error = sendCommand(CMD_SEND_SCR(), 0);
    if (error != SDCard::Error::OK) {
        return error;
    }
    error = waitForInterrupt(INT_READ_RDY);
    if (error != SDCard::Error::OK) {
        return error;
    }
    
    uint32_t index = 0;
    for (uint32_t count = 10000; index < 2 && count > 0; --count) {
        if (emmc().status & SR_READ_AVAILABLE) {
            _scr[index++] = emmc().data;
        } else {
            Timer::usleep(1000);
        }
    }
    if (index != 2) {
        ERROR_LOG("SDCard: ERROR getSCRValues SR_READ_AVAILABLE never true\n");
        return SDCard::Error::Error;
    }
    return SDCard::Error::OK;
}

SDCard::SDCard()
{
    DEBUG_LOG("SDCard: Start init\n");

    // Initialize GPIO for all the SD pins
    uint32_t reg = GPIO::reg(GPIO::Register::GPHEN1);
    reg = reg | 1<<(47-32);
    GPIO::reg(GPIO::Register::GPHEN1) = reg;

    GPIO::setFunction(GPIO_DAT3, GPIO::Function::Alt3,GPIO::Pull::Up);
    GPIO::setFunction(GPIO_DAT2, GPIO::Function::Alt3, GPIO::Pull::Up);
    GPIO::setFunction(GPIO_DAT1, GPIO::Function::Alt3, GPIO::Pull::Up);
    GPIO::setFunction(GPIO_DAT0, GPIO::Function::Alt3, GPIO::Pull::Up);
    GPIO::setFunction(GPIO_CMD, GPIO::Function::Alt3, GPIO::Pull::Up);
    GPIO::setFunction(GPIO_CLK, GPIO::Function::Alt3, GPIO::Pull::Up);

    uint32_t hostVersion = (emmc().slotISRVersion & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;

    // Reset the card.
    emmc().control0 = 0;
    emmc().control1 |= C1_SRST_HC;
    
    for (int i = 0; ; ++i) {
        Timer::usleep(10000);
        if (i >= 10000) {
            ERROR_LOG("SDCard: Failed to reset EMMC\n");
            finishFail();
            return;
        }
        if ((emmc().control1 & C1_SRST_HC) == 0) {
            break;
        }
    }
    
    DEBUG_LOG("SDCard: EMMC reset succeeded\n");
    
    emmc().control1 |= C1_CLK_INTLEN | C1_TOUNIT_MAX;
    Timer::usleep(10000);

    // Set clock to setup frequency.
    if ((setClock(400000, hostVersion)) != Error::OK) {
        finishFail();
        return;
    }
    
    emmc().interruptEnable = 0xffffffff;
    emmc().interruptMask = 0xffffffff;

    if (sendCommand(CMD_GO_IDLE(), 0) != Error::OK) {
        finishFail();
        return;
    }

    if (sendCommand(CMD_SEND_IF_COND(), 0x000001AA) != Error::OK) {
        finishFail();
        return;
    }
    
    uint32_t count = 6;
    uint32_t response = 0;
    Error error = Error::OK;
    
    while(!(response & ACMD41_CMD_COMPLETE) && count--) {
        Timer::usleep(100);
        error = sendCommand(CMD_SEND_OP_COND(), ACMD41_ARG_HC, response);
        DEBUG_LOG("EMMC: CMD_SEND_OP_COND returned %s%s%s 0x%08x\n",
            (response & ACMD41_CMD_COMPLETE) ? "COMPLETE " : "",
            (response & ACMD41_VOLTAGE) ? "VOLTAGE " : "",
            (response & ACMD41_CMD_CCS) ? "CCS " : "",
            response);
        if(error != Error::Timeout && error != Error::OK ) {
            ERROR_LOG("ERROR: EMMC ACMD41 returned error\n");
            finishFail();
            return;
        }
    }
    
    if (!(response & ACMD41_CMD_COMPLETE) || !count) {
        finishFail();
        return;
    }
    if (!(response & ACMD41_VOLTAGE)) {
        finishFail();
        return;
    }
    
    uint32_t ccs = (response & ACMD41_CMD_CCS) ? SCR_SUPP_CCS : 0;
    sendCommand(CMD_ALL_SEND_CID(), 0);
    sendCommand(CMD_SEND_REL_ADDR(), 0, _rca);
    DEBUG_LOG("EMMC: CMD_SEND_REL_ADDR returned 0x%08x\n", _rca);
    if (error != Error::OK) {
        finishFail();
        return;
    }
    
    if (setClock(25000000, hostVersion) != Error::OK) {
        finishFail();
        return;
    }

    if (sendCommand(CMD_CARD_SELECT(), _rca) != Error::OK) {
        finishFail();
        return;
    }

    if (readStatus(SR_DAT_INHIBIT) != Error::OK) {
        finishFail();
        return;
    }
    
    emmc().blockSizeCount = (1 << 16) | 8;

    if (setSCRValues() != Error::OK) {
        finishFail();
        return;
    }
    
    DEBUG_LOG("SDCard: setSCRValues succeeded 0x%08x 0x%08x\n", _scr[0], _scr[1]);

    if (_scr[0] & SCR_SD_BUS_WIDTH_4) {
        if (sendCommand(CMD_SET_BUS_WIDTH(), _rca | 2) != Error::OK) {
            finishFail();
            return;
        }
        emmc().control0 |= C0_HCTL_DWITDH;
    }
    
    // add software flag
    DEBUG_LOG("EMMC: supports %s%s\n",
        (_scr[0] & SCR_SUPP_SET_BLKCNT) ? "SET_BLKCNT " : "",
        ccs ? "CCS " : "");
        
    _scr[0] &= ~SCR_SUPP_CCS;
    _scr[0] |= ccs;
    DEBUG_LOG("SDCard: EMMC init succeeded\n");
}

Volume::Error SDCard::read(char* buf, Block blockAddr, uint32_t blocks)
{
    if (blocks < 1) {
        blocks = 1;
    }
    
    DEBUG_LOG("SDCard: readBlock addr=%d, num=%d\n", blockAddr.value(), blocks);

    if (readStatus(SR_DAT_INHIBIT) != Error::OK) {
        ERROR_LOG("SDCard: readBlock timeout on readStatus(SR_DAT_INHIBIT)\n");
        return Volume::Error::InternalError;
    }
    
    uint32_t* currentPtr = reinterpret_cast<uint32_t*>(buf);
    Error error = Error::OK;
    
    if (_scr[0] & SCR_SUPP_CCS) {
        if (blocks > 1 && (_scr[0] & SCR_SUPP_SET_BLKCNT)) {
            error = sendCommand(CMD_SET_BLOCKCNT(), blocks);
            if(error != Error::OK) {
                ERROR_LOG("SDCard: error sending CMD_SET_BLOCKCNT\n");
                return Volume::Error::InternalError;
            }
        }
        
        emmc().blockSizeCount = (blocks << 16) | 512;
        
        error = sendCommand((blocks == 1) ? CMD_READ_SINGLE() : CMD_READ_MULTI(), blockAddr.value());
        if (error != Error::OK) {
            ERROR_LOG("SDCard: error sending CMD_READ_*\n");
            return Volume::Error::InternalError;
        }
    } else {
        emmc().blockSizeCount = (1 << 16) | 512;
    }
    
    for (uint32_t currentBlock = 0; currentBlock < blocks; ++currentBlock) {
        if(!(_scr[0] & SCR_SUPP_CCS)) {
            error = sendCommand(CMD_READ_SINGLE(), (blockAddr.value() + currentBlock) * 512);
            if (error != Error::OK) {
                ERROR_LOG("SDCard: error sending CMD_READ_SINGLE for addr %d\n", (blockAddr.value() + currentBlock) * 512);
                return Volume::Error::InternalError;
            }
        }
        error = waitForInterrupt(INT_READ_RDY);
        if (error != Error::OK) {
            ERROR_LOG("ERROR: Timeout waiting for ready to read\n");
            return Volume::Error::InternalError;
        }
        
        for (uint32_t d = 0; d < 128; d++) {
            currentPtr[d] = emmc().data;
        }
        
        currentPtr += 128;
    }
    
    if (blocks > 1 && !(_scr[0] & SCR_SUPP_SET_BLKCNT) && (_scr[0] & SCR_SUPP_CCS)) {
        error = sendCommand(CMD_STOP_TRANS(), 0);
        if (error != Error::OK) {
            ERROR_LOG("ERROR: sending CMD_STOP_TRANS\n");
            return Volume::Error::InternalError;
        }
    }
    
    return Volume::Error::OK;
}

Volume::Error SDCard::write(const char* buf, Block blockAddr, uint32_t blocks)
{
    if (blocks < 1) {
        blocks = 1;
    }
    
    DEBUG_LOG("SDCard: writeBlock addr=%d, num=%d\n", blockAddr.value(), blocks);

    if (readStatus(SR_DAT_INHIBIT) != Error::OK) {
        ERROR_LOG("SDCard: writeBlock timeout on readStatus(SR_DAT_INHIBIT)\n");
        return Volume::Error::InternalError;
    }
    
    const uint32_t* currentPtr = reinterpret_cast<const uint32_t*>(buf);
    Error error = Error::OK;
    
    if (_scr[0] & SCR_SUPP_CCS) {
        if (blocks > 1 && (_scr[0] & SCR_SUPP_SET_BLKCNT)) {
            error = sendCommand(CMD_SET_BLOCKCNT(), blocks);
            if(error != Error::OK) {
                ERROR_LOG("SDCard: error sending CMD_SET_BLOCKCNT\n");
                return Volume::Error::InternalError;
            }
        }
        
        emmc().blockSizeCount = (blocks << 16) | 512;
        
        error = sendCommand((blocks == 1) ? CMD_WRITE_SINGLE() : CMD_WRITE_MULTI(), blockAddr.value());
        if (error != Error::OK) {
            ERROR_LOG("SDCard: error sending CMD_WRITE_*\n");
            return Volume::Error::InternalError;
        }
    } else {
        emmc().blockSizeCount = (1 << 16) | 512;
    }
    
    for (uint32_t currentBlock = 0; currentBlock < blocks; ++currentBlock) {
        if(!(_scr[0] & SCR_SUPP_CCS)) {
            error = sendCommand(CMD_WRITE_SINGLE(), (blockAddr.value() + currentBlock) * 512);
            if (error != Error::OK) {
                ERROR_LOG("SDCard: error sending CMD_WRITE_SINGLE for addr %d\n", (blockAddr.value() + currentBlock) * 512);
                return Volume::Error::InternalError;
            }
        }
        error = waitForInterrupt(INT_WRITE_RDY);
        if (error != Error::OK) {
            ERROR_LOG("ERROR: Timeout waiting for ready to write\n");
            return Volume::Error::InternalError;
        }
        
        for (uint32_t d = 0; d < 128; d++) {
            emmc().data = currentPtr[d];
        }
        
        currentPtr += 128;
    }
    
    // Sleep to give write a chance to finish
    Timer::usleep(50000);

    if (blocks > 1 && !(_scr[0] & SCR_SUPP_SET_BLKCNT) && (_scr[0] & SCR_SUPP_CCS)) {
        error = sendCommand(CMD_STOP_TRANS(), 0);
        if (error != Error::OK) {
            ERROR_LOG("ERROR: sending CMD_STOP_TRANS\n");
            return Volume::Error::InternalError;
        }
    }
    
    return Volume::Error::OK;
}
