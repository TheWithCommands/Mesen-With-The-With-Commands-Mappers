#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class Deset573:public BaseMapper
{
    private:
        bool prg_le_cooldown=false;
        bool chr_le_cooldown=false;

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x4000;}
        virtual uint16_t GetCHRPageSize() override {return 0x2000;}
        virtual bool HasBusConflicts() override {return true;}

    void InitMapper() override
    {
        SelectPRGPage(0,0);
        SelectPRGPage(1,-1);

        SelectCHRPage(0,0);
    }

    void StreamState(bool saving) override
    {
        BaseMapper::StreamState(saving);
        Stream(prg_le_cooldown,chr_le_cooldown);
    }

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        if((value&0x40)||prg_le_cooldown)
        {
            SelectPRGPage(0,value&0x1f);

            switch(_romInfo.NesHeader.Byte6&0x09)
            {
                case 8:
                case 9:
                    break;
                default:
                {
                    SetMirroringType(value&0x20?MirroringType::Vertical:MirroringType::Horizontal);
                    break;
                }
            }
        }
        if((value&0x80)||chr_le_cooldown)SelectCHRPage(0,value&0x3f);

        prg_le_cooldown=value&0x40;
        chr_le_cooldown=value&0x80;
    }
};
