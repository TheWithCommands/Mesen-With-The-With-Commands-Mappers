#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class WaitWithoutCycles:public BaseMapper
{
    private:
        uint16_t _irqCounter;
        uint8_t _irqCache;
        bool _irqRunning,_irqEnabled;

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
        Stream(_irqCounter,_irqCache,_irqRunning,_irqEnabled);
    }

    void ProcessCpuClock() override
    {
        if(_irqRunning)
        {
            _irqCounter--;
            if(_irqCounter==0xffff)
            {
                _console->GetCpu()->SetIrqSource(IRQSource::External);
                _irqRunning=false;
            }
        }
    }

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        switch(addr&7)
        {
            case 0:
            {
                SelectPRGPage(0,value&0x3f);
                switch(_romInfo.NesHeader.Byte6&0x09)
                {
                    case 8:
                    case 9:
                        break;
                    default:
                    {
                        if(value&0x80)
                        {
                            SetMirroringType(value&0x40?MirroringType::Vertical:MirroringType::Horizontal);
                        }
                        else
                        {
                            SetMirroringType(value&0x40?MirroringType::ScreenBOnly:MirroringType::ScreenAOnly);
                        }
                        break;
                    }
                }
                break;
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
                switch(value&0x01)
                {
                    case 0:
                    {
                        if(_irqEnabled==false)_irqCounter=(_irqCounter&0xff00)|_irqCache;
                        break;
                    }
                    case 1:
                    {
                        if(_irqEnabled==false)
                        {
                            _irqCounter=(_irqCounter&0xff)|(_irqCache<<8);
                            _irqRunning=true;
                            _irqEnabled=true;
                        }
                        break;
                    }
                }
                break;
            }
            case 6:
            {
                if(_irqEnabled==false)_irqCache=value;
                break;
            }
            case 7:
            {
                _irqRunning=false;
                _irqEnabled=false;
                _irqCounter=0;
                _console->GetCpu()->ClearIrqSource(IRQSource::External);
                break;
            }
        }
    }
};
