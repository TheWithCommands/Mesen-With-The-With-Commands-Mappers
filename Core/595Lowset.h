#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class Lowset595:public BaseMapper
{
    private:
        uint8_t _595data;

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x4000;}
        virtual uint16_t GetCHRPageSize() override {return 0x2000;}
        virtual bool HasBusConflicts() override {return true;}

    void InitMapper() override
    {
        SelectPRGPage(0,GetPowerOnByte());
        SelectPRGPage(1,-1);

        SelectCHRPage(0,GetPowerOnByte());

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
                switch(GetPowerOnByte()%4)
                {
                    case 0:
                    {
                        SetMirroringType(MirroringType::ScreenAOnly);
                        break;
                    }
                    case 1:
                    {
                        SetMirroringType(MirroringType::ScreenBOnly);
                        break;
                    }
                    case 2:
                    {
                        SetMirroringType(MirroringType::Horizontal);
                        break;
                    }
                    case 3:
                    {
                        SetMirroringType(MirroringType::Vertical);
                        break;
                    }
                }
                break;
            }
        }
    }

    void StreamState(bool saving) override
	{
		BaseMapper::StreamState(saving);
		Stream(_595data);
	}

    #define _595busSer 0x20
    #define _595busRclk 0x40
    #define _595busSrclk 0x80

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        SelectPRGPage(0,value&0x1f);
        if(value&_595busSrclk)
        {
            _595data<<=1;
            _595data+=(value&_595busSer)>>5;
        }
        if(value&_595busRclk)
        {
            SelectCHRPage(0,_595data&0x3f);

            switch(_romInfo.NesHeader.Byte6&0x09)
            {
                case 8:
                case 9:
                    break;
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
    }
};
