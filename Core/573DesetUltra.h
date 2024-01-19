#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class Deset573Ultra:public BaseMapper
{
    private:
        bool _chr0000Flag=false;
        bool _chr1000Flag=false;
        bool _mirrorModeFlag=true;
        bool _mirrorModeData; //Here, '_romInfo.NesHeader.Byte6' cannot be used, so its initialization is handled by 'InitMapper()'
        uint8_t chrCache=0;

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x4000;}
        virtual uint16_t GetCHRPageSize() override {return 0x1000;}
        virtual bool HasBusConflicts() override {return false;}

    void InitMapper() override
    {
        SelectPRGPage(0,0);
        SelectPRGPage(1,-1);

        SelectCHRPage(0,0);
        SelectCHRPage(1,1);

        _mirrorModeData=_romInfo.NesHeader.Byte6&0x01?true:false;
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
