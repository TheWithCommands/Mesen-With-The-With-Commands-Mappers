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

    private:
        bool chr0000RangeUseUpper;
        bool chr1000RangeUseUpper;
        uint8_t chrBank0Cache;
        uint8_t chrBank1Cache;
        uint8_t chrBank2Cache;
        uint8_t chrBank3Cache;
        uint8_t chrBank4Cache;
        uint8_t chrBank5Cache;
        uint8_t chrBank6Cache;
        uint8_t chrBank7Cache;

    private:
        void submitChr()
        {
            SelectCHRPage(0,chrBank0Cache+(chr0000RangeUseUpper?0x0100:0x0000));
            SelectCHRPage(1,chrBank1Cache+(chr0000RangeUseUpper?0x0100:0x0000));
            SelectCHRPage(2,chrBank2Cache+(chr0000RangeUseUpper?0x0100:0x0000));
            SelectCHRPage(3,chrBank3Cache+(chr0000RangeUseUpper?0x0100:0x0000));
            SelectCHRPage(4,chrBank4Cache+(chr1000RangeUseUpper?0x0100:0x0000));
            SelectCHRPage(5,chrBank5Cache+(chr1000RangeUseUpper?0x0100:0x0000));
            SelectCHRPage(6,chrBank6Cache+(chr1000RangeUseUpper?0x0100:0x0000));
            SelectCHRPage(7,chrBank7Cache+(chr1000RangeUseUpper?0x0100:0x0000));
        }

    protected:
        virtual uint16_t GetPRGPageSize() override {return 0x2000;}
        virtual uint16_t GetCHRPageSize() override {return 0x0400;}
        virtual bool HasBusConflicts() override {return false;}

    void InitMapper() override
    {
        SelectPrgPage2x(0,GetPowerOnByte()*2);
        SelectPRGPage(2,GetPowerOnByte());
        SelectPRGPage(3,-1);

        chr0000RangeUseUpper=GetPowerOnByte()%2;
        chr1000RangeUseUpper=GetPowerOnByte()%2;
        chrBank0Cache=GetPowerOnByte();
        chrBank1Cache=GetPowerOnByte();
        chrBank2Cache=GetPowerOnByte();
        chrBank3Cache=GetPowerOnByte();
        chrBank4Cache=GetPowerOnByte();
        chrBank5Cache=GetPowerOnByte();
        chrBank6Cache=GetPowerOnByte();
        chrBank7Cache=GetPowerOnByte();
        submitChr();

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
        Stream(irqCounter,irqCache,irqLowInput,irqHighInput,irqEnabled,chr0000RangeUseUpper,chr1000RangeUseUpper,chrBank0Cache,chrBank1Cache,chrBank2Cache,chrBank3Cache,chrBank4Cache,chrBank5Cache,chrBank6Cache,chrBank7Cache);
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
                SelectPrgPage2x(0,(value&0x1f)*2);
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
                chr0000RangeUseUpper=value&0x04;
                chr1000RangeUseUpper=value&0x08;
                submitChr();
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
                chrBank0Cache=value;
                submitChr();
                break;
            }
            case 9:
            {
                chrBank1Cache=value;
                submitChr();
                break;
            }
            case 10:
            {
                chrBank2Cache=value;
                submitChr();
                break;
            }
            case 11:
            {
                chrBank3Cache=value;
                submitChr();
                break;
            }
            case 12:
            {
                chrBank4Cache=value;
                submitChr();
                break;
            }
            case 13:
            {
                chrBank5Cache=value;
                submitChr();
                break;
            }
            case 14:
            {
                chrBank6Cache=value;
                submitChr();
                break;
            }
            case 15:
            {
                chrBank7Cache=value;
                submitChr();
                break;
            }
        }
    }
};
