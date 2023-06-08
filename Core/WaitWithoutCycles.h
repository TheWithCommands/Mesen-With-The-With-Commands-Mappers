#pragma once
#include "stdafx.h"
#include <random>
#include "BaseMapper.h"

class WaitWithoutCycles:public BaseMapper
{
    private:
        uint16_t _irqCounter;
        uint8_t _irqInput;
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
        Stream(_irqCounter,_irqEnabled,_irqInput);
    }

    void ProcessCpuClock() override
    {
        if(_irqEnabled)
        {
            _irqCounter++;
            std::random_device rd;
            std::mt19937 mt(rd());
            std::uniform_int_distribution<> dist(0, 255);
            if(_irqInput==1)_irqCounter=(_irqCounter&0xff00)|dist(mt);
            else if(_irqInput==2)_irqCounter=(_irqCounter&0xff)|(dist(mt)<<8);
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
                switch(value&0x03)
                {
                    case 0:
                    {
                        _irqInput=1;
                        break;
                    }
                    case 1:
                    {
                        _irqInput=2;
                        break;
                    }
                    case 2:
                    {
                        _irqInput=0;
                        _irqEnabled=true;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case 6:
            {
                if(_irqEnabled==false)
                {
                    if(_irqInput==1)_irqCounter=(_irqCounter&0xff00)|value;
                    else if(_irqInput==2)_irqCounter=(_irqCounter&0xff)|(value<<8);
                }
                break;
            }
            case 7:
            {
                _irqEnabled=false;
                _irqCounter=0;
                _console->GetCpu()->ClearIrqSource(IRQSource::External);
            }
        }
    }
};
