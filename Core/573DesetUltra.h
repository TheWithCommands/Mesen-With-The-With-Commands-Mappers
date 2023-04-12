#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class Deset573Ultra:public BaseMapper
{
    private:
        bool _chr0000Flag;
        bool _chr1000Flag;
        bool _mirrorModeFlag;
        bool _mirrorModeData;
        uint8_t chrCache;

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x4000;}
        virtual uint16_t GetCHRPageSize() override {return 0x1000;}
        virtual bool HasBusConflicts() override {return false;}

    void InitMapper() override
    {
        SelectPRGPage(0,GetPowerOnByte());
        SelectPRGPage(1,-1);

        SelectCHRPage(0,GetPowerOnByte());
        SelectCHRPage(1,GetPowerOnByte());

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
		Stream(_chr0000Flag,_chr1000Flag,_mirrorModeFlag,_mirrorModeData,chrCache);
	}

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        if(addr>=0xc000)
        {
            chrCache=value&0x7f;
            _mirrorModeFlag=value&0x80;
        }
        else
        {
            SelectPRGPage(0,value&0x1f);
            _chr0000Flag=value&0x20;
            _chr1000Flag=value&0x40;
            _mirrorModeData=value&0x80;
        }

        if(_chr0000Flag)SelectCHRPage(0,chrCache);
        if(_chr1000Flag)SelectCHRPage(1,chrCache);

        switch(_romInfo.NesHeader.Byte6&0x09)
        {
            case 8:
            case 9:
                break;
            default:
            {
                if(_mirrorModeFlag)
                {
                    SetMirroringType(_mirrorModeData?MirroringType::Vertical:MirroringType::Horizontal);
                }
                else
                {
                    SetMirroringType(_mirrorModeData?MirroringType::ScreenBOnly:MirroringType::ScreenAOnly);
                }
                break;
            }
        }
    }
};
