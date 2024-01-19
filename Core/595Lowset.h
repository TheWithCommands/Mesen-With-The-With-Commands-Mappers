#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class Lowset595:public BaseMapper
{
    private:
        uint8_t _595data=_romInfo.NesHeader.Byte6&0x01?0xc0:0x80;
        bool _595busSrclkTriggered=false;
        bool _595busRclkTriggered=false;

    private:
        void set595Data()
        {
            SelectCHRPage(0,_595data&0x3f);
            switch(_romInfo.NesHeader.Byte6&0x09)
            {
                case 8:
                case 9:
                {
                    SetMirroringType(MirroringType::FourScreens);
                    break;
                }
                default:
                {
                    if(_595data&0x80)
                    {
                        SetMirroringType(_595data&0x40?MirroringType::Vertical:MirroringType::Horizontal);
                    }
                    else
                    {
                        SetMirroringType(_595data&0x40?MirroringType::ScreenBOnly:MirroringType::ScreenAOnly);
                    }
                    break;
                }
            }
        }

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x4000;}
        virtual uint16_t GetCHRPageSize() override {return 0x2000;}
        virtual bool HasBusConflicts() override {return true;}

    void InitMapper() override
    {
        SelectPRGPage(0,0);
        SelectPRGPage(1,-1);

        set595Data();
    }

    void StreamState(bool saving) override
    {
        BaseMapper::StreamState(saving);
        Stream(_595data,_595busSrclkTriggered,_595busRclkTriggered);
    }

    #define _595busSer 0x20
    #define _595busRclk 0x40
    #define _595busSrclk 0x80

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        SelectPRGPage(0,value&0x1f);
        if(value&_595busSrclk)
        {
            if(_595busSrclkTriggered==false)
            {
                _595data<<=1;
                _595data+=(value&_595busSer)>>5;
                _595busSrclkTriggered=true;
            }
        }
        else
        {
            _595busSrclkTriggered=false;
        }
        if(value&_595busRclk)
        {
            if(_595busRclkTriggered==false)
            {
                set595Data();
                _595busRclkTriggered=true;
            }
        }
        else
        {
            _595busRclkTriggered=false;
        }
    }
};
