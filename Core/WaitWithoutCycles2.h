#pragma once
#include "stdafx.h"
#include "BaseMapper.h"

class WaitWithoutCycles2:public BaseMapper
{
    private:
        uint16_t irqCounter;
        uint8_t irqCache;
        bool irqLowInput,irqHighInput;
        bool irqEnabled;

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x2000;}
        virtual uint16_t GetCHRPageSize() override {return 0x0400;}
        virtual bool HasBusConflicts() override {return false;}

    void InitMapper() override
    {
        irqCache=GetPowerOnByte()*2; //Temporary, will GetPowerOnByte() again after use
        SelectPRGPage(0,irqCache);
        SelectPRGPage(1,irqCache+1);
        SelectPRGPage(2,GetPowerOnByte());
        SelectPRGPage(3,-1);

        SelectCHRPage(0,GetPowerOnByte());
        SelectCHRPage(1,GetPowerOnByte());
        SelectCHRPage(2,GetPowerOnByte());
        SelectCHRPage(3,GetPowerOnByte());
        SelectCHRPage(4,GetPowerOnByte());
        SelectCHRPage(5,GetPowerOnByte());
        SelectCHRPage(6,GetPowerOnByte());
        SelectCHRPage(7,GetPowerOnByte());

        irqCounter=(GetPowerOnByte()<<8)+GetPowerOnByte();
        irqCache=GetPowerOnByte();
        irqLowInput=GetPowerOnByte()%2;
        irqHighInput=GetPowerOnByte()%2;
        irqEnabled=GetPowerOnByte()%2;

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
        Stream(irqCounter,irqCache,irqLowInput,irqHighInput,irqEnabled);
    }

    void ProcessCpuClock() override
    {
        if(irqLowInput)irqCounter=(irqCounter&0xff00)|irqCache;
        else if(irqHighInput)irqCounter=(irqCounter&0xff)|(irqCache<<8);

        if(irqEnabled)
        {
            irqCounter++;
            if(irqCounter==0)
            {
                _console->GetCpu()->SetIrqSource(IRQSource::External);
                irqEnabled=false;
            }
        }
    }

    void WriteRegister(uint16_t addr,uint8_t value) override
    {
        switch(addr&0x0f)
        {
            case 0:
            {
                SelectPRGPage(0,(value&0x1f)*2);
                SelectPRGPage(1,(value&0x1f)*2+1);
                break;
            }
            case 1:
            {
                SelectPRGPage(2,value&0x3f);
                break;
            }
            case 2:
            {
                switch(_romInfo.NesHeader.Byte6&0x09)
                {
                    case 8:
                    case 9:
                        break;
                    default:
                    {
                        if(value&0x02)
                        {
                            SetMirroringType(value&0x01?MirroringType::Vertical:MirroringType::Horizontal);
                        }
                        else
                        {
                            SetMirroringType(value&0x01?MirroringType::ScreenBOnly:MirroringType::ScreenAOnly);
                        }
                        break;
                    }
                }
                break;
            }
            case 3:
            {
                irqLowInput=true;
                irqHighInput=false;
                break;
            }
            case 4:
            {
                irqLowInput=false;
                irqHighInput=true;
                break;
            }
            case 5:
            {
                irqCache=value;
                break;
            }
            case 6:
            {
                irqLowInput=false;
                irqHighInput=false;
                irqEnabled=true;
                break;
            }
            case 7:
            {
                irqEnabled=false;
                irqCounter=0;
                _console->GetCpu()->ClearIrqSource(IRQSource::External);
                break;
            }
            case 8:
            {
                SelectCHRPage(0,value);
                break;
            }
            case 9:
            {
                SelectCHRPage(1,value);
                break;
            }
            case 10:
            {
                SelectCHRPage(2,value);
                break;
            }
            case 11:
            {
                SelectCHRPage(3,value);
                break;
            }
            case 12:
            {
                SelectCHRPage(4,value);
                break;
            }
            case 13:
            {
                SelectCHRPage(5,value);
                break;
            }
            case 14:
            {
                SelectCHRPage(6,value);
                break;
            }
            case 15:
            {
                SelectCHRPage(7,value);
                break;
            }
        }
    }
};
