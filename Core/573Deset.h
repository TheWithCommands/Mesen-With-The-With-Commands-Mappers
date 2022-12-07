#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class Deset573:public BaseMapper
{
    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x4000;}
        virtual uint16_t GetCHRPageSize() override {return 0x2000;}
        virtual bool HasBusConflicts() override {return true;}

    void InitMapper() override
    {
        SelectPRGPage(0,GetPowerOnByte());
        SelectPRGPage(1,-1);

        SelectCHRPage(0,GetPowerOnByte());
    }

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        if(value&0x40)
        {
            SelectPRGPage(0,value&0x1f);
            SetMirroringType(value&0x20?MirroringType::Vertical:MirroringType::Horizontal);
        }
        if(value&0x80)SelectCHRPage(0,value&0x3f);
    }
};
