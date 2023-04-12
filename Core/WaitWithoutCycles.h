#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class WaitWithoutCycles:public BaseMapper
{
    private:
        uint16_t _irqCounter;
        bool _irqEnabled;

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x4000;}
        virtual uint16_t GetCHRPageSize() override {return 0x0800;}
        virtual bool HasBusConflicts() override {return false;}

    void InitMapper() override
    {
        SelectPRGPage(0,GetPowerOnByte());
        SelectPRGPage(1,-1);

        SelectCHRPage(0,GetPowerOnByte());
        SelectCHRPage(1,GetPowerOnByte());
        SelectCHRPage(2,GetPowerOnByte());
        SelectCHRPage(3,GetPowerOnByte());

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
        Stream(_irqCounter,_irqEnabled);
    }

    void ProcessCpuClock() override
    {
        if(_irqEnabled)
        {
            _irqCounter++;
            if(_irqCounter==0)
            {
                _console->GetCpu()->SetIrqSource(IRQSource::External);
                _irqEnabled=false;
            }
        }
    }

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        switch(addr&7)
        {
            case 0:
            {
                SelectPRGPage(0,value&0x1f);
                switch(_romInfo.NesHeader.Byte6&0x09)
                {
                    case 8:
                    case 9:
                        break;
                    default:
                    {
                        if(value&0x40)
                        {
                            SetMirroringType(value&0x20?MirroringType::Vertical:MirroringType::Horizontal);
                        }
                        else
                        {
                            SetMirroringType(value&0x20?MirroringType::ScreenBOnly:MirroringType::ScreenAOnly);
                        }
                        break;
                    }
                }
                if(value&0x80)
                {
                    _irqEnabled=false; 
                    _irqCounter=0; 
                    _console->GetCpu()->ClearIrqSource(IRQSource::External);
                }
            }
            case 1:
            {
                SelectCHRPage(0,value);
                break;
            }
            case 2:
            {
                SelectCHRPage(1,value);
                break;
            }
            case 3:
            {
                SelectCHRPage(2,value);
                break;
            }
            case 4:
            {
                SelectCHRPage(3,value);
                break;
            }
            case 5:
            {
                _irqCounter=(_irqCounter&0xff00)|value;
                break;
            }
            case 6:
            {
                _irqCounter=(_irqCounter&0xff)|(value<<8);
                _irqEnabled=true;
                break;
            }
            case 7:break;
        }
    }
};
